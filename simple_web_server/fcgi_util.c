#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <errno.h>
#include "fastcgi.h"
#include "fcgi_util.h"

#define PARAMS_BUFF_LEN 4096
#define CONTENT_BUFF_LEN 4096



void FCGI_init(Fcgi_t *c) {
    c->sockfd = 0;
    c->requestId = 0;
	c->flag = 0;
}

void FCGI_close(Fcgi_t *c) {
    close(c->sockfd);
}

void setRequestId(Fcgi_t *c, int requestId) {
    c->requestId = requestId;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeHeader --
 *
 *      Constructs an FCGI_Header struct.
 *
 *----------------------------------------------------------------------
 */
static FCGI_Header MakeHeader(
        int type,
        int requestId,
        int contentLength,
        int paddingLength)
{
    FCGI_Header header;
    header.version = FCGI_VERSION_1;
    header.type             = (unsigned char) type;
    header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
    header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
    header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
    header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
    header.paddingLength    = (unsigned char) paddingLength;
    header.reserved         =  0;
    return header;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeBeginRequestBody --
 *
 *      Constructs an FCGI_BeginRequestBody record.
 *
 *----------------------------------------------------------------------
 */
static FCGI_BeginRequestBody MakeBeginRequestBody(
        int role,
        int keepConnection)
{
    FCGI_BeginRequestBody body;
    body.roleB1 = (unsigned char) ((role >>  8) & 0xff);
    body.roleB0 = (unsigned char) (role         & 0xff);
    body.flags  = (unsigned char) ((keepConnection) ? FCGI_KEEP_CONN : 0);
    memset(body.reserved, 0, sizeof(body.reserved));
    return body;
}

/*
 *----------------------------------------------------------------------
 *
 * FCGIUtil_BuildNameValueHeader --
 *
 *      Builds a name-value pair header from the name length
 *      and the value length.  Stores the header into *headerBuffPtr,
 *      and stores the length of the header into *headerLenPtr.
 *
 * Side effects:
 *      Stores header's length (at most 8) into *headerLenPtr,
 *      and stores the header itself into
 *      headerBuffPtr[0 .. *headerLenPtr - 1].
 *
 *----------------------------------------------------------------------
 */
static void FCGIUtil_BuildNameValueHeader(
		char *name,
        int nameLen,
		char *value,
        int valueLen,
        unsigned char *headerBuffPtr,
        int *headerLenPtr) {
    unsigned char *startHeaderBuffPtr = headerBuffPtr;

    if (nameLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) nameLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((nameLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 16);
        *headerBuffPtr++ = (unsigned char) (nameLen >> 8);
        *headerBuffPtr++ = (unsigned char) nameLen;
    }
    if (valueLen < 0x80) {
        *headerBuffPtr++ = (unsigned char) valueLen;
    } else {
        *headerBuffPtr++ = (unsigned char) ((valueLen >> 24) | 0x80);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 16);
        *headerBuffPtr++ = (unsigned char) (valueLen >> 8);
        *headerBuffPtr++ = (unsigned char) valueLen;
    }

	while(*name != '\0'){
		*headerBuffPtr++ = *name++;
	}
 
	while(*value != '\0'){
		*headerBuffPtr++ = *value++;
	}

    *headerLenPtr = headerBuffPtr - startHeaderBuffPtr;
}

void sockConnect(Fcgi_t *c, char *host, int port) {
    int rc;
    int sockfd;
    struct sockaddr_in server_address;

    const char *ip = host;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);

    bzero(&server_address, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(ip);
    server_address.sin_port = htons(port);

    rc = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    assert(rc >= 0);

    c->sockfd = sockfd;
}

int sendBeginRequestRecord(Fcgi_t *c) {
    int rc;
    FCGI_BeginRequestRecord beginRecord;

    beginRecord.header = MakeHeader(FCGI_BEGIN_REQUEST, c->requestId, sizeof(beginRecord.body), 0);
    beginRecord.body = MakeBeginRequestBody(FCGI_RESPONDER, 0);

    rc = write(c->sockfd, (char *)&beginRecord, sizeof(beginRecord));
    assert(rc == sizeof(beginRecord));

    return 1;
}

int sendParams(Fcgi_t *c, char *name, char *value) {
    int rc;
	unsigned char bodyBuff[PARAMS_BUFF_LEN];
	bzero(bodyBuff, PARAMS_BUFF_LEN);
	int nameLen, valueLen, bodyLen;

	nameLen = strlen(name);
	valueLen = strlen(value);
	FCGIUtil_BuildNameValueHeader(name, nameLen, value, valueLen, &bodyBuff[0], &bodyLen);
	
	FCGI_Header nameValueHeader;
	nameValueHeader = MakeHeader(FCGI_PARAMS, c->requestId, bodyLen, 0);

	int valuenameRecordLen = bodyLen + FCGI_HEADER_LEN;
	char valuenameRecord[valuenameRecordLen];

    memcpy(valuenameRecord, (char *)&nameValueHeader, FCGI_HEADER_LEN);
	memcpy(valuenameRecord + FCGI_HEADER_LEN, bodyBuff, bodyLen);

    rc = write(c->sockfd, (char *)&valuenameRecord, valuenameRecordLen);
	if (rc != valuenameRecordLen) {
		printf("write record error.len:%d,send:%d", valuenameRecordLen, rc);
		perror("write");
		exit(1);
	}
    return rc;
}

int sendEndRequestRecord(Fcgi_t *c) {
    int rc;
    FCGI_Header endHeader;
    endHeader = MakeHeader(FCGI_PARAMS, c->requestId, 0, 0);

    rc = write(c->sockfd, (char *)&endHeader, FCGI_HEADER_LEN);
    assert(rc == FCGI_HEADER_LEN);

    return rc;
}

void explode(char *str, char *delim, Fcgi_res_head *head[]) {
	char *token = strtok(str, delim);
	int i = 0;
	while (token != NULL) {
		//printf("token=%s\n", token);
		char *p = strstr(token, ": ");
		if (p != NULL) {
			Fcgi_res_head *subhead = (Fcgi_res_head *)malloc(sizeof(Fcgi_res_head));
			memset(subhead, 0, sizeof(subhead));
			subhead->value = strdup(p+2);
			*p = '\0';
			subhead->name = strdup(token);
			//printf("name=%s, value=%s, i=%d\n", subhead->name, subhead->value, i);
			head[i] = subhead;
			i++;
		}
		token = strtok(NULL, delim);
	}
}

void renderContent(Fcgi_t *c, char **headstr, char **content) {
    FCGI_Header responderHeader;
	int contentLen, count;
	char *section = NULL;
	char tmp[8];

    while (read(c->sockfd, &responderHeader, FCGI_HEADER_LEN) > 0) {
        if (responderHeader.type == FCGI_STDOUT) {
            contentLen = (responderHeader.contentLengthB1 << 8) + (responderHeader.contentLengthB0);
			section = (char *)malloc(contentLen + 1);
            bzero(section, contentLen);

            count = read(c->sockfd, section, contentLen);
			//printf("count = %d, contentLen = %d, msg: = %s, section = %s\n", count, contentLen, strerror(errno), section);
			if (count != contentLen){
				//printf("count = %d, contentLen = %d, msg: = %s, content = %s\n", count, contentLen, strerror(errno), content);
				perror("read1");
			}
			section[contentLen] = '\0';
			if (c->flag == 0 && strncmp(section, "X-Powered-By", 12) == 0) {
				//分解
				char *p = strstr(section, "\r\n\r\n");
				if (strlen(p+4) > 0) {
					*content = (char *)realloc(*content, strlen(*content) + strlen(p+4) + 1);
					strcat(*content, p+4);
				}
				*p = '\0';
				//printf("section=%s\n", section);
				*headstr = (char *)realloc(*headstr, strlen(*headstr) + strlen(section) + 1);
				strcat(*headstr, section);
				//explode(section, "\r\n", head);
				c->flag = 1;
			} else {
				*content = (char *)realloc(*content, strlen(*content) + contentLen + 1);
				strcat(*content, section);
			}
			free(section);

			if (responderHeader.paddingLength > 0) {
				count = read(c->sockfd, tmp, responderHeader.paddingLength);
				if (count != responderHeader.paddingLength) {
					perror("read2");
				}
			}
        } else if (responderHeader.type == FCGI_STDERR) {
			contentLen = (responderHeader.contentLengthB1 << 8) + (responderHeader.contentLengthB0);
			section = (char *)malloc(contentLen + 1);
            bzero(section, contentLen);
			count = read(c->sockfd, section, contentLen);
			if (count != contentLen) {
				perror("read3");
			}
			section[contentLen] = '\0';
			*content = (char *)realloc(*content, strlen(*content) + contentLen + 1);
			strcat(*content, section);
			fprintf(stdout,"error:%s\n", section);
			free(section);
 
			//跳过填充部分
			if (responderHeader.paddingLength > 0) {
                count = read(c->sockfd, tmp, responderHeader.paddingLength);
                if (count != responderHeader.paddingLength) {
					perror("read4");
				}
            }            
        } else if (responderHeader.type == FCGI_END_REQUEST) {
			FCGI_EndRequestBody endRequest;
			count = read(c->sockfd, &endRequest, sizeof(endRequest));
			if (count != sizeof(endRequest)) {
				perror("read5");
			}
			//fprintf(stdout,"\nendRequest:appStatus:%d,protocolStatus:%d\n",(endRequest.appStatusB3<<24)+(endRequest.appStatusB2<<16) + (endRequest.appStatusB1<<8)+(endRequest.appStatusB0),endRequest.protocolStatus);
        }
    }
}

void sendBody(Fcgi_t *c, char *body) {
	FCGI_Header head = MakeHeader(FCGI_STDIN, c->requestId, strlen(body), 0);
	write(c->sockfd, &head, sizeof(head));		//send header
	write(c->sockfd, body, strlen(body));			//send body
	FCGI_Header endHeader;
	endHeader = MakeHeader(FCGI_STDIN, c->requestId, 0, 0);
    write(c->sockfd, &endHeader, sizeof(endHeader));
}

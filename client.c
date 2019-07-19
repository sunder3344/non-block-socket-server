#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
	int sockfd, numbytes;
	char buf[4096];
	struct sockaddr_in their_addr;
	printf("start!\n");
	while ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1);
	printf("get the socket!\n");
	
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(8888);
	their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	bzero(&(their_addr.sin_zero), 8);

	while(connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1);
	printf("Get the Server peer!\n");
	while (1) {
		printf("Entersome thing: ");
		scanf("%s", buf);
		numbytes = send(sockfd, buf, strlen(buf), 0);
		printf("received: %s\n", buf);
		printf("numbytes: %d\n", numbytes);
	}
	close(sockfd);
	return 0;
}

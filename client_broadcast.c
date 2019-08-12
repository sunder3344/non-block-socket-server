#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#define BUFLEN 255
 
int main(int argc, char **argv) {
	struct sockaddr_in localaddr;
	int sockfd,n;
	char msg[BUFLEN+1];
 
	if(argc != 2)
	{
		perror("port error \n");
		_exit(1);
	}
 
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if( sockfd < 0)
	{
		perror("sock fail \n");
		_exit(1);
	}
	
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(atoi(argv[1]));
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
	int optval = SO_REUSEADDR ;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	
	if(bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0 )
	{
		perror("bind fail \n");
		_exit(1);
	}
 
	if( n = read(sockfd, msg,BUFLEN) == -1 ) {
		perror("read error \n");
		_exit(1);
	}
	else {
		//msg[n] = '0';
		printf("read time : %s \n",msg);
	}
	return 1;
}

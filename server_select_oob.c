#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

int sockfd, new_fd;

int main(int argc, char **argv) {
	int n;
	char buff[100];
	int on = 1;
	fd_set rset, xset;

	struct sockaddr_in cliaddr;
	while ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1);
	printf("start the socket!\n");
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(8888);
	cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(cliaddr.sin_zero), 8);

	while (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr)) == -1);
	while (listen(sockfd, 10) == -1);
	new_fd = accept(sockfd, NULL, NULL);
	printf("new_fd = %d\n", new_fd);

	FD_ZERO(&rset);
	FD_ZERO(&xset);	

	for (;;) {
		FD_SET(new_fd, &rset);
		FD_SET(new_fd, &xset);
		
		select(new_fd + 1, &rset, NULL, &xset, NULL);

		if (FD_ISSET(new_fd, &xset)) {
			n = recv(new_fd, buff, sizeof(buff)-1, MSG_OOB);
			buff[n] = 0;
			printf("read %d OOB byte: %s\n", n, buff);
			memset(&buff, 0, sizeof(buff));
		}

		if (FD_ISSET(new_fd, &rset)) {
			if ((n = read(new_fd, buff, sizeof(buff)-1)) == 0) {
				printf("received EOF\n");
				exit(1);
			}
			buff[n] = 0;
			printf("read %d bytes: %s\n", n, buff);
			memset(&buff, 0, sizeof(buff));
		}
	}
	return 0;
}

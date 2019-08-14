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

void sig_urg(int);

int main(int argc, char **argv) {
	int n;
	char buff[100];
	int on = 1;

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
	if (new_fd != -1) {
		signal(SIGURG, sig_urg);
		int z = fcntl(new_fd, F_SETOWN, getpid());		//使用fcntl设置已连接套接字的属主

		printf("get the server! z = %d\n", z);
	
		while ((n = read(new_fd, buff, sizeof(buff)-1)) != 0) {
			buff[n] = 0;
			printf("read %d bytes: %s\n", n, buff);
			memset(&buff, 0, sizeof(buff));
        	}
	}
	return 1;
}

void sig_urg(int signo) {
	int n;
	char buf[100];
	printf("SIGURG received\n");
	n = recv(new_fd, buf, sizeof(buf)-1, MSG_OOB);
	buf[n] = 0;
	printf("read %d OOB bytes: %s\n", n, buf);
}


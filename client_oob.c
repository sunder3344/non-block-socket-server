#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include <netdb.h>
#include <errno.h>
#include <gdbm.h>
#include <time.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "unp.h"

int main(int argc, char * argv[]) {
	int sockfd;
	int new_fd;
	struct sockaddr_in addr;
	if (argc != 3) {
		perror("param error!");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8888);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	bzero(&(addr.sin_zero), 8);

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	while (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1);
	sleep(1);	

	write(sockfd, "123", 3);	
	printf("wrote 3 bytes of normal data\n");	
	sleep(1);
	
	send(sockfd, "4", 1, MSG_OOB);
	printf("wrote 1 byte of OOB data\n");
	sleep(1);

	write(sockfd, "56", 2);
	printf("wrote 2 bytes of normal data\n");
	sleep(1);

	send(sockfd, "7", 1, MSG_OOB);
	printf("wrote 1 bytep of OOB data\n");
	sleep(1);

	write(sockfd, "89", 2);
	printf("wrote 2 bytes of normal data\n");
	sleep(1);
	return 0;
}


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

#define BUFLEN 255
#define TTL 64

void getcurtime(char *curtime) {
	time_t tm = time(NULL);
	sprintf(curtime, "realtime:%d\n", tm);
}

int main(int argc, char * argv[]) {
	struct sockaddr_in peeraddr;
	int sockfd;
	int optval = 1;
	char msg[BUFLEN+1];

	if (argc != 3) {
		perror("can't input ADDRESS and PORT \n");
	}
	
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		perror("socket failed\n");
	
	int timelive = TTL;
	setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&timelive, sizeof(timelive));
	memset(&peeraddr, 0, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_addr.s_addr = inet_addr(argv[1]);
	peeraddr.sin_port = htons(atoi(argv[2]));
	printf("------start multicast-------\n");
	while (1) {
		getcurtime(msg);
		int len = read(STDIN_FILENO, msg, sizeof(msg));
		sendto(sockfd, msg, len, 0, (struct sockaddr *)&peeraddr, sizeof(struct sockaddr));
		fflush(stdout);
		sleep(1);
		
	}
	return 0;
}

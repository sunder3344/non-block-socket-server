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
#include <time.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define BUFLEN 255

void getcurtime(char *curtime) {
	time_t tm = time(NULL);
	sprintf(curtime, "realtime:%d\n", tm);
}

int main(int argc, char * argv[]) {
	struct sockaddr_in peeraddr;
	int sockfd;
	int optval = 1;			//设置为1就可以开启广播
	char msg[BUFLEN+1];

	if (argc != 3) {
		perror("can't input ADDRESS and PORT \n");
	}
	
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		perror("socket failed\n");

	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
	memset(&peeraddr, 0, sizeof(peeraddr));
	peeraddr.sin_family = AF_INET;
	peeraddr.sin_addr.s_addr = inet_addr(argv[1]);
	peeraddr.sin_port = htons(atoi(argv[2]));
	printf("------start broadcast-------\n");
	while (1) {
		getcurtime(msg);
		//printf("time: %s\n", msg);
		sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&peeraddr, sizeof(struct sockaddr));
		fflush(stdout);
		sleep(3);
		
	}
	return 0;
}

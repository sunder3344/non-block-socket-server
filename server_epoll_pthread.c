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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define MAXLINE 4096
#define OPEN_MAX 100
#define LISTENQ 2048
#define SOCKET_NUM 4096
#define SERV_PORT 8888
#define INFTIM 1000
#define IP_ADDR "127.0.0.1"
#define PTHREAD_NUM 10

struct Threadparam {
	int epfd_param;
	int listenfd_param;
	struct epoll_event ev_param;
};

void *loop(struct Threadparam *threadp) {
	struct epoll_event events[SOCKET_NUM];
	char buf[MAXLINE];
	int sock_fd, conn_fd;
	int nfds;
	int n, i;
	socklen_t clilen;
	struct sockaddr_in clientaddr;
	
	while(1) {
		nfds = epoll_wait(threadp->epfd_param, events, SOCKET_NUM, 1);	//等待事件发生
		for (i = 0; i < nfds; i++) {		//处理所发生的全部事件
			if (events[i].data.fd == threadp->listenfd_param) {	//有新的连接
				clilen = sizeof(struct sockaddr_in);
				conn_fd = accept(threadp->listenfd_param, (struct sockaddr *)&clientaddr, &clilen);
				printf("accept a new client: %s in thread:%ld\n", inet_ntoa(clientaddr.sin_addr), pthread_self());
				threadp->ev_param.data.fd = conn_fd;
				threadp->ev_param.events = EPOLLIN;		//设置监听事件为可写
				epoll_ctl(threadp->epfd_param, EPOLL_CTL_ADD, conn_fd, &(threadp->ev_param));	//新增套接字
			} else if (events[i].events & EPOLLIN) {	//可读事件
				if ((sock_fd = events[i].data.fd) < 0) {
					continue;
				}
				memset(&buf, 0, sizeof(buf));
				if ((n = recv(sock_fd, buf, MAXLINE, 0)) < 0) {
					if (errno == ECONNRESET) {
						close(sock_fd);	
						events[i].data.fd = -1;
					} else {
						printf("readline error\n");
					}
				} else if (n == 0) {
					close(sock_fd);
					printf("关闭\n");	
					events[i].data.fd = -1;
				}
				printf("thread:%ld  %d -- > %s\n", pthread_self(), sock_fd, buf);
				threadp->ev_param.data.fd = sock_fd;
				threadp->ev_param.events = EPOLLOUT;
				epoll_ctl(threadp->epfd_param, EPOLL_CTL_MOD, sock_fd, &(threadp->ev_param));	//改动监听事件为可读
			} else if (events[i].events & EPOLLOUT) {	//可写事件
				sock_fd = events[i].data.fd;
				printf("thread:%ld   OUT\n", pthread_self());
				send(sock_fd, buf, MAXLINE, 0);
				threadp->ev_param.data.fd = sock_fd;
				threadp->ev_param.events = EPOLLIN;
				epoll_ctl(threadp->epfd_param, EPOLL_CTL_MOD, sock_fd, &(threadp->ev_param));
			}
		}
	}
}

int main(int argc, char * argv[]) {
	struct epoll_event ev;
	struct sockaddr_in serveraddr;
	int epfd;
	int listenfd;		//监听fd
	int maxi;
	int i;
	pthread_t tid;

	epfd = epoll_create(256);		//生成epoll句柄
	listenfd = socket(AF_INET, SOCK_STREAM, 0);	//创建套接字
	ev.data.fd = listenfd;		//设置与要处理事件相关的文件描写叙述符
	//ev.events = EPOLLIN|EPOLLET;		//设置要处理的事件类型(打开ET模式，可选;当设置ET时，需要用fcntl将socket设置为非阻塞模式)
	ev.events = EPOLLIN;
	
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);	//注冊epoll事件

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));	//绑定套接口
	listen(listenfd, LISTENQ);	//转为监听套接字
	int n;

	struct Threadparam *threadp = (struct Threadparam *)malloc(sizeof(struct Threadparam));
	threadp->epfd_param = epfd;
	threadp->listenfd_param = listenfd;
	threadp->ev_param = ev;
	for (i = 0; i < PTHREAD_NUM; i++) {
		pthread_create(&tid, NULL, (void *)loop, threadp);
	}

	pthread_join(tid, NULL);
	while (1) {
		pause();
	}
	return 0;
}

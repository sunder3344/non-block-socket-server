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
#define SERVER_DIR "./"

void render(int sock_fd, char *buf) {
	char buffer[MAXLINE];
	char *url = strtok(buf + 4, " ");
	char *path = strtok(url, "?");
	char file_path[MAXLINE];
	sprintf(file_path, "./%s", path);
	printf("file_path: = %s\n", file_path);
	FILE *file = fopen(file_path, "r");
	if (file != NULL) {
		char *response = "HTTP/1.1 200 OK\n\n";
		send(sock_fd, response, strlen(response), 0);
		
		memset(buffer, 0, MAXLINE);
		while (fgets(buffer, MAXLINE, file) != NULL) {
			send(sock_fd, buffer, strlen(buffer), 0);
		}
		fclose(file);
	} else {
		char not_found[] = "HTTP/1.1 404 Not Found\n\n";
		send(sock_fd, not_found, strlen(not_found), 0);
	}
	close(sock_fd);
}

int main(int argc, char * argv[]) {
	struct epoll_event ev, events[SOCKET_NUM];
	struct sockaddr_in clientaddr, serveraddr;
	int epfd;
	int listenfd;		//监听fd
	int maxi;
	int nfds;
	int i;
	int sock_fd, conn_fd;
	char buf[MAXLINE];

	epfd = epoll_create(256);		//生成epoll句柄
	listenfd = socket(AF_INET, SOCK_STREAM, 0);	//创建套接字
	int on = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
		perror("port reuse error!");
	}
	ev.data.fd = listenfd;		//设置与要处理事件相关的文件描写叙述符
	//ev.events = EPOLLIN|EPOLLET;		//设置要处理的事件类型(打开ET模式，可选;当设置ET时，需要用fcntl将socket设置为非阻塞模式)
	ev.events = EPOLLIN;
	
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);	//注冊epoll事件

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERV_PORT);
	int bind_port = bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));	//绑定套接口
	if (bind_port < 0) {
		perror("bind port error!");
		exit -1;
	}
	socklen_t clilen;
	listen(listenfd, LISTENQ);	//转为监听套接字
	int n;
	while(1) {
		nfds = epoll_wait(epfd, events, SOCKET_NUM, 1000);	//等待事件发生
		for (i = 0; i < nfds; i++) {		//处理所发生的全部事件
			if (events[i].data.fd == listenfd) {	//有新的连接
				clilen = sizeof(struct sockaddr_in);
				conn_fd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
				printf("accept a new client: %s\n", inet_ntoa(clientaddr.sin_addr));
				int flags = fcntl(conn_fd, F_SETFL, 0);
				fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);
				ev.data.fd = conn_fd;
				ev.events = EPOLLIN | EPOLLET;		//设置监听事件为可写
				epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);	//新增套接字
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
				printf("%d -- > %s\n", sock_fd, buf);
				render(sock_fd, buf);
				
				

				ev.data.fd = sock_fd;
				ev.events = EPOLLOUT;
				//epoll_ctl(epfd, EPOLL_CTL_MOD, sock_fd, &ev);	//改动监听事件为可读
			} else if (events[i].events & EPOLLOUT) {	//可写事件
				/*(sock_fd = events[i].data.fd;
				printf("OUT\n");
				send(sock_fd, buf, 4096, 0);
				ev.data.fd = sock_fd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sock_fd, &ev);*/
			}
		}
	}
	return 0;
}

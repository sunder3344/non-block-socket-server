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

#define MAXCLIENT 10
int fd_a[MAXCLIENT];

void showclient() {
	int i;
	for (i = 0; i < MAXCLIENT; i++) {
		if (fd_a[i] > 0)
			printf("%d client id %d\n", i, fd_a[i]);
	}
}

int main(int argc, char * argv[]) {
	int fd, new_fd, struct_len, numbytes, i;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	char buff[4096];
	int current_link = 0;
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8888);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero), 8);
	struct_len = sizeof(struct sockaddr_in);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	while (bind(fd, (struct sockaddr *)&server_addr, struct_len) == -1);
	printf("Bind Success!\n");
	while (listen(fd, 10) == -1);
	printf("Listening...\n");
	printf("Ready for Accept,Waitting...\n");
	
	fd_set fds;
	int maxfd;
	struct timeval timeout = {0, 0};
	while (1) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;		//每次都必须重新赋值(或采用pselect)，否则cpu居高不下
		//printf("&&&start&&&\n");
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		if (maxfd < fd) {
			maxfd = fd;
		}
		for (i = 0; i < MAXCLIENT; i++) {
			if (fd_a[i] != 0) {
				FD_SET(fd_a[i], &fds);
				if (maxfd < fd_a[i]) {
					maxfd = fd_a[i];
				}
			}
		}

		int result = select(maxfd + 1, &fds, NULL, NULL, &timeout);
		//printf("select value = %d\n", result);
		if (result == -1) {
			printf("select error %d %s", errno, strerror(errno));
                 	exit(-1);
		} else if (result == 0) {
			//printf("---------------------\n");
			goto receive;
			//do nothing just wait
		} else {
			//printf("result = %d\n", result);
			//printf("fd isset value === %d\n", FD_ISSET(fd, &fds));
			//首先判断是否有客户端连接进来
			if (FD_ISSET(fd, &fds)) {
				new_fd = accept(fd, NULL, NULL);
				if (new_fd <= 0) {
					printf("accept client failed!\n");
					continue;
				} else {
					printf("accept client success, new_fd = %d\n", new_fd);
					if (current_link >= MAXCLIENT) {
						current_link = 0;
					}
					fd_a[current_link] = new_fd;
					FD_SET(fd_a[current_link], &fds);
					if (maxfd < fd_a[current_link]) {
						maxfd = fd_a[current_link];
					}
					current_link++;
				}
			}
			//showclient();
			goto receive;

			receive:
			//判断是否有客户端数据发送进来
			for (i = 0; i < MAXCLIENT; i++) {
				//printf("in client %d value %d\n", i, fd_a[i]);
				//printf("fd_a[%d] = %d isset value =============== %d\n", i, fd_a[i], FD_ISSET(fd_a[i], &fds));
				if (FD_ISSET(fd_a[i], &fds) && fd_a[i] != 0) {
					bzero(buff, 4096);
					printf("--%d--%d\n", i, fd_a[i]);
					numbytes = recv(fd_a[i], buff, 4096, 0);
					printf("numbytes = %d\n", numbytes);
					if (numbytes > 0) {
						printf("received no.%d msg: %s\n", fd_a[i], buff);
					} else if (numbytes < 0) {
						printf("client close\n");
						close(fd_a[i]);
						FD_CLR(fd_a[i], &fds);
						fd_a[i] = 0;
					} else {
						printf("client shutdown\n");
						close(fd_a[i]);
						FD_CLR(fd_a[i], &fds);
						fd_a[i] = 0;
					}
					printf("---in the end---\n");
				}
			}
			//printf("===outer end===\n");
		}
		//printf("***outer loop***\n");
	}
	printf("all end!!!!\n");
	close(new_fd);
	close(fd);
	return 0;
}

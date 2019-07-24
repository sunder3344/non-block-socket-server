# non-block-socket-server
非阻塞socket，使用C的select多路复用，可以分别监听客户端连接已经客户端发送数据，可以保证服务端非阻塞，并能及时响应客户端的连接以及数据发送。

注意：
select:
timeout.tv_sec = 1;
timeout.tv_usec = 0;            //每次都必须重新赋值(或采用pselect)，否则cpu居高不下，select函数会不断修改timeout的值，所以每次循环都应该重新赋值[windows不受此影响]

epoll：
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
timeout参数如果设为0，即非阻塞，会导致内核不停的调用epoll_wait()，从而导致CPU占满

gcc -o server server.c


gcc -o client client.c

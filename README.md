# non-block-socket-server
非阻塞socket，使用C的select多路复用，可以分别监听客户端连接已经客户端发送数据，可以保证服务端非阻塞，并能及时响应客户端的连接以及数据发送。（优化空间：后续使用epoll改进）

gcc -o server server.c
gcc -o client client.c
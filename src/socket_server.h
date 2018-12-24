#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H
/*
*侦听socket初始化
*/
int init_listen_sock(unsigned short port, int *listen_socket);
/*
*数据接收接口
*/
int data_recv(int socket, char *buff, int data_len);

/*
*数据发送接口
*/
int data_send(int socket, char *buff, unsigned int data_len);

#endif

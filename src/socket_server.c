#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define BACKLOG         5       //侦听队列长度

/*
*侦听socket初始化
*/
int init_listen_sock(unsigned short port, int *listen_socket)
{
    int opt = 1;
    int fd = -1;
    struct sockaddr_in servaddr;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("create server socket error:%s (errno:%d)", strerror(errno), errno);
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind server socket error:%s(errno:%d)", strerror(errno), errno);
        close(fd);
        return -1;
    }
    
    if (listen(fd, BACKLOG) == -1) {
        printf("listen server socket error:%s(errno:%d)", strerror(errno), errno);
        close(fd);
        return -1;
    }

    *listen_socket = fd;
    
    return 0;
}

/*
*数据接收接口
*/
int data_recv(int socket, char *buff, int data_len)
{
    int nbytes = 0;
    
    if ((socket <= 0) || (NULL == buff) || (0 == data_len)){
        return -1;
    }

    while (nbytes <= 0) {   
        nbytes = recv(socket, buff, data_len, 0);
        if (0 == nbytes) {
            printf("receive socket shutdown");            
            return -1;
        }
        else if (-1 == nbytes) {
            if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)){
                continue;
            } 
            printf("receive error:%s(errno:%d)", strerror(errno), errno);
            return -1;
        }        
    }
    
    return nbytes;
}

/*
*数据发送接口
*/
int data_send(int socket, char *buff, unsigned int data_len)
{   
    int wrote = 0;
    int len = data_len;
    char *ptr = buff;

    if ((socket <= 0) || (NULL == buff) || (0 == data_len)){
        return -1;
    }

    while (len) {
        wrote = send(socket, ptr, len, 0);
        if (wrote == -1) {
            if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)){
                continue;
            }
            else {
                printf("send to cliend error:%s(errno:%d)", strerror(errno), errno);
                return -1;
            }
        }

        ptr += wrote;
        len -= wrote;
  }

    return 0;
}

#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <signal.h>
#include <pthread.h>
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include<unistd.h> 
#include <arpa/inet.h> 

#define SERVER_PORT 8887
#define MAXDATASIZE 4096 
#define SERVER_IP "127.0.0.1" 

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

static int run_flag = TRUE;

static int data_send(int socket, char *buff, unsigned int data_len)
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
                perror("send");
                return -1;
            }
        }

        ptr += wrote;
        len -= wrote;
  }

    return 0;
}

/*
*数据接收接口
*/
static int data_recv(int socket, char *buff, int data_len)
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

static unsigned long get_tick_count()  
{  
#if (defined(PJ_WIN32) && PJ_WIN32) || (defined(PJ_SUNOS) && PJ_SUNOS)
    return GetTickCount();
#else   
    struct timespec ts;  
    clock_gettime(CLOCK_MONOTONIC, &ts);  
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);  
#endif  
} 


int main(int argc, char *argv[])
{ 
    int sockfd; 
    struct sockaddr_in server_addr;
    int port = SERVER_PORT;
    char ip[32] = {0};

    if (argc < 3){
        printf("for example: ./test.bin 127.0.0.1 8887\n"
           "Ctrl+c--------exit the program\n");
        exit(0);
    }
    else{
        (void)snprintf(ip, sizeof(ip), "%s", argv[1]);
        port = atoi(argv[2]);

        printf("server: %s port: %d", ip, port);
    }

    printf("\n======================client initialization======================\n"); 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){ 
        perror("socket"); 
        exit(1); 
    }    
    
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(port); 
    server_addr.sin_addr.s_addr = inet_addr(ip);
    bzero(&(server_addr.sin_zero),sizeof(server_addr.sin_zero)); 

    if (connect(sockfd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr_in)) == -1){ 
        perror("connect");
        exit(1); 
    }
    
    char  test[256] = "环境高级编程";
    data_send(sockfd, test, 256);
    data_recv(sockfd, test, 256);
    printf("%s",test);

    if (sockfd > 0) close(sockfd); 
    
    return 0;
} 


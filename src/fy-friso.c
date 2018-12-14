/*
 * Friso test program.
 *     Of couse you can make it a perfect demo for friso.
 * all threads or proccess share the same friso_t,
 *     defferent threads/proccess use defferent friso_task_t.
 * and you could share the friso_config_t if you wish...
 *
 * @author chenxin <chenxin619315@gmail.com>
 */
#include "friso_API.h"
#include "friso.h"

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

#define __LENGTH__ 15
#define __INPUT_LENGTH__ 20480
#define ___EXIT_INFO___                    \
    println("Thanks for trying friso.");        \
break;

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define RUN_ARG_NUM     1
#define BACKLOG         5       //侦听队列长度
#define READ_TIMEOUT_SEC     1//秒
#define READ_TIMEOUT_USEC    0//微秒
#define READ_TIMEOUT_MSEC    100//100ms

#define ___ABOUT___                    \
    println("+-----------------------------------------------------------+");    \
    println("| friso - a chinese word segmentation writen by c.          |");    \
    println("+-----------------------------------------------------------+");

char word_type[16][32] = {
    "CJK_WORDS",
    "CJK_UNITS",
    "ECM_WORDS",
    "CEM_WORDS",
    "CN_LNAME",
    "CN_SNAME",
    "CN_DNAME1",
    "CN_DNAME2",
    "CN_LNA",
    "STOPWORDS",
    "ENPUN_WORDS",
    "EN_WORDS",
    "OTHER_WORDS",
    "NCSYN_WORDS",
    "PUNC_WORDS",
    "UNKNOW_WORDS"
};

/*service 配置*/
typedef struct run_arg_tag{
    int     listen_fd;      //侦听socket
    int     port;           //端口
    char    path[256];      //配置文件路径
}SZ_RUN_ARG_S;

/*关键词词语及拼音*/
typedef struct key_word_pinyin_tag{
    char word[64];
    char pinyin[512];
}key_word_pinyin;

key_word_pinyin  key_test[5] = {
    {"价格","jia4,ge2"},
    {"哪里","na3,li3"},
    {"位置","wei4,zhi4"},
    {"层高","ceng2,gao1"},
    {"几室","ji3,shi4"}
};

#ifndef SAFE_CLOSE_SOCKET
#define SAFE_CLOSE_SOCKET(fd){\
    if (fd > 0){\
        close(fd);\
        fd = 0;\
    }\
}
#endif

static int run_flag = TRUE;     //运行控制变量
SZ_RUN_ARG_S g_sz_run_arg = {0};  //运行参数，通过参数传入

/**  
* @Description:解析进程入参
* @argc[IN]- 参数个数
* @argv[IN]- 参数列表
* @pst_arg[IN/OUT] 解析后的参数
* @return 成功:0 失败:-1
*/
static int get_options(int argc, char *argv[], SZ_RUN_ARG_S *pst_arg)
{
    int ch = 0;  
    int argc_num = 0;
    int i = 0;
    

    if (NULL == pst_arg){
        return -1;
    }

    for(; i< argc; i++){
        printf("argv[%d]: %s", i, argv[i]);
    }
    
    /*解析参数*/
    while ((ch = getopt(argc,argv,"P:L:"))!=-1)  
    {  
        switch(ch)  
        {              
            case 'P': { /*侦听端口*/
                pst_arg->port = atoi(optarg);
                argc_num++;
                break;
            } 
            case 'L':{  /*云服务器地址*/ 
                (void)snprintf(pst_arg->path, sizeof(pst_arg->path), "%s", optarg);
                argc_num++;
                break;  
            }
            default: {
                printf("arguments error!\n");
                break;
            }
        }  
    }

    if((pst_arg->port == 0) || (pst_arg->path[0] == 0) ){
        printf("arguments error!\n You must do like: friso -P 8080 -L /usr/friso.ini");
        return -1;
    }

    return 0;
}

/*
*侦听socket初始化
*/
static int init_listen_sock(unsigned short port, int *listen_socket)
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

/*
*数据发送接口
*/
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
                printf("send to cliend error:%s(errno:%d)", strerror(errno), errno);
                return -1;
            }
        }

        ptr += wrote;
        len -= wrote;
  }

    return 0;
}

/* 
 *子进程退出的时候，会发送SIGCHLD信号，默认的POSIX不响应， 
 *所以，用该函数处理SIGCHLD信号便可，同时使用signal设置处理信号量的规则(或跳转到的函数) 
 */  
static void sig_handle( int num )  
{  
    int status;  
    pid_t pid;  
  
    while( (pid = waitpid(-1,&status,WNOHANG)) > 0)  
    {  
        if ( WIFEXITED(status) )  
        {  
            printf("child process revoked. pid[%6d], exit code[%d]\n",pid, WEXITSTATUS(status));  
        }  
        else { 
            printf("child process revoked.but ...\n");  
        }
    }  
} 
//word:待匹配语句拼音  key:关键词拼音
int compute_edit_distance(char*  word, char*  key)
{
    int similarity, mini_tmp, mini_distance, py_add, py_delete, py_change;
    int len1 = strlen(word);
    int len2 = strlen(key);
    printf("len1=%d", len1);
    printf("len2=%d\n", len2);
    mini_distance = len2;
    if (len1 == 0) {
        printf("<error>len1=%d", len1);
        return -1;
    }
    if (len2 == 0) {
        printf("<error>len2=%d", len2);
        return -1;
    }
    int i = 0;
    int j = 0;
    int matrix[len1 + 1][len2 + 1];
    for ( ; i <= len1; ++i) {
        matrix[i][0] = 0;
    }
    for ( ; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    // 动态规划
    for ( i = 1; i <= len1; ++i) {
        for ( j = 1; j <= len2; ++j) {
            int cost = 0;
            if (word[i - 1] != key[j - 1]) {
                cost = 1;
            }
            //假设所有操作均是对key而言
            py_add = matrix[i][j - 1] + 1;
            py_delete = matrix[i -1][j] + 1;
            py_change = matrix[i -1][j - 1] + cost;

            //matrix[i][j] = std::min(std::min(py_add, py_delete), py_change);
            mini_tmp = (py_delete < py_change) ? py_delete : py_change;
            matrix[i][j] = (py_add < mini_tmp) ? py_add : mini_tmp;
            //printf("py_add:%d,py_delete:%d,py_change:%d,mini:%d\n",py_add, py_delete, py_change, matrix[i][j]);
        }
    }
    for(i = 0; i <= len1; ++i){
        //printf("%d|",matrix[i][len2]);
        if(matrix[i][len2] < mini_distance)
            mini_distance = matrix[i][len2];
    }
    similarity = (len2 - mini_distance) * 100 / len2;
    printf("similarity of %s(word) and %s(key) is :%d\n", word, key, similarity);
    return similarity;    
}
/**  
* @Description:业务处理函数
* @arg: [IN] 
* @return 成功: 0 失败: -1
*/
static int work_child_process( int client_sockfd, friso_t friso, friso_config_t config)
{
    
        friso_task_t task; 
        clock_t s_time, e_time;
        char pinyin[4096] = {0};
        char word[4096] = {0};
        unsigned int idex = 0,j = 0, data_len = 0; 
        
        while(1)
        {
            idex = 0;
            pinyin[0] = 0;
            data_len = data_recv(client_sockfd, word, sizeof(word));
            printf("word:%s,data_len:%d\n",word, data_len);
            //set the task.
            task = friso_new_task();
            friso_set_text( task, word );
            println("分词结果:");
            s_time = clock();
            while ( ( config->next_token( friso, config, task ) ) != NULL ) {
            //printf("word:%s[%d, %d, %d] ", task->token->word, 
            //        task->token->offset, task->token->length, task->token->rlen );
            
                printf("result: word:%s, pinyin:%s, type:%s\n", task->token->word ,task->token->py, word_type[task->token->type]);
                j = 0;
                while( task->token->py[j] != '\0'){
                pinyin[idex++] = task->token->py[j++];
                }  
                pinyin[idex++] = ',';          
            }
            pinyin[--idex] = '\n';
            pinyin[++idex] = '\0';        
            printf("\n完整拼音：%s\n",pinyin);
            data_send(client_sockfd, pinyin, idex); 
            int similarity = -1, sim_tmp, sim_idex;
            char sim_buffer[128];
            for(idex = 0; idex < 5; idex++){
                if((sim_tmp = compute_edit_distance(pinyin, key_test[idex].pinyin)) > 90){
                    snprintf(sim_buffer, 128, "匹配结果：%s, 匹配度：%d \n", key_test[idex].word, sim_tmp);
                    printf("%s",sim_buffer);
                    data_send(client_sockfd, sim_buffer, strlen(sim_buffer) + 1); 
                }
                if(sim_tmp > similarity){
                    similarity = sim_tmp;
                    sim_idex = idex;
                }
                if(idex == 4){
                    if(similarity > 50){
                        snprintf(sim_buffer, 128, "匹配最高结果：%s, 匹配度：%d \n", key_test[sim_idex].word, similarity);
                        printf("%s",sim_buffer);
                        data_send(client_sockfd, sim_buffer, strlen(sim_buffer) + 1); 
                    }else{
                        similarity = 0;
                        printf("none result\n");
                    }
                }
            }   
                            
            e_time = clock();
            printf("\nDone, cost < %fsec\n", ( (double)(e_time - s_time) ) / CLOCKS_PER_SEC );
            friso_free_task( task );
        }
        
        return 0;   
}

int main(int argc, char **argv) 
{

    clock_t s_time, e_time;
    fstring __path__ = NULL, mode = NULL;   
    fd_set server_fd_set;
    int max_fd = -1;
    struct timeval tv;
    pid_t ppid;
    int client_sockfd;
    int ret = 0;


    friso_t friso;
    friso_config_t config;
    
    s_time = clock();

    //initialize
    signal(SIGCHLD, sig_handle);

    if (0 != get_options(argc, argv, &g_sz_run_arg)){
        printf("input parameters parse fail");
        exit(-1);
    }

    if (0 != init_listen_sock(g_sz_run_arg.port, &g_sz_run_arg.listen_fd)){
        printf("init_sock fail");
        exit(-1);
    }

    printf("listen port: %d listen_fd: %d", g_sz_run_arg.port, g_sz_run_arg.listen_fd);

    __path__ = g_sz_run_arg.path;
    printf("lexion path: %s", g_sz_run_arg.path);
    if ( __path__ == NULL ) {
        println("Usage: friso -init lexicon path");
        exit(0);
    }

    friso = friso_new();
    config = friso_new_config();
    
    if ( friso_init_from_ifile(friso, config, __path__) != 1 ) {
        printf("fail to initialize friso and config.\n");
        goto err;
    }

    switch ( config->mode ) {
    case __FRISO_SIMPLE_MODE__:
        mode = "Simple";
        break;
    case __FRISO_COMPLEX_MODE__:
        mode = "Complex";
        break;
    case __FRISO_DETECT_MODE__:
        mode = "Detect";
        break;
    }

    e_time = clock();

    printf("Initialized in %fsec\n", (double) ( e_time - s_time ) / CLOCKS_PER_SEC );
    printf("Mode: %s\n", mode);
    printf("+-Version: %s (%s)\n", friso_version(), friso->charset == FRISO_UTF8 ? "UTF-8" : "GBK" );
    ___ABOUT___;
    
   while(run_flag){
        tv.tv_sec = READ_TIMEOUT_SEC;
        tv.tv_usec = READ_TIMEOUT_USEC;

        FD_ZERO(&server_fd_set);
        FD_SET(g_sz_run_arg.listen_fd, &server_fd_set);
        
        max_fd = g_sz_run_arg.listen_fd;

        switch (select(max_fd+1, &server_fd_set, NULL, NULL, &tv))
        {
            case -1:{ //出错
                if (EINTR != errno){ //EINTR/EAGAIN/EWOULDBLOCK 这几个不能当作错误处理
                    printf("select error:%s(errno:%d)", strerror(errno), errno);
                }
                //run_flag = FALSE;
                break;
            }
                
            case 0: //超时
                break;
                
            default: { //新连接
                if (!FD_ISSET(g_sz_run_arg.listen_fd, &server_fd_set)) { 
                    break;
                }

                if ((client_sockfd = accept(g_sz_run_arg.listen_fd, NULL, NULL)) == -1) {
                    printf("accept send client socket error:%s(errno:%d)", strerror(errno), errno);
                    break;
                }

                ppid = fork();

                if (-1 == ppid){
                    printf("fork error:%s(errno:%d)", strerror(errno), errno);
                    //run_flag = FALSE;
                    break;
                }
                else if (0 == ppid){ //子进程
                    //关闭侦听socket
                    SAFE_CLOSE_SOCKET(g_sz_run_arg.listen_fd);
                    
                    ret = work_child_process(client_sockfd, friso, config);
                        
                    SAFE_CLOSE_SOCKET(client_sockfd);

                    exit(ret);
                }
                else{//父进程
                    //关闭新建socket
                    SAFE_CLOSE_SOCKET(client_sockfd);
                }              
            }
        }
    }    
    SAFE_CLOSE_SOCKET(g_sz_run_arg.listen_fd);

    return 0;

    //error block.
err:
    friso_free_config(config);
    friso_free(friso);
    

    return 0;
}

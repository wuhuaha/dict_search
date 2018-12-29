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
#include "socket_server.h"
#include "key.h"
#include "pinyin.h"

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
/*
extern pinyin_convert_arry pinyin_convert[1459];
extern pinyin_convert_code yunmu_code[37];
extern pinyin_convert_code shengmu_code[24]; 
*/
#define __DOMAIN_NUM__  2
#define __DOMAIN_PUBLIC__ 0
char domain_name[__DOMAIN_NUM__][32] = {
    "public",
    "house"
};

/*service 配置*/
typedef struct run_arg_tag{
    int     listen_fd;      //侦听socket
    int     port;           //端口
    char    path[256];      //配置文件路径
}SZ_RUN_ARG_S;

typedef struct {
    fstring word;//词内容
    fstring pinyin;//词拼音
    fstring label; //标签
    friso_array_t word_list;
} key_entry;

#ifndef SAFE_CLOSE_SOCKET
#define SAFE_CLOSE_SOCKET(fd){\
    if (fd > 0){\
        close(fd);\
        fd = 0;\
    }\
}
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(ptr) \
{ \
    if (NULL != ptr) { \
        free(ptr); \
        ptr = NULL; \
    } \
}
#endif

static int run_flag = TRUE;     //运行控制变量
SZ_RUN_ARG_S g_sz_run_arg = {0};  //运行参数，通过参数传入

int free_key_entry( key_entry *entry )
{
    SAFE_FREE(entry->word)
    SAFE_FREE(entry->pinyin)
    SAFE_FREE(entry->label)
    SAFE_FREE(entry)
    return 0;
}

int free_key_items(friso_array_entry* items)
{
    register int i = 0;
    for(i = 0; i < items->length; i++)
    {
        free_key_entry(*(items->items + i));
    }    
    SAFE_FREE(items->items)
    return 0;
}

int print_key_items(friso_array_entry* items)
{
    register int i = 0, j = 0;
    register key_entry* entry;
    for(i = 0; i < items->length; i++)
    {
        entry = *(items->items + i);
        printf("key[%d]\tword:%s\tlabel:%s\t", i, entry->word, entry->label);
        for(j = 0; j < entry->word_list->length; j++)
            printf("——word_list[%d]:%s——", j, (char *)(*(entry->word_list->items + j)));
        printf("\n");
    }   
    return 0;
}

int rex_string(fstring string,friso_array_entry* items)
{
    register int i = 0, j = 0;
    register key_entry* entry;
    register char *tmp = NULL;
    for(i = 0; i < items->length; i++)
    {
        entry = *(items->items + i);
        for(j = 0; j < entry->word_list->length; j++){
            printf("——word_list[%d]:%s——", j, (char *)(*(entry->word_list->items + j)));    
            tmp = (char *)(*(entry->word_list->items + j));        
            if(strstr(string, tmp) == NULL)
                break;
            /*
            if((tmp_last != NULL) && (tmp < tmp_last))
                break;
            */
            if(j == (entry->word_list->length - 1)){
                return (i + 1);
            }
            printf("flag");                
            //tmp_last = tmp;
        }            
        printf("flag:%d\n",i);
    } 
    return 0;
}

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
            case 'L':{  /*配置文件路径*/ 
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
        printf("arguments error!     You must do like:\nfriso -P 8080 -L /usr/friso.ini\n");
        return -1;
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
/**  
* @Description:业务处理函数
* @arg: [IN] 
* @return 成功: 0 失败: -1
*/
static int work_child_process( int client_sockfd, friso_t *friso_list, friso_config_t *config_list, friso_array_t key_arry)
{
    
        friso_task_t task; 
        clock_t s_time, e_time;
        char pinyin[4096] = {0};
        char label[4096] = {0};
        char word[4096] = {0};        
        char send_buffer[1024];
        unsigned int idex = 0,j = 0, data_len = 0; 

        memset(word, 0, sizeof(word));
        data_len = data_recv(client_sockfd, word, sizeof(word));
        idex = atoi(word) < __DOMAIN_NUM__  ? atoi(word) : __DOMAIN_PUBLIC__;
        printf("domain:%d\n",idex);
        friso_t friso  = friso_list[idex]; 
        friso_config_t config = config_list[idex];

        while(1)
        {
            idex = 0;
            pinyin[0] = 0;
            *label = 0;
            memset(word, 0, sizeof(word));
            data_len = data_recv(client_sockfd, word, sizeof(word));
            printf("word:%s,data_len:%d\n",word, data_len);
            //set the task.
            task = friso_new_task();
            friso_set_text( task, word );
            println("分词结果:");
            s_time = clock();
            snprintf(send_buffer, sizeof(send_buffer), "包含词:");
            data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1);
            while ( ( config->next_token( friso, config, task ) ) != NULL ) {
            //printf("word:%s[%d, %d, %d] ", task->token->word, 
            //        task->token->offset, task->token->length, task->token->rlen );
                printf("result: word:%s, pinyin:%s, type:%s\n", task->token->word ,task->token->py, word_type[task->token->type]);
                if(*task->token->label != 0){
                    printf("标签：%s\n", task->token->label);
                    if(*label != 0)
                        strcat(label, "|");
                    strcat(label, task->token->label);    
                }
                snprintf(send_buffer, sizeof(send_buffer), "%s  ", task->token->word);
                data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1);
                j = 0;
                *task->token->label = 0;
                while( task->token->py[j] != '\0'){
                pinyin[idex++] = task->token->py[j++];
                }  
                pinyin[idex++] = ',';      
            }
            printf("\n");
            data_send(client_sockfd, "\n", 2);
            pinyin[--idex] = '\0';   
            if(*label != 0){
                snprintf(send_buffer, sizeof(send_buffer), "直接匹配标签：[%s]\n", label);
                data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1); 
            }
            if(*label == 0)
            {
                if((idex = rex_string(word, key_arry)) > 0){
                    key_entry* entry = *(key_arry->items + idex - 1);
                    snprintf(label, sizeof(label), "%s", entry->label);
                    snprintf(send_buffer, sizeof(send_buffer),"正则匹配项：[%s], 正则匹配标签：[%s]\n", entry->word, entry->label);
                    data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1); 
                }else{
                    *label = 0;
                }
            }
            if(*label == 0)
            {
                int similarity = -1, sim_tmp, sim_idex = 0;
                for(idex = 0; idex < key_fangchan.key_num; idex++){
                    if((sim_tmp = compute_code_edit_distance(pinyin, key_fangchan.key_list[idex].pinyin)) == 100){
                        snprintf(send_buffer, sizeof(send_buffer), "拼音完全匹配，匹配结果:%s,标签:%s\n", key_fangchan.key_list[idex].word, key_fangchan.key_list[idex].label);
                        printf("%s",send_buffer);
                        data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1); 
                        //break;
                    }
                    if(sim_tmp > similarity){
                        similarity = sim_tmp;
                        sim_idex = idex;
                        printf("目前匹配度最高为：%d\n",similarity);
                    }
                    if((idex == key_fangchan.key_num - 1) && (similarity < 100)){
                        if(similarity > 50){
                            snprintf(send_buffer, sizeof(send_buffer) , "拼音非完整匹配, 拼音匹配最高结果:[%s],标签:[%s],匹配度:%d \n", key_fangchan.key_list[sim_idex].word,key_fangchan.key_list[sim_idex].label, similarity);
                            printf("%s",send_buffer);
                            data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1); 
                        }else{
                            snprintf(send_buffer, sizeof(send_buffer), "无匹配项\n");
                            printf("%s",send_buffer);
                            data_send(client_sockfd, send_buffer, strlen(send_buffer) + 1); 
                            similarity = 0;
                            printf("none result\n");
                        }
                    }
                }
            }  
                            
            e_time = clock();
            printf("\nDone, cost < %fsec\n", ( (double)(e_time - s_time) ) / CLOCKS_PER_SEC );
            friso_free_task( task );
        }
        
        return 0;   
}

int add_dict_to_arry(char *file_path,friso_array_entry *items)
{
    FILE *fd;
    char line_buffer[512], buffer[256];
    fstring line;
    string_split_entry sse;
    fd = fopen(file_path, "r+");
    while((line = file_get_line(line_buffer, fd)) != NULL)
    {
        //printf("get %s from file \n",line);
        string_split_reset( &sse, "/", line);
        if ( string_split_next( &sse, buffer ) == NULL ) {
                continue;
        }
        key_entry* entry_test = (key_entry*)malloc(sizeof(key_entry));
        array_list_add(items, entry_test);

        entry_test->word = string_copy_heap(buffer, strlen(buffer));
        //printf("word:%s\n",entry_test->word);

        string_split_next( &sse, buffer );
        if ( strcmp(buffer, "null") != 0 ) {
            //printf("syn:%s\n",buffer);
        }
        string_split_next( &sse, buffer );
        if ( strcmp(buffer, "null") != 0 ) {
            //printf("fre:%s\n",buffer);
        }
        string_split_next( &sse, buffer );
        if ( strcmp(buffer, "null") != 0 ) {
            entry_test->pinyin = string_copy_heap(buffer, strlen(buffer));
            //printf("pinyin:%s\n",entry_test->pinyin);
        }else{
            entry_test->pinyin = NULL;
        }
        string_split_next( &sse, buffer );
        if ( strcmp(buffer, "null") != 0 ) {
            entry_test->label = string_copy_heap(buffer, strlen(buffer));
            //printf("label:%s\n",entry_test->label);
        }
        entry_test->word_list = new_array_list_with_opacity(2);
        string_split_reset( &sse, "*", entry_test->word);
        while(string_split_next( &sse, buffer ) != NULL)
        {
            fstring word = string_copy_heap(buffer, strlen(buffer));
            array_list_add(entry_test->word_list, word);
            //printf("%s",buffer);
        } 
        //printf("\n");
        //free_key_entry(entry_test);
    }
    return 0;
}

int main(int argc, char **argv) 
{

    clock_t s_time, e_time;
    fstring  mode = NULL;   
    char path[512] = {0};
    fd_set server_fd_set;
    int max_fd = -1;
    struct timeval tv;
    pid_t ppid;
    int client_sockfd;
    int ret = 0;
    friso_array_t key_arry = new_array_list_with_opacity(512);

    friso_t friso_list[__DOMAIN_NUM__];
    friso_config_t config_list[__DOMAIN_NUM__];
    
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

    register int i = 0;
    for(i = 0 ;i < __DOMAIN_NUM__; i++)
    {
        friso_list[i] = friso_new();
        config_list[i] = friso_new_config();
        snprintf(path, sizeof(path), "%s/%s.ini", g_sz_run_arg.path, domain_name[i]);

        if ( path == NULL ) {
            println("Usage: friso -init lexicon path");
            exit(0);
        }

        if ( friso_init_from_ifile(friso_list[i], config_list[i], path) != 1 ) {
            printf("fail to initialize friso and config.\n");
            goto err;
        }

        switch ( config_list[i]->mode ) {
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
        printf("+-Version: %s (%s)\n", friso_version(), friso_list[i]->charset == FRISO_UTF8 ? "UTF-8" : "GBK" );

    }
     //test_py
    char code[64], single[64];
    py_to_code("yuan2,jiao3,fen1", code, single);
    printf("code:%s;single:%s\n", code, single);

    add_dict_to_arry("/root/dict/house/house_rex.txt", key_arry);
    printf("there %d arry in key_arry\n", key_arry->length);
    //print_key_items(key_arry);

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
                    
                    ret = work_child_process(client_sockfd, friso_list, config_list, key_arry);
                        
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
    for(i = 0 ;i < __DOMAIN_NUM__; i++)
    {
        friso_free_config(config_list[i]);
        friso_free(friso_list[i]);  
    }      

    free_key_items(key_arry);

    return 0;
}

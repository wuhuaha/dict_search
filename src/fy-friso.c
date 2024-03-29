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
#include "pinyin.h"
#include "zlog.h"
#include "nlp_log.h"


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
    "CJK_WORDS",    //0
    "CJK_UNITS",    //1
    "ECM_WORDS",    //2
    "CEM_WORDS",    //3
    "CN_LNAME",     //4
    "CN_SNAME",     //5
    "CN_DNAME1",    //6
    "CN_DNAME2",    //7
    "CN_LNA",       //8
    "STOPWORDS",    //9
    "ENPUN_WORDS",  //10
    "EN_WORDS",     //11
    "OTHER_WORDS",  //12
    "NCSYN_WORDS",  //13
    "PUNC_WORDS",   //14
    "UNKNOW_WORDS"  //15
};

zlog_category_t * sa_log = NULL;

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

int print_key_items(friso_array_entry* items)
{
    register int i = 0, j = 0;
    if (items == NULL){
        return 0;
    }
    register key_rex_entry_t entry;
    for(i = 0; i < items->length; i++)
    {
        entry = *(items->items + i);
        log_debug(sa_log, "main", "key[%d]\tword:%s\tlable:%s\t", i, entry->word, entry->lable);
        for(j = 0; j < entry->word_list->length; j++)
            log_debug(sa_log, "main", "——word_list[%d]:%s——", j, (char *)(*(entry->word_list->items + j)));
        log_debug(sa_log, "main", "");
    }   
    return 0;
}

static int rex_string(fstring string,friso_array_entry* items, int* result)
{
    register int i = 0, j = 0;
    register key_rex_entry_t entry;
    register char *tmp = NULL;
    if( (items == NULL) || (result == NULL) ){
        return 0;
    }
    for(i = 0; i < items->length; i++)
    {
        entry = *(items->items + i);
        for(j = 0; j < entry->word_list->length; j++){
            //log_debug(sa_log, "main", "——word_list[%d]:%s——", j, (char *)(*(entry->word_list->items + j)));    
            tmp = (char *)(*(entry->word_list->items + j));        
            if(strstr(string, tmp) == NULL)
                break;
            if(j == (entry->word_list->length - 1)){
                *result = i;
                return 1;
            }                

        }     
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
        log_debug(sa_log, "main", "argv[%d]: %s", i, argv[i]);
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
                log_err(sa_log, "main", "arguments error!");
                break;
            }
        }  
    }

    if((pst_arg->port == 0) || (pst_arg->path[0] == 0) ){
        log_err(sa_log, "main", "arguments error!     You must do like:friso -P 8080 -L /usr/friso.ini");
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
            log_debug(sa_log, "main", "child process revoked. pid[%6d], exit code[%d]",pid, WEXITSTATUS(status));  
        }  
        else { 
            log_debug(sa_log, "main", "child process revoked.but ...");  
        }
    }  
} 

/**  
* @Description:解析读取到的数据
* @arg: [IN] 
* @return 成功: 0 失败: -1
*/
static int  analy_buffer(char *buffer, char **class, char **word)
{
    register int i = 0;
    *word = buffer;
    *class = domain_name[0];

    if( (buffer == NULL) || (*buffer == 0)){
        log_err(sa_log, "main", "buffer is NULL!");
        return -1;
    }
    if( strchr(buffer, '#') == NULL){
        return 0;
    }
    while(buffer[i] != 0){
        if(buffer[i] == '#'){
            break;
        }
        i++;
    }
    buffer[i] = 0;
    *class = buffer;
    *word = buffer + i + 1;
    return 0;
}

/**  
* @Description:获取类型对应的编号
* @arg: [IN] 
* @return 成功: 对应的编号 失败: -1
*/

static int num_of_class(char *class, friso_array_t friso_array)
{
    register int  j = 0;
	friso_t friso_tmp = NULL;
    if( (class == NULL) || (*class == 0)){
        log_err(sa_log, "main", "class is NULL!\t set it as base");
        return 0;
    }
    for(j = 0; j < friso_array->length; j++){
		friso_tmp = array_list_get(friso_array, j);
		if(strcmp(friso_tmp->domain, class) == 0)
			return j;
	}
    return 0;
}

/**  
* @Description:业务处理函数
* @arg: [IN] 
* @return 成功: 0 失败: -1
*/
static int work_child_process( int client_sockfd, friso_array_t friso_array, friso_config_t config)
{
    
        friso_task_t task; 
        clock_t s_time, e_time;
        char read_buffer[8192],  send_buffer[8192];
        char send_buffer_tmp[8192], detail[8192];   
        char pinyin[2048] = {0}, lable[128] = {0};
        register int j = 0, k = 0; 
        int data_len = 0, similarity = 0, idex = 0, confidence = 0;
        char *class, *word, *word_lable;
        *send_buffer = *send_buffer_tmp = *detail = 0;
		friso_t friso_tmp;

        //读取数据，并将其中的领域名及主要内容分析出
        memset(read_buffer, 0, sizeof(read_buffer));
        data_len = data_recv(client_sockfd, read_buffer, sizeof(read_buffer));
        log_debug(sa_log, "main", "read_buffer:%s, data_len:%d", read_buffer, data_len);
        analy_buffer(read_buffer, &class, &word);
        idex = num_of_class(class, friso_array);
		friso_tmp = array_list_get(friso_array, idex);
        log_debug(sa_log, "main", "domain:%s, idex:%d, domain_name:%s", class, idex, friso_tmp->domain);
        log_info(sa_log, class, "word:%s",word);

        //根据领域，选取相应的firso结构
        friso_t friso  = array_list_get(friso_array, idex); 
        //set the task.
        task = friso_new_task();
        friso_set_text( task, word );
        s_time = clock();

        //分词主程序
        while ( ( config->next_token( friso, config, task ) ) != NULL ) {
            //本次token结果
           //printf("word:%s[%d, %d, %d] syn:%s", task->token->word, 
           //         task->token->offset, task->token->length, task->token->rlen, task->token->syn);
            if(task->token->type >= 9){
                log_debug(sa_log, class, "result: word:%s, pinyin:%s, type:%s", task->token->word ,task->token->py, word_type[task->token->type]);
                log_debug(sa_log, class, "不是汉字，跳过!");
                continue;
            }
            word_lable = NULL;
            log_debug(sa_log, class, "result: word:%s, pinyin:%s, type:%s", task->token->word ,task->token->py, word_type[task->token->type]);
            snprintf(send_buffer_tmp, sizeof(send_buffer_tmp),"{\"word\":\"%s\",\"pinyin\":\"%s\"", task->token->word, task->token->py);
            if(*detail == 0){
                strcat(detail, "[");
            }else{
                strcat(detail, ",");
            }
            strcat(detail, send_buffer_tmp);
            //如果有对应的标签则进行打印
            if(*task->token->lable != 0){
                confidence = 100;
                snprintf(send_buffer_tmp, sizeof(send_buffer_tmp),",\"lable\":\"%s\",\"confidence\":\"%d\",\"key_word\":\"%s\",\"lable_type_alias\":\"直接匹配\"}", task->token->lable, confidence, task->token->word);
                strcat(detail, send_buffer_tmp);
                    //已经存在标签了则添加在尾部
                if(*lable != 0){
                    strcat(lable, "|");
                }
                strcat(lable, task->token->lable);    
            }else{//没有对应的标签                    
                    if(*task->token->syn != 0){   //如果含有同义词，则查询同义词的标签              
                        link_node_t node, next;
                        lex_entry_t entry;
                        j = 0;
                        for ( node = task->token->syn_list->head; node != NULL; ) {   //依次查询所有的同义词有没有标签   
                            if(node->value != NULL){
                                entry = (lex_entry_t)(node->value);
                                if((entry->lable != NULL)&&(strcmp(entry->lable, "null") != 0)){
                                    word_lable = entry->lable;              //单个词的
                                    //将标签添加到整句的标签的尾部
                                    if(*lable != 0)
                                        strcat(lable, "|");
                                    strcat(lable, entry->lable);
                                    break;      //只要有一个同义词含有标签就退出
                                }
                            }  
                            next = node->next;
                            node = next;
                        }
                        if(word_lable == NULL){
                            //所有同义词             
                            snprintf(send_buffer_tmp, sizeof(send_buffer_tmp),",\"syn\":\"%s\"}", task->token->syn);
                            strcat(detail, send_buffer_tmp);
                        }else{
                            //查询到同义词有标签的话，输出同义词及其标签
                            confidence = 100;
                            snprintf(send_buffer_tmp, sizeof(send_buffer_tmp),",\"syn\":\"%s\",\"lable\":\"%s\",\"confidence\":\"%d\",\"key_word\":\"%s\",\"lable_type_alias\":\"直接匹配\"}", task->token->syn, entry->lable, confidence, entry->word);
                            strcat(detail, send_buffer_tmp);
                        }
                   
                    }else{
                        strcat(detail, "}");//既没有同义词也没有标签
                    }
            }
            
            j = 0;
            *task->token->lable = 0;
            //复制本token的拼音到尾部
            while( task->token->py[j] != '\0'){
                pinyin[k++] = task->token->py[j++];
            }  
            pinyin[k++] = ',';      
        }
        pinyin[--k] = '\0'; 
        log_debug(sa_log, class, "pinyin：%s",pinyin);

        //改为必然进行正则匹配
        if(1)
        {
                if((rex_string(word, friso->domain_rex, &idex)) > 0){
                    key_rex_entry* entry = *(friso->domain_rex->items + idex);//将标签添加到整句的标签的尾部
                    if(*lable != 0)
                        strcat(lable, "|");
                    strcat(lable, entry->lable);
                    if(confidence < 95 ) confidence = 95;
                    snprintf(send_buffer_tmp, sizeof(send_buffer_tmp),",{\"lable\":\"%s\",\"confidence\":\"%d\",\"key_word\":\"%s\",\"lable_type_alias\":\"正则匹配\"}", entry->lable, 95, entry->word);
                    strcat(detail, send_buffer_tmp);
                }
        }

        //if(*lable == 0) //因为拼音匹配比较消耗资源，依旧没有标签，才进行拼音匹配
        if(1) //调试阶段，必然进行拼音匹配吧
        {
                if((similarity = search_pinyin(pinyin, friso->domain_pinyin, &idex, word)))
                {
                    lex_entry_t  entry = *(friso->domain_pinyin->items + idex);
                    if(strstr(lable, entry->lable) == NULL){
                        if(*lable != 0)
                            strcat(lable, "|");
                        strcat(lable, entry->lable);
                        snprintf(send_buffer_tmp, sizeof(send_buffer_tmp), "%s", (similarity == 100) ? "拼音完全匹配" : ((similarity == 89) ? "拼音分隔匹配" : ((similarity == 87) ? "拼音发音类型匹配" : "拼音非完全匹配")) );
                        similarity = (int)(similarity * 0.9);
                        snprintf(send_buffer, sizeof(send_buffer),",{\"lable\":\"%s\",\"confidence\":\"%d\",\"key_word\":\"%s\",\"lable_type_alias\":\"%s\"}", entry->lable, similarity, entry->word, send_buffer_tmp);
                        strcat(detail, send_buffer);
                        if(confidence < similarity){
                            confidence = similarity;
                        }
                    }
                }
                if((similarity = search_pinyin_rex(pinyin, friso->domain_rex, &idex, word)))
                {
                    key_rex_entry* entry = *(friso->domain_rex->items + idex);
                    if(strstr(lable, entry->lable) == NULL){
                        if(*lable != 0)
                            strcat(lable, "|");
                        strcat(lable, entry->lable);
                        snprintf(send_buffer_tmp, sizeof(send_buffer_tmp), "%s", (similarity == 100) ? "拼音完全匹配" : ((similarity == 89) ? "拼音分隔匹配" : ((similarity == 87) ? "拼音发音类型匹配" : "拼音非完全匹配")) );
                        similarity = (int)(similarity * 0.8);
                        snprintf(send_buffer, sizeof(send_buffer),",{\"lable\":\"%s\",\"confidence\":\"%d\",\"key_word\":\"%s\",\"lable_type_alias\":\"%s\"}", entry->lable, similarity, entry->word, send_buffer_tmp);
                        strcat(detail, send_buffer);
                        if(confidence < similarity){
                            confidence = similarity;
                        }
                    }
                }
        }  

        //计算用时
        e_time = clock();
        //log_debug(sa_log, "main", "Done, cost < %fsec", ( (double)(e_time - s_time) ) / CLOCKS_PER_SEC );

        strcat(detail, "]");
        log_debug(sa_log, class, "%s", detail);

        //匹配失败
        if(*lable == 0)
        {                
                snprintf(send_buffer, sizeof(send_buffer), "{\"lable\":\"null\", \"confidence\":\"null\", \"cost_time\":\"%f\", \"detail\":%s}",( (double)(e_time - s_time) ) / CLOCKS_PER_SEC, detail);
                data_send(client_sockfd, send_buffer, strlen(send_buffer));
        }else{            
            snprintf(send_buffer, sizeof(send_buffer), "{\"lable\":\"%s\", \"confidence\":\"%d\", \"cost_time\":\"%f\", \"detail\":%s}", lable, confidence , ( (double)(e_time - s_time) ) / CLOCKS_PER_SEC, detail);
            data_send(client_sockfd, send_buffer, strlen(send_buffer));
        }
        //记录结果
        log_info(sa_log, class, "%s", send_buffer);
        friso_free_task( task );
        
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
	int j = 0;
    
    friso_array_t friso_array = new_array_list_with_opacity( 12);
	friso_t friso_tmp;
    friso_config_t config = friso_new_config();;
    
    s_time = clock();

    //initialize
    signal(SIGCHLD, sig_handle);

    if (search_zlog_init(&sa_log) != 0) {
		log_err(sa_log, "main", "init failed");
		return -1;
	}

    if (0 != get_options(argc, argv, &g_sz_run_arg)){
        log_err(sa_log, "main", "input parameters parse fail");
        exit(-1);
    }

    if (0 != init_listen_sock(g_sz_run_arg.port, &g_sz_run_arg.listen_fd)){
        log_err(sa_log, "main", "init_sock fail");
        exit(-1);
    }

    log_debug(sa_log, "main", "listen port: %d listen_fd: %d", g_sz_run_arg.port, g_sz_run_arg.listen_fd);

    snprintf(path, sizeof(path), "%s", g_sz_run_arg.path);

     if ( path == NULL ) {
            println("Usage: friso -init lexicon path");
            exit(0);
    }

    if ( friso_mysql_init_from_ifile(friso_array, config, path) != 1 ) {
            log_err(sa_log, "main", "fail to initialize friso and config.");
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
	friso_tmp = array_list_get(friso_array, 0);
    log_debug(sa_log, "main", "Initialized in %fsec", (double) ( e_time - s_time ) / CLOCKS_PER_SEC );
    log_debug(sa_log, "main", "Mode: %s", mode);
    log_debug(sa_log, "main", "+-Version: %s (%s)", friso_version(), friso_tmp->charset == FRISO_UTF8 ? "UTF-8" : "GBK" );
	printf("Initialized in %fsec\t", (double) ( e_time - s_time ) / CLOCKS_PER_SEC );
	printf("Mode: %s\t", mode);
	printf("%s\n", friso_tmp->charset == FRISO_UTF8 ? "UTF-8" : "GBK" );

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
                    log_debug(sa_log, "main", "select error:%s(errno:%d)", strerror(errno), errno);
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
                    log_err(sa_log, "main", "accept send client socket error:%s(errno:%d)", strerror(errno), errno);
                    break;
                }

                ppid = fork();

                if (-1 == ppid){
                    log_err(sa_log, "main", "fork error:%s(errno:%d)", strerror(errno), errno);
                    //run_flag = FALSE;
                    break;
                }
                else if (0 == ppid){ //子进程
                    //关闭侦听socket
                    SAFE_CLOSE_SOCKET(g_sz_run_arg.listen_fd);
                    
                    ret = work_child_process(client_sockfd, friso_array, config);
                        
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
    for(j = 0; j < friso_array->length; j++){
		friso_tmp = array_list_get(friso_array, j);
		friso_free(friso_tmp);
	}     

    nlp_zlog_uninit(&sa_log);
    return 0;
}

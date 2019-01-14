#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "zlog.h"
#include "nlp_log.h"

#define YFS_ZLOG_BUFFER_SIZE    1024

int nlp_zlog_init(zlog_category_t** cat, const char* log_path, const char* log_conf_path)
{
    char mk_log_path[512];

    if (0 > access(log_path, F_OK)){
        sprintf(mk_log_path, "mkdir -p %s", log_path);
        system(mk_log_path);
    }
    
    if (zlog_init(log_conf_path)){
        printf("zlog_init fail\n");
        return -1;
    }

    *cat = zlog_get_category("my_cat");
	if (!(*cat)) {
		printf("zlog_get_category fail\n");
		zlog_fini();
		return -1;
	}

    if(zlog_put_mdc("class", "default")){
        printf("zlog_put_mdc(class) fail\n");
		zlog_fini();
		return -1;
    }

    return 0;
}

void nlp_zlog_uninit(zlog_category_t** cat)
{
    zlog_fini();
    *cat = NULL;    
}


int nlp_zlog_mdc_set()
{
    return zlog_put_mdc("class", "default");
}

void nlp_zlog_mdc_remove()
{
    zlog_remove_mdc("class");
    return;
}


int nlp_log_class(zlog_category_t * cat, int level, const char *class, const char *fmt, ...)
{
    va_list ap;
    char log_buf[YFS_ZLOG_BUFFER_SIZE] = {0};

    va_start (ap, fmt);                 
    (void)vsnprintf(log_buf, YFS_ZLOG_BUFFER_SIZE, fmt, ap);
    va_end(ap);

    if (!cat){
        fprintf(stdout, "%s", log_buf);
        return 0;
    }

    zlog_put_mdc("class", class);

    switch(level){
        case DBG_FATAL:
            zlog_fatal(cat, "%s", log_buf);
            break;

        case DBG_ERR:
            zlog_error(cat, "%s", log_buf);
            break;

        case DBG_WARN:
            zlog_warn(cat, "%s", log_buf);
            break;

        case DBG_NOTICE:
            zlog_notice(cat, "%s", log_buf);
            break;

         case DBG_INFO:
            zlog_info(cat, "%s", log_buf);
            break;

        case DBG_DEBUG:
            zlog_debug(cat, "%s", log_buf);
            break;

        default:   
            zlog_debug(cat, "%s", log_buf);    
            break;
    }

    return 0;
}

int search_zlog_init(zlog_category_t** cat)
{
    int ret = -1;
    ret = nlp_zlog_init(cat, "/nlp/search/log", "/nlp/nlp_log.conf");
    return ret;
}
/*
int main(int argc, char** argv)
{
    zlog_category_t * cat = NULL;

	if (search_zlog_init(&cat) != 0) {
		printf("init failed\n");
		return -1;
	}

    nlp_log_fatal(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_warn(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_notice(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_info(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_err(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_debug(cat, "house", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_err(cat, "bank", "test-info-%s-%d-%c", "test_info", 34, 'a');
    nlp_log_debug(cat, "car", "test-info-%s-%d-%c", "test_info", 34, 'a');

	nlp_zlog_uninit(&cat);
	
	return 0;
}
*/

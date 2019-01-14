#ifndef __NLP_LOG_H__
#define __NLP_LOG_H__

    typedef enum log_level_flag
    {
        DBG_FATAL = 0x0,
        DBG_ERR   = 0x01,
        DBG_WARN  = 0x02,
        DBG_NOTICE = 0x04,
        DBG_INFO = 0x08,
        DBG_DEBUG = 0x10,
        DBG_ALL  = DBG_FATAL|DBG_ERR|DBG_WARN|DBG_NOTICE|DBG_INFO|DBG_DEBUG
    }YFS_LOG_LEVEL_E;

    #define log_fatal(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_FATAL, class, fmt,  ##arg);
    #define log_err(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_ERR, class, fmt,  ##arg);
    #define log_warn(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_WARN, class, fmt,  ##arg);
    #define log_notice(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_NOTICE, class, fmt,  ##arg);
    #define log_info(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_INFO, class, fmt,  ##arg);
    #define log_debug(cat, class, fmt, arg...)   nlp_log_class(cat, DBG_DEBUG, class, fmt,  ##arg);

    #define YFS_ZLOG_BUFFER_SIZE    1024

    int nlp_zlog_init(zlog_category_t** cat, const char* log_path, const char* log_conf_path);

    int search_zlog_init(zlog_category_t** cat);

    void nlp_zlog_uninit(zlog_category_t** cat);

    int nlp_log_class(zlog_category_t * cat, int level, const char *class, const char *fmt, ...);

#endif
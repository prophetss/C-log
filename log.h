#ifndef _LOG_H_
#define _LOG_H_

#define LOG_SUCCESS	0
#define LOG_FAILED	-1

#define END ""    /*可变参末尾标识*/
#define INVAILD_LINE_NUM	-1	/* 非法行号，用于非调试打印输入 */

/*运行级别*/
#define LOG_DEBUG	   0
#define LOG_RUN        1
#define LOG_WARNING    2
#define LOG_ERROR	   3


/*********************************用户配置*********************************/

#define RUN_LEVEL (LOG_DEBUG)    /*运行级别：0调试模式，1全输出，2输出告警和错误，3输出错误，4全不输出*/

#define MAX_FILE_SIZE (1024*1024*2)    /*日志文件大小最大上限*/

#define MBUF_MAX 160    /*每次打印可输出最大字符串长度*/

#define RUN_LOG_PATH "run_log.log"    /*日志文件路径*/

#define RUN_LOG_BAK_PATH "run_log_bak.log"    /*备份日志文件路径*/

#define IOBUF_SIZE	1024	/*设置写用户IO缓存大小，不需要时删除此宏即可*/

#define TRACE_PRINT_PATH 	"trace_info"	/*堆栈打印输出路径*/

#define TRACE_SIZE	(1024)	/*堆栈打印大小限制*/

/*********************************用户配置*********************************/


/* 日志加密 password-密码（任意长字符串）, in_filepath-待加密文件路径，out_filepath-加密后文件输出路径*/
void cipher_log(const char *password, const char *in_filepath, const char *out_filepath);

/* 日志解密 password-密码（必须与对应加密密码相同）, in_filepath-待解密文件路径，out_filepath-解密后文件输出路径*/
void decipher_log(const char *password, const char *in_filepath, const char *out_filepath);	

/* MD5散列，filepath-待散列文件路径，digest-散列输出128位 */
void md5_log(const char* filepath, unsigned char *digest);

/*过滤打印,level:运行级别{0，1，2，3}*/
#define write_log(level, ...) do { \
	if (level - RUN_LEVEL < 0)	\
		break; \
	if (level == LOG_DEBUG)	\
		run_log(__LINE__, __FILE__, __VA_ARGS__, END);	\
   	else	\
   		run_log(INVAILD_LINE_NUM, __VA_ARGS__, END); \
    } while(0)

/*日志打印,可输入变参字符串，不可输入空字符串，否则会发生截断*/
void run_log(int line_num, ...);

#endif	/*!_LOG_H_*/
#ifndef _LOG_H_
#define _LOG_H_

#define LOG_SUCCESS	0
#define LOG_FAILED	-1

#define RUN_LEVEL (0)    /*运行级别：0全输出，1输出告警和错误，2输出错误，3全不输出*/

#define MAX_FILE_SIZE (1024*1024*2)    /*日志文件大小最大上限*/

#define MBUF_MAX 160    /*输入最大字符串长度*/

#define RUN_LOG_PATH "run_log.log"    /*日志文件路径*/

#define RUN_LOG_BAK_PATH "run_log_bak.log"    /*备份日志文件路径*/

#define END ""    /*可变参末尾标识*/

#define IOBUF_SIZE	1024	/*设置写IO缓存大小*/

/*运行级别*/
#define LOG_DEBUG	   0
#define LOG_RUN        1
#define LOG_WARNING    2
#define LOG_ERROR	   3

/*非法行号，用于非调试打印输入*/
#define INVAILD_LINE_NUM	-1

/*日志打印对外接口，过滤打印,level:LOG_LEVEL*/
#define write_log(level, ...) do { \
	if (level - RUN_LEVEL < 0)	\
		break; \
	if (level == LOG_DEBUG)	\
		run_log(__LINE__, __FILE__, __VA_ARGS__, END);	\
   	else	\
   		run_log(INVAILD_LINE_NUM, __VA_ARGS__, END); \
    } while(0)

/* 日志打印内部接口，变参中不可输入空字符串，否则会发生截断 */
void run_log(int line_num, ...);

#endif	/*!_LOG_H_*/
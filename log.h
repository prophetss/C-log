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
#define LOG_RUN        0
#define LOG_WARNING    1
#define LOG_ERROR	   2

/*过滤打印,level:运行级别{0，1，2}*/
#define WRITE_LOG(level, ...) do{ \
    if (level - RUN_LEVEL < 0) \
        break; \
    run_log(__FILE__, __LINE__, __VA_ARGS__, END); \
    } while(0)

/*日志打印*/
extern void run_log(const char* file_name, int line_num, ...);

/* 计时器数量=TIMEKEEPER_NUM-2 */
#define TIMEKEEPER_NUM	20

/* 计时器设置与获取n - [1，TIMEKEEPER_NUM-1] */
extern void timekeeper_set(int n);
extern double timekeeper_get(int n);

#endif	/*!_LOG_H_*/
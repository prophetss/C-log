#ifndef _CONFIG_H_
#define _CONFIG_H_


/*********************************用户配置*********************************/

/*运行级别：0调试模式，1全输出，2输出告警和错误，3输出错误，4全不输出*/
#define RUN_LEVEL			0    			

/*日志文件最大上限*/
#define LOG_FILE_SIZE_MAX	(1024*1024)    

/*每条日志最大长度*/
#define LOG_BUF_MAX			160    			 

/*日志文件路径*/
#define LOG_PATH			"run_log.log"    

/*备份日志文件数量上限, 可以设置为0--没有备份*/
#define LOG_BAK_MAX			10   			 

/*日志打印IO缓存大小，不需要时设置为0*/
#define LOG_IO_BUF_MAX		0		 	 

/*堆栈打印文件输出路径*/
#define TRACE_PRINT_PATH	"trace_info"	 

/*堆栈打印大小限制*/
#define TRACE_BUF_MAX		1024			 
		 
/*********************************用户配置*********************************/


#endif	/*!_CONFIG_H_*/
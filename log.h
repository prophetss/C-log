#ifndef _LOG_H_
#define _LOG_H_


#include "config.h"


/*运行级别*/
#define LOG_DEBUG	   0
#define LOG_RUN        1
#define LOG_WARNING    2
#define LOG_ERROR	   3

/*可变参末尾标识*/
#define END 				""

/*非法行号，用于非调试打印输入*/   
#define INVAILD_LINE_NUM	-1	  


/*日志加密密码设置（任意字符串)，设置后对新生成日志文件生效，开始加密*/
int set_key(const char *password, int len);

/*日志解密 password-密码（必须与对应加密密码相同）, in_filepath-待解密文件路径*/
int decipher_log(const char *password, int pw_len, const char *in_filepath);

/*MD5散列，filepath-待散列文件路径，digest-散列输出，外部申请内存，128bit，16byte*/
void md5_log(const char* filepath, unsigned char *digest);

/*内部申请内存，与上接口不同的是散列结果将每4位转化成对应16进制表示符，一共32个字符*/
char* md5_log_s(const char* filepath, char *buf);

/*日志解压，不删除源文件*/
int decompress_log(const char *src_filepath, const char *dst_filepath);


/*日志打印对外接口，过滤打印,level:运行级别{0，1，2，3}*/
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
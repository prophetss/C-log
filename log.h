#ifndef _LOG_H_
#define _LOG_H_


#include <pthread.h>
#include <stdio.h>
#include <stdint.h>


// log level

/* print debug msg(time, lineno, filepath, tid)
 * and all level msg
 */
#define LOG_DEBUG	0 

/* print log_error,log_warn and log_info */
#define LOG_INFO	1

/* print log_error and log_warn */
#define LOG_WARN	2

/* only print log_error */
#define LOG_ERROR	3

/* close logging function and do nothing ,
 * the value is 9 for extendion
 */
#define LOG_CLOSE	9


/***************user configuration***************/

/* log level */
#define LOG_LEVEL			LOG_DEBUG

/* max size(byte) of every log*/
#define SINGLE_LOG_SIZE		1024

/* trace printed path for exit signal */
#define TRACE_PRINT_PATH	"trace_info"

/***************user configuration***************/


/*removing the unnecessary log at compile time based on the log level*/
#define UNUSED		((void)0)

#if (LOG_LEVEL >= LOG_INFO)
	#define _log_debug(lh, format, ...)		UNUSED
#else
	#define _log_debug(lh, format, ...)		write_log(lh, LOG_DEBUG, format, __VA_ARGS__)
#endif

#if (LOG_LEVEL >= LOG_WARN)
	#define _log_info(lh, format, ...)		UNUSED
#else 
	#define _log_info(lh, format, ...)		write_log(lh, LOG_INFO, format,  __VA_ARGS__)
#endif

#if (LOG_LEVEL >= LOG_ERROR)
	#define _log_warn(lh, format, ...)		UNUSED
#else
	#define _log_warn(lh, format, ...)		write_log(lh, LOG_WARN, format,  __VA_ARGS__)
#endif

#if (LOG_LEVEL >= LOG_CLOSE)
	#define _log_error(lh, format, ...)		UNUSED
#else
	#define _log_error(lh, format, ...)		write_log(lh, LOG_ERROR, format, __VA_ARGS__)
#endif

#if (LOG_LEVEL == LOG_DEBUG)
	#define _STR(x) _VAL(x)
	#define _VAL(x) #x
	#define write_log(lh, level, format,  ...)		_log_write(lh, level, format, _STR(__LINE__), __FILE__, __VA_ARGS__)
#else
	#define write_log(lh, level, format,  ...)		_log_write(lh, level, format, __VA_ARGS__)
#endif


typedef struct
{
	uint8_t 		*wkey;
	int    	    	cflag;
	char 			*file_path;
	FILE			*f_log;
	char 			*io_buf;
	size_t 			io_cap;
	size_t 			max_file_size;
	size_t 			cur_file_size;
	size_t 			cur_bak_num;
	size_t 			max_bak_num;
	pthread_mutex_t mutex;
} log_t;

#define S_LOG_SIZE	sizeof(log_t)


/*log level*/
typedef enum
{
	_DEBUG = LOG_DEBUG,
	_INFO = LOG_INFO,
	_WARN = LOG_WARN,
	_ERROR = LOG_ERROR,
	_CLOSE = LOG_CLOSE
} log_level_t;


/* option for log handle create */
#define NORMALIZE	0	//normal
#define ENCRYPT		1	//log encryption
#define COMPRESS	2	//log compress


void _log_write(log_t *lh, const log_level_t level, const char *format, ...);


/**
 * create the log handle.
 * @param log_filename - log file path.
 * @param max_file_size - max size of every log file.
 * @param max_file_bak -  the maximum number of backup log files,
 * backup file's name is the log filename add back number at the end.
 * @param max_iobuf_size - log IO buffer, 0 is allowed(no buffer).
 * @param cflag - option, NORMALIZE, ENCRYPT, COMPRESS or ENCRYPT|COMPRESS,
 * encrypted file extension is filename, compressed file extension is
 * filename is add ".lz4" at the end. 
 * @param password - works if cflag & ENCRYPT.
 * @return log handle if successful or return NULL
 */
log_t* log_create(const char *log_filename, size_t max_file_size, size_t max_file_bak, size_t max_iobuf_size, int cflag, const char *password);


/**
 * log print.
 * @param lh - log handle.
 * @param format - is similar to printf format.
 */
#define log_debug(lh, format, ...)		_log_debug(lh, format, __VA_ARGS__)

#define log_info(lh, format, ...)		_log_info(lh, format, __VA_ARGS__)

#define log_warn(lh, format, ...)		_log_warn(lh, format, __VA_ARGS__)

#define log_error(lh, format, ...)		_log_error(lh, format, __VA_ARGS__)


/**
 * flush io buffer to file.
 * @param lh - log handle.
 */
void log_flush(log_t *lh);

/**
 * destory log_handle.
 * @param lh - log handle.
 */
void log_destory(log_t *lh);


/**
 * decipher log file, do not remove source file.
 * @param in_filename - source filename.
 * @param out_filename - destination filename
 * @param password - password
 */
int log_decipher(const char *in_filename, const char *out_filename, const char *password);


/**
 * uncompress log file, do not remove source file.
 * @param src_filename - source filename.
 * @param dst_filename - destination filename
 */
int log_uncompress(const char *src_filename, const char *dst_filename);


/**
 * md5 file.
 * @param filename - source filename.
 * @param digest - if digest is NULL, there will memory allocation.
 * if digest is not NULL, digest size must greater than (32+1('\0')) byte.
 * @return digest, a 32-character fixed-length string.
 */
char* log_md5(const char *filename, char *digest);


#endif	/*!_LOG_H_*/

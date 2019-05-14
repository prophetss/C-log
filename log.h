#ifndef _LOG_H_
#define _LOG_H_


#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include "list.h"

/*log level*/

/* print all level msg */
#define LOG_DEBUG	0
/* print log_error,log_warn and log_info */
#define LOG_INFO	1
/* print log_error and log_warn */
#define LOG_WARN	2
/* only print log_error */
#define LOG_ERROR	3
/* close logging function and do nothing */
#define LOG_CLOSE	9

/***************user configuration***************/

/* print debug message(time, lineno, filepath,
 * thread id).0:no,else:yes
 */
#define LOG_DEBUG_MESSAGE			1

 /* log file level */
#define LOG_FILE_LEVEL				LOG_DEBUG

/* log stream level */
#define LOG_STREAM_LEVEL			LOG_DEBUG

/* the maximum size(byte) of a logging*/
#define SINGLE_LOG_SIZE		1024

/* trace printed path for exit signal */
#define TRACE_PRINT_PATH	"trace_info"

/***************user configuration***************/


// handle mode

/*file handle*/
#define		F_MODE		0x01

/*stream handle*/
#define		S_MODE		0x10

/*file and stream*/
#define		F_S_MODE	0x11

/*removing the unnecessary log at compile time based on the log level*/
#define UNUSED		((void)0)

#define _STR(x) _VAL(x)
#define _VAL(x) #x

#define GET_MODE(fl, sl, ll)	(fl < ll ? (sl < ll ? F_S_MODE : F_MODE) : S_MODE)

#if (LOG_FILE_LEVEL >= LOG_INFO && LOG_STREAM_LEVEL >= LOG_INFO)
#define _log_debug(lhs, format, ...)	UNUSED
#else
#define _log_debug(lhs, format, ...) write_log(GET_MODE(LOG_FILE_LEVEL, LOG_STREAM_LEVEL, LOG_DEBUG), lhs, LOG_DEBUG, format, __VA_ARGS__)
#endif

#if (LOG_FILE_LEVEL >= LOG_WARN && LOG_STREAM_LEVEL >= LOG_WARN)
#define _log_info(lhs, format, ...)	UNUSED
#else
#define _log_info(lhs, format, ...) write_log(GET_MODE(LOG_FILE_LEVEL, LOG_STREAM_LEVEL, LOG_INFO), lhs, LOG_INFO, format, __VA_ARGS__)
#endif

#if (LOG_FILE_LEVEL >= LOG_ERROR && LOG_STREAM_LEVEL >= LOG_ERROR)
#define _log_warn(lhs, format, ...)	UNUSED
#else
#define _log_warn(lhs, format, ...) write_log(GET_MODE(LOG_FILE_LEVEL, LOG_STREAM_LEVEL, LOG_WARN), lhs, LOG_WARN, format, __VA_ARGS__)
#endif

#if (LOG_FILE_LEVEL == LOG_CLOSE && LOG_STREAM_LEVEL == LOG_CLOSE)
#define _log_error(lhs, format, ...)	UNUSED
#else
#define _log_error(lhs, format, ...) write_log(GET_MODE(LOG_FILE_LEVEL, LOG_STREAM_LEVEL, LOG_ERROR), lhs, LOG_ERROR, format, __VA_ARGS__)
#endif

#if (LOG_DEBUG_MESSAGE != 0)
#define write_log(mode, lhs, level, format,  ...)		_log_write(mode, lhs, level, format, _STR(__LINE__), __FILE__, __VA_ARGS__)
#else
#define write_log(mode, lhs, level, format,  ...)		_log_write(mode, lhs, lh, level, format, __VA_ARGS__)
#endif

typedef list_node log_t;

#define S_LOG_T	(sizeof(log_t))

typedef struct log_handle_t
{
	uint32_t   		tag;
	void* hld;
} log_handle_t;

#define S_LOG_HANDLE_T	sizeof(log_handle_t)

typedef enum log_level_t
{
	_DEBUG_LEVEL = LOG_DEBUG,
	_INFO_LEVEL = LOG_INFO,
	_WARN_LEVEL = LOG_WARN,
	_ERROR_LEVEL = LOG_ERROR,
} log_level_t;

#define S_LOG_LEVEL	sizeof(log_level_t)

void _log_write(uint32_t mode, log_t * lhs, log_level_t level, const char* format, ...);

/* option for file handle create */
#define NORMALIZE	0	//normal
#define ENCRYPT		1	//log file encryption
#define COMPRESS	2	//log file compress

/**
 * create the log file handle.
 * @param log_filename - log file path.
 * @param max_file_size - max size of every log file.
 * @param max_file_bak -  the maximum number of backup log files,
 * backup file's name is the log filename add back number at the end.
 * @param max_iobuf_size - log IO buffer, 0 is allowed(no buffer).
 * @param cflag - option, NORMALIZE, ENCRYPT, COMPRESS or ENCRYPT|COMPRESS.
 * @param password - works if cflag & ENCRYPT.
 * @return file handle if successful or return NULL
 */
log_handle_t* file_handle_create(const char* log_filename, size_t max_file_size, size_t max_file_bak, size_t max_iobuf_size, uint8_t cflag, const char* password);

/* option for stream handle create */
#define DEBUG_STDERR	1
#define INFO_STDERR		2
#define WARN_STDERR		4
#define ERROR_STDERR	8

/**
 * create the log stream handle.
 * @param streams-standard streams for output and error output.
 * values for streams are constructed by a bitwise-inclusive or
 * of flags from the above macros list.and 0 indicates stdout
 * @return stream handle
 */
log_handle_t* stream_handle_create(uint8_t streams);

//font color
#define FBLACK		"\x1B[30m"
#define FRED		"\x1B[31m"
#define FGREEN		"\x1B[32m"
#define FYELLOW		"\x1B[33m"
#define FBLUE		"\x1B[34m"
#define FMAGENTA	"\x1B[35m"
#define FCYAN		"\x1B[36m"
#define FWHITE		"\x1B[37m"
//background color
#define BGBLACK		"\x1B[40m"
#define BGRED		"\x1B[41m"
#define BGGREEN		"\x1B[42m"
#define BGYELLOW	"\x1B[43m"
#define BGBLUE		"\x1B[44m"
#define BGMAGENTA	"\x1B[45m"
#define BGCYAN		"\x1B[46m"
#define BGWHITE		"\x1B[47m"
//font style
#define HIGHLIGHT	"\x1B[1m"
#define UNDERLINE	"\x1B[4m"
#define BLINK		"\x1B[5m"
#define REVERSE		"\x1B[7m"
#define BLANK		"\x1B[8m"

/**
 * set stream handle parameters.
 * @param sh - stream handle.
 * @param level - stream level
 * @param color - font color
 * @param bgcolor - background color
 * @param style - font style
 * @return stream handle(sh).
 */
void* set_stream_param(void* sh, log_level_t level, const char* color, const char* bgcolor, const char* style);

/**
 * add file or stream handle to handle list.
 * @param lhs - handle list.this parameter can be NULL.
 * @param hld - log handle(file or stream).
 * @return handle list.if lhs is not NULL, return itself.
 */
log_t * add_to_handle_list(log_t * lhs, void* hld);

/**
 * log print.
 * @param lhs - log handle list.
 * @param format - is similar to printf format.
 */
#define log_debug(lhs, format, ...)		_log_debug(lhs, format, __VA_ARGS__)

#define log_info(lhs, format, ...)		_log_info(lhs, format, __VA_ARGS__)

#define log_warn(lhs, format, ...)		_log_warn(lhs, format, __VA_ARGS__)

#define log_error(lhs, format, ...)		_log_error(lhs, format, __VA_ARGS__)

 /**
  * flush io buffer to file.
  * @param lh - log handle list.
  */
void log_flush(log_t * lhs);

/**
 * destory log handle list.
 * @param lhs - log handle list.
 */
void log_destory(log_t * lhs);

/**
 * decipher log file, not remove source file.
 * @param in_filename - source filename.
 * @param out_filename - destination filename
 * @param password - password
 */
int log_file_decipher(const char* in_filename, const char* out_filename, const char* password);

/**
 * uncompress log file, not remove source file.
 * @param src_filename - source filename.
 * @param dst_filename - destination filename
 */
int log_file_uncompress(const char* src_filename, const char* dst_filename);

/**
 * md5 file.
 * @param filename - source filename.
 * @param digest - if digest is NULL, there will be memory allocation.
 * if digest is not NULL, digest size must greater than (32+1('\0')) byte.
 * @return digest, a 32-character fixed-length string.
 */
char* log_file_md5(const char* filename, char* digest);


#endif	/*!_LOG_H_*/

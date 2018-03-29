#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/syscall.h> 
#include "file_util.h"
#include "trace.h"
#include "lz4.h"
#include "aes.h"
#include "md5.h"
#include "log.h"


#if (LOG_IO_BUF_MAX > LOG_BUF_MAX)
#define _setvbuf(a, b, c, d)	setvbuf(a, b ,c, d)
#else
#define _setvbuf(a, b, c, d)
#endif /* LOG_IO_BUF_MAX */

#define atomic_inc(x) __sync_add_and_fetch((x),1)

/*非16整数倍向上扩展*/
#define aes_expand(n)	(n&0x000F ? (n+0x000F)&0xFFF0 : n)

#define EXPAND_BUF_MAX	aes_expand(LOG_BUF_MAX)

#define _error_display(...)	fprintf(stderr, __VA_ARGS__)

#define _sys_error_display(...) do {			\
	_error_display(__VA_ARGS__);				\
	_error_display("%s\n", strerror(errno));	\
    } while(0)

#define exit_throw(errno, ...) do {												\
{                                                                         		\
    _error_display("Error defined at %s, line %i : \n", __FILE__, __LINE__); 	\
    _sys_error_display(1, "Error %i : ", errno);                                \
    _error_display(__VA_ARGS__);                                        		\
    _error_display(" \n");                                               		\
    exit(error);                                                          		\
} while(0)

static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

/* 日志文件描述符 */
static FILE *f_log = NULL;

/* 多线程初始化同步 */
static volatile int _log_sync = 0;

/* 初始化标志 */
static volatile int _is_initialized = 0;

/* IO缓存 */
static char *io_buf = NULL;

/* IO缓存剩余大小（当小于LOG_BUF_MAX时刷新写文件）*/
static int io_buf_size = LOG_IO_BUF_MAX;

/* 密钥 */
static unsigned char *key = NULL;


static void _log_initialize()
{
	if (!_is_initialized) {
		if (atomic_inc(&_log_sync) == 1) {
			/* 用户IO缓存初始化 */
		#if (LOG_IO_BUF_MAX > LOG_BUF_MAX)
			if (!io_buf)
				io_buf = (char*)malloc(LOG_IO_BUF_MAX);
		#endif

			/* 异常堆栈打印初始化 */
			trace_init();
			_is_initialized = 1;
		}
		else {
			while (!_is_initialized) sleep(0);
		}
	}
}

static void _log_lock(void)
{
	_log_initialize();

	pthread_mutex_lock(&mMutex);
}

static void _log_unlock(void)
{
	pthread_mutex_unlock(&mMutex);
}

static void cipher_buf(unsigned char *buf, size_t expand_buf_len)
{
	unsigned char cipher_buf[AES_BUFSIZ], out[AES_BUFSIZ];
	int uncipher_io_size = expand_buf_len;

	while (uncipher_io_size > 0) {
		memcpy(cipher_buf, buf + expand_buf_len - uncipher_io_size, AES_BUFSIZ);
		if (aes_cipher_data(cipher_buf, AES_BUFSIZ, out, key, AES_128) != AES_SUCCESS) {
			_error_display("failure to encrypt!");
			return;
		}
		memcpy(buf + expand_buf_len - uncipher_io_size, out, AES_BUFSIZ);
		uncipher_io_size -= AES_BUFSIZ;
	}
}

void run_log(int line_num, ...)
{
	va_list args;
	unsigned char buf[EXPAND_BUF_MAX] = { 0 };
	size_t buf_len = 0;
	size_t mlen = 0;

	va_start(args, line_num);
	char* message = va_arg(args, char*);
	mlen = strlen(message);

	if (line_num != INVAILD_LINE_NUM) {
		/* DEBUG 模式信息 */
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		pid_t tid = syscall(SYS_gettid);
		sprintf((char*)buf, "%04d-%02d-%02d %02d:%02d:%02d %s[%d]%d:", timeinfo->tm_year + 1900,
		        timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
		        timeinfo->tm_sec, message, line_num, tid);
		buf_len =  strlen((char*)buf);
	}
	else {
		strncat((char*)buf, message, mlen);
		buf_len =  mlen;
	}

	while (mlen != 0 &&  buf_len < LOG_BUF_MAX - 1) {
		message = va_arg(args, char*);
		mlen = strlen(message);
		strncat((char*)buf, message, mlen);
		buf_len += mlen;
	}
	va_end(args);

	_log_lock();

	if (NULL == f_log) {
		if ((f_log = fopen(LOG_PATH, "a+")) != NULL) {
			_setvbuf(f_log, io_buf, _IOFBF, LOG_IO_BUF_MAX);
		}
		else {
			_sys_error_display("failure to open log!");
			_log_unlock();
			return;
		}
	}

	if (NULL != key) {
		buf_len = aes_expand(buf_len);
		cipher_buf(buf, buf_len);
	}

	int print_size = fwrite(buf, sizeof(unsigned char), buf_len, f_log);

	io_buf_size = io_buf_size - print_size;
	/* IO缓存区已满,刷新 */
	if (io_buf_size < LOG_BUF_MAX) {
		fclose(f_log);
		f_log = NULL;
		io_buf_size = LOG_IO_BUF_MAX;
	}

	if (get_file_size(LOG_PATH) > LOG_FILE_SIZE_MAX)
	{
		static int bak_num = 0;
		char path_bak[160];
		sprintf(path_bak, "%s.bak%d", LOG_PATH, bak_num);
		compress_file(LOG_PATH, path_bak);
		remove(LOG_PATH);
		bak_num++;
	}

	_log_unlock();
}

int set_key(const char *password, int len)
{
	_log_lock();

	if (NULL == password) {
		free(key);
		key = NULL;
		return OP_SUCCESS;
	}

	if (NULL == key)
		key = (unsigned char*)malloc(MD5_HASHBYTES);
	if (NULL != key) {
		MD5Data((unsigned char*)password, len, key);
	}
	else {
		_log_unlock();
		_sys_error_display("memory allication failed!");
		return OP_FAILED;
	}

	aes_set_key(key, len);

	_log_unlock();
	return OP_SUCCESS;
}

int decipher_log(const char *password, int pw_len, const char *in_filepath)
{
	set_key(password, pw_len);
	char tmpname[] = "tmp.XXXXXX";
	mkstemp(tmpname);
	rename(in_filepath, tmpname);
	/*后两个参数暂时没有*/
	if (AES_SUCCESS != aes_decipher_file(tmpname, in_filepath , 0, AES_128)) {
		rename(tmpname, in_filepath);
		_error_display("decipher log failed!");
		return OP_FAILED;
	}
	remove(tmpname);
	return OP_SUCCESS;
}

void md5_log(const char *filepath, unsigned char *digest)
{
	MD5File(filepath, digest);
}

char* md5_log_s(const char* filepath, char *buf)
{
	return MD5File_S(filepath, buf);
}

int decompress_log(const char *src_filepath, const char *dst_filepath)
{
	return decompress_file(src_filepath, dst_filepath);
}
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include "aes.h"
#include "sha2.h"
#include "md5.h"
#include "log.h"

#if (IOBUF_SIZE > MBUF_MAX)
#define _setvbuf(a, b, c, d)	setvbuf(a, b ,c, d)
#else
#define _setvbuf(a, b, c, d)
#endif /* IOBUF_SIZE */

#define atomic_inc(x) __sync_add_and_fetch((x),1)

static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

/* 日志文件描述符 */
static FILE* f_log = NULL;

/* 多线程初始化同步 */
static volatile int _log_sync = 0;

/* 初始化标志 */
static volatile int _is_initialized = 0;

/*IO缓存*/
static char* iobuf = NULL;

static int iobuf_size = IOBUF_SIZE;

static long long getfilesize()
{
	struct stat sbuf;
	if (stat(RUN_LOG_PATH, &sbuf) != LOG_SUCCESS)
	{
		perror("get file size failed!");
		return LOG_FAILED;
	}

	return sbuf.st_size;
}

static void trace_print(int signal_type)
{
	int trace_id = -1;
	int i = 0;
	void *buffer[100];
	char **info = NULL;
	char trace_buff[TRACE_SIZE];

	trace_id = backtrace(buffer, TRACE_SIZE);

	info = backtrace_symbols(buffer, trace_id);
	if (NULL == info)
		return;

	for (i = 0; i < trace_id; i++)
	{
		sprintf(trace_buff, "echo \"%s\" >> %s_%d", info[i], TRACE_PRINT_PATH, signal_type);
		system(trace_buff);
	}

	sprintf(trace_buff, "echo \"###################################\" >> %s_%d", TRACE_PRINT_PATH, signal_type);
	system(trace_buff);
}

static void signal_hadle_fun(int signal_type)
{
	trace_print(signal_type);
	exit(0);
}

static void trace_init()
{
	/* 异常退出信号注册 */
	signal(SIGHUP, signal_hadle_fun);
	signal(SIGINT, signal_hadle_fun);
	signal(SIGQUIT, signal_hadle_fun);
	signal(SIGILL, signal_hadle_fun);
	signal(SIGTRAP, signal_hadle_fun);
	signal(SIGABRT, signal_hadle_fun);
	signal(SIGBUS, signal_hadle_fun);
	signal(SIGFPE, signal_hadle_fun);
	signal(SIGKILL, signal_hadle_fun);
	signal(SIGSEGV, signal_hadle_fun);
	signal(SIGPIPE, signal_hadle_fun);
	signal(SIGTERM, signal_hadle_fun);
}

static void _log_initialize()
{
	if (!_is_initialized)
	{
		if (atomic_inc(&_log_sync) == 1)
		{
			/* 用户IO缓存初始化 */
#if (IOBUF_SIZE > MBUF_MAX)
			if (!iobuf)
				iobuf = (char*)malloc(IOBUF_SIZE);
#endif

			/* 异常堆栈打印初始化 */
			trace_init();
			_is_initialized = 1;
		}
		else
		{
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

void run_log(int line_num, ...)
{
	va_list args;
	char buf[MBUF_MAX] = { 0 };
	size_t buf_len = 0;
	size_t mlen = 0;

	va_start(args, line_num);
	char* message = va_arg(args, char*);
	mlen = strlen(message);

	if (line_num != INVAILD_LINE_NUM)
	{
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		pid_t tid = syscall(SYS_gettid);
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d %s[%d]%d:", timeinfo->tm_year + 1900,
		        timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
		        timeinfo->tm_sec, message, line_num, tid);
		buf_len =  strlen(buf);
	}
	else
	{
		strncat(buf, message, mlen);
		buf_len =  mlen;
	}

	while (mlen != 0 &&  buf_len < MBUF_MAX - 1)
	{
		message = va_arg(args, char*);
		mlen = strlen(message);
		strncat(buf, message, mlen);
		buf_len += mlen;
	}
	va_end(args);

	_log_lock();

	if (NULL == f_log)
	{
		if ((f_log = fopen(RUN_LOG_PATH, "a+")) != NULL)
		{
			_setvbuf(f_log, iobuf, _IOFBF, IOBUF_SIZE);
		}
		else
		{
			perror("file open failed!");
			_log_unlock();
			return;
		}
	}

	int print_size = fprintf(f_log, "%s\n", buf);

	iobuf_size = iobuf_size - print_size;
	/* IO缓存区已满,刷新 */
	if (iobuf_size < MBUF_MAX)
	{
		fclose(f_log);
		f_log = NULL;
		iobuf_size = IOBUF_SIZE;
	}

	if (getfilesize() > MAX_FILE_SIZE)
	{
		remove(RUN_LOG_BAK_PATH);
		rename(RUN_LOG_PATH, RUN_LOG_BAK_PATH);
	}

	_log_unlock();
}


void cipher_log(const char *password, const char *in_filepath, const char *out_filepath)
{
	if (!password)
		return;
	unsigned char digest[SHA256_DIGEST_SIZE];
	sha256(password, strlen(password), digest);
	aes_cipher_file(in_filepath, out_filepath, digest, AES_256);
}

void decipher_log(const char *password, const char *in_filepath, const char *out_filepath)
{
	if (!password)
		return;
	unsigned char digest[SHA256_DIGEST_SIZE];
	sha256(password, strlen(password), digest);
	aes_decipher_file(in_filepath, out_filepath, digest, AES_256);
}


void md5_log(const char* filepath, unsigned char *digest)
{
	MD5File(filepath, digest);
}
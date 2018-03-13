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
#include "log.h"

static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

/*日志文件描述符*/
static FILE* f_log = NULL;

#if defined(IOBUF_SIZE)  || defined(TRACE_PRINT)
	#define atomic_inc(x) __sync_add_and_fetch((x),1)
	static volatile int _log_sync = 0;
	static volatile int _is_initialized = 0;
#endif

/*IO缓存*/
#ifdef IOBUF_SIZE
static int iobuf_size = IOBUF_SIZE;
static char* iobuf = NULL;
#endif /* IOBUF_SIZE */

static int log_open()
{
	if (f_log)
		return LOG_SUCCESS;
#ifdef IOBUF_SIZE
	if ((f_log = fopen(RUN_LOG_PATH, "a+")) != NULL)
	{
		int nRet = setvbuf(f_log, iobuf, _IOFBF, IOBUF_SIZE);
		return nRet;
	}
	else
		return LOG_FAILED;
#else
	f_log = fopen(RUN_LOG_PATH, "a+");
	return f_log ? LOG_SUCCESS : LOG_FAILED;
#endif /* IOBUF_SIZE */
}

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
	if (NULL == info){
		return;
	}

	for (i = 0; i < trace_id; i++) {
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
		if(atomic_inc(&_log_sync) == 1)
		{
			/* 用户IO缓存初始化 */
			#ifdef IOBUF_SIZE
			if (!iobuf)
				iobuf = (char*)malloc(IOBUF_SIZE);
			#endif

			/* 异常堆栈打印初始化 */
			#ifdef TRACE_PRINT
			trace_init();
			#endif

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
	#if defined(IOBUF_SIZE)  || defined(TRACE_PRINT)
	_log_initialize();
	#endif

	pthread_mutex_lock(&mMutex);
}

static void _log_unlock(void)
{
	pthread_mutex_unlock(&mMutex);
}

void run_log(int line_num, ...)
{
	_log_lock();

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
			timeinfo->tm_mon + 1, timeinfo->tm_mday,timeinfo->tm_hour, timeinfo->tm_min,
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

	if (LOG_SUCCESS != log_open())
	{
		perror("log open failed!");
		_log_unlock();
		return;
	}
	
	int print_size = fprintf(f_log, "%s\n", buf);

#ifdef IOBUF_SIZE
	iobuf_size = iobuf_size - print_size;
	/* »º³åÇøÊ£Óà¿Õ¼ä²»×ãË¢ÐÂ */
	if (iobuf_size < MBUF_MAX)
	{
		fclose(f_log);
		f_log = NULL;
		iobuf_size = IOBUF_SIZE;
	}
#else
	fclose(f_log);
#endif /* IOBUF_SIZE */

	if (getfilesize() > MAX_FILE_SIZE)
	{
		remove(RUN_LOG_BAK_PATH);
		rename(RUN_LOG_PATH, RUN_LOG_BAK_PATH);
	}

	_log_unlock();
}
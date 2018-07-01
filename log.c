#include <string.h>
#include <apr_file_io.h>
#include <apr_portable.h>
#include <apr_atomic.h>
#include <apr_time.h>
#include "log.h"


static apr_pool_t *root;

static apr_thread_mutex_t *log_mutex;

/*日志文件描述符*/
static apr_file_t *f_log = NULL;

static apr_file_t *f_err = NULL;

static volatile apr_uint32_t _log_sync = 1;

static volatile apr_uint32_t _is_initialized = 0;

static apr_uint32_t iobuf_size = IOBUF_SIZE;

static char *iobuf = NULL;

static apr_int32_t log_open()
{
	if (f_log) {
		return LOG_SUCCESS;
	}

	if (apr_file_open(&f_log, RUN_LOG_PATH, APR_FOPEN_WRITE | APR_FOPEN_APPEND | APR_FOPEN_CREATE,
		APR_FPROT_OS_DEFAULT, root) == 0) {
		apr_status_t nRet = apr_file_buffer_set(f_log, iobuf, IOBUF_SIZE);
		return nRet;
	}
	else {
		return LOG_FAILED;
	}	
}

static long long getfilesize()
{
	apr_finfo_t sbuf;
	apr_stat(&sbuf, RUN_LOG_PATH, APR_FINFO_SIZE, root);
	if (sbuf.valid != APR_FINFO_SIZE) {
		perror("get file size failed!");
		return LOG_FAILED;
	}
	return sbuf.size;
}

static void _log_initialize()
{
	if (!_is_initialized) {
		if (apr_atomic_dec32(&_log_sync)== 0) {
			apr_pool_initialize();
			apr_pool_create(&root, NULL);
			apr_file_open_stderr(&f_err, root);
			iobuf = (char*)apr_palloc(root, IOBUF_SIZE);
			apr_thread_mutex_create(&log_mutex, APR_THREAD_MUTEX_DEFAULT, root);
			_is_initialized = 1;
		}
		else {
			while (!_is_initialized) apr_sleep(0);
		}
	}
}

static void _log_lock(void)
{
	_log_initialize();
	apr_thread_mutex_lock(log_mutex);
}

static void _log_unlock(void)
{
	apr_thread_mutex_unlock(log_mutex);
}

void run_log(int line_num, ...)
{
	_log_lock();

	va_list args;
	char buf[MBUF_MAX] = { 0 };
	size_t buf_len = 0;
	size_t mlen = 0;

	va_start(args, line_num);
	char *message = va_arg(args, char*);
	mlen = strlen(message);

	if (line_num != INVAILD_LINE_NUM)
	{
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);

		apr_os_thread_t tid = apr_os_thread_current();

		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d %s[%d]%d:", timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
			timeinfo->tm_sec, message, line_num, tid);
		buf_len = strlen(buf);
	}
	else
	{
		strncat(buf, message, mlen);
		buf_len = mlen;
	}

	while (mlen != 0 && buf_len < MBUF_MAX - 1)
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

	apr_int32_t print_size = apr_file_printf(f_log, "%s\n", buf);

#ifdef IOBUF_SIZE
	iobuf_size = iobuf_size - print_size;

	if (iobuf_size < MBUF_MAX)
	{
		apr_file_close(f_log);
		f_log = NULL;
		iobuf_size = IOBUF_SIZE;
	}
#else
	apr_file_close(f_log);
#endif /* IOBUF_SIZE */
	
	if (getfilesize() > MAX_FILE_SIZE)
	{
		apr_file_remove(RUN_LOG_BAK_PATH, root);
		apr_file_rename(RUN_LOG_PATH, RUN_LOG_BAK_PATH, root);
	}

	_log_unlock();
}
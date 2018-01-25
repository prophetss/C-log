#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <pthread.h>
#endif	/* _WIN32 || _WIN64 */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "log.h"

typedef long long	int64;

#if defined(_WIN32) || defined(_WIN64)
static HANDLE hMutex = NULL;
#else
static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;
#endif	/* _WIN32 || _WIN64 */

#define NSEC_PER_SEC	1000000000

/*计时器数组*/
static struct timespec timekeeper[TIMEKEEPER_NUM] = { 0 };

/*日志文件描述符*/
static FILE* f_log = NULL;

/*IO缓存*/
#ifdef IOBUF_SIZE
static int iobuf_size = IOBUF_SIZE;
static char* iobuf = NULL;
#endif /* IOBUF_SIZE */

#ifndef TIME_UTC
#define TIME_UTC	1
#endif  /* !TIMT_UTC */

void timekeeper_set(int n)
{
	if (n > TIMEKEEPER_NUM - 1 || n < 1)
	{
		perror("timekeeper_set illegal input!");
		return;
	}
	timespec_get(&timekeeper[n], TIME_UTC);
}

double timekeeper_get(int n)
{
	if (n > TIMEKEEPER_NUM-1 || n < 1)
	{
		perror("timekeeper_get illegal input!");
		return -1.0;
	}
	timespec_get(&timekeeper[0], TIME_UTC);
	return (timekeeper[0].tv_sec - timekeeper[n].tv_sec) + 
		(double)(timekeeper[0].tv_nsec - timekeeper[n].tv_nsec) / NSEC_PER_SEC;
}

static int log_open()
{
#ifdef IOBUF_SIZE
	if (!iobuf)
		iobuf = (char*)malloc(IOBUF_SIZE);
	if (f_log)
		return LOG_SUCCESS;
	if (iobuf && (f_log = fopen(RUN_LOG_PATH, "a+")) != NULL)
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
#if  defined(_WIN32) || defined(_WIN64)
	struct _stati64 sbuf;
	if (_stati64(RUN_LOG_PATH, &sbuf) != LOG_SUCCESS)
	{
		perror("get file size failed!");
		return LOG_FAILED;
	}
#else
	struct stat sbuf;
	if (stat(RUN_LOG_PATH, &sbuf) != LOG_SUCCESS)
	{
		perror("get file size failed!");
		return LOG_FAILED;
	}
#endif	/* _WIN32 || _WIN64 */

	return sbuf.st_size;
}

static void log_update()
{
	if (getfilesize() > MAX_FILE_SIZE)
	{
		remove(RUN_LOG_BAK_PATH);
		rename(RUN_LOG_PATH, RUN_LOG_BAK_PATH);
	}
}

#ifdef IOBUF_SIZE
static void log_flush()
{
	fclose(f_log);
	f_log = NULL;
	iobuf_size = IOBUF_SIZE;
	log_update();
}
#endif /* IOBUF_SIZE */

void run_log(const char* file_name, int line_num, ...)
{
#if  defined(_WIN32) || defined(_WIN64)
	if (!hMutex)
		hMutex = CreateMutex(NULL, FALSE, NULL);
	WaitForSingleObject(hMutex, INFINITE);
#else
	pthread_mutex_lock(&mMutex);
#endif	/* _WIN32 || _WIN64 */

	va_list args;
	char buf[MBUF_MAX] = { 0 };

	va_start(args, line_num);
	char* message = va_arg(args, char*);
	int mlen = strlen(message);
	while (mlen != 0 && strlen(buf) + mlen < MBUF_MAX - 1)
	{
		strncat(buf, message, mlen);
		message = va_arg(args, char*);
		mlen = strlen(message);
	}
	va_end(args);

	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	if (LOG_SUCCESS != log_open())
	{
		perror("log open failed!");
		return;
	}

	int print_size = fprintf(f_log, "%04d-%02d-%02d %02d:%02d:%02d %s[%d]:%s\n",
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, file_name,
		line_num, buf);

#ifdef IOBUF_SIZE
	iobuf_size = iobuf_size - print_size;
	/* 粗略判断缓冲区剩余空间不足刷新 */
	if (iobuf_size < MBUF_MAX * 2)
		log_flush();
#else
	fclose(f_log);
	log_update();
#endif /* IOBUF_SIZE */

#if  defined(_WIN32) || defined(_WIN64)
	ReleaseMutex(hMutex);
#else 
	pthread_mutex_unlock(&mMutex);
#endif	/* _WIN32 || _WIN64 */
}
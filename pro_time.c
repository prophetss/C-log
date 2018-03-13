#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "pro_time.h"

struct safe_timespec
{
	struct timespec spec;
	/* 0-未被使用，1-已被占用 */
	long is_ref;
};

#define NSEC_PER_SEC    1000000000

#define atomic_inc(x) __sync_add_and_fetch((x),1)

/*计时器临界区*/
static pthread_mutex_t _timekeeper_mutex = PTHREAD_MUTEX_INITIALIZER;

/*计时器数组*/
static struct safe_timespec _ptimekeeper[TIMEKEEPER_NUM] = { 0 };

/* 多线程初始化标记,防止多次初始化 */
static volatile int _timekeeper_sync = 0;

/* 初始化标志 */
static volatile int _is_initialized = 0;

static void _timekeeper_initialize(void)
{
	if (!_is_initialized)
	{
		/* 原子操作加1，如果结果为1当前只有本线程调用可以初始化，不为1（>1)
		** 说明有其他线程调用等待其他线程初始化完毕即可 */
		if(atomic_inc(&_timekeeper_sync) == 1)
		{
			/* 初始化 */
			memset(_ptimekeeper, 0, TIMEKEEPER_NUM * sizeof(struct safe_timespec));
			_is_initialized = 1;
		}
		else
		{
			/* 等待其他线程初始化完毕 */
			while (!_is_initialized) sleep(0);
		}
	}
}

static void _timekeeper_lock(void)
{
	_timekeeper_initialize();

	pthread_mutex_lock(&_timekeeper_mutex);
}

static void _timekeeper_unlock(void)
{
	pthread_mutex_unlock(&_timekeeper_mutex);
}

static int _timekeeper_get(int n)
{
	_timekeeper_lock();

	if (_ptimekeeper[n].is_ref != 0)
	{
		_timekeeper_unlock();
		return -2;
	}
	timespec_get(&(_ptimekeeper[n].spec), TIME_UTC);
	_ptimekeeper[n].is_ref = 1;

	_timekeeper_unlock();
	return 0;
}

int timekeeper_start_man(int n)
{
	return _timekeeper_get(n);
}

int timekeeper_start_auto()
{
	for (int i = 0; i < TIMEKEEPER_NUM; i++)
	{
		if (0 == _timekeeper_get(i))
			return i;
	}
	return -1;
}

int timekeeper_pause(int thnd, double* time)
{
	struct timespec tk;
	if (timespec_get(&tk, TIME_UTC) != 0)
		*time = tk.tv_sec - (_ptimekeeper[thnd].spec).tv_sec +
		        (double)(tk.tv_nsec - (_ptimekeeper[thnd].spec).tv_nsec) / NSEC_PER_SEC;
	else
		return -4;

	return 0;
}

int timekeeper_shutoff(int thnd, double* time)
{
	int ret;
	if ((ret = timekeeper_pause(thnd, time)) != 0)
		return ret;

	_timekeeper_lock();
	_ptimekeeper[thnd].is_ref = 0;
	_timekeeper_unlock();

	return 0;
}

void timekeeper_destory()
{
	_timekeeper_lock();
	_is_initialized = 0;
	_timekeeper_unlock();
}

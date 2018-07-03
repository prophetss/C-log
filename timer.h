#ifndef _PRO_TIME_H_
#define _PRO_TIME_H_


/* 获取运行时钟周期,比clock()快至少一个数量级，除以主频
** 等于时间秒，调用需要引<linux/types.h>此文件 */
#define rdtsc(x)	\
{	\
	__u32 lo, hi;	\
	__asm__ __volatile__	\
	(	\
	    "rdtsc":"=a"(lo), "=d"(hi)	\
	);	\
	x = (__u64)hi << 32 | lo;	\
}

/* 多线程加锁，单线程可以去掉此宏节省资源 */
#define MULTITHREAD

/* 计时器可使用最大个数 */
#define TIMEKEEPER_NUM    100

#ifdef __cplusplus
extern "C" {
#endif

/* 手动计时器设置n - [0，TIMEKEEPER_NUM-1] */
int timekeeper_start_man(int n);

/* 自动选取一个空闲计时器，返回值（>0）为输入thnd */
int timekeeper_start_auto();

/* 获取时间，若手动设置thnd为输入参数n，若自动设置则为
** 其返回值（>0。time为输出参数为距上次经过时间，单位s */
int timekeeper_pause(int thnd, double* time);

/* 获取时间，与pause不同的是获取完会清除释放计时
** 器，下次无法使用 */
int timekeeper_shutoff(int thnd, double* time);

/* 全部销毁，谨慎使用 */
void timekeeper_destory();

#ifdef __cplusplus
}
#endif


#endif    /* !_PRO_TIME_H_ */

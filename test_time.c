
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <pthread.h>
#endif	/* _WIN32 || _WIN64 */
#include <stdio.h>
#include "pro_time.h"


void* test(void *arg)
{
	int i;
	int t = *(int*)arg;
    for (i = 0; i < 11; i++)
    {
        int n = timekeeper_start_auto();
        printf("thread%d,i=%d,", t, n);
        double tm;
        timekeeper_pause(n, &tm);
        printf("%f\n", tm);
    }
}		

int main()
{
	pthread_t tpid[10];
	int i;
	for (i = 0; i < 10; i++)
	{
	#if defined(_WIN32) || defined(_WIN64)
		_beginthread(fun, 0, &i);
	#else
		pthread_create(&tpid[i], NULL, &test, &i);
	#endif	/* _WIN32 || _WIN64 */
	}
}

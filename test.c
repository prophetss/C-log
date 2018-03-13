#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "pro_time.h"
#include "log.h"


void write_log(void* arg) 
{
	int t = *(int*)arg;
	int thd = timekeeper_start_auto();
	int m;
	for (m =0; m < 1000; m++)
		WRITE_LOG(LOG_DEBUG, "ssssssssssssssssss");
	double tim;
	timekeeper_pause(thd, &tim);
	printf("thread%d runtime=%f\n", t, tim);
}
int main()
{
	int i;
	pthread_t tid[10];
	for (i = 1; i < 10; i++)
	{
		pthread_create(&tid[i], NULL, (void*)&write_log, (void*)&i);
		sleep(1);
	}
}

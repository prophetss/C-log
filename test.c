#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "pro_time.h"
#include "log.h"


void test(void* arg) 
{
	int t = *(int*)arg;
	int thd = timekeeper_start_auto();
	int m;
	for (m =0; m < 1000; m++)
		write_log(LOG_DEBUG, "ssssssssssssssssss");
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
		pthread_create(&tid[i], NULL, (void*)&test, (void*)&i);
		sleep(1);
	}
	unsigned char digest[32];
	md5_log(RUN_LOG_PATH, digest);
	printf("filepath:%s,md5-digest:%s\n", RUN_LOG_PATH, digest);

	cipher_log("~!@qwe`12", RUN_LOG_PATH, "cipher.log");
	decipher_log("~!@qwe`12", "cipher.log", "decipher.log");
}

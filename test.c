#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "pro_time.h"
#include "log.h"
#include "file_util.h"



void test(void* arg) 
{
	int t = *(int*)arg;
	int thd = timekeeper_start_auto();
	int m;
	for (m =0; m < 2000; m++)
	{
		write_log(0, "aaaaaaaaaaaaaaaaaaaaaa\n");
		write_log(0, "ssssssssssssssssssssss\n");
		write_log(0, "tttttttttttttttttttttt\n");
		write_log(0, "mmmmmmmmmmmmmmmmmmmmmm\n");
		write_log(0, "qqqqqqqqqqqqqqqqqqqqqq\n");
		write_log(0, "wwwwwwwwwwwwwwwwwwwwww\n");
		write_log(0, "eeeeeeeeeeeeeeeeeeeeee\n");
		write_log(0, "rrrrrrrrrrrrrrrrrrrrrr\n");
		write_log(0, "tttttttttttttttttttttt\n");
		write_log(0, "yyyyyyyyyyyyyyyyyyyyyy\n");
	}
	double tim;
	timekeeper_pause(thd, &tim);
	printf("thread%d runtime=%f\n", t, tim);
}
int main()
{
	int i;
	pthread_t tid[10];
	char *pw = "~!@`12qwe1";
	set_key(pw, 10);

	for (i = 1; i < 10; i++) {
		pthread_create(&tid[i], NULL, (void*)&test, (void*)&i);
	}
	void *val;
	for (i = 1; i < 10; i++) {
		pthread_join(tid[i], &val);
	}

	decompress_file("run_log.log.bak0", "run_log0.log");
	decompress_file("run_log.log.bak1", "run_log1.log");
	decipher_log(pw, 10, "run_log0.log");
	decipher_log(pw, 10, "run_log1.log");
}

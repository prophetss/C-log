#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pro_time.h"
#include "log.h"
#include "file_util.h"



/*获取随机字符串*/
void genrs(char * buff, int n)
{
    char genchar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    srand(time(NULL));
    for (int i = 0; i < 80; i++)
    {
        buff[i] = genchar[rand()*n % 62];
    }
    buff[79] = '\0';
}

void test(void* arg) 
{
	int t = *(int*)arg;
	int thd = timekeeper_start_auto();
	int m;
	for (m =0; m < 1000; m++)
	{
		char arr[80];
		genrs(arr, m);
		write_log(0, arr);
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
		sleep(1);
	}
	void *val;
	for (i = 1; i < 10; i++) {
		pthread_join(tid[i], &val);
	}

	decompress_file("run_log.log.bak0", "run_log0.log");
	decipher_log(pw, 10, "run_log0.log");

	char *buf = NULL;
	printf("run_log.log md5 buf: %s\n", md5_log_s("run_log.log", buf));


	/**********************************压缩验证**********************************/

	/*压缩结果升成MD5校验码*/
	unsigned char digest0[16];
	md5_log("run_log.log", digest0);

	/*压缩然后解压,结果生成MD5校验码*/
	compress_file("run_log.log", "run_log.lz4");
	decompress_file("run_log.lz4", "lz4.log");

	unsigned char digest1[16];
	md5_log("run_log.log", digest1);

	assert(strncmp((char*)digest0, (char*)digest1, 16) == 0);
}

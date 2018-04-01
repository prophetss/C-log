#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "pro_time.h"
#include "log.h"
#include "file_util.h"


void test(void* arg)
{
	int t = *(int*)arg;
	int thd = timekeeper_start_auto();
	int m;
	for (m = 0; m < 10000; m++) {
		write_log(0, "So we beat on, boats against the current, borne back ceaselessly into the past.\n");
	}
	double tim;
	timekeeper_pause(thd, &tim);
	printf("thread%d runtime=%f\n", t, tim);
}

void test_cipher()
{	
	/*不全的面测试，存在两个问题，1.此测试通过条件为IO缓存LOG_IO_BUF_MAX宏设置0关闭。如果开启，由于同一程
	  序运行导致两次文件内容不一致造成运行结果失败。2.由于加密是在写文件前进行，而AES加密会对数据扩展，因
	  此造成日志文件大小达到上限时加密和未加密可能包含的日志条数不一致，所以下面测试输出大小是小于所设文件
	  大小上限（宏LOG_FILE_SIZE_MAX）的*/
	remove("run_log.log");
	/*不加密*/
	for (int i = 0; i < 1000; i++) {
		write_log(1, "So we beat on, boats against the current, borne back ceaselessly into the past.\n");
	}
	rename("run_log.log", "run_log.log.ori");

	/*加密*/
	char *pw = "~!@`12qwe1";
	set_key(pw, 10);
	for (int i = 0; i < 1000; i++) {
		write_log(1, "So we beat on, boats against the current, borne back ceaselessly into the past.\n");
	}
	/*解密*/
	decipher_log(pw, 10, "run_log.log");

	unsigned char digest0[16], digest1[16];
	md5_log("run_log.log.ori", digest0);
	md5_log("run_log.log", digest1);

	assert(strncmp((char*)digest0, (char*)digest1, 16) == 0);
	printf("test_cipher success!\n");
}

void test_compress()
{
	/*压缩结果升成MD5校验码*/
	unsigned char digest0[16];
	md5_log("run_log.log", digest0);

	/*压缩然后解压,结果生成MD5校验码*/
	compress_file("run_log.log", "run_log.lz4");
	decompress_file("run_log.lz4", "lz4.log");

	unsigned char digest1[16];
	md5_log("lz4.log", digest1);

	assert(strncmp((char*)digest0, (char*)digest1, 16) == 0);
	printf("test_compress success!\n");
}



int main()
{
	int i;
	pthread_t tid[10];

	void *val;
	for (i = 1; i < 10; i++) {
		pthread_create(&tid[i], NULL, (void*)&test, (void*)&i);
		pthread_join(tid[i], &val);
	
	}

	char *buf = NULL;
	printf("run_log.log md5 buf: %s\n", md5_log_s("run_log.log", buf));

	//加密验证
	test_cipher();

	//压缩验证
	test_compress();
}

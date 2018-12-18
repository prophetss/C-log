#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "timer.h"
#include "../log.h"


#define NUM_THREADS	3

#define PASSWORD	"aaa"

#define FLAG	(ENCRYPT | COMPRESS)

#define MAX_FILE_SIZE	1024000	//byte

#define MAX_BACK_FILE_NUM	10

#define IO_BUFFER_SIZE	10240

#define PRINT_TIMES		10000


void test_write(void* arg)
{
	int t = *(int*)arg;
	
	int thd = timekeeper_start_auto();
	
	char name[64] = {0};
	sprintf(name, "./tmp/TEST%d.TXT", t);
	log_t *l = log_create(name, MAX_FILE_SIZE, MAX_BACK_FILE_NUM, IO_BUFFER_SIZE, FLAG, PASSWORD);
	const char const msg[] = "So we beat on, boats against the current, borne back ceaselessly into the past.";
	for (int m = 0; m < PRINT_TIMES; m++) {
		// only >= LOG_LEVEL will be printed
		log_debug(l, "[DEBUG]...%s%d\n", msg, m);
		log_info(l, "[INFO]...%s%d\n", msg, m);
		log_warn(l, "[WARN]...%s%d\n", msg, m);
		log_error(l, "[ERROR]...%s%d\n", msg, m);
	}
	log_destory(l);
	
	double tim;
	timekeeper_pause(thd, &tim);
	
	printf("thread%d runtime=%f\n", t, tim);
}


void restore_file(char *fn)
{
	char nfn[PATH_MAX] = {0};
	sprintf(nfn, "%s.ori", fn);	// raw result
	
	char tmpname[] = "tmp.XXXXXX"; // for uncompress
	(void)mkstemp(tmpname);

	char *tfn = fn;
	
	if (FLAG & COMPRESS) { // uncompress
		log_uncompress(fn, tmpname);
		tfn = tmpname;
	}

	if (FLAG & ENCRYPT) {	// decipher
		log_decipher(tfn, nfn, PASSWORD);
	}
		
	remove(tmpname);
}


void restore()
{
	if (FLAG == NORMALIZE) return;

	printf("restore start!\n");
	
	DIR *dir;
	struct dirent *ptr;

	if (chdir("./tmp/") == -1) {
		perror("change the dir error!");
		return;
	}
	
	if ((dir=opendir("./")) == NULL) {
		perror("open the dir error!");
		return;
	}

	while ((ptr=readdir(dir)) != NULL) {
		if(ptr->d_type == 8) {	//file
			restore_file(ptr->d_name);
		}
	}
	closedir(dir);

	printf("restore end!\n");
}

int main()
{
	// make a temparary directory for result
	(void)mkdir("./tmp", 0755);
	
	pthread_t tid[NUM_THREADS];
	
	int i;
	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_create(&tid[i], NULL, (void*)&test_write, (void*)&i);
		sleep(1);
	}
	
	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(tid[i], NULL);
	}

	restore();
}

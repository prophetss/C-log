#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "timer.h"
#include "log.h"


#define NUM_THREADS	3

#define PASSWORD	"aaa"

#define FLAG	(ENCRYPT|COMPRESS)

#define MAX_FILE_SIZE	102400	//byte

#define MAX_BACK_FILE_NUM	2

#define IO_BUFFER_SIZE	10240

#define PRINT_TIMES		1000

#define UNUSED_RETURN(x)	(void)((x)+1)

#define PATH_MAX        4096

void test_write(void* arg)
{
	int t = *(int*)arg;
	
	int thd = timekeeper_start_auto();
	
	//create two file handles and one stream handle
	char file_path1[64] = {0};
	char file_path2[64] = {0};
	sprintf(file_path1, "./%d/fh1", t);
	sprintf(file_path2, "./%d/fh2", t);
	UNUSED_RETURN(mkdir(file_path1, 0755));
	UNUSED_RETURN(mkdir(file_path2, 0755));
	char file_name1[64] = {0};
	char file_name2[64] = {0};
	sprintf(file_name1, "%s/sample.TXT", file_path1);
	sprintf(file_name2, "%s/sample.TXT", file_path2);
	void *fh1 = file_handle_create(file_name1, MAX_FILE_SIZE, MAX_BACK_FILE_NUM, IO_BUFFER_SIZE, FLAG, PASSWORD);
	void *fh2 = file_handle_create(file_name2, 51200, 1, 0, 0, NULL);
	void* sh1 = stream_handle_create(ERROR_STDERR);
	sh1 = set_stream_param(sh1, LOG_ERROR, FRED, NULL, NULL);
	sh1 = set_stream_param(sh1, LOG_DEBUG, FGREEN, NULL, BLINK);
	sh1 = set_stream_param(sh1, LOG_WARN, NULL, BGCYAN, UNDERLINE);
	
	//add to handle list
	log_t *lh = add_to_handle_list(NULL, fh1);
	lh = add_to_handle_list(lh, fh2);
	lh = add_to_handle_list(lh, sh1);

	static const char *msg = "So we beat on, boats against the current, borne back ceaselessly into the past.";
	for (int m = 0; m < PRINT_TIMES; m++) {
		log_debug(lh, "[DEBUG]...%d--%s\n", m, msg);
		log_info(lh, "[INFO]...%d--%s\n", m, msg);
		log_warn(lh, "[WARN]...%d--%s\n", m, msg);
		log_error(lh, "[ERROR]...%d--%s\n", m, msg);
	}
	
	log_destory(lh);
	
	double tim;
	timekeeper_pause(thd, &tim);
	
	printf("thread%d runtime=%f\n", t, tim);
}


void restore_file(char *fn)
{
	char nfn[PATH_MAX] = {0};
	sprintf(nfn, "%s.ori", fn);	// raw result
	
	char tmpname[] = "tmp.XXXXXX"; // for uncompress
	UNUSED_RETURN(mkstemp(tmpname));

	char *tfn = fn;
	
	if (FLAG & COMPRESS) { // uncompress
		log_file_uncompress(fn, tmpname);
		tfn = tmpname;
	}

	if (FLAG & ENCRYPT) {	// decipher
		log_file_decipher(tfn, nfn, PASSWORD);
	}
		
	remove(tmpname);
}


void restore()
{
	printf("restore start!\n");
	
	int i = 0;
	for (i = 0; i < NUM_THREADS; i++) {
		DIR *dir;
		struct dirent *ptr;
		char file_path[64] = {0};
		sprintf(file_path, "./%d/fh1/", i);
		if ((dir=opendir(file_path)) == NULL) {
			perror("open the dir error!");
			return;
		}
		int j = 0, k = 0;
		char file_paths[MAX_BACK_FILE_NUM][PATH_MAX] = {0};
		while ((ptr=readdir(dir)) != NULL) {
			if(ptr->d_type == 8) {	//file
				const char *c = strrchr(ptr->d_name, '.');
				char full_path[PATH_MAX] = { 0 };
				sprintf(full_path, "%s%s", file_path, ptr->d_name);
				if (strcmp(c, ".ori") == 0) {
					// remove old raw log file
					remove(full_path);
				}
				else {
					//save new log file path
					if (j < MAX_BACK_FILE_NUM) {
						memcpy(file_paths[j++], full_path, PATH_MAX);
					}
					else {
						printf("restore error!\n");
						return;
					}
				}
			}
		}
		closedir(dir);
		for (; k <j; k++) {
			// generates raw log files
			restore_file(file_paths[k]);
		}
	}
	printf("restore end!\n");
}

int main()
{
	// make a temparary directory for save result
	UNUSED_RETURN(mkdir("./tmp", 0755));

	if (chdir("./tmp/") == -1) {
		perror("change the dir error!");
		exit(0);
	}

	int i;
	for (i = 0; i < NUM_THREADS; ++i) {
		char dir[8] = { 0 };
		sprintf(dir, "%d", i);
		UNUSED_RETURN(mkdir(dir, 0755));
	}

	pthread_t tid[NUM_THREADS];
	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_create(&tid[i], NULL, (void*)&test_write, (void*)&i);
		sleep(1);
	}
	
	for (i = 0; i < NUM_THREADS; ++i) {
		pthread_join(tid[i], NULL);
	}

	// the results are predictable
	restore();
}

#include <stdio.h>
#include <process.h>
#include <Windows.h>
#include "log.h"


void write_log(void* arg) 
{
	int t = *(int*)arg;
	timekeeper_set(t);
	int m;
	for (m =0; m < 1000; m++)
		WRITE_LOG(LOG_RUN, "ssssssssssssssssss");
	printf("thread%d runtime=%f\n", t, timekeeper_get(t));
}
int main()
{
	int i;
	for (i = 1; i < 10; i++)
	{
		_beginthread(write_log, 0, &i);
		Sleep(2);
	}
	system("pause");
}
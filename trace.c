#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include "config.h"
#include "trace.h"

static void trace_print(int signal_type)
{
	int trace_id = -1;
	int i = 0;
	void *buffer[100];
	char **info = NULL;
	char trace_buff[TRACE_BUF_MAX];

	trace_id = backtrace(buffer, TRACE_BUF_MAX);

	info = backtrace_symbols(buffer, trace_id);
	if (NULL == info)
		return;

	for (i = 0; i < trace_id; i++) {
		sprintf(trace_buff, "echo \"%s\" >> %s_%d", info[i], TRACE_PRINT_PATH, signal_type);
		if(system(trace_buff));
	}

	sprintf(trace_buff, "echo \"###################################\" >> %s_%d", TRACE_PRINT_PATH, signal_type);
	if(system(trace_buff));
}

static void signal_hadle_fun(int signal_type)
{
	trace_print(signal_type);
	fprintf(stderr, "receive a exit signal!\n");
	exit(0);
}

void trace_init()
{
	/* 异常退出信号注册 */
	signal(SIGHUP, signal_hadle_fun);
	signal(SIGINT, signal_hadle_fun);
	signal(SIGQUIT, signal_hadle_fun);
	signal(SIGILL, signal_hadle_fun);
	signal(SIGTRAP, signal_hadle_fun);
	signal(SIGABRT, signal_hadle_fun);
	signal(SIGBUS, signal_hadle_fun);
	signal(SIGFPE, signal_hadle_fun);
	signal(SIGKILL, signal_hadle_fun);
	signal(SIGSEGV, signal_hadle_fun);
	signal(SIGPIPE, signal_hadle_fun);
	signal(SIGTERM, signal_hadle_fun);
}
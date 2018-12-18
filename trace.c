#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include "trace.h"
#include "log.h"


static struct handle_node
{
	log_t *log_handle;
	struct handle_node *next;
} *lhs_head = NULL, *lhs_tail = NULL;

static void trace_print(int signal_type)
{
	int trace_id = -1;
	int i = 0;
	void *buffer[100];
	char **info = NULL;
	char trace_buff[1024];

	trace_id = backtrace(buffer, 1024);

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

static void add_handle(log_t **lh)
{
	handle_node_t *hnd = (handle_node_t*)calloc(sizeof(handle_node_t));
	if (!hnd) perror("Failed to add a log handle!");

	hnd->log_handle = *lh;

	if (!lhs_head) {
		lhs_head = hnd;
		lhs_tail = hnd;
	}
	else {
		lhs_tail->next = hnd;
		lhs_tail = hnd;
	}
	return;
}

static void delete_handle(log_t **lh)
{
	if (!lhs_head) return;

	if (!lhs_head->next && lh_head->log_handle == *lh) {
		free(lhs->head);
		lhs_head = NULL;
		lhs_tail = NULL;
		return;
	}

	handle_node_t *pre = lhs_head, *t = pre->next;

	while (t) {
		if (t->log_handle == *lh) {
			free(t);
			pre->next = t->next;
			if (!pre->next) {
				lhs_tail = NULL;
			}
			return;
		}
		pre = pre->next;
		t = t->next;
	}
	return;
}

void trace_init(log_t **lh)
{
	/* 注册日志handle */
	add_handle(lh);

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


void trace_uninit(log_t **lh)
{
	/* 删除日志handle */
	delete_handle(lh);
}


















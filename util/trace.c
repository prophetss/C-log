#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include "list.h"
#include "log.h"


#define UNUSED_RETURN(x)	(void)((x)+1)

static volatile int _is_initialized = 0;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static list_node *g_handle_list = NULL;

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
		UNUSED_RETURN(system(trace_buff));
	}

	sprintf(trace_buff, "echo \"###################################\" >> %s_%d", TRACE_PRINT_PATH, signal_type);
	UNUSED_RETURN(system(trace_buff));
}

static void signal_hadle_fun(int signal_type)
{
	fprintf(stderr, "receive a exit signal!\n");
	pthread_mutex_lock(&g_mutex);
	list_node *it = g_handle_list;
	while(it) {
		log_flush((log_t*)it->data);
		it = it->next;
	}
	pthread_mutex_unlock(&g_mutex);
	fprintf(stderr, "all logger handles flush!\n");
	trace_print(signal_type);
	exit(0);
}

static void trace_init()
{
	if (_is_initialized) return;

	g_handle_list = list_create(NULL);
	
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
	
	_is_initialized = 1;
}

void trace_add_handle(void **l)
{
	pthread_mutex_lock(&g_mutex);
	trace_init();
	g_handle_list = list_insert_beginning(g_handle_list, *l);
	pthread_mutex_unlock(&g_mutex);
}

void trace_remove_handle(void **l)
{
	pthread_mutex_lock(&g_mutex);
	list_remove_by_data(&g_handle_list, *l);
	if (!g_handle_list->data && !g_handle_list->next) {
		list_destroy(&g_handle_list);
		_is_initialized = 0;
	}
	pthread_mutex_unlock(&g_mutex);
}

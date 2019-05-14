
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "error.h"
#include "log_stream.h"

#define RESET		"\x1B[0m"

#define NON_NULL(x)	((x) ? x : "")

void write_stream(handle_stream_t* sh, log_level_t level, char* msg)
{
	fprintf((sh->streams)[level],"%s%s%s", (sh->style)[level], msg, RESET);
}

void* _stream_handle_create(uint8_t streams)
{
	handle_stream_t* sh = calloc(1, S_LOG_STREAM_SIZE);
	assert(sh != NULL);
	int i = _ERROR_LEVEL+1;
	while (--i >= _DEBUG_LEVEL) {
		(sh->streams)[i] = streams & (i + 1) ? stderr : stdout;
	}
	return (void*)sh;
}

void* set_stream_param(void* sh, log_level_t level, const char* color, const char* bgcolor, const char* style)
{
	if (!sh || ((log_handle_t*)sh)->tag != S_MODE || !(((log_handle_t*)sh)->hld)) {
		error_display("INPUT Stream Handle ERROR!");
		return sh;
	}
	memset((((handle_stream_t*)(((log_handle_t*)sh)->hld))->style)[level], 0, STYLE_SIZE);
	sprintf((((handle_stream_t*)(((log_handle_t*)sh)->hld))->style)[level], "%s%s%s", NON_NULL(color), NON_NULL(bgcolor), NON_NULL(style));
	return sh;
}

void stream_handle_flush()
{
	fflush(stdout);
}

void stream_handle_destory(handle_stream_t* sh)
{
	stream_handle_flush();
	free(sh);
}

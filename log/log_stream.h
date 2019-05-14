#ifndef _LOG_STREAM_H_
#define _LOG_STREAM_H_

#include "log.h"

#define STYLE_SIZE	64

typedef struct handle_stream_t
{
	char			style[S_LOG_LEVEL][STYLE_SIZE];
	FILE*			streams[S_LOG_LEVEL];
} handle_stream_t;

#define S_LOG_STREAM_SIZE	sizeof(handle_stream_t)


void* _stream_handle_create(uint8_t streams);

void write_stream(handle_stream_t* sh, log_level_t level, char* msg);

void stream_handle_flush();

void stream_handle_destory(handle_stream_t* sh);


#endif // !_LOG_STREAM_H_
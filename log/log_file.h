#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#include "log.h"


typedef struct handle_file_t
{
	uint8_t 		*wkey;
	int    	    	cflag;
	char 			*file_path;
	FILE			*f_log;
	char 			*io_buf;
	size_t 			io_cap;
	size_t 			max_file_size;
	size_t 			cur_file_size;
	size_t 			cur_bak_num;
	size_t 			max_bak_num;
	pthread_mutex_t mutex;
} handle_file_t;

#define S_LOG_FILE_SIZE	sizeof(handle_file_t)

#define AES_128 	0x10

#define AES_EX(n)	(((n)&(AES_128-1)) != 0 ? ((n)|(AES_128-1))+1 : (n))

//round up to the next power of 16 for aes
#define EX_SINGLE_LOG_SIZE AES_EX(SINGLE_LOG_SIZE)


void* _file_handle_create(const char* log_filename, size_t max_file_size, size_t max_file_bak, size_t max_iobuf_size, uint8_t cflag, const char* password);



/**
 * write message to file.
 * @param fh - log flie handle.
 * @param msg - message.
 */
void write_file(handle_file_t *fh, char *msg, size_t len);


/**
 * flush io buffer to file.
 * @param lh - log flie handle.
 */
void file_handle_flush(handle_file_t *lh);

/**
 * destory flie handle.
 * @param lh - log handle.
 */
void file_handle_destory(handle_file_t *lh);



#endif //!_LOG_FILE_H_


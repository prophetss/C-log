#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "config.h"
#include "lz4.h"
#include "file_util.h"


#define	LOG_COMPRESS_BUF (LOG_FILE_SIZE_MAX+LOG_IO_BUF_MAX)

int get_file_size(const char *fullfilepath)
{
	struct stat sbuf;
	if (stat(fullfilepath, &sbuf) != SUCCESS) {
		perror("access file attributes failed!");
		return -1;
	}
	return sbuf.st_size;
}

char* get_file_name(const char *fullfilepath)
{
	const char *c = strrchr(fullfilepath, '/');
	(NULL == c) ? (c = fullfilepath) : (c = fullfilepath + 1);
	size_t file_name_size = fullfilepath + strlen(fullfilepath) + 1 - c;
	char *file_name = NULL;
	file_name = (char*)malloc(file_name_size);
	if (NULL == file_name)
		return NULL;
	memcpy(file_name, c, file_name_size);
	return file_name;
}

char* get_file_path(const char *fullfilepath)
{
	/*unimplemented for the moment*/
	return NULL;
}

typedef struct
{
	FILE   *src_file;
	char   *src_buf;
	size_t  src_buf_size;
	FILE   *dst_file;
	char   *dst_buf;
	size_t  dst_buf_size;
} s_res_t;

static int init_compress(const char *src_filepath, const char *dst_filepath, s_res_t *ress)
{
	ress->src_file = fopen(src_filepath, "r");
	if (NULL == ress->src_file) {
		perror("compress failed to open src_filepath!");
		return FAILED;
	}

	ress->dst_file = fopen(dst_filepath, "w+");
	if (NULL == ress->dst_file)	{
		perror("compress failed to open dst_filepath!");
		fclose(ress->src_file);
		return FAILED;
	}

	ress->src_buf = malloc(LOG_COMPRESS_BUF * sizeof(char));
	if (NULL == ress->src_buf) {
		perror("compress src memory allocation failed!");
		exit(errno);
	}
	ress->src_buf_size = LOG_COMPRESS_BUF;

	const int max_dst_size = LZ4_compressBound(LOG_COMPRESS_BUF);
	ress->dst_buf_size = max_dst_size;
	ress->dst_buf = malloc(max_dst_size * sizeof(char));
	if (NULL == ress->dst_buf) {
		perror("compress dst memory allocation failed!");
		exit(errno);
	}

	return SUCCESS;
}

static int init_decompress(const char *src_filepath, const char *dst_filepath, s_res_t *ress)
{
	ress->src_file = fopen(src_filepath, "r");
	if (NULL == ress->src_file) {
		perror("decompress failed to open src_filepath!");
		return FAILED;
	}

	if (fseek(ress->src_file, 0L, SEEK_END) != 0) {
		perror("decompress init failed!");
		exit(errno);
	}

    size_t src_file_size = ftell(ress->src_file);
    	if (fseek(ress->src_file, 0L, SEEK_SET) != 0) {
		perror("decompress init failed!");
		exit(errno);
	}

	ress->dst_file = fopen(dst_filepath, "w+");
	if (NULL == ress->dst_file)	{
		perror("decompress failed to open dst_filepath!");
		fclose(ress->src_file);
		return FAILED;
	}

	ress->src_buf = malloc(src_file_size * sizeof(char)+1024);
	if (NULL == ress->src_buf) {
		perror("decompress src memory allocation failed!");
		exit(errno);
	}
	ress->src_buf_size = src_file_size+1024;

	ress->dst_buf = malloc(LOG_COMPRESS_BUF * sizeof(char));
	if (NULL == ress->dst_buf) {
		perror("decompress dst memory allocation failed!");
		exit(errno);
	}
	ress->dst_buf_size = LOG_COMPRESS_BUF;

	return SUCCESS;
}

static void uninit_resources(s_res_t *ress)
{
	fclose(ress->src_file);
	fclose(ress->dst_file);
	free(ress->src_buf);
	free(ress->dst_buf);
	ress->src_buf_size = 0;
	ress->dst_buf_size = 0;
}

int compress_file(const char *src_filepath, const char *dst_filepath)
{
	s_res_t compress_res;
	if (init_compress(src_filepath, dst_filepath, &compress_res) != SUCCESS) {
		return FAILED;
	}

	int compressed_data_size, read_data_size;
	if ((read_data_size = fread(compress_res.src_buf, sizeof(char), LOG_COMPRESS_BUF, compress_res.src_file)) == 0) {
		perror("compress failed to read data!");
		uninit_resources(&compress_res);
		return FAILED;
	}
	compress_res.src_buf_size = read_data_size;

	compressed_data_size = LZ4_compress_default(compress_res.src_buf, compress_res.dst_buf, compress_res.src_buf_size, compress_res.dst_buf_size);
	if (compressed_data_size <= 0) {
		fprintf(stderr, "%s\n", "compress failed,a negative result from LZ4_compress_default!");
		uninit_resources(&compress_res);
		return FAILED;
	}

	if (fwrite(compress_res.dst_buf, sizeof(char), compressed_data_size, compress_res.dst_file) != compressed_data_size) {
		perror("compress failed to write data!");
		uninit_resources(&compress_res);
		return FAILED;
	}

	uninit_resources(&compress_res);

	return SUCCESS;
}

int decompress_file(const char *src_filepath, const char *dst_filepath)
{
	s_res_t decompress_res;
	if (init_decompress(src_filepath, dst_filepath, &decompress_res) != SUCCESS) {
		return FAILED;
	}

	int decompressed_data_size, read_data_size;
	if ((read_data_size = fread(decompress_res.src_buf, sizeof(char), decompress_res.src_buf_size, decompress_res.src_file)) == 0) {
		perror("decompress failed to read data!");
		uninit_resources(&decompress_res);
		return FAILED;
	}
	decompress_res.src_buf_size = read_data_size;

	decompressed_data_size = LZ4_decompress_safe(decompress_res.src_buf, decompress_res.dst_buf, decompress_res.src_buf_size, decompress_res.dst_buf_size);
	if (decompressed_data_size <= 0) {
		fprintf(stderr, "%s\n", "decompress failed,a negative result from LZ4_decompress_safe!");
		uninit_resources(&decompress_res);
		return FAILED;
	}

	if (fwrite(decompress_res.dst_buf, sizeof(char), decompressed_data_size, decompress_res.dst_file) != decompressed_data_size) {
		perror("decompress failed to write data!");
		uninit_resources(&decompress_res);
		return FAILED;
	}

	uninit_resources(&decompress_res);

	return SUCCESS;
}

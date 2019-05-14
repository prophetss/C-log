#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "lz4.h"
#include "md5.h"


#define KB *( 1<<10)
#define MB *( 1<<20)
#define GB *(1U<<30)

#define MAX_MEM    ((2 GB - 64 MB) / 2)


/*************************************
*  Basic Types
**************************************/
#if defined(__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
# include <stdint.h>
  typedef  uint8_t BYTE;
  typedef uint16_t UINT16;
  typedef uint32_t UINT32;
  typedef  int32_t INT32;
  typedef uint64_t UINT64;
  typedef int64_t  INT64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      UINT16;
  typedef unsigned int        UINT32;
  typedef   signed int        INT32;
  typedef unsigned long long  UINT64;
  typedef long long  		  INT64;
#endif


#define _exit_throw(...) do {	\
	fprintf(stderr, "Errno : %d, msg : %s\n", errno, strerror(errno));	\
	fprintf(stderr, __VA_ARGS__);	\
} while(0)



//every lz4 block header
typedef struct {
	UINT32	uncomp_size;	//uncompressed size
	UINT32	comp_size;	//compressed size
	BYTE	hash_code[MD5_HASHBYTES];	//md5
} lz4_header_t;

#define HEADER_SIZE	sizeof(lz4_header_t)

#define ADD_HEAD(n)	((n) + HEADER_SIZE)


#if !defined(S_ISREG)
	#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

static UINT64 _get_file_size(const char *path)
{
	int ret;
	struct stat sbuf;
	ret = stat(path, &sbuf);
	if (ret || !S_ISREG(sbuf.st_mode)) return 0;
    return (UINT64)sbuf.st_size;
}

static size_t _find_available_mem(UINT64 src_size)
{
	UINT64 required_mem = src_size;

	size_t const step = 64 MB;
	required_mem = (((required_mem >> 26) + 1) << 26);
	required_mem += 2*step;

    if (required_mem > MAX_MEM) required_mem = MAX_MEM;

	void *test_mem = NULL;
    while (!test_mem) {
    	if (required_mem > step) required_mem -= step;
        else required_mem >>= 1;
        test_mem = malloc((size_t)(required_mem + LZ4_compressBound(required_mem)));
    }
    free (test_mem);

    /* keep some memory */
    if (required_mem > step) required_mem -= step;
    else required_mem >>= 1;

    return (size_t)required_mem;
}

static size_t _select_mem_size(const char *src_filepath)
{
	UINT64 src_file_size = _get_file_size(src_filepath);
	const size_t available_mem_size = _find_available_mem(src_file_size);
	if (available_mem_size > src_file_size) return src_file_size;
	/* there may be insufficient memory to read the entire file */
	return available_mem_size;
}

static size_t _data_assemble(char *data, const UINT32 src_size, const UINT32 dst_size)
{
	BYTE digest[MD5_HASHBYTES];
	MD5Data((BYTE*)ADD_HEAD(data), dst_size, digest);
	lz4_header_t *lhd = (lz4_header_t*)data;
	lhd->uncomp_size = src_size;
	lhd->comp_size = dst_size;
	memcpy(lhd->hash_code, digest, MD5_HASHBYTES);
	return ADD_HEAD(dst_size);
}


FILE* _open_src_file(const char *in_filename)
{
	return fopen(in_filename, "r");
}

FILE* _open_dst_file(const char *out_filename)
{
	return fopen(out_filename, "w+");
}

void lz4_file_compress(const char *in_filename, const char *out_filename)
{
	assert(in_filename);
	assert(out_filename);
	
	const size_t available_size = _select_mem_size(in_filename);
	const int compress_bound_size = LZ4_compressBound(available_size);

	char *src_buf = (char*)malloc(available_size);
	char *dst_buf =  (char*)malloc(ADD_HEAD(compress_bound_size));
	if (!src_buf || !dst_buf) _exit_throw("Failed to alloc the memory!");

	FILE *src_fd, *dst_fd;
	if ((src_fd = _open_src_file(in_filename)) == NULL) _exit_throw("Failed to open the in file!");
	if ((dst_fd = _open_dst_file(out_filename)) == NULL) _exit_throw("Failed to open the out file!");

	while (1) {
		const size_t read_data_size = fread(src_buf, sizeof(char), available_size, src_fd);
		if (read_data_size == 0) break;
		assert(read_data_size <= available_size);
	 	const int compressed_data_size = LZ4_compress_default(src_buf, ADD_HEAD(dst_buf), read_data_size, compress_bound_size);
		assert(compressed_data_size > 0);
		assert(compressed_data_size < compress_bound_size);
		const size_t out_size = _data_assemble(dst_buf, (UINT32)read_data_size, (UINT32)compressed_data_size);
		const size_t write_data_size = fwrite(dst_buf, sizeof(char), out_size, dst_fd);
		if (write_data_size != out_size) _exit_throw("Failed to write to the out file!");
	}
	
	fclose(src_fd);
	fclose(dst_fd);
	free(src_buf);
}

int _check_block(lz4_header_t *lhd, char *data)
{
	BYTE digest[MD5_HASHBYTES];
	MD5Data((BYTE*)data, lhd->comp_size, digest);
	if (memcmp(digest, lhd->hash_code, MD5_HASHBYTES) != 0) return 1;
	return 0;
}

int _data_disassemble(FILE *f, char **block_data, lz4_header_t *lhd)
{
	if (fread(lhd, sizeof(char), HEADER_SIZE, f) != HEADER_SIZE) return 1;
	if (lhd->uncomp_size + lhd->comp_size > MAX_MEM) return 1;
	if (*block_data == NULL) *block_data = (char*)malloc(lhd->comp_size);
	if (*block_data == NULL) return 1;
	size_t data_size = fread(*block_data, sizeof(char), lhd->comp_size, f);
	if ((UINT32)data_size != lhd->comp_size) return 1;
	if (0 != _check_block(lhd, *block_data)) return 1;
	return 0;
}

int _lz4_uncompress(const char *src, size_t src_size, char **dst, size_t dst_size)
{
	*dst = realloc(*dst, dst_size);
	if (*dst == NULL) return -1;
	memset(*dst, 0, dst_size);
	return LZ4_decompress_safe(src, *dst, src_size, dst_size);
}

int lz4_file_uncompress(const char *in_filename, const char *out_filename)
{
	assert(in_filename);
	assert(out_filename);

	INT64 file_size = (INT64)_get_file_size(in_filename);
	assert(file_size > 0);

	FILE *src_fd, *dst_fd;
	if ((src_fd = _open_src_file(in_filename)) == NULL) _exit_throw("Failed to open the in file!");
	if ((dst_fd = _open_dst_file(out_filename)) == NULL) _exit_throw("Failed to open the out file!");
	int uncompressed_data_size;
	lz4_header_t lz4_header;
	char *src_buf = NULL, *dst_buf = NULL;
	while (file_size > 0) {
		if(0 != _data_disassemble(src_fd, &src_buf, &lz4_header)) break;
		uncompressed_data_size = _lz4_uncompress(src_buf, lz4_header.comp_size, &dst_buf, lz4_header.uncomp_size);
	 	if(uncompressed_data_size <= 0) break;
		if (fwrite(dst_buf, sizeof(char), uncompressed_data_size, dst_fd) != (size_t)uncompressed_data_size) break;
		file_size -= (lz4_header.comp_size + HEADER_SIZE);
	}
	
	fclose(src_fd);
	fclose(dst_fd);
	if (src_buf != NULL) free(src_buf);
	if (dst_buf != NULL) free(dst_buf);

	if (file_size != 0) return 1;
	
	return 0;
}

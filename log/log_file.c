#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include "md5.h"
#include "lz4_file.h"
#include "trace.h"
#include "error.h"
#include "log_file.h"

#define PATH_MAX        4096

extern int aes_init(uint8_t* key, size_t key_len, uint8_t** w);

extern void cipher(uint8_t* in, uint8_t* out, uint8_t* w);

extern void inv_cipher(uint8_t* in, uint8_t* out, uint8_t* w);

static void _update_file(handle_file_t* fh);

typedef struct
{
	uint8_t* wkey;
	int    	    	cflag;
	char* file_path;
	FILE* f_log;
	char* io_buf;
	size_t 			io_cap;
	size_t 			max_file_size;
	size_t 			cur_file_size;
	size_t 			cur_bak_num;
	size_t 			max_bak_num;
	pthread_mutex_t mutex;
} file_handle_t;


#if !defined(S_ISREG)
#  define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

static uint64_t _get_file_size(const char* fullfilepath)
{
	int ret;
	struct stat sbuf;
	ret = stat(fullfilepath, &sbuf);
	if (ret || !S_ISREG(sbuf.st_mode)) return 0;
	return (uint64_t)sbuf.st_size;
}

static void _log_lock(handle_file_t * l)
{
	pthread_mutex_lock(&l->mutex);
}

static void _log_unlock(handle_file_t * l)
{
	pthread_mutex_unlock(&l->mutex);
}

static int _pwd_init(uint8_t * *wkey, const char* pwd)
{
	uint8_t key[MD5_HASHBYTES];		//MD5_HASHBYTES==AES_128==16byte
	MD5Data((uint8_t*)pwd, strlen(pwd), key);
	return aes_init(key, AES_128, wkey);
}

static int _iobuf_init(handle_file_t * fh, size_t io_ms, int flag, const char* pwd)
{
	fh->io_cap = io_ms;

	fh->io_buf = calloc(1, fh->io_cap);

	if (!fh->io_buf) return 1;

	if (flag & ENCRYPT) {
		assert(pwd != NULL);
		if (0 != _pwd_init(&fh->wkey, pwd)) {
			free(fh->io_buf);
			return 1;
		}
	}

	return 0;
}

static int _file_init(handle_file_t * fh, const char* lp, size_t ms, size_t mb)
{
	fh->max_file_size = ms;
	fh->file_path = strdup(lp);
	if (!fh->file_path) return 1;
	fh->cur_file_size = _get_file_size(lp);
	fh->cur_bak_num = 0;
	fh->max_bak_num = mb;
	return 0;
}


void* _file_handle_create(const char* log_filename, size_t max_file_size, size_t max_file_bak, size_t max_iobuf_size, uint8_t cflag, const char* password)
{
	if (!log_filename) return NULL;
	handle_file_t* l = calloc(1, S_LOG_FILE_SIZE);
	if (!l) exit_throw("failed to calloc!");
	int ret;
	ret = _file_init(l, log_filename, max_file_size, max_file_bak);
	if (ret != 0) {
		free(l);
		return NULL;
	}

	ret = _iobuf_init(l, max_iobuf_size, cflag, password);
	if (ret != 0) {
		free(l->file_path);
		free(l);
		return NULL;
	}

	ret = pthread_mutex_init(&l->mutex, NULL);
	assert(ret == 0);

	trace_add_handle((void**)& l);

	l->cflag = cflag;

	return (void*)l;
}

static void _lock_uninit(handle_file_t * l)
{
	pthread_mutex_destroy(&l->mutex);
}

static size_t _real_len(const char* s)
{
	for (int i = 0; i < AES_128; i++)
		if (!s[i]) return i;
	return AES_128;
}

static int _file_decipher(const char* in_filepath, size_t in_filesize, const char* out_filepath, uint8_t * w)
{
	FILE* fi, * fo;

	fi = fopen(in_filepath, "r");
	if (fi == NULL) {
		perror("Failed to open the in file!");
		return 1;
	}

	fo = fopen(out_filepath, "w+");
	if (fo == NULL) {
		fclose(fi);
		perror("Failed to open the out file!");
		return 1;
	}

	while (in_filesize > 0) {
		char outbuf[AES_128];
		if (fread(outbuf, sizeof(char), AES_128, fi) != AES_128) break;
		inv_cipher((uint8_t*)outbuf, (uint8_t*)outbuf, w);
		//cut off the end of zeros
		const size_t len = _real_len(outbuf);
		if (fwrite(outbuf, sizeof(char), len, fo) != len) break;
		in_filesize -= AES_128;
	}

	fclose(fi);
	fclose(fo);

	if (in_filesize > 0) return 1;

	return 0;
}

static void  _file_compress(const char* src_filename, const char* dst_filename)
{
	lz4_file_compress(src_filename, dst_filename);
}

static void _backup_file(handle_file_t * fh)
{
	char new_filename[PATH_MAX] = { 0 };

	if (fh->cflag & COMPRESS) {	//compress
		sprintf(new_filename, "%s.bak%lu.lz4", fh->file_path, fh->cur_bak_num % fh->max_bak_num);
		_file_compress(fh->file_path, new_filename);
		remove(fh->file_path);
	}
	else {
		sprintf(new_filename, "%s.bak%lu", fh->file_path, fh->cur_bak_num % fh->max_bak_num);
		rename(fh->file_path, new_filename);
	}
}

static void _update_file(handle_file_t * fh)
{
	if (!fh->f_log) return;

	fclose(fh->f_log);
	fh->f_log = NULL;
	if (fh->max_bak_num == 0) {	//without backup
		remove(fh->file_path);
	}
	else {
		_backup_file(fh);
	}
	fh->cur_bak_num++;
	fh->cur_file_size = 0;
}

void file_handle_flush(handle_file_t * fh)
{
	if (fh == NULL) return;

	_log_lock(fh);

	fclose(fh->f_log);
	fh->f_log = NULL;

	_log_unlock(fh);
}

void file_handle_destory(handle_file_t * fh)
{
	if (fh == NULL) return;

	_log_lock(fh);

	fclose(fh->f_log);

	fh->f_log = NULL;
	
	_backup_file(fh);

	free(fh->io_buf);

	free(fh->wkey);

	trace_remove_handle((void**)& fh);

	_log_unlock(fh);

	_lock_uninit(fh);

	free(fh);

	fh = NULL;
}

static int _file_handle_request(handle_file_t * fh)
{
	if (fh->f_log) return 0;

	fh->f_log = fopen(fh->file_path, "a+");

	if (!fh->f_log) return 1;

	if (setvbuf(fh->f_log, fh->io_buf, _IOFBF, fh->io_cap) != 0) return 1;

	return 0;
}

char* _buf_encrypt(const char* msg, uint8_t * wkey, size_t len)
{
	uint8_t* e_msg = (uint8_t*)malloc(len);
	if (!e_msg) exit_throw("Failed to Memory Allocation!");
	uint8_t out[AES_128];
	size_t i = 0;
	while (len > i) {
		cipher((uint8_t*)msg + i, out, wkey);
		memcpy(e_msg + i, out, AES_128);
		i += AES_128;
	}
	return (char*)e_msg;
}

void write_file(handle_file_t * fh, char* msg, size_t len)
{
	if (fh->cflag & ENCRYPT) {
		len = AES_EX(len);
		msg = _buf_encrypt(msg, fh->wkey, len);
	}

	_log_lock(fh);

	if (0 != _file_handle_request(fh)) {
		perror("Failed to open the file\n");
		return;
	}

	size_t print_size = fwrite(msg, sizeof(char), len, fh->f_log);
	if (print_size != len) {
		error_display("Failed to write the file path(%s), len(%lu)\n", fh->file_path, print_size);
		return;
	}
	fh->cur_file_size += print_size;

	if (fh->cur_file_size > fh->max_file_size) {
		_update_file(fh);
	}

	_log_unlock(fh);

	if (fh->cflag & ENCRYPT) free(msg);
}

int log_file_decipher(const char* in_filename, const char* out_filename, const char* password)
{
	if (!in_filename || !out_filename || !password) {
		error_display("Input error!\n");
		return 1;
	}

	uint64_t file_size = _get_file_size(in_filename);
	if (file_size == 0 || file_size % AES_128 != 0) {
		error_display("Input file error!(%s)\n", in_filename);
		return 1;
	}

	uint8_t* w = NULL;
	if (0 != _pwd_init(&w, password)) return 1;

	int ret = _file_decipher(in_filename, file_size, out_filename, w);

	free(w);

	return ret;
}
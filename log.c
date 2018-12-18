#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include "md5.h"
#include "lz4_file.h"
#include "log.h"



#if !defined(S_ISREG)
#  define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#define AES_128 	16

#define aes_ex(n)	((n)%AES_128 != 0 ? ((n)/AES_128+1)*AES_128 : (n))

#define EX_SINGLE_LOG_SIZE aes_ex(SINGLE_LOG_SIZE)	//must be sized as a multiple of 16 bytes for aes

extern void trace_init(log_t **lh);

extern void trace_uninit(log_t **lh);

extern int aes_init(uint8_t *key, size_t key_len, uint8_t **w);

extern void cipher(uint8_t *in, uint8_t *out, uint8_t *w);

extern void inv_cipher(uint8_t *in, uint8_t *out, uint8_t *w);

static void _update_file(log_t *lh);

static uint64_t _get_file_size(const char *fullfilepath)
{
	int ret;
	struct stat sbuf;
	ret = stat(fullfilepath, &sbuf);
	if (ret || !S_ISREG(sbuf.st_mode)) return 0;
    return (uint64_t)sbuf.st_size;
}

static void _log_lock(log_t *l)
{
	pthread_mutex_lock(&l->mutex);
}

static void _log_unlock(log_t *l)
{
	pthread_mutex_unlock(&l->mutex);
}

static int _pwd_init(uint8_t **wkey, const char *pwd)
{
	uint8_t key[AES_128];
	MD5Data((uint8_t*)pwd, strlen(pwd), key);
	if ( 0 != aes_init(key, AES_128, wkey)) return 1;
	return 0;
}

static int _iobuf_init(log_t *lh, size_t io_ms, int flag, const char *pwd)
{
	lh->io_cap = io_ms;
	
	if (flag & ENCRYPT) {
		assert(pwd != NULL);
		if (0 != _pwd_init(&lh->wkey, pwd)) return 1;
	}

	lh->io_buf = calloc(1, lh->io_cap);
	
	if (!lh->io_buf) {
		if (!lh->wkey) free(lh->wkey);
		return 1;
	}
	
	return 0;
}

static int _file_init(log_t *lh, const char *lp, size_t ms, size_t mb)
{
	lh->max_file_size = ms;
	lh->file_path = strdup(lp);
	if (!lh->file_path) return 1;
	lh->cur_file_size = _get_file_size(lp);
	lh->cur_bak_num = 0;
	lh->max_bak_num = mb;
	return 0;
}


log_t* log_create(const char *log_file_path, size_t max_file_size, size_t max_file_bak, size_t max_iobuf_size, int cflag, const char *password)
{
	assert(log_file_path);

	log_t *l = calloc(1, S_LOG_SIZE);
	if (!l) return NULL;

	int ret;
	ret =  _iobuf_init(l, max_iobuf_size, cflag, password);
	if (ret != 0) {
		free(l);
		return NULL;
	}
	
	ret = _file_init(l, log_file_path, max_file_size, max_file_bak);
	if(ret != 0) {
		free(l->io_buf);
		free(l);
		return NULL;
	}

	ret = pthread_mutex_init(&l->mutex, NULL);
	if (ret != 0) {
		free(l->io_buf);
		free(l->file_path);
		free(l);
		return NULL;
	}

	l->cflag = cflag;
	
	return l;
}

static void _lock_uninit(log_t *l)
{
	pthread_mutex_destroy(&l->mutex);
}

void log_destory(log_t *lh)
{
	if (lh == NULL) return;

	_log_lock(lh);

	_update_file(lh);

	free(lh->io_buf);

	free(lh->wkey);

	_log_unlock(lh);

	_lock_uninit(lh);

	free(lh);
}

static size_t _real_len(const char * s)
{
	for (int i =0; i < AES_128; i++)
		if (!s[i]) return i;
	return AES_128;
}

static int _file_decipher(const char *in_filepath, size_t in_filesize, const char *out_filepath, uint8_t *w)
{
	FILE *fi, *fo;
	
	fi = fopen(in_filepath, "r");
	if (fi == NULL) {
		perror("Failed to open the in file!");
		return 1;
	}
	
	fo = fopen(out_filepath, "a+");
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

int log_decipher(const char *in_filename, const char *out_filename, const char *password)
{
	assert(in_filename != NULL);
	assert(out_filename != NULL);
	assert(password != NULL);
	
	uint8_t *w = NULL;
	if (0 != _pwd_init(&w, password)) return 1;
	uint64_t file_size = _get_file_size(in_filename);
	if (file_size == 0 || file_size % AES_128 != 0) {
		fprintf(stderr, "Input file error!\n");
		free(w);
		return 1;
	}

	int ret = _file_decipher(in_filename, file_size, out_filename, w);

	free(w);

	if (ret != 0) return ret;
	
	return 0;
}

static void  _file_compress(const char *src_filename, const char *dst_filename)
{
	lz4_file_compress(src_filename, dst_filename);
}

static void _backup_file(log_t *lh)
{
	char new_filename[PATH_MAX] = {0};
	
	if (lh->cflag & COMPRESS) {	//compress
		sprintf(new_filename, "%s.bak%lu.lz4", lh->file_path, lh->cur_bak_num % lh->max_bak_num);
		_file_compress(lh->file_path, new_filename);
		remove(lh->file_path);
	}
	else {
		sprintf(new_filename, "%s.bak%lu", lh->file_path, lh->cur_bak_num % lh->max_bak_num);
		rename(lh->file_path, new_filename);
	}
}

static void _update_file(log_t *lh)
{
	if (!lh->f_log) return;
	
	fclose(lh->f_log);
	lh->f_log = NULL;
	if (lh->max_bak_num == 0) {	//without backup
		remove(lh->file_path);
	}
	else {
		_backup_file(lh);
	}
	lh->cur_bak_num++;
	lh->cur_file_size = 0;
}

void log_flush(log_t *lh)
{
	assert(lh != NULL);
	
	_log_lock(lh);

	_update_file(lh);

	_log_unlock(lh);
}


static int _file_handle_request(log_t *lh)
{
	if (lh->f_log) return 0;

	lh->f_log = fopen(lh->file_path, "a+");

	if (!lh->f_log) return 1;

	if (setvbuf(lh->f_log, lh->io_buf, _IOFBF, lh->io_cap) != 0) return 1;
	
	return 0;
}

void _buf_encrypt(char *msg, uint8_t *wkey, size_t len)
{
	uint8_t out[AES_128];
	size_t i = 0;
	while(len > i) {
		cipher((uint8_t*)msg + i, out, wkey);
		memcpy(msg+i, out, AES_128);
		i+= AES_128;
	}
}

static void _write_buf(log_t *lh, char *msg, size_t len)
{
	if (lh->cflag & ENCRYPT) {
		len = aes_ex(len);
		_buf_encrypt(msg, lh->wkey, len);
	}

	_log_lock(lh);

	if (0 != _file_handle_request(lh)) {
		fprintf(stderr, "Failed to open the file, path: %s\n", lh->file_path);
		return;
	}

	size_t print_size = fwrite(msg, sizeof(char), len, lh->f_log);
	if (print_size != len) {
		fprintf(stderr, "Failed to write the file path(%s), msg(%s)\n", lh->file_path, msg);
		return;
	}
	lh->cur_file_size += print_size;
	
	if (lh->cur_file_size > lh->max_file_size) {
		_update_file(lh);
	}

	_log_unlock(lh);
	
}


void _log_write(log_t *lh, const log_level_t level, const char *format, ...)
{
	assert(lh != NULL);
	assert(format != NULL);

	char log_msg[EX_SINGLE_LOG_SIZE];
	memset(log_msg, 0, EX_SINGLE_LOG_SIZE);

	va_list args;
	va_start(args, format);

	static const char* const severity[] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};

/* debug message */
#if (LOG_LEVEL == LOG_DEBUG)
	time_t rawtime = time(NULL);
	struct tm timeinfo;
	localtime_r(&rawtime, &timeinfo);
	pid_t tid = syscall(SYS_gettid);
	int info_len = snprintf(log_msg, SINGLE_LOG_SIZE, "%s%04d-%02d-%02d %02d:%02d:%02d %s<%s> %d: ", severity[level], timeinfo.tm_year + 1900,
						timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, va_arg(args, char*), va_arg(args, char*), tid);
#else
	int info_len = strlen(severity[level]);
	strncpy(log_msg, severity[level], info_len);
#endif

	int total_len = vsnprintf(log_msg + info_len, SINGLE_LOG_SIZE - info_len, format, args);
	va_end(args);
	
	if (total_len <= 0 || info_len <= 0 || total_len > SINGLE_LOG_SIZE - info_len) {
		fprintf(stderr, "Failed to vsnprintf a text entry: (total_len) %d\n", total_len);
		return;
	}
	
	total_len += info_len;
	
	_write_buf(lh, log_msg, total_len);
}

char* log_md5(const char *filename, char *digest)
{
	assert(filename != NULL);
	return MD5File_S(filename, digest);
}

int log_uncompress(const char *src_filename, const char *dst_filename)
{
	assert(src_filename != NULL);
	assert(dst_filename != NULL);
 	return lz4_file_uncompress(src_filename, dst_filename);
}


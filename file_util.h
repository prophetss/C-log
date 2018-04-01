#ifndef _FILE_UTIL_H_
#define _FILE_UTIL_H_

#define SUCCESS	0
#define FAILED	1

/*获取文件大小（不打开文件）*/
int get_file_size(const char *fullfilepath);

/*全路径解析文件名（内部申请内存）*/
char* get_file_name(const char *fullfilepath);

/*lz4文件压缩(不删除源文件)*/
int compress_file(const char *src_filepath, const char *dst_filepath);

/*lz4文件解压(不删除源文件)*/
int decompress_file(const char *src_filepath, const char *dst_filepath);


#endif	/*!_FILE_UTIL_H_*/
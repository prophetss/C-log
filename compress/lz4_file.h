#ifndef _LZ4_FILE_H_
#define _LZ4_FILE_H_



int lz4_file_compress(const char *in_filename, const char *out_filename);


int lz4_file_uncompress(const char *in_filename, const char *out_filename);


#endif	/*!_LZ4_FILE_H_*/
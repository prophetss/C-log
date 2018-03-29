#ifndef _MD5_H_
#define _MD5_H_

#define MD5_HASHBYTES 16

#ifdef __cplusplus
extern "C" {
#endif

void MD5File(const char *filepath, unsigned char *digest);

char* MD5File_S(const char *filepath, char *buf);

void MD5Data(const unsigned char *data, unsigned int len, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* !_MD5_H_ */

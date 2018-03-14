#ifndef _AES_H_
#define _AES_H_

/* out buffer length */
#define AES_BUFSIZ	16

/* key length */
#define AES_128	16
#define AES_192	24
#define AES_256	32

#define AES_SUCCESS	0
#define AES_ERROR	-1

typedef unsigned char	uint8_t;

#ifdef __cplusplus
extern "C" {
#endif

int aes_cipher_data(uint8_t *in, size_t in_len, uint8_t *out, uint8_t *key, size_t key_len);

int aes_decipher_data(uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len, uint8_t *key, size_t key_len);

int aes_cipher_file(const char *in_filename, const char *out_filename, uint8_t *key, size_t key_len);

int aes_decipher_file(const char *in_filename, const char *out_filename, uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif /* !_AES_H_ */


#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef _DEBUG

#define info_display(...) fprintf(stdout, __VA_ARGS__)

/* generic error */
#define error_display(...) do {		\
	fprintf(stderr, __VA_ARGS__);	\
	assert(0);						\
} while(0)
	
/* fatal error */
#define exit_throw(...) do {                                             		\
    fprintf(stderr, "Error defined at %s, line %i : \n", __FILE__, __LINE__); 	\
    fprintf(stderr, "Errno : %d, msg : %s\n", errno, strerror(errno));          \
    fprintf(stderr, "%s\n", __VA_ARGS__);                                       \
    exit(errno);                                                           		\
} while(0)

#else

#define info_display(...)

#define error_display(...)

#define exit_throw(...) do {                                             		\
    error_display("Errno : %d, msg : %s\n", errno, strerror(errno));           	\
    exit(errno);                                                           		\
} while(0)

#endif /*_DEBUG*/


#endif /*!_ERROR_H_*/


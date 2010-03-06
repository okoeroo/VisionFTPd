#include <string.h>

#ifndef UNSIGNED_STRING_H
    #define UNSIGNED_STRING_H

unsigned char * u_strstr(register unsigned char *buf, register unsigned char *sub);
unsigned char * u_strnstr(register unsigned char *buf, register unsigned char *sub, register size_t haystacklen);

size_t u_strlen (const unsigned char * s);
unsigned char * u_strcpy (unsigned char * dest, const unsigned char * src);
unsigned char * u_strncpy (unsigned char * dest, const unsigned char * src, size_t len);


#endif /* UNSIGNED_STRING_H */

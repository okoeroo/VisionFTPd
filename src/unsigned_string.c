#include "unsigned_string.h"


unsigned char * u_strstr(register unsigned char *buf, register unsigned char *sub)
{
    register unsigned char *bp;

    if (!*sub)
        return buf;
    for (;;) /* Haystack */
    {
        if (!*buf)
            break;
        bp = buf;
        for (;;) /* Needle in hay stack */
        {
            if (!*sub)
                return buf;
            if (*bp++ != *sub++)
                break;
        }
        sub -= (unsigned long) bp;
        sub += (unsigned long) buf;
        buf += 1;
    }
    return 0;
}

unsigned char * u_strnstr(register unsigned char *buf, register unsigned char *sub, register size_t haystacklen)
{
    register unsigned char *bp;
    register int i;

    if (!*sub)
        return buf;
    for (i = 0; i < haystacklen; i++) /* Haystack */
    {
        if (!*buf)
            break;
        bp = buf;
        for (;;) /* Needle in hay stack */
        {
            if (!*sub)
                return buf;
            if (*bp++ != *sub++)
                break;
        }
        sub -= (unsigned long) bp;
        sub += (unsigned long) buf;
        buf += 1;
        i++;
    }
    return 0;
}


size_t u_strlen (const unsigned char * s)
{
    register size_t i;

    for (i = 0; s[i] != '\0'; i++) ;

    return i;
}

unsigned char * u_strcpy (unsigned char * dest, const unsigned char * src)
{
    register size_t i = 0;
    bzero (dest, u_strlen (src) + 1);

    for (i = 0; i < u_strlen (src); i++)
    {
        dest[i] = src[i];
    }

    return dest;
}

unsigned char * u_strncpy (unsigned char * dest, const unsigned char * src, size_t len)
{
    register size_t i = 0;
    bzero (dest, u_strlen (src) + 1);

    for (i = 0; (i < u_strlen (src)) && (i != len); i++)
    {
        dest[i] = src[i];
    }

    return dest;
}


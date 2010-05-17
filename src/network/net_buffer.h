#ifndef NET_BUFFER_H
    #define NET_BUFFER_H


#include <stdlib.h>
#include <errno.h>

#include "unsigned_string.h"


typedef struct buffer_state_s {
    unsigned char * buffer;
    size_t          buffer_size;
    size_t          num_bytes;
    size_t          offset;
    size_t          bytes_commited;
    int             state;
} buffer_state_t;


int copy_buffer_to_buffer (buffer_state_t * src, buffer_state_t * dest);
buffer_state_t * init_buffer_state (size_t buffer_size);
int free_buffer_state (buffer_state_t ** buf);


#endif /* NET_BUFFER_H */

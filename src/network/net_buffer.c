#include "net_buffer.h"


int copy_buffer_to_buffer (buffer_state_t * src, buffer_state_t * dest)
{
    if (!((dest && dest -> buffer) && (src && src -> buffer)))
    {
        return 1;
    }

    /* Check if the data would fit in the dest buffer */
    if (src -> num_bytes > dest -> buffer_size)
        return 1;

    bcopy (src -> buffer, dest -> buffer, src -> num_bytes);
    dest -> num_bytes      = src -> num_bytes;
    dest -> offset         = src -> offset;
    dest -> bytes_commited = src -> bytes_commited;
    dest -> state          = src -> state;

    /* String tolerance */
    dest -> buffer[dest -> num_bytes] = '\0';

    return 0;
}


buffer_state_t * init_buffer_state (size_t buffer_size)
{
    buffer_state_t * buf = NULL;

    if ((buf = calloc (1, sizeof (buffer_state_t))) == NULL)
    {
        return NULL;
    }

    buf -> buffer = calloc (buffer_size, sizeof (unsigned char));
    if (buf -> buffer == NULL)
    {
        free (buf);
        return NULL;
    }

    buf -> buffer_size    = buffer_size;
    buf -> num_bytes      = 0;
    buf -> offset         = 0;
    buf -> bytes_commited = 0;
    buf -> state          = 0;

    return buf;
}


int free_buffer_state (buffer_state_t ** buf)
{
    (*buf) -> buffer_size    = 0;
    (*buf) -> num_bytes      = 0;
    (*buf) -> offset         = 0;
    (*buf) -> bytes_commited = 0;
    (*buf) -> state          = 0;

    free((*buf) -> buffer);
    (*buf) -> buffer = NULL;
    free((*buf));
    (*buf) = NULL;

    return 0;
}

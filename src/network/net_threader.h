#ifndef NET_THREADER
    #define NET_THREADER

#include "main.h"
#include "net_buffer.h"
#include "net_common.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <pthread.h>

#include "unsigned_string.h"



typedef struct net_threader_parameters_s {
    int    client_fd;
    int    read_byte_size;
    int    write_byte_size;
    char * hostname;
    int (* net_thread_active_io_func)(buffer_state_t *, buffer_state_t *, void **);
    int (* net_thread_idle_io_func)(buffer_state_t *, void **);
    int (* net_thread_state_initiator_func)(void **, void *);
    void * net_thread_state_initiator_arg;
    int (* net_thread_state_liberator_func)(void **);
} net_threader_parameters_t;

typedef struct net_thread_pool_s {
    struct net_thread_pool_s * next;
    pthread_t                  threadid;
    net_threader_parameters_t  net_thread_parameters;
} net_thread_pool_t;


void * threadingDaemonClientHandler (void * arg);
int threadingDaemonStart (const int listening_port, 
                          const int max_clients, 
                          int read_chunk_size,
                          int write_chunk_size,
                          int (* net_thread_active_io_func)(buffer_state_t *, buffer_state_t *, void **),
                          int (* net_thread_idle_io_func)(buffer_state_t *, void **),
                          int (* net_thread_state_initiator_func)(void **, void *),
                          void * net_thread_state_initiator_arg,

                          int (* net_thread_state_liberator_func)(void **));

int liberate_net_thread_pool_node (net_thread_pool_t * net_thread_pool_node, int close_the_fd);


#endif /* NET_THREADER */

#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "main.h"
#include "net_common.h"
#include "ftpd.h"

#include "net_threader.h"
#include "net_messenger.h"
#include "unsigned_string.h"

#include "vfs.h"

#ifndef DISPATCHER_H
    #define DISPATCHER_H

typedef struct dispatcher_options_s {
    unsigned short port;
    unsigned int   max_clients;
    vfs_t * vfs_root;
} dispatcher_options_t;


typedef struct dispatcher_state_s {
    short slave_init;
    short slave_online;
    net_msg_queue_t * inbox_q;
    net_msg_queue_t * outbox_q;
} dispatcher_state_t;


int dispatcher_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state);
int dispatcher_idle_io   (buffer_state_t * write_buffer_state, void ** state);
int dispatcher_state_initiator (void ** state, void * vfs);
int dispatcher_state_liberator (void ** state);

void * startdispatcher (void * arg);

#endif /* DISPATCHER_H */

#ifndef TRANS_MAN_H
    #define TRANS_MAN_H

#include "vfs.h"
#include "net_common.h"
#include "net_threader.h"
#include "net_messenger.h"


typedef struct master_node_s {
    pthread_t tid;
    char * master_node;
    short  port;
    int    socket;
    net_msg_queue_t * input_q;
    net_msg_queue_t * output_q;

    struct master_node_s * next;
} master_node_t;


typedef enum CONN_TYPE_e {
    CONNECT,
    LISTEN
} CONN_TYPE_T;


typedef struct data_transfers_s {
    CONN_TYPE_T      conn_action;
    char *           ip;
    short            port; 
    buffer_state_t * buffer;
    vfs_t *          file;
} data_transfer_t;



int TM_init (master_node_t ** master_nodes,
             char * master_node,
             short port,
             int max_con_transfers);
int slave_comm_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state);
int slave_comm_idle_io (buffer_state_t * write_buffer_state, void ** state);
int slave_comm_state_initiator (void ** state, void * arg);
int slave_comm_state_liberator (void ** state);
void * slave_comm_to_master (void * args);


#endif /* TRANS_MAN_H */

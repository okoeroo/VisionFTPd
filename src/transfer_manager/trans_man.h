#ifndef TRANS_MAN_H
    #define TRANS_MAN_H

#include "vfs.h"
#include "net_common.h"
#include "net_threader.h"


typedef struct master_node_s {
    char * master_node;
    short  port;
    int    socket;
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



int TM_init (char * master_node,
             short port,
             int max_con_transfers);
void * TM_add (void * args);


#endif /* TRANS_MAN_H */

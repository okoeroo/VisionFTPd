#include "net_threader.h"

#ifndef NET_MESSENGER_H
    #define NET_MESSENGER_H


typedef struct net_msg_s {
    int src_sock; /* Source socket */
    int dst_sock; /* Destination socket */
    buffer_state_t * msg;
    struct net_msg_s * next; /* Next message - 
                                When this is input then the first message is the chronologically first message arrived to the stack; 
                                on output, then the first message in the list is the one that needs to be send first. */
} net_msg_t;

typedef struct net_msg_queue_s {
    pthread_mutex_t lock;
    net_msg_t * in_process;
    net_msg_t * on_queue;
} net_msg_queue_t;


net_msg_t * create_net_msg (size_t buffer_size);
int delete_net_msg_list (net_msg_t ** list);
net_msg_queue_t * create_net_msg_queue (void);
int delete_net_msg_queue (net_msg_queue_t ** q);

net_msg_t * pop_net_msg (net_msg_queue_t * q);

#endif /* NET_MESSENGER_H */

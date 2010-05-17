#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>

#include "net_buffer.h"


#ifndef NET_MESSENGER_H
    #define NET_MESSENGER_H


typedef struct net_msg_s {
    long               author_id;   /* Identifier of the process or thread that has written the message */
    int                category_id; /* Category of messages, will be able to steer (broadcast) message */

    pthread_mutex_t    lock;        /* Implement an exclusive lock for delete or updates to this message */
    int                lock_id;     /* A threadid or a processid that pushed the lock */

    int                src;         /* A threadid or a processid, or type of process are candidates for src IDs */
    int                dst;         /* see 'src' */

    buffer_state_t *   msg;         /* Buffer that holds the data */
    struct net_msg_s * next;        /* Next message in a list/queue */
} net_msg_t;


/*
typedef struct net_msg_list_s {
    net_msg_t * msg;
    struct net_msg_list_s * next;
} net_msg_list_t;
*/


typedef struct net_msg_queue_s {
    pthread_mutex_t   q_lock;   /* Locking the entire queue */
    net_msg_t *       list;     /* List of messages on the queue */
} net_msg_queue_t;



typedef struct net_msg_mailbox_s {
    long             owner_id;    /* Owner of the mailbox */
    int              category_id; /* Category of messages, will be able to steer (broadcast) message */

    net_msg_queue_t * inbox;
    net_msg_queue_t * outbox;
} net_msg_mailbox_t;


typedef struct net_msg_mailbox_handle_s {
    long             owner_id;
    int              category_id;
} net_msg_mailbox_handle_t;


typedef struct net_msg_postoffice_s {
    net_msg_mailbox_t * mailbox;
    struct net_msg_postoffice_s * next;
} net_msg_postoffice_t;



net_msg_t * net_msg_create (net_msg_mailbox_handle_t * handle, size_t buffer_size);
net_msg_t * net_msg_pop_from_queue (net_msg_queue_t * q);
int net_msg_push_to_queue (net_msg_queue_t * q, net_msg_t * pushed);
net_msg_queue_t * net_msg_queue_create (void);
int net_msg_delete_msg (net_msg_t * msg);
int net_msg_queue_clean (net_msg_queue_t * q);
int net_msg_queue_delete (net_msg_queue_t * q);
net_msg_mailbox_handle_t * net_msg_mailbox_create_handle (int category_id);
int net_msg_mailbox_delete (net_msg_mailbox_t * mailbox);
net_msg_mailbox_handle_t * net_msg_mailbox_create (int category_id);
int net_msg_add_mailbox_to_postoffice (net_msg_mailbox_t * mailbox);
int net_msg_clean_postoffice (void);

net_msg_mailbox_t * net_msg_search_on_handle (net_msg_mailbox_handle_t * handle);

#endif /* NET_MESSENGER_H */

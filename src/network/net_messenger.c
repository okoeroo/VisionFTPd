#include "net_messenger.h"


net_msg_t * create_net_msg (size_t buffer_size)
{
    net_msg_t * msg = malloc(sizeof(net_msg_t));

    if (msg)
    {
        msg -> src_sock = -1;
        msg -> dst_sock = -1;
        msg -> msg      = init_buffer_state (buffer_size);
        msg -> next     = NULL;
    }
    return msg;
}


net_msg_t * pop_net_msg (net_msg_queue_t * q)
{
    net_msg_t * popped = NULL;

    pthread_mutex_lock (&(q -> lock));
    popped = q -> on_queue;
    q -> on_queue = q -> on_queue -> next;
    popped -> next = NULL;

    pthread_mutex_unlock (&(q -> lock));

    return popped;
}


int delete_net_msg_list (net_msg_t ** list)
{
    net_msg_t * del_list = NULL;

    if (!list)
        return 1;
    else
    {
        del_list = *list;
        while (del_list)
        {
            del_list -> src_sock = -1;
            del_list -> dst_sock = -1;
            free_buffer_state(&(del_list -> msg));
            del_list = del_list -> next;
        }
        *list = del_list;
        return 0;
    }
}


net_msg_queue_t * create_net_msg_queue (void)
{
    net_msg_queue_t * q = malloc(sizeof(net_msg_queue_t));

    if (q)
    {
        q -> in_process = NULL;
        q -> on_queue   = NULL;
        pthread_mutex_init (&(q -> lock), NULL);
    }

    return q;
}


int delete_net_msg_queue (net_msg_queue_t ** q)
{
    net_msg_queue_t * del_q = NULL;

    if (!q)
        return 1;
    else
    {
        del_q = *q;

        pthread_mutex_lock (&(del_q -> lock));
        delete_net_msg_list (&(del_q -> in_process));
        delete_net_msg_list (&(del_q -> on_queue));

        pthread_mutex_unlock (&(del_q -> lock));
        pthread_mutex_destroy (&(del_q -> lock));
       
        free(del_q);
        *q = NULL;
    }
    return 0;
}

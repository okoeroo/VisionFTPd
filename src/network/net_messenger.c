#include "net_messenger.h"


net_msg_t * net_msg_create (size_t buffer_size)
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


net_msg_t * net_msg_pop_on_queue (net_msg_queue_t * q)
{
    net_msg_t * popped = NULL;

    if (q)
    {
        pthread_mutex_lock (&(q -> lock));
        if (q -> on_queue)
        {
            popped = q -> on_queue;
            q -> on_queue = q -> on_queue -> next;
            popped -> next = NULL;
        }
        pthread_mutex_unlock (&(q -> lock));
    }

    return popped;
}


int net_msg_push_on_queue (net_msg_queue_t * q, net_msg_t * pushed)
{
    net_msg_t * helper = NULL;

    if (q && pushed)
    {
        pthread_mutex_lock (&(q -> lock));
        if (q -> on_queue)
        {
            helper = q -> on_queue;
            while (helper)
            {
                if (helper -> next == NULL)
                {
                    helper -> next = pushed;
                    pushed -> next = NULL;
                    break;
                }
                helper = helper -> next;
            }
        }
        else
        {
            q -> on_queue = pushed;
        }
        pthread_mutex_unlock (&(q -> lock));
        return 0;
    }
    else
        return 1;
}

int net_msg_delete_list (net_msg_t ** list)
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


net_msg_queue_t * net_msg_queue_create (void)
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


int net_msg_queue_delete (net_msg_queue_t ** q)
{
    net_msg_queue_t * del_q = NULL;

    if (!q)
        return 1;
    else
    {
        del_q = *q;

        pthread_mutex_lock (&(del_q -> lock));
        net_msg_delete_list (&(del_q -> in_process));
        net_msg_delete_list (&(del_q -> on_queue));

        pthread_mutex_unlock (&(del_q -> lock));
        pthread_mutex_destroy (&(del_q -> lock));
       
        free(del_q);
        *q = NULL;
    }
    return 0;
}

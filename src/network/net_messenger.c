#include "net_messenger.h"


net_msg_postoffice_t * central_postoffice = NULL;


net_msg_t * net_msg_create (net_msg_mailbox_handle_t * handle, size_t buffer_size)
{
    net_msg_t * msg_container = malloc(sizeof(net_msg_t));

    if (msg_container)
    {
        if (handle)
        {
            msg_container -> author_id   = handle -> owner_id;
            msg_container -> category_id = handle -> category_id;
        }
        else
        {
            msg_container -> author_id   = -1;
            msg_container -> category_id = -1;
        }
        
        pthread_mutex_init (&(msg_container -> lock), NULL);
        msg_container -> lock_id     = msg_container -> author_id;
        msg_container -> src         = msg_container -> author_id;
        msg_container -> dst         = msg_container -> author_id;

        msg_container -> msg         = init_buffer_state (buffer_size);
    }
    return msg_container;
}



net_msg_t * net_msg_pop_from_queue (net_msg_queue_t * q)
{
    net_msg_t * popped = NULL;

    if (q && q -> list)
    {
        pthread_mutex_lock (&(q -> q_lock));
        if (q -> list) 
        {
            popped = q -> list;
            q -> list = q -> list -> next;
            popped -> next = NULL;
        }
        pthread_mutex_unlock (&(q -> q_lock));
    }

    return popped;
}



int net_msg_push_to_queue (net_msg_queue_t * q, net_msg_t * pushed)
{
    net_msg_t * helper = NULL;

    if (q && pushed)
    {
        /* Lock entire queue */
        pthread_mutex_lock (&(q -> q_lock));
        
        /* Add message to the queue, when there was set yet */
        if (!(q -> list))
        {
            q -> list = pushed;
        }
        else
        {
            /* Attach pushed message to the end of the list */
            helper = q -> list;
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

        /* Unlock queue */
        pthread_mutex_unlock (&(q -> q_lock));
        return 0;
    }
    else
        return 1;
}


net_msg_queue_t * net_msg_queue_create (void)
{
    net_msg_queue_t * q = malloc(sizeof(net_msg_queue_t));

    if (q)
    {
        q -> list = NULL;
        pthread_mutex_init (&(q -> q_lock), NULL);
    }

    return q;
}



int net_msg_delete_msg (net_msg_t * msg)
{
    if (msg)
    {
        pthread_mutex_lock    (&(msg -> lock));

        free_buffer_state(&(msg -> msg));

        msg -> author_id   = -1;
        msg -> category_id = -1;
        msg -> lock_id     = -1;
        msg -> src         = -1;
        msg -> dst         = -1;
        msg -> msg         = NULL;

        pthread_mutex_unlock  (&(msg -> lock));
        pthread_mutex_destroy (&(msg -> lock));

        free(msg);
    }
    return 0;
}


int net_msg_queue_clean (net_msg_queue_t * q)
{
    net_msg_t * msg = NULL;

    if (q && q -> list)
    {
        /* Pop the queue */
        while ((msg = net_msg_pop_from_queue(q)))
        {
            net_msg_delete_msg(msg);
        }
        /* Clean the list */
        q -> list = NULL;
    }
    return 0;
}

int net_msg_queue_delete (net_msg_queue_t * q)
{
    if (!q)
    {
        return 1;
    }
    else
    {
        pthread_mutex_lock (&(q -> q_lock));

        /* Clean all messages from the queue */
        net_msg_queue_clean (q);

        pthread_mutex_unlock (&(q -> q_lock));
        pthread_mutex_destroy (&(q -> q_lock));
       
        free(q);
    }
    return 0;
}


net_msg_mailbox_handle_t * net_msg_mailbox_create_handle (int category_id)
{
    pid_t             tid;
    struct timeval    tv;
    net_msg_mailbox_handle_t * handle = NULL;

    handle = malloc (sizeof(net_msg_mailbox_handle_t));
    if (handle)
    {
        /* Create handle for mailbox */
        tid = syscall(SYS_gettid);
        gettimeofday(&tv, NULL);

        handle -> owner_id = tid * (int)tv.tv_sec * (int)tv.tv_usec;
    }

    return handle;
}


int net_msg_mailbox_delete (net_msg_mailbox_t * mailbox)
{
    if (mailbox)
    {
        mailbox -> category_id = -1;
        mailbox -> owner_id    = -1;
        net_msg_queue_delete(mailbox -> inbox);
        net_msg_queue_delete(mailbox -> outbox);

        free(mailbox);
        return 0;
    }
    else
    {
        return 1;
    }
}


net_msg_mailbox_handle_t * net_msg_mailbox_create (int category_id)
{
    net_msg_mailbox_handle_t * handle = NULL;
    net_msg_mailbox_t *        mailbox = NULL;

    if ((handle = net_msg_mailbox_create_handle (category_id)))
    {
        mailbox = malloc(sizeof(net_msg_mailbox_t));
        if (!mailbox)
        {
            free(handle);
            handle = NULL;
            return NULL;
        }
        else
        {
            mailbox -> owner_id    = handle -> owner_id;
            mailbox -> category_id = handle -> category_id;
            mailbox -> inbox       = net_msg_queue_create();
            mailbox -> outbox      = net_msg_queue_create();

            /* If anything didn't went ok with this, clean up now */
                /* Mailbox created, adding to Post Office object */
            if (!(mailbox -> inbox) || 
                (!(mailbox -> outbox)) ||
                (net_msg_add_mailbox_to_postoffice (mailbox) != 0)
                )
            {
                net_msg_mailbox_delete (mailbox);
                mailbox = NULL;

                free(handle);
                handle = NULL;
                return NULL;
            }
        }
    }
    return handle;
}


int net_msg_add_mailbox_to_postoffice (net_msg_mailbox_t * mailbox)
{
    net_msg_postoffice_t * cpo = NULL;

    /* No central post office yet */
    if (!central_postoffice)
    {
        central_postoffice = malloc (sizeof(net_msg_postoffice_t));
        central_postoffice -> mailbox = mailbox;
        central_postoffice -> next    = NULL;

        return 0;
    }
    else
    {
        /* Find an empty spot */
        cpo = central_postoffice;
        while (cpo)
        {
            if (!cpo -> mailbox)
            {
                cpo -> mailbox = mailbox;
                return 0;
            }
            cpo = cpo -> next;
        }

        /* Added mailbox at the end */
        cpo = central_postoffice;
        while (cpo -> next)
        { 
            cpo = cpo -> next;
        }
        cpo -> next = malloc (sizeof(net_msg_postoffice_t));
        cpo -> next -> mailbox = mailbox; 
        cpo -> next -> next    = NULL;

        return 0;
    }
}


int net_msg_remove_mailbox (net_msg_mailbox_handle_t * handle)
{
    net_msg_postoffice_t * cpo     = NULL;
    /* net_msg_mailbox_t *    mailbox = NULL; */

    if (!handle)
        return 1;

    cpo = central_postoffice;
    while (cpo)
    {
        if (cpo -> mailbox)
        {
            if (cpo -> mailbox -> owner_id == handle -> owner_id)
            {
                if (net_msg_mailbox_delete(cpo -> mailbox) == 0)
                {
                    cpo -> mailbox = NULL;
                    return 0;
                }
                else
                    return 1;
            }
        }
        cpo = cpo -> next;
    }
    return 0;
}


int net_msg_clean_postoffice (void)
{
    net_msg_postoffice_t * cpo = NULL;

    while (central_postoffice)
    {
        net_msg_mailbox_delete (central_postoffice -> mailbox);
        central_postoffice -> mailbox = NULL;

        cpo = central_postoffice;
        central_postoffice = central_postoffice -> next;
        free(cpo);
    }
    return 0;
}


net_msg_mailbox_t * net_msg_search_on_handle (net_msg_mailbox_handle_t * handle)
{
    net_msg_postoffice_t * cpo     = NULL;
    /* net_msg_mailbox_t *    mailbox = NULL; */

    if (!handle)
        return NULL;

    cpo = central_postoffice;
    while (cpo)
    {
        if (cpo -> mailbox)
        {
            if (cpo -> mailbox -> owner_id == handle -> owner_id)
            {
                return cpo -> mailbox;
            }
        }
        cpo = cpo -> next;
    }
    return 0;
}


net_msg_t * net_msg_pop_from_inbox (net_msg_mailbox_handle_t * handle)
{
    net_msg_mailbox_t * mailbox = NULL;

    if (!handle)
    {
        return NULL;
    }
    else
    {
        if (!(mailbox = net_msg_search_on_handle (handle)))
        {
            return NULL;
        }
        else
        {
            return net_msg_pop_from_queue (mailbox -> inbox);
        }
    }
}


net_msg_t * net_msg_pop_from_outbox (net_msg_mailbox_handle_t * handle)
{
    net_msg_mailbox_t * mailbox = NULL;

    if (!handle)
    {
        return NULL;
    }
    else
    {
        if (!(mailbox = net_msg_search_on_handle (handle)))
        {
            return NULL;
        }
        else
        {
            return net_msg_pop_from_queue (mailbox -> outbox);
        }
    }
}

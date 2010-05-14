#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "main.h"
#include "net_common.h"
#include "dispatcher.h"

#include "net_threader.h"
#include "net_messenger.h"
#include "unsigned_string.h"

#include "ftpd.h"

#include "vfs.h"


/****************************************************************************************************

Dispatcher

****************************************************************************************************/



int dispatcher_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state)
{
    dispatcher_state_t * dispatcher_state = *(dispatcher_state_t **)state;
    int rc               = 0;
    net_msg_t *          recv_msg = NULL;


    /* Dispatcher connection state initialization */
    if (dispatcher_state == NULL)
    {
        scar_log (1, "%s: Error: the dispatcher_state object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

#ifdef DEBUG
    scar_log (3, "   << %s\n", read_buffer_state -> buffer);
#endif

    /* Handle PASS message */
    /* if ((rc = handle_ftp_PASS (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED) */
        /* goto finalize_message_handling; */


    /* Push message to the inbox_q */
    if (read_buffer_state && (read_buffer_state -> num_bytes > 0))
    {
        recv_msg = net_msg_create (read_buffer_state -> num_bytes);
        if (!recv_msg)
        {
            scar_log (1, "%s: Error: Couldn't create a net_message object. Out of memory\n", __func__);
            rc = NET_RC_DISCONNECT;
            goto finalize_message_handling;
        }
        /* Returning 0 is ok, 1 is not ok */
        copy_buffer_to_buffer (read_buffer_state, recv_msg -> msg);
    }



finalize_message_handling:
#ifdef DEBUG
    scar_log (3, "   >> %s\n", write_buffer_state -> buffer);
#endif
    *state = (void *) dispatcher_state;
    return rc;
}



int dispatcher_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    dispatcher_state_t * dispatcher_state = *(dispatcher_state_t **)state;
    int rc               = NET_RC_IDLE;
    net_msg_t *          send_msg = NULL;

    /* Dispatcher connection state initialization */
    if (dispatcher_state == NULL)
    {
        scar_log (1, "%s: Error: the dispatcher_state object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

    if (dispatcher_state -> slave_init == 0)
    {
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "Welcome to the VisionFTPd Dispatcher.\r\n Submit to me as my slave and provide the cluster secret.\r\nPassword:\r\n");
        dispatcher_state -> slave_init = 1;

        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            rc = NET_RC_DISCONNECT;
            goto finalize_message_handling;
        }
        else
        {
            rc = NET_RC_MUST_WRITE;
            goto finalize_message_handling;
        }
    }

    /* Pop Outbox Q and push here */
    if (dispatcher_state -> outbox_q)
    {
        send_msg = net_msg_pop_on_queue (dispatcher_state -> outbox_q);
        if (send_msg)
        {
            /* Returning 0 is ok, 1 is not ok */
            if (copy_buffer_to_buffer (send_msg -> msg, write_buffer_state) == 0)
            {
                rc = NET_RC_MUST_WRITE;
                goto finalize_message_handling;
            }
            else
            {
                /* Buffer overrun - probably */
                rc = NET_RC_DISCONNECT;
                goto finalize_message_handling;
            }

            /* Free popped msg */
            net_msg_delete_list (&send_msg);
        }
    }



finalize_message_handling:
#ifdef DEBUG
    if (write_buffer_state -> num_bytes)
    {
        scar_log (3, "   >> %s\n", write_buffer_state -> buffer);
    }
#endif
    *state = (void *) dispatcher_state;
    return rc;
}


int dispatcher_state_initiator (void ** state, void * foo)
{
    dispatcher_state_t * dispatcher_state = NULL;

    dispatcher_state = malloc (sizeof(dispatcher_state_t));

    dispatcher_state -> slave_init   = 0;
    dispatcher_state -> slave_online = 0;
    dispatcher_state -> inbox_q  = net_msg_queue_create(); 
    dispatcher_state -> outbox_q = net_msg_queue_create();

    *state = dispatcher_state;
    return 0;
}

int dispatcher_state_liberator (void ** state)
{
    dispatcher_state_t * dispatcher_state = *(dispatcher_state_t **)state;

    if (dispatcher_state)
    {
        dispatcher_state -> slave_init   = 0;
        dispatcher_state -> slave_online = 0;
        net_msg_queue_delete (&(dispatcher_state -> inbox_q));
        net_msg_queue_delete (&(dispatcher_state -> outbox_q));

        free(dispatcher_state);

        *state = NULL;
        return 0;
    }
    else
        return 1;
}


/* Main daemon start for the dispatcher */
/* int startdispatcher (int port, int max_clients, char * ftp_banner) */
void * startdispatcher (void * arg)
{
    dispatcher_options_t * dispatcher_options = *(dispatcher_options_t **) arg;


    threadingDaemonStart (dispatcher_options -> port, 
                          dispatcher_options -> max_clients, 
                          4096, 
                          4096, 
                          dispatcher_active_io, 
                          dispatcher_idle_io,
                          dispatcher_state_initiator,
                          NULL,
                          dispatcher_state_liberator);
    return NULL;
}

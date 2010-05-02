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
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = 0;


    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        scar_log (1, "%s: Error: the ftp_state object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

#ifdef DEBUG
    scar_log (1, "   << %s\n", read_buffer_state -> buffer);
#endif


finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int dispatcher_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = NET_RC_IDLE;
    net_msg_t * msg_to_send = NULL;


finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int dispatcher_state_liberator (void ** state)
{
    return 0;
}

int dispatcher_state_initiator (void ** state, void * vfs)
{
    return 0;
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

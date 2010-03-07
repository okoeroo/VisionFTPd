#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "main.h"
#include "net_common.h"
#include "commander.h"

#include "net_threader.h"
#include "unsigned_string.h"

#include "ftpd.h"




/****************************************************************************************************

 ***********  ***********  **********
 *                 *       *         *               
 *                 *       *         *              
 *                 *       *         *              
 *******           *       **********                      
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        

****************************************************************************************************/



int commander_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = 0;

    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        ftp_state = malloc (sizeof (ftp_state_t));
        memset (ftp_state, 0, sizeof (ftp_state_t));
    }

#ifdef DEBUG
    scar_log (1, "   << %s\n", read_buffer_state -> buffer);
#endif

    /* Init phase for the FTP command channel */
    if (ftp_state -> init == 0)
    {
        ftp_state -> init = 1;

        rc = handle_ftp_initialization (ftp_state, NULL, write_buffer_state);
        goto finalize_message_handling;
    }

    /* Handle ABOR message */
    if ((rc = handle_ftp_ABOR (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle QUIT message */
    if ((rc = handle_ftp_QUIT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle USER message */
    if ((rc = handle_ftp_USER (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PASS message */
    if ((rc = handle_ftp_PASS (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle SYST message */
    if ((rc = handle_ftp_SYST (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle FEAT message */
    if ((rc = handle_ftp_FEAT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PWD  message */
    if ((rc = handle_ftp_PWD  (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle CWD  message */
    if ((rc = handle_ftp_CWD  (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle TYPE message */
    if ((rc = handle_ftp_TYPE (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle SIZE message */
    if ((rc = handle_ftp_SIZE (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle LPRT message */
    if ((rc = handle_ftp_LPRT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Handle PORT message */
    if ((rc = handle_ftp_PORT (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

    /* Don't know what to do - list as Idle */
    /* rc = NET_RC_IDLE; */

    /* 500 - Syntax error, command unrecognized. */
    if ((rc = handle_message_not_understood (ftp_state, read_buffer_state, write_buffer_state)) != NET_RC_UNHANDLED)
        goto finalize_message_handling;

finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int commander_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;
    int rc                  = NET_RC_IDLE;

    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        ftp_state = malloc (sizeof (ftp_state_t));
        memset (ftp_state, 0, sizeof (ftp_state_t));
    }

    /* Init phase for the FTP command channel */
    if (ftp_state -> init == 0)
    {
        ftp_state -> init = 1;

        rc = handle_ftp_initialization (ftp_state, NULL, write_buffer_state);
        goto finalize_message_handling;
    }


finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int commander_state_liberator (void ** state)
{
    ftp_state_t *     ftp_state  = *(ftp_state_t **)state;
    file_transfer_t * helper     = NULL;
    
    if (ftp_state)
    {
        ftp_state -> init       = 0;
        ftp_state -> mode       = ASCII;
        free(ftp_state -> ftp_user);
        ftp_state -> ftp_user   = NULL;
        free(ftp_state -> ftp_passwd);
        ftp_state -> ftp_passwd = NULL;
        free(ftp_state -> cwd);
        ftp_state -> cwd        = NULL;

        while (ftp_state -> in_transfer)
        {
            ftp_state -> in_transfer -> size = 0;
            free(ftp_state -> in_transfer -> path);
            ftp_state -> in_transfer -> path = NULL;
            ftp_state -> in_transfer -> data_address_family = '\0';
            free(ftp_state -> in_transfer -> data_address);
            ftp_state -> in_transfer -> data_address = NULL;
            ftp_state -> in_transfer -> data_port = 0;

            helper = ftp_state -> in_transfer;
            ftp_state -> in_transfer = ftp_state -> in_transfer -> next;
            free(helper);
        }
    }
    return 0;
}

int commander_state_initiator (void ** state)
{
    ftp_state_t * ftp_state = *(ftp_state_t **)state;

    /* FTP connection state initialization */
    if (ftp_state == NULL)
    {
        ftp_state = malloc (sizeof (ftp_state_t));
    }
    
    if (ftp_state == NULL)
    {
        /* Out of memory! */
        return 1;
    }
    
    memset (ftp_state, 0, sizeof (ftp_state_t));
    if (ftp_state)
    {
        ftp_state -> init       = 0;
        ftp_state -> mode       = ASCII;
        free(ftp_state -> ftp_user);
        ftp_state -> ftp_user   = NULL;
        free(ftp_state -> ftp_passwd);
        ftp_state -> ftp_passwd = NULL;
        free(ftp_state -> cwd);
        ftp_state -> cwd        = NULL;

        ftp_state -> in_transfer = NULL;
    }
    return 0;
}

/* TODO: I need a state liberator! */

/* Main daemon start for the commander */
int startCommander (int port, int max_clients, char * ftp_banner)
{
    set_ftp_service_banner(ftp_banner);
    return threadingDaemonStart (port, 
                                 max_clients, 
                                 4096, 
                                 4096, 
                                 commander_active_io, 
                                 commander_idle_io,
                                 NULL,
                                 commander_state_liberator);
}

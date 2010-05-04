#include "trans_man.h"

static int max_concurrent_transfers = 1;

int TM_init (master_node_t ** master_nodes,
             char * master_node,
             short port,
             int max_con_transfers)
{
    max_concurrent_transfers = max_con_transfers;

    master_node_t * this_master_node = NULL;
    master_node_t * tmp_list         = NULL;

    this_master_node = malloc (sizeof (master_node_t));
    this_master_node -> master_node = strdup(master_node);
    this_master_node -> port        = port;
    this_master_node -> socket      = -1;
    this_master_node -> input_q     = NULL;
    this_master_node -> output_q    = NULL;

    this_master_node -> next        = NULL;


    /* Register master to master node list */
    if (!*master_nodes)
        *master_nodes = this_master_node;
    else
    {
        tmp_list = *master_nodes;
        while (tmp_list -> next)
        {
            tmp_list = tmp_list -> next;
        }
        tmp_list -> next = this_master_node;
    }

    return 0;
}


int slave_comm_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state)
{
    void * ftp_state = *(void **)state;
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


int slave_comm_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    void * ftp_state = *(void **)state;
    int rc                  = NET_RC_IDLE;
    net_msg_t * msg_to_send = NULL;


finalize_message_handling:
    *state = (void *) ftp_state;
    return rc;
}


int slave_comm_state_initiator (void ** state, void * arg)
{
    return 0;
}

int slave_comm_state_liberator (void ** state)
{
    return 0;
}


void * slave_comm_to_master (void * args)
{
    master_node_t * my_master = (master_node_t *) args;
    net_thread_pool_t *         net_thread_pool_node  = NULL;
    int rc = 0;


    /* use pool node functionality */
    net_thread_pool_node = malloc (sizeof (net_thread_pool_t));
    net_thread_pool_node -> next                                                      = NULL;
    net_thread_pool_node -> net_thread_parameters.client_fd                           = -1;
    net_thread_pool_node -> net_thread_parameters.read_byte_size                      = 1500 - 160; /* Default MTU - header */
    net_thread_pool_node -> net_thread_parameters.write_byte_size                     = 1500 - 160; /* Default MTU - header */
    net_thread_pool_node -> net_thread_parameters.hostname                            = my_master -> master_node;
    net_thread_pool_node -> net_thread_parameters.net_thread_active_io_func           = slave_comm_active_io;
    net_thread_pool_node -> net_thread_parameters.net_thread_idle_io_func             = slave_comm_idle_io;
    net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_func     = slave_comm_state_initiator;
    net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_arg      = NULL;
    net_thread_pool_node -> net_thread_parameters.net_thread_state_liberator_func     = slave_comm_state_liberator; 

    while (1)
    {
        if (!my_master)
        {
            scar_log (1, "%s: Error: no Master Node object set\n", __func__);
            return NULL;
        }

        /* pthread_cond_wait(&cv, &count_mutex); */

        /* Fire up connection to home base */
        scar_log (1, "%s: Connecting to \"%s\" on port \"%d\"\n", 
                    __func__,
                    my_master -> master_node, 
                    my_master -> port);
        my_master -> socket = firstTCPSocketConnectingCorrectly (my_master -> master_node, my_master -> port);
        net_thread_pool_node -> net_thread_parameters.client_fd = my_master -> socket;
        if (my_master -> socket < 0)
        {
            scar_log (1, "%s: Error: unable to establish connection to the master node \"%s\" on port \"%d\"\n",
                         __func__,
                         my_master -> master_node,
                         my_master -> port);
        }
        else
        {
            /* Re-use of code FTW! */
            threadingDaemonClientHandler (&net_thread_pool_node);
            rc = pthread_create(&(net_thread_pool_node -> threadid), NULL, threadingDaemonClientHandler, (void*)(&net_thread_pool_node));
            if (rc != 0)
            {
                scar_log (1, "%s: Failed to spawn thread. To bad for this client.\n", __func__);
                liberate_net_thread_pool_node (net_thread_pool_node, 1);
            }
        }

        /* Thread safe sleep */
        scar_log (1, "Delay: 5 seconds before retry...\n");
        thread_sleep(5);
    }

    return NULL;
}

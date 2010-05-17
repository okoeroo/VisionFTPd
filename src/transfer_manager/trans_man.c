#include "trans_man.h"

static int max_concurrent_transfers = 1;

int TM_init (master_node_t ** master_nodes,
             char * master_node,
             short port,
             int max_con_transfers,
             char * password)
{
    master_node_t * this_master_node = NULL;
    master_node_t * tmp_list = NULL;


    max_concurrent_transfers = max_con_transfers;


    this_master_node = malloc (sizeof (master_node_t));
    this_master_node -> master_node = strdup(master_node);
    this_master_node -> port        = port;
    this_master_node -> socket      = -1;
    this_master_node -> password    = password;

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


int slave_comm_PASS (slave_comm_state_t * state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "gimme_access_now\r\n");
    if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
    {
        /* Buffer overrun */
        return NET_RC_DISCONNECT;
    }
    else
    {
        return NET_RC_MUST_WRITE;
    }
}

int slave_comm_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state)
{
    slave_comm_state_t * my_state = *(slave_comm_state_t **) state;
    int rc                  = 0;


    if (my_state == NULL)
    {
        scar_log (1, "%s: Error: the slave_comm_state_t object was not yet created!\n", __func__);
        rc = NET_RC_DISCONNECT;
        goto finalize_message_handling;
    }

#ifdef DEBUG
    scar_log (3, "   << %s\n", read_buffer_state -> buffer);
#endif

    if (!(my_state -> initialized))
    {
        /* Send password */
        rc = slave_comm_PASS (my_state, read_buffer_state, write_buffer_state);
        my_state -> initialized = 1;
        scar_log (1, "\nSlave initialized\n");
        goto finalize_message_handling;
    }


finalize_message_handling:
#ifdef DEBUG
    scar_log (3, "   >> %s\n", write_buffer_state -> buffer);
#endif
    *state = (void *) my_state;
    return rc;
}


int slave_comm_idle_io (buffer_state_t * write_buffer_state, void ** state)
{
    slave_comm_state_t * my_state = *(slave_comm_state_t **) state;
    void * ftp_state = *(void **)state;
    int rc                  = NET_RC_IDLE;
    /* net_msg_t * msg_to_send = NULL; */


finalize_message_handling:
#ifdef DEBUG
    if (write_buffer_state -> num_bytes)
    {
        scar_log (3, "   >> %s\n", write_buffer_state -> buffer);
    }
#endif
    *state = (void *) ftp_state;
    return rc;
}


int slave_comm_state_initiator (void ** state, void * arg)
{
    slave_comm_state_t * my_state = NULL;
    if (!state)
    {
        scar_log (1, "%s: Error: couldn't initiate and back-register state object\n", __func__);
        return 1;
    }

    my_state = malloc (sizeof (slave_comm_state_t));
    if (!my_state)
    {
        scar_log (1, "%s: Error: couldn't initiate and back-register state object\n");
        return 1;
    }

    my_state -> password = (char *) arg;

    *state = my_state;
    return 0;
}

int slave_comm_state_liberator (void ** state)
{
    slave_comm_state_t * my_state = (slave_comm_state_t *) *state;

    free(my_state -> password);
    free(my_state);

    *state = NULL;

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
    if (my_master -> password)
        net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_arg  = strdup(my_master -> password);
    else
        net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_arg  = NULL;
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
            rc = pthread_create(&(net_thread_pool_node -> threadid), NULL, threadingDaemonClientHandler, (void*)(&net_thread_pool_node));
            if (rc != 0)
            {
                scar_log (1, "%s: Failed to spawn thread.\n", __func__);
            }
            else
            {
                pthread_join (net_thread_pool_node -> threadid, NULL);
            }
        }

        /* Thread safe sleep */
        scar_log (1, "Delay: 5 seconds before retry...\n");
        thread_sleep(5);
    }

    return NULL;
}

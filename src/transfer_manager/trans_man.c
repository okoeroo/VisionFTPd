#include "trans_man.h"


static data_transfer_t * transfers  = NULL;
static int max_concurrent_transfers = 10;


static master_node_t * master_nodes = NULL;



int TM_init (char * master_node,
             short port,
             int max_con_transfers)
{
    max_concurrent_transfers = max_con_transfers;

    master_node = malloc (sizeof (master_node_t));
    master_node -> master_node = strdup(master_node);
    master_node -> port        = port;
    master_node -> socket      = -1;
    master_node -> next        = NULL;

    /* Fire up connection to home base */
    master_node -> socket = firstTCPSocketConnectingCorrectly (master_node -> master_node, 
                                                               master_node -> port);
    if (master_node -> socket < 0)
    {
        scar_log (1, "%s: Error: unable to establish connection to the master node \"%s\" on port \"%d\"\n",
                     master_node -> master_node,
                     master_node -> port);
        return 1;
    }

    /* Register master */
    master_nodes = master_node;

    return 0;
}



void * TM_add (void * args)
{
    data_transfer_t * dt = args;

    if (!dt)
        return NULL;


    if (dt -> file)
    {

        /* Fire off a connection back to the Client on the given host and port info */
        s = firstTCPSocketConnectingCorrectly (host, port);
        if (s < 0)
        {
            /* Failed to open data port */
            return NULL;
        }
        else
        {




        write (ftp_state -> data_channel -> data_sock, output, strlen(output));
        shutdown (ftp_state -> data_channel -> data_sock, SHUT_RDWR);
        close (ftp_state -> data_channel -> data_sock);
        ftp_state -> data_channel -> data_sock = -1;

        msg = net_msg_create (100);
        msg -> msg -> num_bytes = snprintf ((char *) msg -> msg -> buffer, 
                                                     msg -> msg -> buffer_size,
                                                     "226 transfer finished.\r\n");
        net_msg_push_on_queue (ftp_state -> output_q, msg);



    }
    else if (file)
    {
    }
}

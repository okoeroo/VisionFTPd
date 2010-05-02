#include "trans_man.h"


static data_transfer_t * transfers  = NULL;
static int max_concurrent_transfers = 10;


static master_node_t * master_nodes = NULL;



int TM_init (char * master_node,
             short port,
             int max_con_transfers)
{
    max_concurrent_transfers = max_con_transfers;

    master_node_t * this_master_node = NULL;

    this_master_node = malloc (sizeof (master_node_t));
    this_master_node -> master_node = strdup(master_node);
    this_master_node -> port        = port;
    this_master_node -> socket      = -1;
    this_master_node -> next        = NULL;

    /* Fire up connection to home base */
    this_master_node -> socket = firstTCPSocketConnectingCorrectly (this_master_node -> master_node, this_master_node -> port);
    if (this_master_node -> socket < 0)
    {
        scar_log (1, "%s: Error: unable to establish connection to the master node \"%s\" on port \"%d\"\n",
                     this_master_node -> master_node,
                     this_master_node -> port);
        return 1;
    }

    /* Register master */
    master_nodes = this_master_node;

    return 0;
}



void * TM_add (void * args)
{
    data_transfer_t * dt = (data_transfer_t *) args;
    int s = -1;

    if (!dt)
        return NULL;


    /* if (dt -> file) */
    {

        /* Fire off a connection back to the Client on the given host and port info */
        /* s = firstTCPSocketConnectingCorrectly (host, port); */
        if (s < 0)
        {
            /* Failed to open data port */
            return NULL;
        }
        else
        {
        }

    }
    /* else */
    {
    }

    return NULL;
}

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


void * slave_comm_to_master (void * args)
{
    master_node_t * my_master = (master_node_t *) args;
    pthread_mutex_t mutex;
    pthread_cond_t cv;
    struct timespec abstime = { 0, 0 };


    while (1)
    {
        pthread_cond_init(&cv, NULL);
        pthread_mutex_init(&mutex, NULL);
        pthread_mutex_lock(&mutex);

        abstime.tv_sec = time(NULL) + 5;
        printf ("%d\n", (int) time(NULL));
        pthread_cond_timedwait(&cv, &mutex, &abstime);
        printf ("%d\n", (int) time(NULL));


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
        if (my_master -> socket < 0)
        {
            scar_log (1, "%s: Error: unable to establish connection to the master node \"%s\" on port \"%d\"\n",
                         __func__,
                         my_master -> master_node,
                         my_master -> port);
        }
    }

    return NULL;
}

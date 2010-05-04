#include "main.h"
/* #include "internal_helper_funcs.h" */

#include "_scar_log.h"
#include "net_common.h"

#include <sys/types.h>
#include <netinet/in.h>

#include "vfs.h"
#include "commander.h"
#include "dispatcher.h"
#include "trans_man.h"

#include <signal.h>

int process_type = MASTER;


void usage (void)
{
    printf ("%s %s\n", APP_NAME, APP_VERSION);
    printf ("\t--help               : This garbage called \'help\'...\n");
    printf ("\t--slave              : Starts this process as a Data-move aka Slave. It must register with a Master to be useful\n");
    printf ("\t--masterhost <arg>   : Requires an argument that specifies a Hostname, IPv4 or IPv6 address to the Master\n");
    printf ("\t--masterport <arg>   : Requires an argument that specifies the port number to use. The Master must already be listening to this port.\n");
    printf ("\t--chroot <arg>       : Requires an argument that specifies the root of the data to share through the Virtual File System hosted on the Master, pushed by the Slaves\n");
    printf ("\t--password <arg>     : Password used to authenticate the Master and the Slaves. Only when the keys are the same, the Master is trusted\n");
    printf ("\n");
}


/* Main */
int main (int argc, char * argv[])
{
    pthread_t                 vfs_thread;
    pthread_t                 cmd_thread;
    pthread_t                 dispatcher_thread;
    commander_options_t *     commander_options = NULL;
    dispatcher_options_t *    dispatcher_options = NULL;
    master_node_t *           master_nodes = NULL;
    master_node_t *           tmp_master_nodes = NULL;
    char *                    password = NULL;
    char *                    vision_chroot = NULL;
    char *                    master_node_addr = NULL;
    short                     master_node_port = -1;
    int                       i = 0;

    vfs_t *                   vfs_root = NULL;

    pthread_attr_t            attr;
    size_t                    stacksize             = 0;


    signal (SIGPIPE, SIG_IGN);

    scar_set_log_line_prefix (APP_NAME);
    scar_log_open (NULL, NULL, DO_ERRLOG);

    
    for (i = 1; i < argc; i++)
    {
        if ((strcasecmp(argv[i], "--help") == 0) ||
            (strcasecmp(argv[i], "-help") == 0) ||
            (strcasecmp(argv[i], "-h") == 0))
        {
            usage();
            return 0;
        }
        else if (strcasecmp(argv[i], "--password") == 0)
        {
            if ((i + 1) < argc)
            {
                password = argv[i + 1];
            }
            else
            {
                scar_log (1, "Failed to supply --password <secret> option\n");
                return 1;
            }
        }
        else if (strcasecmp(argv[i], "--slave") == 0)
        {
            process_type = SLAVE;
        }
        else if (strcasecmp(argv[i], "--masterhost") == 0)
        {
            if ((i + 1) < argc)
            {
                master_node_addr = argv[i + 1];
            }
            else
            {
                scar_log (1, "Failed to supply --masterhost <host/ipv4/ipv6> option\n");
                return 1;
            }
        }
        else if (strcasecmp(argv[i], "--masterport") == 0)
        {
            if ((i + 1) < argc)
            {
                master_node_port = strtol (argv[i + 1], NULL, 10);
            }
            else
            {
                scar_log (1, "Failed to supply --masterport <port> option\n");
                return 1;
            }
        }
        else if (strcasecmp(argv[i], "--chroot") == 0)
        {
            if ((i + 1) < argc)
            {
                vision_chroot = argv[i + 1];
            }
            else
            {
                scar_log (1, "Failed to supply --chroot <dir> option\n");
                return 1;
            }
        }
    }

    /* Integrity checks */
    if (!vision_chroot)
    {
        scar_log (1, "Failed to supply --chroot <dir> option\n");
        return 1;
    }

    if ((process_type == SLAVE) && (master_node_port == -1))
    {
        scar_log (1, "Error: Slaves must have a configured Master port: use --masterport <port>\n");
        return 1;
    }
    if ((process_type == SLAVE) && (master_node_addr == NULL))
    {
        scar_log (1, "Error: Slaves must have a configured Master host: use --masterhost <hostname/IPv4/IPv6>\n");
        return 1;
    }


    /* Upgrade memory boundry of threads */
    pthread_attr_init(&attr);
    pthread_attr_getstacksize (&attr, &stacksize);
    scar_log (1, "Default stack size = %li\n", stacksize);
    stacksize = sizeof(double)*1000000+1000000;

    scar_log (1, "Amount of stack needed per thread = %li\n",stacksize);
    pthread_attr_setstacksize (&attr, stacksize);


    
    /* vfs_main */
    if (0 != pthread_create (&vfs_thread, &attr, vfs_main, (void *) (vision_chroot)))
    {
        scar_log (1, "Failed to start VFS index thread. Out of memory\n");
        return 1;
    }
    pthread_join (vfs_thread, (void **) &vfs_root);
    if (vfs_root == NULL)
    {
        scar_log (1, "Error: no VFS available\n");
        return 1;
    }
    else
    {
        scar_log (1, "The Virtual File System\n");
        VFS_print (vfs_root);
    }

    /* Master process type */
    if (process_type == MASTER)
    {
        /* Start Dispatcher */
        dispatcher_options = calloc (1, sizeof (dispatcher_options_t));
        if (dispatcher_options == NULL)
            scar_log (1, "Out of memory\n");

        dispatcher_options -> port        = 6666;
        dispatcher_options -> max_clients = 100;
        dispatcher_options -> vfs_root    = vfs_root;

        /* Fire up the dispatcher */
        if (0 != pthread_create (&dispatcher_thread, NULL, startdispatcher , (void *)(&dispatcher_options)))
        {
            scar_log (1, "Failed to start FTP Commander thread. Out of memory\n");
            return 1;
        }

        /* Start Commander */
        commander_options = calloc (1, sizeof (commander_options_t));
        if (commander_options == NULL)
            scar_log (1, "Out of memory\n");

        commander_options -> port        = 6621;
        commander_options -> max_clients = 100;
        commander_options -> ftp_banner  = "VisionFTPd v0.1";
        commander_options -> vfs_root    = vfs_root;

        /* Fire up the commander */
        if (0 != pthread_create (&cmd_thread, NULL, startCommander, (void *)(&commander_options)))
        {
            scar_log (1, "Failed to start FTP Commander thread. Out of memory\n");
            return 1;
        }

        /* Wait for commander thread join */
        pthread_join (cmd_thread, NULL);
    }
    else
    {
        /* Initialize a master */
        if (TM_init (&master_nodes, master_node_addr, master_node_port, 10) != 0)
        {
            scar_log (1, "Failed to connect to VisionFTPd Master\n");

            /* Easy exit */
            scar_log_close();
            return 1;
        }

        /* Connection to Master nodes for instructions */
        tmp_master_nodes = master_nodes;
        while (tmp_master_nodes)
        {
            if (0 != pthread_create (&(tmp_master_nodes -> tid), 
                                     NULL, 
                                     slave_comm_to_master, 
                                     (void *)tmp_master_nodes))
            {
                scar_log (1, "Failed to submit to Master node. Out of memory\n");
            }
            /* Bow to next Master */
            tmp_master_nodes = tmp_master_nodes -> next;
        }
        
        /* Await release from Masters bidding */
        tmp_master_nodes = master_nodes;
        while (tmp_master_nodes)
        {
            if (0 != pthread_join (tmp_master_nodes -> tid,
                                   NULL))
            {
                scar_log (1, "Failed to submit to Master node. Out of memory\n");
            }
            /* Bow to next Master */
            tmp_master_nodes = tmp_master_nodes -> next;
        }
    }

    /* Easy exit */
    scar_log_close();
    return 0;
}

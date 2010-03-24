#include "main.h"
/* #include "internal_helper_funcs.h" */

#include "_scar_log.h"
#include "net_common.h"

#include <sys/types.h>
#include <netinet/in.h>


#include "commander.h"
#include "vfs.h"


/* Main */
int main (int argc, char * argv[])
{
    pthread_t             vfs_thread;
    pthread_t             cmd_thread;
    commander_options_t * commander_options;
    char *                vision_chroot = NULL;
    int                   i = 0;

    vfs_t *               vfs_root = NULL;

    pthread_attr_t        attr;
    size_t                stacksize             = 0;


    scar_set_log_line_prefix ("VisionFTPd");
    scar_log_open (NULL, NULL, DO_ERRLOG);

    
    for (i = 1; i < argc; i++)
    {
        if (strcasecmp(argv[i], "--chroot") == 0)
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

    if (!vision_chroot)
    {
        scar_log (1, "Failed to supply --chroot <dir> option\n");
        return 1;
    }


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


    /* Easy exit */
    scar_log_close();
    return 0;
}

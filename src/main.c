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

    scar_set_log_line_prefix ("VisionFTPd");
    scar_log_open (NULL, NULL, DO_ERRLOG);

    

    /* Start Commander */
    commander_options = calloc (1, sizeof (commander_options_t));
    if (commander_options == NULL)
        scar_log (1, "Out of memory\n");

    commander_options -> port        = 6621;
    commander_options -> max_clients = 100;
    commander_options -> ftp_banner  = "VisionFTPd v0.1";

    /* Fire up the commander */
    if (0 != pthread_create (&cmd_thread, NULL, startCommander, (void *)(&commander_options)))
    {
        scar_log (1, "Failed to start FTP Commander thread. Out of memory\n");
        return 1;
    }

    /* vfs_main */
    /* vfs_main ("/tmp/"); */

    /* startSlave(); */
    pthread_join (cmd_thread, NULL);

    /* Easy exit */
    scar_log_close();
    return 0;
}

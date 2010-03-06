#include "main.h"
/* #include "internal_helper_funcs.h" */

#include "_scar_log.h"
#include "net_common.h"

#include <sys/types.h>
#include <netinet/in.h>


#include "commander.h"


/* Main */
int main (int argc, char * argv[])
{
    scar_set_log_line_prefix ("VisionFTPd");
    scar_log_open (NULL, NULL, DO_ERRLOG);


    /* Start Commander */
    startCommander(6621, 100, "VisionFTPd v0.1");


    /* startSlave(); */

    /* Easy exit */
    scar_log_close();
    return 0;
}

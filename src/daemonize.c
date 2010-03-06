#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include "main.h"


RC_PROC_TYPE daemonize(void) 
{
    int fd = -1;
    pid_t pid = 0;

    switch (pid = fork()) 
    {
        case 0:
            switch (pid = fork())
            {
                case 0:
                    break;
                case -1:
                    exit(0);
                default:
                    exit(0);
            }
            break;
        case -1:
            return ERROR;
        default:
            return ANGEL;
           /* _exit(0); */
    }

    if (setsid() == 0) 
    {
        fprintf(stderr, "Error demonizing (setsid)!\n");
        return ERROR;
    }

#if 0
    switch (fork()) 
    {
        case 0:
            break;
        case -1:
            // Error
            fprintf(stderr, "Error demonizing (fork2)! %d - %s\n", errno, strerror(errno));
            exit(0);
            break;
        default:
            _exit(0);
    }
#endif

    /* Redirect input of stdin from /dev/null */
    fd = open("/dev/null", O_RDONLY);
    if (fd != 0) 
    {
        dup2(fd, 0);
        close(fd);
    }

    /* Redirect stdout to /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd != 1) 
    {
        dup2(fd, 1);
        close(fd);
    }

    /* Redirect stderr to /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd != 2) 
    {
       dup2(fd, 2);
       close(fd);
    }

    return DAEMON;
}



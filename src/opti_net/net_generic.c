#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/param.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <syslog.h>
#include <netdb.h>
#include <netinet/in.h>

#include <time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/wait.h>

#if defined(TARGET_ARCH_AIX)
#define netlen_t size_t
#else
#define netlen_t int
#endif

#include <arpa/inet.h> /* for inet_ntoa() */



/******************************************************************************
Function:    net_accept()
Description: Accept a connection on socket skt and return fd of new connection. 
Parameters:
Returns:
******************************************************************************/
static int
net_accept(int skt)
{
    netlen_t           fromlen;
    int                client = 0;
    int                gotit = 0;
    struct sockaddr_in from;

    fromlen = sizeof(from);
    gotit = 0;

    while (!gotit)
    {
        fd_set         fdset;
        struct timeval timeout;
        int            n;

        FD_ZERO(&fdset);
        FD_SET(skt, &fdset);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        n = select(skt + 1, &fdset, (fd_set *) 0, &fdset, &timeout);

        if (n < 0 && errno != EINTR)
        {
            error_check(n, "net_accept select");
        }
        else if (n > 0)
        {
            long flags;

            skt2 = accept(skt, (struct sockaddr *) &from, &fromlen);

            if (skt2 == -1)
            {
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    error_check(skt2, "net_accept accept");
                }
            }
            else
                gotit = 1;
            flags = fcntl(skt2, F_GETFL, 0);
            flags &= ~O_NONBLOCK;
            fcntl(skt2, F_SETFL, flags);
        }
    }

    return(skt2);
}



/******************************************************************************
Function:       net_setup_listener()
Description:    
Parameters:
Returns:
******************************************************************************/
static void
net_setup_listener(int backlog,
                   int * port,
                   int * skt)
{
    netlen_t        sinlen;
    struct sockaddr_in sin;
    long flags;
    int one=1;

    *skt = socket(AF_INET, SOCK_STREAM, 0);
    error_check(*skt,"net_setup_anon_listener socket");

    flags = fcntl(*skt, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(*skt, F_SETFL, flags);

    error_check(setsockopt(*skt, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)),
                "net_setup_anon_listener setsockopt");

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(*port);

    sinlen = sizeof(sin);

    error_check(bind(*skt,(struct sockaddr *) &sin,sizeof(sin)),
                "net_setup_anon_listener bind");


    error_check(listen(*skt, backlog), "net_setup_anon_listener listen");

    getsockname(*skt, (struct sockaddr *) &sin, &sinlen);
    *port = ntohs(sin.sin_port);
}

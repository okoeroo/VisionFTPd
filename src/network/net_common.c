#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>

#include "net_common.h"
#include "scar_log.h"

#include <fcntl.h>

#define TRUE   1
#define FALSE  0


#define	IP_MAX_CHARS			64
#define	PORT_MAX_DIGITS			8
#define	PREFIX_V4_MAPPED		"::ffff:"




int select_and_wait_for_activity_loop (int          this_socket,
                                       time_t       seconds,
                                       suseconds_t  microseconds)
{
    int activity = 0;
    fd_set readfds;

    struct timeval timeout;

    timeout.tv_sec  = seconds;
    timeout.tv_usec = microseconds;


    /* scar_log (5, "%s: Waiting for socket descriptor %d activity...\n", __func__, this_socket); */
    for (;;)
    {
        FD_ZERO(&readfds);

        /* setup which sockets to listen on */
        FD_SET(this_socket, &readfds);

        /* wait for connection, forever if we have to */
        activity = select(this_socket + 1, &readfds, (fd_set*)0, (fd_set*)0, &timeout);

        if ((activity < 0) && (errno!=EINTR))
        {
            /* there was an error with select() */
#ifdef DEBUG
            scar_log (3, "%s: There was an error with the select() on socket descriptor %d.\n", __func__, this_socket);
#endif
            return 1;
        }
        else if (activity == 0)
        {
            /* Time out reached */
            /* scar_log (3, "%s: Time out reached on socket descriptor %d.\n", __func__, this_socket); */
            return 2;
        }
        else
        {
            if (FD_ISSET(this_socket, &readfds))
            {
                /* scar_log (5, "%s: Got client activity on socket descriptor %d.\n", __func__, this_socket); */
                break;
            }
        }
    }

    return 0;
}


/* Function returning the IP string representation of a socket address */
void getIPString(struct sockaddr *paddress, char *ip, int ip_length)
{
	/* retrieve the ip */
	getnameinfo(paddress, sizeof(struct sockaddr_storage), ip, ip_length, 0, 0, NI_NUMERICHOST);

	/* if ipv4-mapped address, we remove the mapping notation in order to get only  */
	/* the IPv4 address */
	if (strncasecmp(ip, PREFIX_V4_MAPPED, strlen(PREFIX_V4_MAPPED)) == 0)
	{	
		memmove(ip, ip + strlen(PREFIX_V4_MAPPED),
			strlen(ip) - strlen(PREFIX_V4_MAPPED) +1);
	}
}


/* gethostbyaddr wrapper, first trying IPv6, then IPv4 */
struct hostent *gethostbyaddr_wrapper(const char *address)
{
    struct in_addr addr;
    struct in6_addr addr6;
    int    rc = 0;

    /* addr.s_addr = str_to_addr(address); */
    if ((rc = inet_pton (AF_INET6, address, &(addr6.s6_addr))) > 0)
    {
        return gethostbyaddr((char *) &addr6, 16, AF_INET6);       /* IPv4 only for now */
    }
    if ((rc = inet_pton (AF_INET, address, &(addr.s_addr))) > 0)
    {
        return gethostbyaddr((char *) &addr, 4, AF_INET);       /* IPv4 only for now */
    }

    return NULL;
}



/****************************************************
Function: net_addr2host
Variables:
          char ** hostname            : buffer in which the hostname will be written,
                                        Will allocate if not allocated yet.
          int     bufsize             : size of hostname buffer
          struct  sockaddr_storage ss : socket address information (for Internet)
Return:
    0 : Good
   !0 : Not good
****************************************************/
int net_addr2host (char **               hostname,
                   int                   bufsize,
                   struct sockaddr_storage ss)
{
    int rc  = 0;
    int bsz = bufsize;

    if (bsz <= 0)
        bsz = 256;

    /* Allocate if you didn't yet */
    if (*hostname == NULL)
    {
        *hostname = (char *) malloc (sizeof (char) * bsz);
    }

    if (getnameinfo ((struct sockaddr *) &ss, sizeof (ss),
                     *hostname, bsz, NULL, 0, 0) != 0)
    { 
        rc = getnameinfo ((struct sockaddr *) &ss, sizeof (ss),
                          *hostname, bsz, NULL, 0, NI_NUMERICHOST);
    }
    return rc;
}




void net_printErrNo (int the_errno, const char * logstr)
{
    switch (the_errno) 
    {   
        case EBADF :       
             scar_log (5, "%s: Socket descriptor not a valid descriptor\n", logstr);
             break;
        case ENOTSOCK    : 
             scar_log (5, "%s: The (socket) descriptor references a file, not a socket.\n", logstr);
             break;
        case EMFILE      : 
             scar_log (5, "%s: The per-process descriptor table is full.\n", logstr);
             break;
        case ENFILE      : 
             scar_log (5, "%s: The system file table is full.\n", logstr);
             break;
        case EWOULDBLOCK : 
             scar_log (5, "%s: The socket is marked non-blocking and no connections are present to be accepted.\n", logstr);
             break;
        case EOPNOTSUPP  : 
             scar_log (5, "%s: The referenced socket is not of type SOCK_STREAM.\n", logstr);
             break;
        case EADDRNOTAVAIL :
             scar_log (5, "%s: The specified address is not available on this machine\n", logstr);
             break;
        case EAFNOSUPPORT :
             scar_log (5, "%s: Addresses in the specified address family cannot be used with this socket\n", logstr);
             break;
        case EISCONN :
             scar_log (5, "%s: The socket is already connected\n", logstr);
             break;
        case ETIMEDOUT :
             scar_log (5, "%s: Connection establishment timed out without establishing a connection\n", logstr);
             break;
        case ECONNREFUSED :
             scar_log (5, "%s: The attempt to connect was forcefully rejected\n", logstr);
             break;
        case ENETUNREACH :
             scar_log (5, "%s: The network isn't reachable from this host\n", logstr);
             break;
        case EADDRINUSE :
             scar_log (5, "%s: The address is already in use\n", logstr);
             break;
        case EFAULT :
             scar_log (5, "%s: The name parameter specifies an area outside the process address space\n", logstr);
             break;
        case EINPROGRESS :
             scar_log (5, "%s: The socket is non-blocking and the connection cannot be completed immediately. It is possible to select(2) for completion by selecting the socket for writing\n", logstr);
             break;
        case EALREADY :
             scar_log (5, "%s: The socket is non-blocking and a previous connection attempt has not yet been completed\n", logstr);
             break;
        case EACCES :
             scar_log (5, "%s: The destination address is a broadcast address and the socket option SO_BROADCAST is not set\n", logstr);
             break;
                           
    }
}


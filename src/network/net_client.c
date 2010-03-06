#include "net_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>


#include "scar_log.h"

#include <fcntl.h>
#include <sys/ioctl.h>

#define TRUE   1
#define FALSE  0


#define	IP_MAX_CHARS			64
#define	PORT_MAX_DIGITS			8
#define	PREFIX_V4_MAPPED		"::ffff:"



/* function trying to connect to all available addresses of an host */
/* and returning the first socket successfully connected, or -1 */
/* ------------------------------------------------------------------------------------ */
int firstTCPSocketConnectingCorrectly(char *host, int port)
{
	int connection_socket = -1;
	struct addrinfo hints, *address_list, *paddress;
	char port_string[PORT_MAX_DIGITS];
    int res = 1;
    int flag = 1;
    fd_set wfds;
    /* struct timeval tv[1]; */
    struct timeval tv;
    struct timeval *                    wait_tv = NULL;
    struct timeval                      preset_tv;
    unsigned int                        preset_tvlen = sizeof preset_tv;
    int time_out_milliseconds = 30 * 1000;
    socklen_t lon;
    int valopt = 0;


	/* getaddrinfo parameters */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;		/* find both ipv4 and ipv6 addresses */
	hints.ai_socktype = SOCK_STREAM;	/* type of socket */

	sprintf (port_string, "%d", port);
	getaddrinfo(host, port_string, &hints, &address_list);

	paddress = address_list;
	while (paddress)
	{
		/* create the socket */
		connection_socket = socket(paddress->ai_family, paddress->ai_socktype, 
                                   paddress->ai_protocol);
		if (connection_socket == -1)
		{
			paddress = paddress->ai_next;
            /* Try next address */
			continue;
		}


        /* Grab timeout setting */
        if (getsockopt (connection_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&preset_tv, &preset_tvlen) < 0)
        {
#ifdef DEBUG
            scar_log (1, "%s: Error: Failed to get the timeout setting\n", __func__);
#endif
            return 1;
        }
        /* Set connection timeout on the socket */
        wait_tv = (struct timeval *) malloc (sizeof (struct timeval));
        wait_tv->tv_sec = (time_out_milliseconds - (time_out_milliseconds % 1000)) / 1000;
        wait_tv->tv_usec = (time_out_milliseconds % 1000) * 1000;
        if (setsockopt (connection_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)wait_tv, sizeof *wait_tv) < 0)
        {
#ifdef DEBUG
            scar_log (1, "%s: Error: Failed to set the timeout setting\n", __func__);
#endif
            return 1;
        }
        free (wait_tv);
        wait_tv = NULL;


        /* Enable KeepAlive */
        valopt = 1;
        if (setsockopt(connection_socket, SOL_SOCKET, SO_KEEPALIVE, (char *) &valopt, sizeof(valopt)) < 0)
        {
            scar_log (1, "%s: Error: Failed to set the keep-alive setting\n", __func__);
            close (connection_socket);
            connection_socket = -1;
            continue;
        }


        /* Set socket to non-blocking */
#if 1
#ifdef DEBUG
        scar_log (5, "%s: Activate non-blocking mode.\n", __func__);
#endif
        if (fcntl(connection_socket, F_SETFL, fcntl(connection_socket, F_GETFL)|O_NONBLOCK) == -1)
        {
#ifdef DEBUG
            scar_log (5, "%s: Setting socket to non-blocking failed.\n", __func__);
#endif
            continue;
        }
#endif

#if 1
#ifdef TCP_NODELAY
        /* turn off TCP coalescence */
        valopt = 1;
        setsockopt (connection_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &valopt, sizeof (int));
 
#endif
        if ((flag = fcntl(connection_socket, F_GETFL, 0)) != -1) 
        {
            flag |= O_NDELAY;
            fcntl(connection_socket, F_SETFL, flag);
        }
#endif

		/* connect */
        if (connect(connection_socket, paddress->ai_addr, paddress->ai_addrlen)	== -1)
        {
            switch (errno)
            {
#ifdef _WIN32
                case WSAEINPROGRESS :
#else
                case EINPROGRESS :
#endif
#ifdef DEBUG
                    scar_log (5, "%s: Connection in progress...\n", __func__);
#endif
                    do 
                    {
                        tv.tv_sec  = 30;
                        tv.tv_usec = 0;

                        FD_ZERO(&wfds); 
                        FD_SET(connection_socket, &wfds); 
                        res = select(connection_socket + 1, NULL, &wfds, NULL, &tv); 
                        if (res < 0 && errno != EINTR)
                        {
                            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
                            connection_socket = -1;
                            goto bail_out;
                        } 
                        else if (res > 0) 
                        { 
                            /* Socket selected for write  */
                            lon = sizeof(int); 
                            if (getsockopt(connection_socket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) 
                            { 
                                fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                                connection_socket = -1;
                                goto bail_out;
                            } 
                            /* Check the value returned...  */
                            if (valopt) 
                            { 
                                fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)); 
                                connection_socket = -1;
                                goto bail_out;
                            } 
                            break; 
                        } 
                        else 
                        { 
                            fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
                            connection_socket = -1;
                            goto bail_out;
                        } 
                    } while (1); 


                    /* TODO: I should go here... But I don't! */

                    break;
                default : 
#ifdef DEBUG
                    scar_log (5, "%s: Connect failed: %s\n", __func__, strerror(errno));
#endif
                    close(connection_socket); connection_socket = -1;
                    paddress = paddress->ai_next;
                    /* Try next address */
                    continue;
            }
		}


        /* Everything went fine */
		break;
	}


bail_out:
	/* free the list */
	freeaddrinfo(address_list);

	return connection_socket;
}




int net_create_client_socket (int * client_socket, 
                              const char * server, 
                              int port, 
                              int time_out_milliseconds)
{
    char                               *logstr = "create_client_socket";
    struct addrinfo                     hints;
    struct addrinfo                    *res;
    int                                 rc;
    int                                 mysock = -1;
    char                                portstr[24];

    struct timeval *                    wait_tv = NULL;
    struct timeval                      preset_tv;
    unsigned int                        preset_tvlen = sizeof preset_tv;


    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family   = AF_INET;

    /* Get addrinfo */
    snprintf(portstr, 24, "%d", port);
    rc = getaddrinfo(server, &portstr[0], &hints, &res);
    if (rc != 0)
    {
#ifdef DEBUG
        scar_log (1, "Error: Failed to getaddrinfo (%s, %s, *, *)\n", server, portstr);
#endif
        return 1;
    }


    /* Create new socket */
    if ((mysock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
#ifdef DEBUG
        scar_log (1, "Error: Failed to create socket\n");
#endif
        return 1;
    }

    
    /* Grab timeout setting */
    if (getsockopt (mysock, SOL_SOCKET, SO_RCVTIMEO, (char *)&preset_tv, &preset_tvlen) < 0)
    {
#ifdef DEBUG
        scar_log (0, "%s: Error: Failed to get the timeout setting\n", logstr);
#endif
        return 1;
    }


    /* Set connection timeout on the socket */
    wait_tv = (struct timeval *) malloc (sizeof (struct timeval));
    wait_tv->tv_sec = (time_out_milliseconds - (time_out_milliseconds % 1000)) / 1000;
    wait_tv->tv_usec = (time_out_milliseconds % 1000) * 1000;
    if (setsockopt (mysock, SOL_SOCKET, SO_RCVTIMEO, (char *)wait_tv, sizeof *wait_tv) < 0)
    {
#ifdef DEBUG
        scar_log (0, "%s: Error: Failed to set the timeout setting\n", logstr);
#endif
        return 1;
    }
    free (wait_tv);
    wait_tv = NULL;


    /* Connecting socket to host on port with timeout */
    if (connect(mysock, res -> ai_addr, res -> ai_addrlen) < 0)
    {
#ifdef DEBUG
        scar_log (1, "Failed to connect\n");
#endif
        return 1;
    }
    else
    {
        /* Socket is succesfuly connected */
        setsockopt (mysock, SOL_SOCKET, SO_KEEPALIVE, 0, 0);

        *client_socket = mysock;
        return 0;
    }

    /* Failure */
    return 1;
}





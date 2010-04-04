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



/*
 * Case of a server with two sockets:
 * function trying to create and set up all (ipv4 and ipv6) available server sockets of
 * the host. When one socket is IPv6, the IPV6_V6ONLY socket option is set.
 * The list of sockets is passed as a parameter to be completed.
 */
void createAndSetUpAllTCPServerSockets(int *nb_sockets, int *tb_sockets,
                                int port, int max_connections)
{
	int server_socket;
	struct addrinfo hints, *address_list, *paddress;
	int on = 1;
	char ip[IP_MAX_CHARS];
	char port_string[PORT_MAX_DIGITS];

	*nb_sockets = 0;
	
	/* getaddrinfo parameters */
	memset(&hints, 0, sizeof(hints));	/* init */
	hints.ai_flags |= AI_PASSIVE;		/* server behaviour: accept connection on any network address */
	hints.ai_family = AF_UNSPEC;		/* find both ipv4 and ipv6 addresses */
	hints.ai_socktype = SOCK_STREAM;	/* type of socket */

	sprintf (port_string, "%d", port);
	getaddrinfo(NULL, port_string, &hints, &address_list);

	paddress = address_list;
	while (paddress)
	{
		/* create the socket */
		server_socket = socket(paddress->ai_family, paddress->ai_socktype, paddress->ai_protocol);
		if (server_socket == -1)
		{
			/* try next address */
			paddress = paddress->ai_next;
			continue;
		}

		/* set SO_REUSEADDR option */
		setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		
        /* we must set IPV6_V6ONLY in order to allow an IPv4 socket on the same port number. */
		if (paddress->ai_family == AF_INET6)
		{	
			setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(on));
		}

		/* bind and listen */
		if (	(bind(server_socket, paddress->ai_addr, paddress->ai_addrlen)	== -1) ||
			(listen(server_socket, max_connections)	 			== -1))
		{
			close(server_socket);
            /* try next address */
			paddress = paddress->ai_next;
			continue;
		}

		/* get the ip string, display it with the port number  */
		getIPString(paddress->ai_addr, ip, IP_MAX_CHARS);
		printf("Server socket now listening on %s port %d\n", ip, port);
		
		tb_sockets[*nb_sockets] = server_socket;
		*nb_sockets += 1;
		paddress = paddress->ai_next;
	}

	/* free the list */
	freeaddrinfo(address_list);
}


/*
 * Case of a server with one socket:
 * function trying to create a server socket using all available addresses of the host
 * until one is successfully created. If the socket is IPv6, the IPV6_V6ONLY
 * socket option is unset.
 * Then the socket is returned, or -1 in case of failure.
 */
int createAndSetUpATCPServerSocket(int port, int max_connections)
{
	int server_socket;
	struct addrinfo hints, *address_list, *paddress;
	int on = 1, off = 0;
	char ip[IP_MAX_CHARS];
	char port_string[PORT_MAX_DIGITS];

	server_socket = -1;

    scar_log(1, "Try address...\n");
	
	/* getaddrinfo parameters */
	memset(&hints, 0, sizeof(hints));	/* init */
	hints.ai_flags |= AI_PASSIVE;		/* server behaviour: accept connection on any network address */
	hints.ai_family = AF_UNSPEC;		/* find both ipv4 and ipv6 addresses */
	hints.ai_socktype = SOCK_STREAM;	/* type of socket */

	sprintf (port_string, "%d", port);
	getaddrinfo(NULL, port_string, &hints, &address_list);

	paddress = address_list;
	while (paddress)
	{
		/* create the socket */
		server_socket = socket(paddress->ai_family, paddress->ai_socktype, paddress->ai_protocol);
		if (server_socket == -1)
		{
            /* try next address */
			paddress = paddress->ai_next;
			continue;
		}

		/* set SO_REUSEADDR option */
		setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		
        /* we must unset IPV6_V6ONLY in order to allow IPv4 client to connect to the IPv6 socket */
		if (paddress->ai_family == AF_INET6)
		{	
			setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
		}

        /* Make the master socket non-blocking */
        if (fcntl (server_socket, 
                   F_SETFL, 
                   fcntl(server_socket, F_GETFL)|O_NONBLOCK) == -1)
        {
            net_printErrNo (errno, __func__);
            return -1;
        }

		/* bind and listen */
		if ((bind (server_socket, paddress->ai_addr, paddress->ai_addrlen) == -1) ||
            (listen (server_socket, max_connections) == -1))
		{
			close(server_socket); 
            server_socket = -1;

            /* try next address */
			paddress = paddress->ai_next;
			continue;
		}

		/* get the ip string, display it with the port number  */
		getIPString(paddress->ai_addr, ip, IP_MAX_CHARS);
		scar_log (1, "Server socket now listening on %s port %d (socket fd %d)\n", 
                     ip, 
                     port, 
                     server_socket);
		
        /* break if no error */
		break;
	}

	/* free the list */
	freeaddrinfo(address_list);

	/* return the server socket */
	return server_socket;
}



/****************************************************
Function: net_server
Description:
          Setup Complete Service Side

Variables:
          const int port : listening socket
          int (*net_run_accepted_client_func_ptr)(const int, const char *)
                         : Callback function to execute when a client socket has been fully established
Return:
    0 : Good
   !0 : Not good
****************************************************/
int net_server (const int port,  int (*net_run_accepted_client_func_ptr)(const int, const char *))
{
    int    master          = 0;
    int    pid             = -1;
    int    rc              = 0;
    int    i               = 0;
    char * remote_hostname = NULL;
    int    client_sock     = -1;
    int    process_type    = GRANDPARENT_PROCESS;
    int    worker_cnt      = AMOUNT_OF_WORKERS;
    int    status          = 0;

    int activity = 0;
    fd_set readfds;
    struct timeval timeout;


    if (!net_run_accepted_client_func_ptr)
        return EXIT_CODE_BAD;
        
    if ((rc = net_setup_master_socket (&master, port)))
    {
        net_printErrNo (rc, __func__);
        return EXIT_CODE_BAD;
    }
    else
    {
        /* Auto clean up children */
        signal(SIGCHLD, SIG_IGN);
        scar_log (1, "%s: Dispatching child proc. process ID (%d)\n", __func__, getpid());
        
        /* Fork of a bunch of worker process */
        for(i = 0; i < worker_cnt; i++) 
        {
            pid = fork();
            if (pid < 0)
            {
                scar_log (1, "%s: Grand Parent Process: fork FAIL!\n", __func__);
                continue;
            }
            else if (pid == 0)
            {
                /* Child process, here it's a worker */
                signal(SIGCHLD, SIG_IGN);
                process_type = WORKER_PROCESS;
                scar_log (1, "%s: I will work harder... - parent: (%d) child: (%d)\n", __func__, getppid(), getpid());
                break;
            }
            else
            {
                /* Grand Parent Process */
                waitpid (0, &status, WNOHANG);
                continue;
            }
        }

        for(;;) 
        {
            struct sockaddr_storage address;
            unsigned int addrlen = sizeof (address);


            /********************************************/
            timeout.tv_sec=10;
            timeout.tv_usec=0;

            scar_log (1, "%s: Waiting for clients   - parent: (%d) child: (%d)\n", __func__, getppid(), getpid());
            for (;;)
            {
                FD_ZERO(&readfds);

                /* setup which sockets to listen on */
                FD_SET(master, &readfds);

                /* wait for connection, forever if we have to */
                activity = select(master + 1, &readfds, (fd_set*)0, (fd_set*)0, &timeout);

                if ((activity < 0) && (errno!=EINTR))
                {
                    /* there was an error with select() */
                    return EXIT_CODE_BAD;
                }
                else if (activity == 0)
                {
                    /* Time out reached */
                    /* try again... */
                    /* scar_log_debug (3, "%s: Master activity time out reached.\n", __func__); */
                    /* return EXIT_CODE_BAD; */
                }
                else
                {
                    if (FD_ISSET(master, &readfds))
                    {
                        scar_log (1, "%s: Got client activity   - parent: (%d) child: (%d)\n", __func__, getppid(), getpid());
                        break;
                    }
                }
            }
            /********************************************/

            
            /* Open the new socket as 'client_socket' */
            if ((client_sock = accept(master, (struct sockaddr *)&address, &addrlen)) < 0)
            {
                char * fail = malloc (sizeof (char) * 256);
                snprintf (fail, 255, "%s: pid %d", __func__, getpid());
                net_printErrNo (errno, fail);
                continue;
            }
            scar_log (1, "%s: Client accepted       - parent: (%d) child: (%d)\n", __func__, getppid(), getpid());

            /* Allocate some memory for the client side hostname's referce lookup */
            remote_hostname = (char *) malloc (sizeof(char) * MAX_HOSTNAME);
            net_addr2host (&remote_hostname, MAX_HOSTNAME, address);


            switch (pid = fork())
            {
                case -1 :
                    scar_log (1, "%s: fork FAIL!\n", __func__);
                    close (client_sock);
                    free (remote_hostname);
                    continue;
                case 0  :
                    process_type = CLIENT_PROCESS;
                    scar_log (1, "%s: Work process          -                 child: (%d)  work: (%d)\n", __func__, getppid(), getpid());

                    /* Execute the callback function */
                    rc = net_run_accepted_client_func_ptr(client_sock, remote_hostname);

                    shutdown (client_sock, SHUT_WR);
                    close (client_sock);
                    free (remote_hostname);
                    return rc;
                default :
                    waitpid (0, &status, WNOHANG);

                    scar_log (1, "%s: I will work again ... - parent: (%d) child: (%d)\n", __func__, getppid(), getpid());
                    close (client_sock);
                    free (remote_hostname);
                    continue;
            }
        }
    }
    return EXIT_CODE_GOOD;
}


/****************************************************
Function: net_setup_master_socket
Description:
          Setup Master Socket with all the socket options and such.

Variables:
          int * master_socket : master socket descriptor
          int   port          : port number to listen on
Return:
    0 : Good
   !0 : Not good
****************************************************/
int net_setup_master_socket (int * master_socket, int port)
{
    if ((*master_socket = createAndSetUpATCPServerSocket(port, 512)) < 0)
    /* if ((*master_socket = socket(AF_INET,SOCK_STREAM,0)) < 0) */
    {
        net_printErrNo (errno, __func__);
        return 1;
    }
    else
    {
        return 0;
    }
}


/****************************************************
Function: net_accept_new_client_socket
Description:
          Accepts new client socket from a master socket and returns that with the 
          DNS canonical name of the connected remote host

Variables:
          int master_socket   : master socket descriptor
          int * client_socket : return pointer to the client_socket descriptor
          char ** remote_host : DNS canonical name string of the remote host 
                                connecting to the master socket
Return:
    0 : Good
   !0 : Not good
****************************************************/
int net_accept_new_client_socket (int master_socket, char ** remote_host)
{
    struct sockaddr_storage from;
    unsigned int            fromlen = sizeof (from);
    int                     client_fd = 0;
    int                     gotit = 0;
    long                    flags = 0;
    fd_set                  fdset;
    struct timeval          timeout;
    int                     n = 0;;

    gotit = 0;

    while (!gotit)
    {
        FD_ZERO(&fdset);
        FD_SET(master_socket, &fdset);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        n = select(master_socket + 1, &fdset, (fd_set *) 0, &fdset, &timeout);

        if (n < 0 && errno != EINTR)
        {
            scar_log (1, "%s: Failure in select()\n", __func__);
            return -1;
        }
        else if (n > 0)
        {

            client_fd = accept(master_socket, (struct sockaddr *) &from, &fromlen);

            if (client_fd == -1)
            {
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    scar_log (1, "%s: Failure in accept()\n", __func__);
                    return -1;
                }
            }
            else
                gotit = 1;

            /* Get remote_host string from opened connection */
            if (!*remote_host)
                *remote_host = (char *) malloc (sizeof(char) * MAX_HOSTNAME);

            net_addr2host (remote_host, MAX_HOSTNAME, from);

            /* Set client socket to non-blocking */
            flags = fcntl(client_fd, F_GETFL, 0);
            flags &= ~O_NONBLOCK;
            fcntl(client_fd, F_SETFL, flags);
        }
    }

    return(client_fd);
}


#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <unistd.h>

#include <sys/time.h>
#include <string.h>

#include <errno.h>

#include "internal_codes.h"

#ifndef NET_COMMON_H
  #define NET_COMMON_H


typedef enum NET_RC_CODES_e {
    NET_RC_UNHANDLED,
    NET_RC_IDLE,
    NET_RC_DISCONNECT,
    NET_RC_CONNECTED,
    NET_RC_RECONNECT,
    NET_RC_MUST_READ,
    NET_RC_MUST_WRITE
} NET_RC_CODES;


void createAndSetUpAllTCPServerSockets(int *nb_sockets, int *tb_sockets,
                                int port, int max_connections);
int createAndSetUpATCPServerSocket(int port, int max_connections);
void getIPString(struct sockaddr *paddress, char *ip, int ip_length);
void net_printErrNo (int the_errno, const char * logstr);
int net_addr2host (char **               hostname,
                   int                   bufsize,
                   struct sockaddr_storage ss);
int net_server (const int port,  int (*net_run_accepted_client_func_ptr)(const int, const char *));
int net_setup_master_socket (int * master_socket, 
                             int port);
int net_accept_new_client_socket (int master_socket, char ** remote_host);
/*
int net_accept_new_client_socket (int master_socket, 
                                  int * client_socket, 
                                  char ** remote_host);
*/
int net_create_client_socket (int * client_socket, 
                              const char * server, 
                              int port, 
                              int time_out_seconds);
int firstTCPSocketConnectingCorrectly(char *host, int port);

int select_and_wait_for_activity_loop (int          this_socket,
                                       time_t       seconds,
                                       suseconds_t  microseconds);
/* gethostbyaddr wrapper */
struct hostent *gethostbyaddr_wrapper(const char *address);


#define MAX_HOSTNAME 255
#define AMOUNT_OF_WORKERS 1

#define GRANDPARENT_PROCESS 11000
#define WORKER_PROCESS      11001
#define CLIENT_PROCESS      11002


#endif /* NET_COMMON_H */

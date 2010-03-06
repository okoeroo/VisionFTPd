#include "main.h"
#include "net_common.h"


void * client_on_commander_comm_channel (void *);

int startCommander (int port, int max_clients, char * ftp_banner);

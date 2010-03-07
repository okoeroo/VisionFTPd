#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>

#include "main.h"
#include "net_common.h"
#include "ftpd.h"

#include "net_threader.h"
#include "unsigned_string.h"


#ifndef COMMANDER_H
    #define COMMANDER_H


int commander_active_io (buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state, void ** state);
int commander_idle_io   (buffer_state_t * write_buffer_state, void ** state);
int commander_state_initiator (void ** state);
int commander_state_liberator (void ** state);

int startCommander (int port, int max_clients, char * ftp_banner);


#endif /* COMMANDER_H */

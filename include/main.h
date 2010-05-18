#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "internal_codes.h"
/* #include "internal_helper_funcs.h" */

#include "net_common.h"
#include "scar_log.h"
#include "_scar_log.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <sys/param.h>
#include <time.h>
#include <sys/wait.h>



#ifndef MAIN_H
 #define MAIN_H


#define APP_NAME "VisionFTPd"
#define APP_VERSION "0.1"

#define MASTER 1
#define SLAVE  2

/* This handles the connection retry setting */
#define ABSOLUTE_TIME_LIMIT_OF_CLIENT_RETRY_IN_SECONDS 1000000 /* 10^6 == 1M seconds ~ 11 days */
#define SHORT_RETRY_LIMIT_IN_SECONDS 600


/* The follow setting indicates:
 * - That about the client will wait about 15 minutes time disconnected from the comm channel
 * - The exact sleep time is randomly somewhere between 5 minutes before and 10 minutes */
#define WAIT_MAXMIN_SEC  300 
#define WAIT_AVERAGE_SEC 900


/* After being online for 30 minutes the client will disconnect from the comm channel */
#define ONLINE_TIME_IN_SECONDS 1800




#ifndef BUFFERSIZE
  /* 2^14 = 16384, which is the SSL record buffer size */
  #define BUFFERSIZE 65535
#endif

/* Something significantly smaller then the BUFFERSIZE */
#ifndef SUB_BUFFERSIZE
  #define SUB_BUFFERSIZE 1024 
#endif

#define IP_MAX_CHARS    64
#define PORT_MAX_DIGITS 8

/**************************************/


#define MIN_CLIENTS 1024


typedef enum MAILBOX_CATEGORIES_e {
    CAT_UNDEFINED   = -1,
    CAT_FTP_CLIENTS =  2,
    CAT_DISPATCHERS =  3,
    CAT_FTP_MOVERS  =  4,
    CAT_VFS_CATALOG =  5
} MAILBOX_CATEGORIES_T;



void usage (void);

void   set_irc_channel (char * channel);
char * get_irc_channel (void);
void   set_irc_nick (char * nick);
char * get_irc_nick (void);
void   publish_irc_nick (void);
void   unpublish_irc_nick (void);
void   register_parent_irc_nick (void);
char * get_parent_irc_nick (void);

RC_PROC_TYPE daemonize(void);
int isKilled(void);
void setKilled(void);
time_t getBirthTime (void);
void setBirthTime (void);
int expBackOff (int prevBackOff);
RC_PROC_TYPE angelOrDaemon (void);
void assignProcType (RC_PROC_TYPE);
void setChildPid (pid_t);
pid_t getChildPid (void);

#endif /* MAIN_H */

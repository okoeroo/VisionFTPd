
#ifndef INTERNAL_HELPER_FUNCS_H
    #define INTERNAL_HELPER_FUNCS_H

#include "main.h"
#include "internal_codes.h"
/* #include "ssl-common.h" */
#include "irandom.h"


typedef enum {
    IRC_UNKNOWN,
    IRC_PING,
    IRC_PONG,
    IRC_PRIVMSG_CHANNEL,
    IRC_PRIVMSG_PRIVATE,
    IRC_315,
    IRC_352,
    IRC_404
} IRC_MSG_TYPE;


SSL_CTX * getCTX (void);
void      setCTX (SSL_CTX * val);
SSL *     getSSL (void);
void      setSSL (SSL * ssl);
int       getSocket (void);
void      setSocket (int val);
void      setOnline(void);
void      setOffline(void);
int       getOnline(void);
time_t    onlineSince(void);

char *    getHost (void);
void      setHost (char * val);
int       getPort (void);
void      setPort (int port);
char *    getRestrictedServiceDN (void);
void      setRestrictedServiceDN (char * val);

int       randomAverageDeviationCalc (int average, int deviation);
char *    makeUpPseudoRandomNickName (void);

IRC_MSG_TYPE messageType(char * msg);

int  fireUpConnectionWithRetry (void);
int  fireUpConnection (void);
void terminate_session (void);

#endif

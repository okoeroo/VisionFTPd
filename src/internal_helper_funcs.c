#include "irc_com.h"
#include "internal_helper_funcs.h"


static int          is_killed    = RC_NO;
static time_t       birthtime    = 0;
static RC_PROC_TYPE proc_type    = ANGEL;
static pid_t        child_pid    = 0;
static int          is_online    = RC_NO;
static time_t       online_since = 0;

static char *         the_channel     = NULL;
static char *         the_nick        = NULL;
static char *         the_parent_nick = NULL;

static SSL_CTX *      main_ctx        = NULL;
static SSL *          main_ssl        = NULL;
static int            main_socket     = -1;

static char *         main_host       = NULL;
static int            main_port       = -1;
static char *         restricted_dn   = NULL;


void setOnline(void)
{
    is_online = RC_YES;
    online_since = time(NULL);
}
void setOffline(void)
{
    is_online = RC_NO;
}
int getOnline(void)
{
    return is_online;
}
time_t onlineSince(void)
{
    return online_since;
}

SSL_CTX * getCTX (void)
{
    return main_ctx;
}
void setCTX (SSL_CTX * val)
{
    main_ctx = val;
}
SSL * getSSL (void)
{
    return main_ssl;
}
void setSSL (SSL * ssl)
{
    main_ssl = ssl;
}
int getSocket (void)
{
    return main_socket;
}
void setSocket (int val)
{
    main_socket = val;
}

char * getHost (void)
{
    return main_host;
}
void setHost (char * val)
{
    main_host = val;
}
int getPort (void)
{
    return main_port;
}
void setPort (int val)
{
    main_port = val;
}
char * getRestrictedServiceDN (void)
{
    return restricted_dn;
}
void setRestrictedServiceDN (char * val)
{
    restricted_dn = val;
}



void set_irc_channel (char * channel)
{
    the_channel = channel;
}
char * get_irc_channel (void)
{
    return the_channel;
}

void set_irc_nick (char * nick)
{
    the_nick = nick;
}
char * get_irc_nick (void)
{
    return the_nick;
}

void publish_irc_nick (void)
{
    setenv("PARENT_NICK_NAME", get_irc_nick(), 1);
}
void unpublish_irc_nick (void)
{
    unsetenv ("PARENT_NICK_NAME");
}
void register_parent_irc_nick (void)
{
    the_parent_nick = getenv("PARENT_NICK_NAME");
}
char * get_parent_irc_nick (void)
{
    return the_parent_nick;
}


void setBirthTime (void)
{
    birthtime = time (NULL);
}

time_t getBirthTime (void)
{
    return birthtime;
}

RC_PROC_TYPE angelOrDaemon (void)
{
    return proc_type;
}

void assignProcType (RC_PROC_TYPE t)
{
    if (t != ERROR)
        proc_type = t;
}

void setChildPid (pid_t c)
{
    child_pid = c;
}

pid_t getChildPid (void)
{
    return child_pid;
}


/* Calculate a random waiting time, that is deviated by 'deviation' time around a fix avaerage */
int randomAverageDeviationCalc (int average, int deviation)
{
    return (random() % (deviation * 2)) + (average - deviation);
}


IRC_MSG_TYPE messageType(char * msg)
{
    char   elements[3][40];
    char * ping_str = "PING :";
    int    i = 0;

    for (i = 0; i < 3; i++)
        bzero (elements[i], 40);

#ifdef DEBUG
#if 0
    scar_log (1, "%s: %s\n", __func__, msg);
#endif
#endif

    /* Is the message PING message? */
    if (strncmp (msg, ping_str, strlen(ping_str)) == 0)
    {
        return IRC_PING;
    }

    /* Is it a WHO result list aka 352? */
    if (sscanf (msg, ":%39s %39s %39s *s", 
                             elements[0], /* Typically the message origin (channel or user) */
                             elements[1],
                             elements[2]))
    {
#ifdef DEBUG
#if 0
        for (i = 0; i < 3; i++)
            scar_log (1, "%s: elements[%d]: %s\n", __func__, i, elements[i]);
#endif
#endif
        if ((strcasecmp(elements[0], "wunderbar.geenstijl.de") == 0) &&
            (strcasecmp(elements[1], "352") == 0))
        {
            return IRC_352;
        }
        else if ((strcasecmp(elements[0], "wunderbar.geenstijl.de") == 0) &&
                 (strcasecmp(elements[1], "315") == 0))
        {
            return IRC_315;
        }
        else if ((strcasecmp(elements[0], "wunderbar.geenstijl.de") == 0) &&
                 (strcasecmp(elements[1], "404") == 0))
        {
            return IRC_404;
        }
        else if ((strcasecmp(elements[0], "wunderbar.geenstijl.de") == 0) &&
                 (strcasecmp(elements[1], "PONG") == 0))
        {
            return IRC_PONG;
        }
        else if ((strcasecmp(elements[1], "PRIVMSG") == 0) &&
                 (strcasecmp(elements[2], get_irc_channel()) == 0))
        {
            return IRC_PRIVMSG_CHANNEL;
        }
        else if ((strcasecmp(elements[1], "PRIVMSG") == 0) &&
                 (strcasecmp(elements[2], get_irc_nick()) == 0))
        {
            return IRC_PRIVMSG_PRIVATE;
        }
    }

    /* I couldn't figure out what kind of message this was */
    return IRC_UNKNOWN;
}


int expBackOff (int prevBackOff)
{
    return prevBackOff * 2;
}


char * makeUpPseudoRandomNickName (void)
{
    int i = 0;
    char * pseudo_random_nickname = calloc (sizeof(char), (RFC2812_MAX_NICKNAME_LEN + 1));

    srandom(time(NULL) + getpid());

    if (pseudo_random_nickname)
    {
        pseudo_random_nickname[0] = random_chars();
        for (i = 1; i < 8; i++)
            pseudo_random_nickname[i] = random_alphanum();
        pseudo_random_nickname[8] = '_';
        pseudo_random_nickname[9] = angelOrDaemon() == ANGEL ? 'A' : 'D';
        pseudo_random_nickname[8] = '\0';
    }
    return pseudo_random_nickname;
}


/* Setup a TCP IPv4/IPv6 connection to the service and setup a full SSL handshake */
int fireUpConnection (void)
{
    SSL *     ssl                   = getSSL();
    int       s                     = getSocket();
    SSL_CTX * ctx                   = getCTX();
    char *    host                  = getHost();
    int       port                  = getPort();
    char *    restricted_service_dn = getRestrictedServiceDN();

    printf ("%s\n", __func__);

    /* Establish TCP connection to the service on IPv4 or IPv6 */
    if ((s = firstTCPSocketConnectingCorrectly(host, port)) < 0)
    {
#ifdef DEBUG
        scar_log (1, "%s: Failed to create and connect to %s on port %d\n", __func__, host, port);
#endif
        goto bailout;
    }
    setSocket (s);


    /* Instruct the SSL connection only to be accepted if the service shows the following subject DN */
    SSL_client_authz_service_dn (restricted_service_dn);

    /* Initiate SSL handshake */
    if (!(ssl = SSL_client_connect (s, ctx)))
    {
#ifdef DEBUG
        scar_log (1, "%s: Failed to establish secure connection\n", __func__);
#endif
        goto bailout;
    }

    /* Register SSL object */
    setSSL (ssl);

    /* Set status to Online */
    setOnline();

    /* Successful SSL connection setup */
    return RC_GOOD;

bailout:
    /* Shutdown SSL and TCP */
    terminate_session();
    return RC_BAD;
}



/*
 * Will try to fire up the connection.  On failure an exponential backoff mchanism, 
 * with a back off limit, will be used before retrying.
 * There is also an absolute limit set to a very long period of time so that the client will gracefully die. 
 */
int fireUpConnectionWithRetry (void)
{
    int rc = RC_BAD;
    int wait_seconds_retry = 1;
    int absolute_seconds_of_retry = 0;

    /*
     * Checking if the waiting time until a succesful connection could be established
     * has not exceeded a very long upper limit
     * TODO: try a traceroute after some kind of retries?
     */
    while (absolute_seconds_of_retry < ABSOLUTE_TIME_LIMIT_OF_CLIENT_RETRY_IN_SECONDS)
    {
        /* Clean up the connection, if any */
        terminate_session();
        setOffline();

        /* Try to fire up an SSL connection */
        if (RC_GOOD != (rc = fireUpConnection()))
        {
            /* Be assured that the exponential backoff doesn't exceed an undesired long time between the retries */
            if (wait_seconds_retry < SHORT_RETRY_LIMIT_IN_SECONDS)
            {
                /* Exponential backoff waiting time until a retry is performed */
                wait_seconds_retry = expBackOff (wait_seconds_retry);
            }
            else
            {
                /* Reduce the wait time with a random number that's lower then 10% of the SHORT_RETRY_LIMIT_IN_SECONDS limit */
                wait_seconds_retry -= random() % (SHORT_RETRY_LIMIT_IN_SECONDS / 10);
            }
#ifdef DEBUG
            scar_log (1, "%s: Next connection (retry) in %d seconds...\n", __func__, wait_seconds_retry);
#endif
            absolute_seconds_of_retry += wait_seconds_retry;
            sleep (wait_seconds_retry);
        }
        else
        {
            /* We have a connection, break out of the retry loop */
            break;
        }
    }

    return rc;
}



/* Terminate the connection to the service, by shutting down the SSL layer and socket layer */
void terminate_session (void)
{
    SSL * ssl  = getSSL();
    int   sock = getSocket();

#ifdef DEBUG
    scar_log (1, "%s: terminating SSL.\n", __func__);
#endif
    /* Shutdown active SSL layer */
    if (ssl)
    {
       SSL_connect_shutdown (ssl);
       SSL_free(ssl);
    }

#ifdef DEBUG
    scar_log (1, "%s: terminating socket.\n", __func__);
#endif
    /* Shutdown socket connection */
    if (sock >= 0)
    {
        shutdown (sock, SHUT_RD);
    }

#ifdef DEBUG
    scar_log (1, "%s: connection terminated.\n", __func__);
#endif

    setSSL(NULL);
    setSocket(-1);

    /* Setting offline status */
    setOffline();
}



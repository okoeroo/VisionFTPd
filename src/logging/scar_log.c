/*                                                                                                            
 * Copyright (c) Members of the EGEE Collaboration. 2004.
 * See http://eu-egee.org/partners/ for details on the copyright holders.
 * For license conditions see the license file or
 * http://eu-egee.org/license.html
 */

/*                                                                                                            
 * Copyright (c) 2001 EU DataGrid.                                                                             
 * For license conditions see http://www.eu-datagrid.org/license.html                                          
 *
 * Copyright (c) 2001, 2002 by 
 *     Martijn Steenbakkers <martijn@nikhef.nl>,
 *     David Groep <davidg@nikhef.nl>,
 *     NIKHEF Amsterdam, the Netherlands
 */

/*!
    \file   scar_log.c
    \brief  Logging routines for scar
    \author Martijn Steenbakkers for the EU DataGrid.
*/


/*****************************************************************************
                            Include header files
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <ctype.h>

#include "_scar_log.h"


/******************************************************************************
                          Module specific prototypes
******************************************************************************/
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0 /*!< default debugging level */
#endif /* DEBUG_LEVEL */

/******************************************************************************
                       Define module specific variables
******************************************************************************/
static FILE *  scar_logfp=NULL; /*!< stream associated with logfile \internal */
static int     logging_usrlog=0; /*!< flag to do user logging \internal */
static int     logging_syslog=0; /*!< flag to use syslog \internal */
static int     logging_errlog=0; /*!< flag to use syslog \internal */
static int     debug_level=0; /*!< debugging level \internal */
static char *  extra_logstr = NULL; /*!< string to be included in every log statement \internal */ 
static int     should_close_scar_logfp = 0; /*!< Flag to check if the log stream should be closed \internal */ 
static char *  log_line_prefix = "scar_log";

/******************************************************************************
Function:       scar_log_open()
Description:    Start logging
Parameters:
                path:    path of logfile
                fp:      file pointer to already opened file (or NULL)
                logtype: DO_USRLOG, DO_SYSLOG
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_open(
        char * path,
        FILE * fp,
        unsigned short logtype
        )
    \brief Start logging

    This function should only be used by the scar itself.
    It opens the logfile and tries to set the debugging level in the following order:
    -# Try if DEBUG_LEVEL > 0
    -# Try if environment variable scar_DEBUG_LEVEL is set and if it is an integer > 0
    -# Otherwise set debug_level = 0;

    \param path    path of logfile.
    \param fp      file pointer to already opened file (or NULL)
    \param logtype DO_USRLOG, DO_SYSLOG

    \retval 0 succes.
    \retval 1 failure.
    \internal
*/
int
scar_log_open(char * path, FILE * fp, unsigned short logtype )
{
    char * debug_env = NULL;
    char * logstr_env = NULL;

    if ((logtype & DO_ERRLOG) == DO_ERRLOG)
    {
        logging_errlog=1;
    }

    if ((logtype & DO_SYSLOG) == DO_SYSLOG)
    {
        logging_syslog=1;
#if 0
        /* Not yet applicable, wait for scar to become daemon */
        openlog("EDG scar", LOG_PID, LOG_AUTHPRIV);
#endif
    }
    if ((logtype & DO_USRLOG) == DO_USRLOG)
    {
        logging_usrlog=1;
        if (fp != NULL)
        {
            /* File already opened, should not be closed at the end */
            scar_logfp=fp;
            should_close_scar_logfp = 0;
        }
        else if (path != NULL)
        {
            /* Try to append to file */
            if ((scar_logfp = fopen(path, "a")) == NULL)
            {
                fprintf(stderr, "scar_log_open(): Cannot open logfile %s: %s\n", path, strerror(errno));
                if (logging_syslog)
                {
                    syslog(LOG_ERR, "scar_log_open(): Cannot open logfile %s\n", path);
                }
                return 1;
            }
            should_close_scar_logfp = 1;
        }
        else
        {
            fprintf(stderr, "scar_log_open(): Please specify either (open) file descriptor");
            fprintf(stderr, " or name of logfile\n");
            return 1;
        }
    }
    /*
     * Set the debugging level:
     *    1. Try if DEBUG_LEVEL > 0
     *    2. Try if scar_DEBUG_LEVEL is set and if it is an integer
     *    3. set debug_level = 0;
     */
    if ( (int)(DEBUG_LEVEL) > 0 )
    {
        debug_level = (int)(DEBUG_LEVEL);
    }
    else if ( (debug_env = getenv("SCAR_DEBUG_LEVEL")) )
    {
        /* convert into integer */
        size_t j = 0;

        for (j = 0; j < strlen(debug_env); j++)
        {
            if (!isdigit((debug_env)[j]))
            {
                fprintf(stderr,"scar_log_open(): found non-digit in environment variable in \"scar_DEBUG_LEVEL\" = %s\n", debug_env);
                return 1;
            }
        }
        debug_level = atoi(debug_env);
        if (debug_level < 0)
        {
            fprintf(stderr,"scar_log_open(): environment variable in \"scar_DEBUG_LEVEL\" should be >= 0\n");
            return 1;
        }
    }
    else
    {
        debug_level = 0;
    }

    if (debug_level > 0)
    {
        scar_log(0,"scar_log_open(): setting debugging level to %d\n", debug_level);
    }

    /*
     * Check if there is an extra log string
     * These environment variables are checked: JOB_REPOSITORY_ID and GATEKEEPER_JM_ID
     */
    if ( (logstr_env = getenv("SCAR_LOG_STRING")) != NULL )
    {
        extra_logstr = strdup(logstr_env);
    }

    return 0;
}

/******************************************************************************
Function:       scar_log()
Description:    Log information to file and or syslog
Parameters:
                prty:    syslog priority (if 0 don't syslog)
                fmt:     string format
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log(
        int prty,
        char * fmt,
        ...
        )
    \brief log information
    
    This function logs information for scar and its plugins.
    Syslog() is called with the specified priority. No syslog() is done if the
    priority is 0.

    \param prty syslog priority (if 0 don't syslog).
    \param fmt  string format
    \param ...  variable argument list

    \retval 0 succes.
    \retval 1 failure.
*/
int
scar_log(int prty, char * fmt, ...)
{
    va_list pvar;
    char    buf[MAX_LOG_BUFFER_SIZE];
    int     res;

    va_start(pvar, fmt);
    res=vsnprintf(buf,MAX_LOG_BUFFER_SIZE,fmt,pvar);
    va_end(pvar);
    if ( (res >= MAX_LOG_BUFFER_SIZE) || (res < 0) )
    {
        fprintf(stderr,"scar_log(): log string too long (> %d)\n",
                MAX_LOG_BUFFER_SIZE);
    }
    if (logging_errlog)
    {
        if (extra_logstr == NULL)
        {
            fprintf(stderr, "%s %d: %s", log_line_prefix, prty, buf);
        }
        else
        {
            fprintf(stderr, "%s %d: %s : %s", log_line_prefix, prty, extra_logstr, buf);
        }
        fflush(stderr);
    }
    if (logging_usrlog)
    {
        if (scar_logfp == NULL)
        {
            fprintf(stderr,"scar_log() error: cannot open file descriptor\n");
            return 1;
        }
        if (extra_logstr == NULL)
        {
            fprintf(scar_logfp,"%s %d: %s", log_line_prefix, prty, buf);
            printf("%s %d: %s", log_line_prefix, prty, buf);
        }
        else
        {
            fprintf(scar_logfp,"%s %d: %s : %s", log_line_prefix, prty, extra_logstr, buf);
            printf("%s %d: %s : %s", log_line_prefix, prty, extra_logstr, buf);
        }
        fflush(scar_logfp);
        fflush(stdout);
    }
    if (logging_syslog && prty)
    {
        syslog(prty, buf);
    }

    return 0;
}

/******************************************************************************
Function:       scar_log_a_string()
Description:    Log a string according to the passed format to file and or syslog
Parameters:
                prty:       syslog priority (if 0 don't syslog)
                fmt:        string format
                the_string: the string
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_a_string(
        int prty,
        char * fmt,
        char * the_string
        )
    \brief log information
    
    This function logs information for scar and its plugins.
    Syslog() is called with the specified priority. No syslog() is done if the
    priority is 0.

    \param prty         syslog priority (if 0 don't syslog).
    \param fmt          string format
    \param the_string   the string

    \retval 0 succes.
    \retval 1 failure.
*/
int
scar_log_a_string(int prty, char * fmt, char * the_string)
{
    char    buf[MAX_LOG_BUFFER_SIZE];
    int     res;

    res = snprintf(buf, MAX_LOG_BUFFER_SIZE, fmt, the_string);
    if ( (res >= MAX_LOG_BUFFER_SIZE) || (res < 0) )
    {
        fprintf(stderr,"scar_log_a_string(): log string too long (> %d)\n",
                MAX_LOG_BUFFER_SIZE);
    }

    if (logging_errlog)
    {
        if (extra_logstr == NULL)
        {
            fprintf(stderr, "scar %d: %s", prty, buf);
            printf("scar %d: %s", prty, buf);
        }
        else
        {
            fprintf(stderr, "scar %d: %s : %s", prty, extra_logstr, buf);
            printf("scar %d: %s : %s", prty, extra_logstr, buf);
        }
        fflush(stderr);
    }

    if (logging_usrlog)
    {
        if (scar_logfp == NULL)
        {
            fprintf(stderr,"scar_log() error: cannot open file descriptor\n");
            return 1;
        }
        if (extra_logstr == NULL)
        {
            fprintf(scar_logfp,"scar %d: %s", prty, buf);
            printf("scar %d: %s", prty, buf);
        }
        else
        {
            fprintf(scar_logfp,"scar %d: %s : %s", prty, extra_logstr, buf);
            printf("scar %d: %s : %s", prty, extra_logstr, buf);
        }
        fflush(scar_logfp);
    }
    if (logging_syslog && prty)
    {
        syslog(prty, buf);
    }

    return 0;
}

/******************************************************************************
Function:       scar_log_debug()
Description:    Print debugging information
Parameters:
                debug_lvl: debugging level
                fmt:       string format
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_debug(
        int debug_lvl,
        char * fmt,
        ...
        )
    \brief Print debugging information

    This function prints debugging information (using scar_log with priority 0)
    provided debug_lvl <= DEBUG_LEVEL (default is 0).

    \param debug_lvl debugging level
    \param fmt       string format
    \param ...       variable argument list

    \retval 0 succes.
    \retval 1 failure.
*/
int
scar_log_debug(int debug_lvl, char * fmt, ...)
{
    va_list pvar;
    char    buf[MAX_LOG_BUFFER_SIZE];
    int     res;

    va_start(pvar, fmt);
    res=vsnprintf(buf,MAX_LOG_BUFFER_SIZE,fmt,pvar);
    va_end(pvar);
    if ( (res >= MAX_LOG_BUFFER_SIZE) || (res < 0) )
    {
        fprintf(stderr,"scar_log(): log string too long (> %d)\n",
                MAX_LOG_BUFFER_SIZE);
    }
    if (debug_lvl <= debug_level)
    {
        scar_log(0,buf);
        return 0;
    }
    return 1;
}

/******************************************************************************
Function:       scar_log_a_string_debug()
Description:    Print debugging information
Parameters:
                debug_lvl:  debugging level
                fmt:        string format
                the_string: the string
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_a_string_debug(
        int debug_lvl,
        char * fmt,
        char * the_string
        )
    \brief Print debugging information

    This function prints debugging information (using scar_log with priority 0)
    provided debug_lvl <= DEBUG_LEVEL (default is 0).

    \param debug_lvl    debugging level
    \param fmt          string format
    \param the_string   the string

    \retval 0 succes.
    \retval 1 failure.
*/
int
scar_log_a_string_debug(int debug_lvl, char * fmt, char * the_string)
{
    if (debug_lvl <= debug_level)
    {
        scar_log_a_string(0, fmt, the_string);
        return 0;
    }
    return 1;
}

/******************************************************************************
Function:       scar_log_close()
Description:    Stop logging
Parameters:
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_close()
    \brief Stop logging.

    \internal
    This function should only be used by the scar itself.

    \retval 0 succes.
    \retval 1 failure.
    \internal
*/
int
scar_log_close()
{
    if (extra_logstr != NULL)
    {
        free(extra_logstr);
        extra_logstr = NULL;
    }

    if (should_close_scar_logfp)
    {
        fclose(scar_logfp);
        scar_logfp=NULL;
    }

    return 0;
}


/******************************************************************************
Function:       scar_log_time()
Description:    Log information to file and or syslog with a timestamp
Parameters:
                prty:    syslog priority (if 0 don't syslog)
                fmt:     string format
Returns:        0 succes
                1 failure
******************************************************************************/
/*!
    \fn scar_log_time(
        int prty,
        char * fmt,
        ...
        )
    \brief log information with timestamp
 
    This function logs information with a timestamp for scar and its plugins.
    Syslog() is called with the specified priority. No syslog() is done if the
    priority is 0.
 
    \param prty syslog priority (if 0 don't syslog).
    \param fmt  string format
    \param ...  variable argument list
 
    \retval 0 succes.
    \retval 1 failure.
*/
int
scar_log_time(int prty, char * fmt, ...)
{
    va_list     pvar;
    char        buf[MAX_LOG_BUFFER_SIZE];
    char *      datetime = NULL;
    char *      tmpbuf = NULL;
    int         res;
    time_t      mclock;
    struct tm * tmp = NULL;


    va_start(pvar, fmt);
    res=vsnprintf(buf,MAX_LOG_BUFFER_SIZE,fmt,pvar);
    va_end(pvar);
    if ( (res >= MAX_LOG_BUFFER_SIZE) || (res < 0) )
    {
        fprintf(stderr,"scar_log_time(): log string too long (> %d)\n",
                MAX_LOG_BUFFER_SIZE);
    }

    if (extra_logstr == NULL)
    {
        time(&mclock);
        /* tmp = localtime(&clock); */
        tmp = gmtime(&mclock);

        datetime = malloc(sizeof(char) * 20);

        res=snprintf(datetime, 20, "%04d-%02d-%02d.%02d:%02d:%02d",
               tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday,
               tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
        if ( (res >= 20) || (res < 0) )
        {
            fprintf(stderr,"scar_log_time(): date string too long (> %d)\n",
                    20);
        }

        tmpbuf = (char *) malloc ((strlen(datetime) + strlen(buf) + strlen(" : ")) * sizeof(char) + 1);
        strcpy(tmpbuf, datetime);
        strcat(tmpbuf, " : ");
        strcat(tmpbuf, buf);
    }
    else
    {
        tmpbuf = (char *) malloc ((strlen(extra_logstr) + strlen(buf) + strlen(" : ")) * sizeof(char) + 1);
        strcpy(tmpbuf, extra_logstr);
        strcat(tmpbuf, " : ");
        strcat(tmpbuf, buf);
    }

    if (logging_errlog)
    {
        fprintf(stderr, "scar %d: %s",prty,tmpbuf);
        fflush(stderr);
    }

    if (logging_usrlog)
    {
        if (scar_logfp == NULL)
        {
            fprintf(stderr,"scar_log_time() error: cannot open file descriptor\n");
            return 1;
        }
        fprintf(scar_logfp,"scar %d: %s",prty,tmpbuf);
        fflush(scar_logfp);
    }
    if (logging_syslog && prty)
    {
        syslog(prty, tmpbuf);
    }

    if (datetime != NULL) free(datetime);
    if (tmpbuf != NULL) free(tmpbuf);

    return 0;
}


/******************************************************************************
Function:       scas_log_set_time_indicator()
Description:    Resets the log indicator to the current time
                When set, the logstring will be prefixed by this string
Parameters:
Returns:        
                
******************************************************************************/
int scar_log_set_time_indicator (void)
{
    time_t      _time   = 0;
    struct tm * _time_s = NULL;
    char        _strf_fmt[MAX_TIME_STRING_SIZE];
    int         _strf_size = 0;
    
    _time      = time(NULL);
    _time_s    = localtime(&_time);
    _strf_size = strftime(_strf_fmt, MAX_TIME_STRING_SIZE, "%Y-%m-%d.%H:%M:%S%Z", _time_s);

    /* Free previous set extra_logstr */
    free(extra_logstr);
    
    /* Add locally to globally scope string */
    extra_logstr = malloc (sizeof (char) * _strf_size);
    strncpy (extra_logstr, _strf_fmt, _strf_size);

    return 0;
}



FILE * scar_get_log_file_pointer(void)
{
    return scar_logfp;
}


unsigned short scar_get_log_type (void)
{
    unsigned short flags = 0;
    if (logging_usrlog) flags += DO_USRLOG;
    if (logging_syslog) flags += DO_SYSLOG;
    if (logging_errlog) flags += DO_ERRLOG;

    return flags;
}

void scar_set_log_line_prefix (char * prefix)
{
    log_line_prefix = prefix;
}

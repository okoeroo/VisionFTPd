/*                                                                                                            
 *     Oscar Koeroo <okoeroo@nikhef.nl>,
 */

/*!
    \file   _scar_log.h
    \brief  Internal header file for scar logging routines.
    \author Oscar Koeroo for the EGEE project.
    \internal
*/

#ifndef _SCAR_LOG_H
#define _SCAR_LOG_H

/******************************************************************************
                             Include header files
******************************************************************************/
#include "scar_log.h"

/******************************************************************************
                               Define constants
******************************************************************************/

/******************************************************************************
 *                            Module definition
 *****************************************************************************/

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
extern int scar_log_open(
        char * path,
        FILE * fp,
        unsigned short logtype
);

/******************************************************************************
Function:       scar_log_close()
Description:    Stop logging
Parameters:
Returns:        0 succes
                1 failure
******************************************************************************/
extern int scar_log_close(void);

#endif /* _SCAR_LOG_H */


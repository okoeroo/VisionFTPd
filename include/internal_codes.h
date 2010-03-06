
#ifndef INTERNAL_CODES_H
    #define INTERNAL_CODES_H

#define EXIT_CODE_GOOD 0
#define EXIT_CODE_BAD  1

typedef enum
{
    RC_GOOD,
    RC_BAD,
    RC_YES,
    RC_NO,
    RC_CONTINUE,
    RC_SLEEP,
    RC_RECONNECT,
    RC_STOP
} RC_CODES;

typedef enum
{
    ERROR,
    ANGEL,
    DAEMON
} RC_PROC_TYPE;

#endif /* INTERNAL_CODES_H */

#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#include "main.h"

#include "unsigned_string.h"

#include "ftpd.h"

static char * ftp_service_banner = NULL;


/****************************************************************************************************

 ***********  ***********  **********
 *                 *       *         *               
 *                 *       *         *              
 *                 *       *         *              
 *******           *       **********                      
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        
 *                 *       *                        

****************************************************************************************************/


void set_ftp_service_banner (char * banner)
{
    ftp_service_banner = banner;
}

char * get_ftp_service_banner (void)
{
    return ftp_service_banner;
}



int handle_ftp_initialization (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    int ret = 0;
    char * banner = get_ftp_service_banner();

    /* Set initialize */
    ftp_state -> init = 1;

    ret = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "220 %s\r\n", banner ? banner : "");
    if (ret >= write_buffer_state -> buffer_size)
    {
        /* Buffer error */
        return NET_RC_DISCONNECT;
    }
    else
    {
        write_buffer_state -> num_bytes = ret;
        return NET_RC_MUST_WRITE;
    }
}

int handle_message_not_understood (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    int ret = 0;

    /* Send a WTF message */
    ret = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "500 %s doesn't understand command: %s\r\n", APP_NAME, read_buffer_state -> buffer);
    if (ret >= write_buffer_state -> buffer_size)
    {
        /* Buffer error */
        return NET_RC_DISCONNECT;
    }
    else
    {
        write_buffer_state -> num_bytes = ret;
        return NET_RC_MUST_WRITE;
    }
}


int handle_ftp_USER (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    unsigned char * user = malloc (128 * sizeof(unsigned char));

    if (sscanf ((char *) read_buffer_state -> buffer, "USER %127s*s", user) <= 0)
    {
        /* No match */
        free(user);
        return NET_RC_UNHANDLED;
    }
    else
    {
        ftp_state -> ftp_user = user;

        /* Announce anoymous log-in allowed */
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "331 Password required for %s\r\n", ftp_state -> ftp_user);
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_PASS (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    unsigned char * pass = malloc (128 * sizeof(unsigned char));

    if (sscanf ((char *) read_buffer_state -> buffer, "PASS %127s*s", pass) <= 0)
    {
        /* No match */
        free(pass);
        return NET_RC_UNHANDLED;
    }
    else
    {
        ftp_state -> ftp_passwd = pass;

        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "230 User logged in. Welcome to %s\r\n", ftp_service_banner);
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}


int handle_ftp_SYST (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "SYST";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "215 UNIX Type: L8 Version: %s %s\r\n", APP_NAME, APP_VERSION);
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_FEAT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "FEAT";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "211- Extensions supported:\r\n SIZE\r\n REST STREAM\r\n211 END\r\n");
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_PWD  (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "PWD";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Will need integration with the VFS */
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "257 \"/\" is current directory\r\n");
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_CWD  (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "CWD";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        if (ftp_state -> cwd == NULL)
            ftp_state -> cwd = malloc (4096 * sizeof(char)); /* 4096 is typically PATH_MAX */

        if (sscanf ((char *) read_buffer_state -> buffer, "CWD %4095s\r\n", ftp_state -> cwd) <= 0)
        {
            /* No match */
            return NET_RC_UNHANDLED;
        }

        /* Will need integration with the VFS */
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "250 \"/\" is current directory\r\n");
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_NOOP (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "NOOP";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Will need integration with the VFS */
        write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200 NOOP command successful\r\n");
        if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
        {
            /* Buffer overrun */
            return NET_RC_DISCONNECT;
        }
        else
        {
            return NET_RC_MUST_WRITE;
        }
    }
}

int handle_ftp_ABOR (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "ABOR";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Execute Client Requested Quit */
        return NET_RC_DISCONNECT;
    }
}
int handle_ftp_QUIT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "QUIT";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Execute Client Requested Quit */
        return NET_RC_DISCONNECT;
    }
}

int handle_ftp_TYPE (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger     = "TYPE";
    const char * cmd_trigger_bin = "TYPE I";
    const char * cmd_trigger_asc = "TYPE A";

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) == 0)
    {
        /* It's a TYPE command */
        if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger_bin, strlen (cmd_trigger_bin)) == 0)
        {
            ftp_state -> mode = BINARY;

            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200 OK Set to Binary mode\r\n");
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                return NET_RC_DISCONNECT;
            }
            else
            {
                return NET_RC_MUST_WRITE;
            }
        }
        else if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger_asc, strlen (cmd_trigger_asc)) == 0)
        {
            ftp_state -> mode = ASCII;

            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200 OK Set to ASCII mode\r\n");
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                return NET_RC_DISCONNECT;
            }
            else
            {
                return NET_RC_MUST_WRITE;
            }
        }
        else
        {
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "501 Unsupported TYPE parameter\r\n");
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                return NET_RC_DISCONNECT;
            }
            else
            {
                return NET_RC_MUST_WRITE;
            }
        }
    }
    else
    {
        return NET_RC_UNHANDLED;
    }
}

int handle_ftp_SIZE (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger = "SIZE";
    char *       path = NULL;

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        path = malloc(sizeof(char) * PATH_MAX);
        if (sscanf ((char *) read_buffer_state -> buffer, "SIZE %4095s*s", path) <= 0)
        {
            /* No match */
            free(path);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* TODO: Single transfer support for the moment! */
            if (ftp_state -> in_transfer)
            {
                free (ftp_state -> in_transfer -> path);
            }
            else
            {
                ftp_state -> in_transfer = malloc (sizeof (file_transfer_t));
            }
            ftp_state -> in_transfer -> path = strdup (path);
            ftp_state -> in_transfer -> size = 1337;

            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "213 %d\r\n", (int) ftp_state -> in_transfer -> size);
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                return NET_RC_DISCONNECT;
            }
            else
            {
                return NET_RC_MUST_WRITE;
            }
        }
    }
}


int parse_long_host_port (unsigned char * long_host_port, file_transfer_t ** ft)
{
    int i = 0;

    int  len = 0;
    int plen = 0;

    char * str       = long_host_port;
    char * str_short = NULL;

    unsigned char   address_family = 0;
    unsigned short  len_address    = 0;
    unsigned char * address        = NULL;
    unsigned short  len_port       = 0;
    unsigned short  port           = NULL;


    address_family = (char) strtol(str, &str, 10);
    str = &str[1];

    strtol(str, NULL, 10);

    for (i = 0; str[0] != '\0'; i++)
    {
        str = &str[i];
    }
    return 0;
}

int handle_ftp_LPRT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char * cmd_trigger       = "LPRT";
    unsigned char * long_host_port = NULL;
    file_transfer_t * ft           = NULL;

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        long_host_port = malloc (sizeof (char) * 256);
        if (sscanf ((char *) read_buffer_state -> buffer, 
                    "LPRT %255s*s", 
                    long_host_port) <= 0)
        {
            /* No match */
            free(long_host_port);
            return NET_RC_UNHANDLED;
        }
        else
        {
            parse_long_host_port (long_host_port, &ft);
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200\r\n");
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                free(long_host_port);
                return NET_RC_DISCONNECT;
            }
            else
            {
                free(long_host_port);
                return NET_RC_MUST_WRITE;
            }
        }
    }
}

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


ftp_data_channel_t * create_ftp_data_channel (int data_sock, unsigned char * data)
{
    ftp_data_channel_t * data_channel = NULL;

    /* Default initializing should be with '-1' and 'NULL' */


    data_channel = malloc (sizeof (ftp_data_channel_t));
    if (data_channel ==  NULL)
        return NULL;

    data_channel -> data_sock = data_sock;
    data_channel -> data      = data;
    data_channel -> data_buf  = init_buffer_state (4096);
    if (data_channel -> data_buf == NULL)
    {
        free(data_channel);
        data_channel = NULL;
    }

    return data_channel;
}



void * startFTPCallbckThread (void * arg)
{
    return NULL;
}


void set_ftp_service_banner (char * banner)
{
    ftp_service_banner = banner;
}

char * get_ftp_service_banner (void)
{
    return ftp_service_banner;
}


int move_bytes_commited_to_next_command (buffer_state_t * read_buffer_state)
{
    unsigned char * bufp  = NULL;
    unsigned char * bufpp = NULL;

    /* Sanity check */
    if (read_buffer_state -> bytes_commited > read_buffer_state -> buffer_size)
        return 1;

    bufp = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];
    bufpp = (unsigned char *) strstr((char *) bufp, CRLF);

    /* Bail out when no CRLF was found */
    if (bufpp == NULL)
        return 1;

    /* Move the pointer beyond the CRLF */
    bufpp = &bufpp[strlen(CRLF)];

    if (bufpp == NULL)
    {
        read_buffer_state -> bytes_commited = 0;
        return 0;
    }
    else
    {
        read_buffer_state -> bytes_commited += strlen((char *) bufp) - strlen((char *) bufpp);
        return 0;
    }
}

int EPRT_to_host_port (unsigned char * str, char ** host_ip, unsigned short * port)
{
    unsigned char * tmp_str = str;
    unsigned char * tmp_port = NULL;
    int             i = 0;
    /* EPRT |2|::1|59616| */

    if (!(str && (strlen((char *) str) > 4)))
        return 500;

    if (tmp_str[1] == '1') /* IPv4 */
    {
        scar_log (1, "%s: EPRT signaled IPv4: parsing: %s\n", __func__, tmp_str);
    }
    else if (tmp_str[1] == '2') /* IPv6 */
    {
        scar_log (1, "%s: EPRT signaled IPv6: parsing: %s\n", __func__, tmp_str);
    }

    tmp_str = &tmp_str[3];

    *host_ip = calloc (sizeof (char), 64);
    for (i = 0; (i < 64) && (tmp_str[0] != '\0') && (tmp_str[0] != '|'); i++)
    {
        (*host_ip)[i] = tmp_str;
        tmp_str = &tmp_str[1];
        scar_log (1, "%s: %s\n", __func__, *host_ip);
    }

    if (tmp_str[0] == '\0')
        return 500;

    tmp_str = &tmp_str[1];
    tmp_port = calloc (sizeof (char), 6);
    for (i = 0; (i < 6) && (tmp_str[0] != '\0') && (tmp_str[0] != '|'); i++)
    {
        tmp_port[i] = tmp_str;
        tmp_str = &tmp_str[1];
    }

    scar_log (1, "%s: EPRT host: %s\n", __func__, *host_ip);
    scar_log (1, "%s: EPRT port: %s\n", __func__, tmp_port);

/*
    *port = (p1 << 8) | p2;
    *host_ip = malloc (sizeof (char) * 16);
    snprintf (*host_ip, 15, "%d.%d.%d.%d", a1, a2, a3, a4);
*/
    return 200;
}

int PORT_to_host_port (unsigned char * str, char ** host_ip, unsigned short * port)
{
    unsigned int a1, a2, a3, a4, p1, p2;

    if (sscanf((char *)str, "%u,%u,%u,%u,%u,%u",
                &a1, &a2, &a3, &a4, &p1, &p2) != 6 ||
            a1 > 255 || a2 > 255 || a3 > 255 || a4 > 255 ||
            p1 > 255 || p2 > 255 || (a1|a2|a3|a4) == 0 ||
            (p1 | p2) == 0) {
        return 501;     
    }           
        /* htonl((a1 << 24) | (a2 << 16) | (a3 << 8) | a4); */

    *port = (p1 << 8) | p2;
    *host_ip = malloc (sizeof (char) * 16);
    snprintf (*host_ip, 15, "%d.%d.%d.%d", a1, a2, a3, a4);

    return 200;
}

int parse_long_host_port (unsigned char * long_host_port, file_transfer_t ** ft)
{
    int i = 0;


    unsigned char * str       = long_host_port;

    /* struct sockaddr_storage ss; */

    unsigned char   address_family = 0;
    unsigned short  len_address    = 0;
    unsigned char * address        = NULL;
    unsigned short  len_port       = 0;
    unsigned short  port           = 0;
    unsigned char   port_s[2];

    address_family = (unsigned char) strtol((char *) str, (char **) &str, 10);

    str = &str[1];
    if (str[0] == '\0')
        return 1;

    len_address = (unsigned short) strtol((char *)str, (char **)&str, 10);
    address = calloc (sizeof (unsigned char), len_address + 1);

    for (i = 0; i < len_address; i++)
    {
        str = &str[1];
        if (str[0] == '\0')
            return 1;

        address[i] = str[0];
    }

    str = &str[1];
    len_port = (unsigned char) strtol((char *) str, (char **) &str, 10);
    if (len_port != 2)
        return 1;

    bzero (port_s, 2);

    for (i = 0; i < 2; i++)
    {
        str = &str[1];
        if (str[0] == '\0')
            break;

        port_s[i] = str[0];
    }

    port = (unsigned short) *port_s;

    scar_log (1, "Ok, this is what I made of this: addr_fam %d, len_address %d, address %s, len_port %d, port %d\n", (int) address_family, len_address, address, len_port, port);

    return 0;
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
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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

        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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

        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
    char         fmt_str[256];
    vfs_t *      stated_node = NULL;
    vfs_t *      cwd_to_node = NULL;

    if (strncasecmp ((char *) read_buffer_state -> buffer, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        if (ftp_state -> cwd == NULL)
            ftp_state -> cwd = malloc ((PATH_MAX + 1) * sizeof(char)); /* 4096 is typically PATH_MAX */

        bzero (ftp_state -> cwd, PATH_MAX + 1);

        snprintf (fmt_str, sizeof(fmt_str) - 1, "CWD %%%ds*s", PATH_MAX);
        if (sscanf ((char *) read_buffer_state -> buffer, fmt_str, (char *) ftp_state -> cwd) <= 0)
        {
            /* No match */
            return NET_RC_UNHANDLED;
        }

        /* Search for VFS node to change directory to */
        stated_node = ftp_state -> vfs_root;

        /* Move tmp VFS node to that what is given in STAT call */
        cwd_to_node = VFS_traverse_and_fetch_vfs_node_by_path (stated_node, (char *) ftp_state -> cwd);

        if (cwd_to_node != NULL)
        {
            /* Update the current working directory */
            ftp_state -> vfs_cwd = cwd_to_node;

            /* Will need integration with the VFS */
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "250 Directory successfully changed\r\n");
        }
        else
        {
            /* Directory does not exist */
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "500 directory does not exist\r\n");
        }

        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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
    const char *     cmd_trigger     = "TYPE";
    const char *     cmd_trigger_bin = "TYPE I";
    const char *     cmd_trigger_asc = "TYPE A";
    unsigned char *  bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) == 0)
    {
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

        /* It's a TYPE command */
        if (strncasecmp ((char *) bufp, cmd_trigger_bin, strlen (cmd_trigger_bin)) == 0)
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
        else if (strncasecmp ((char *) bufp, cmd_trigger_asc, strlen (cmd_trigger_asc)) == 0)
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
    const char *        cmd_trigger     = "SIZE";
    char *              path            = NULL;
    unsigned char *     bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        path = malloc(sizeof(char) * PATH_MAX);
        if (sscanf ((char *) bufp, "SIZE %4095s*s", path) <= 0)
        {
            /* No match */
            free(path);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Move commited bytes to next command in the buffer (if there) */
            move_bytes_commited_to_next_command (read_buffer_state);

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


int handle_ftp_LPRT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char *        cmd_trigger     = "LPRT";
    unsigned char *     long_host_port  = NULL;
    file_transfer_t *   ft              = NULL;
    unsigned char *     bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        long_host_port = malloc (sizeof (char) * 256);
        if (sscanf ((char *) bufp,
                    "LPRT %255s*s", 
                    long_host_port) <= 0)
        {
            /* No match */
            free(long_host_port);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Move commited bytes to next command in the buffer (if there) */
            move_bytes_commited_to_next_command (read_buffer_state);

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




int handle_ftp_EPRT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char *        cmd_trigger     = "EPRT";
    unsigned char *     host_port       = NULL;
    /* file_transfer_t *   ft              = NULL; */
    unsigned char *     bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];

    char *            host              = NULL;
    unsigned short    port              = 0;
    /*
    struct sockaddr * addr     = NULL;
    socklen_t         addr_len = 0;
    */


    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        host_port = malloc (sizeof (char) * 256);
        if (sscanf ((char *) bufp,
                    "EPRT %255s*s", 
                    host_port) <= 0)
        {
            /* No match */
            free(host_port);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Parse EPRT info */
            EPRT_to_host_port (host_port, &host, &port);

            /* Move commited bytes to next command in the buffer (if there) */
            move_bytes_commited_to_next_command (read_buffer_state);


            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200\r\n");
            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                free(host_port);
                return NET_RC_DISCONNECT;
            }
            else
            {
                free(host_port);
                return NET_RC_MUST_WRITE;
            }
        }
    }
}



int handle_ftp_PORT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char *      cmd_trigger       = "PORT";
    unsigned char *   short_host_port   = NULL;
    unsigned char *   bufp              = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];
    char *            host              = NULL;
    unsigned short    port              = 0;
    int               s                 = -1;

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        short_host_port = malloc (sizeof (char) * 256);
        if (sscanf ((char *) bufp,
                    "PORT %255s*s", 
                    short_host_port) <= 0)
        {
            /* No match */
            free(short_host_port);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Move commited bytes to next command in the buffer (if there) */
            move_bytes_commited_to_next_command (read_buffer_state);

            PORT_to_host_port (short_host_port, &host, &port);
            if (host != NULL)
            {
                scar_log (5, "%s: Got PORT information: %s:%d\n", __func__, host, port);
                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "200 PORT Command succesful\r\n");


                /* Fire off a connection back to the Client on the given host and port info */
                s = firstTCPSocketConnectingCorrectly (host, port);
                if (s < 0)
                {
                    /* Failed to open data port */
                }
                else
                {
                    ftp_state -> data_channel = create_ftp_data_channel(s, NULL);
                    if (ftp_state -> data_channel == NULL)
                    {
                        /* Out of memory */
                        scar_log (1, "%s: Error: Out of memory when creating Data Channel Object.\n", __func__);
                        close (s);
                        return NET_RC_MUST_WRITE;
                    }
                    else
                    {
                        scar_log (5, "%s: opened client socket to client. fd is %d\n", __func__, ftp_state -> data_channel -> data_sock);


                        /* Signal FTP data transfer ready */
                        /* write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "%s150 Ready for transfer\r\n", (char *) write_buffer_state -> buffer); */
                    }
                }
            }
            else
            {
                scar_log (2, "%s: Parse error in PORT information.\n", __func__);
                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "501 Error in IP Address or Port number in PORT message\r\n");
            }

            /* parse_short_host_port (short_host_port, &ft); */

            if (write_buffer_state -> num_bytes >= write_buffer_state -> buffer_size)
            {
                /* Buffer overrun */
                free(short_host_port);
                return NET_RC_DISCONNECT;
            }
            else
            {
                free(short_host_port);
                return NET_RC_MUST_WRITE;
            }
        }
    }
}


int handle_ftp_STAT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char *        cmd_trigger     = "STAT";
    unsigned char *     bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];
    char *              output          = NULL;
    vfs_t *             stated_node     = NULL;
    unsigned char *     stat_info       = NULL;
    char                fmt_str[256];

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Handle stat */
        if (!ftp_state)
        {
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "500 Unrecoverable error!\r\n");
        }

        stat_info = malloc (sizeof (unsigned char) * PATH_MAX);

        /* Building a dynamic format string, based on the PATH_MAX information */
        snprintf (fmt_str, sizeof(fmt_str) - 1, "STAT %%%ds*s", PATH_MAX);
        if (sscanf ((char *) read_buffer_state -> buffer, fmt_str, (char *) stat_info) <= 0)
        {
            /* No match */
            free(stat_info);
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Search vfs_t * from STAT <path> */
            stated_node = ftp_state -> vfs_root;

            /* Move tmp VFS node to that what is given in STAT call */
            stated_node = VFS_traverse_and_fetch_vfs_node_by_path (stated_node, (char *) stat_info);

            /* Not needed any more */
            free(stat_info);

            /* Fetch output based on path */
            if ((output = VFS_list_by_full_path (stated_node)))
            {
                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "%s", output);

                /* Must free output */
                free(output);
            }
            else
            {
                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "550 Directory is empty.\r\n");
            }
        }
        

        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

        /* write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "211 Server status is OK.\r\n"); */
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
int handle_ftp_LIST (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state)
{
    const char *        cmd_trigger     = "LIST";
    unsigned char *     bufp            = &(read_buffer_state -> buffer)[read_buffer_state -> bytes_commited];
    char                fmt_str[256];
    char *              output          = NULL;
    char *              listed_info     = NULL;
    vfs_t *             listed_node     = NULL;

    if (strncasecmp ((char *) bufp, cmd_trigger, strlen (cmd_trigger)) != 0)
    {
        /* No match */
        return NET_RC_UNHANDLED;
    }
    else
    {
        /* Handle stat */
        if (!ftp_state)
        {
            write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "500 Unrecoverable error!\r\n");
        }

        listed_info = malloc (sizeof (unsigned char) * PATH_MAX);

        /* Building a dynamic format string, based on the PATH_MAX information */
        if (strncmp (read_buffer_state -> buffer, cmd_trigger, strlen(cmd_trigger) != 0))
        {
            return NET_RC_UNHANDLED;
        }
        else
        {
            /* Fetch output based on path */
            if ((output = VFS_list_by_full_path (ftp_state -> vfs_cwd)))
            {
                /* Open data port to send bytes */
                /* Start Data thread */
                if ((ftp_state -> data_channel) && (ftp_state -> data_channel -> data_sock >= 0))
                {
                    /* HACK */
                    write (ftp_state -> data_channel -> data_sock, output, strlen(output));
                    close (ftp_state -> data_channel -> data_sock);
                    ftp_state -> data_channel -> data_sock = -1;
                }

                /* Must free output */
                free(output);

                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "150 Opening ASCII mode data connection for /bin/ls\r\n");
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
                write_buffer_state -> num_bytes = snprintf ((char *) write_buffer_state -> buffer, write_buffer_state -> buffer_size, "550 Directory is empty.\r\n");
            }
        }
        /* Move commited bytes to next command in the buffer (if there) */
        move_bytes_commited_to_next_command (read_buffer_state);

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

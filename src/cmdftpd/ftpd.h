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

#include "main.h"
#include "net_threader.h"


#ifndef FTPD_H
    #define FTPD_H


#define CRLF "\r\n"


typedef enum ftp_mode_e {
    ASCII,
    BINARY
} ftp_mode_t;


/* Tmp - might be overruled by VFS */
typedef struct file_transfer_s {
    size_t                    size;
    char *                    path;
    unsigned char             data_address_family;
    char *                    data_address;
    int                       data_port;
    struct file_transfer_s *  next;
} file_transfer_t;

typedef struct ftp_state_s {
    int init;
    ftp_mode_t       mode;
    unsigned char * ftp_user;
    unsigned char * ftp_passwd;
    unsigned char * cwd;

    file_transfer_t * in_transfer;

} ftp_state_t;



char * get_ftp_service_banner (void);
void set_ftp_service_banner (char *);

int move_bytes_commited_to_next_command (buffer_state_t * read_buffer_state);
int parse_long_host_port (unsigned char * long_host_port, file_transfer_t ** ft);
int parse_short_host_port (unsigned char * short_host_port, file_transfer_t ** ft);

int handle_ftp_initialization (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_message_not_understood (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);

int handle_ftp_USER (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_PASS (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_SYST (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_FEAT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_NOOP (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_PWD  (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_CWD  (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_ABOR (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_QUIT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_TYPE (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_SIZE (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_LPRT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);
int handle_ftp_PORT (ftp_state_t * ftp_state, buffer_state_t * read_buffer_state, buffer_state_t * write_buffer_state);



#endif /* FTPD_H */

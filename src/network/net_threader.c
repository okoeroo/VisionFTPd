#include "net_threader.h"
/* #include "ftpd.h" */


buffer_state_t * init_buffer_state (size_t buffer_size)
{
    buffer_state_t * buf = NULL;

    if ((buf = calloc (1, sizeof (buffer_state_t))) == NULL)
    {
        return NULL;
    }

    buf -> buffer = calloc (buffer_size, sizeof (unsigned char));
    if (buf -> buffer == NULL)
    {
        free (buf);
        return NULL;
    }

    buf -> buffer_size    = buffer_size;
    buf -> num_bytes      = 0;
    buf -> offset         = 0;
    buf -> bytes_commited = 0;
    buf -> state          = 0;

    return buf;
}


int free_buffer_state (buffer_state_t ** buf)
{
    (*buf) -> buffer_size    = 0;
    (*buf) -> num_bytes      = 0;
    (*buf) -> offset         = 0;
    (*buf) -> bytes_commited = 0;
    (*buf) -> state          = 0;

    free((*buf) -> buffer);
    (*buf) -> buffer = NULL;
    free((*buf));
    (*buf) = NULL;

    return 0;
}


void * threadDataChannel (void * arg)
{
    

}


void * threadingDaemonClientHandler (void * arg)
{
    net_thread_pool_t *     net_thread_pool_node = *(net_thread_pool_t **)arg;
    int                     client_socket        = net_thread_pool_node -> net_thread_parameters.client_fd;
    fd_set                  fdset;
    struct timeval          timeout;
    int                     n  = 0;
    int                     rc = 0;
    int                     net_thread_state     = NET_RC_CONNECTED;
    void *                  read_return_state    = NULL;
    buffer_state_t *        read_buffer_state    = NULL;
    buffer_state_t *        write_buffer_state   = NULL;
    size_t                  read_offset_commited = 0;

    /* Allocate buffers - for writing */
    write_buffer_state = init_buffer_state (net_thread_pool_node -> net_thread_parameters.write_byte_size);
    if (write_buffer_state == NULL)
    {
        scar_log (1, "%s: Out of memory! - Disconnecting client from \"%s\"",
                        __func__, 
                        net_thread_pool_node -> net_thread_parameters.hostname);
        goto net_disconnect;
    }
    /* Allocate buffers - for reading */
    read_buffer_state = init_buffer_state (net_thread_pool_node -> net_thread_parameters.read_byte_size);
    if (read_buffer_state == NULL)
    {
        scar_log (1, "%s: Out of memory! - Disconnecting client from \"%s\"",
                        __func__, 
                        net_thread_pool_node -> net_thread_parameters.hostname);
        goto net_disconnect;
    }

    /* Initiate state handle */
    if (net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_func != 0)
    {
        (* net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_func)(&read_return_state, net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_arg);
        /* Debug */
        /* scar_log (1, "--------------- foo! %s\n", ((ftp_state_t *) read_return_state) -> vfs_root -> name); */
    }

    scar_log (1, "Acceptence of the client connection was Perfect! - %d - awaiting info\n", client_socket); 
    while (1)
    {
        FD_ZERO(&fdset);
        FD_SET(client_socket, &fdset);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        n = select(client_socket + 1, &fdset, (fd_set *) 0, &fdset, &timeout);
        if (n < 0 && errno != EINTR)
        {
            scar_log (1, "%s: Failure in select() for client file descriptor %d used by remote host %s\n", 
                        __func__, 
                        client_socket, 
                        net_thread_pool_node -> net_thread_parameters.hostname);
            goto net_disconnect;
        }
        else if (n > 0)
        {
            /* Got something to read */
            /* n_bytes = read (client_socket, buffer, net_thread_pool_node -> net_thread_parameters.read_byte_size); */
            bzero (read_buffer_state -> buffer, read_buffer_state -> buffer_size);
            read_buffer_state -> num_bytes = read (client_socket, read_buffer_state -> buffer, read_buffer_state -> buffer_size);
            read_buffer_state -> bytes_commited = 0;

            if (read_buffer_state -> num_bytes <= 0)
            {
                /* Maybe not... */
                scar_log (1, "Connection closed by host \"%s\"\n", net_thread_pool_node -> net_thread_parameters.hostname);
                goto net_disconnect;
            }
            else
            {
                /* I can haz data! */

                /* If nothing is registered, then report the protocol info through the log facility */
                if (net_thread_pool_node -> net_thread_parameters.net_thread_active_io_func == NULL)
                {
                    scar_log (1, "fd:%00d:%010d bytes: %s\n", client_socket, read_buffer_state -> num_bytes, read_buffer_state -> buffer);
                    if (u_strnstr (read_buffer_state -> buffer, (unsigned char *) "bye", strlen("bye")) != NULL)
                    {
                        scar_log (1, "Connection closed by host \"%s\" due to nice wave good bye\n", net_thread_pool_node -> net_thread_parameters.hostname);
                        goto net_disconnect;
                    }
                }
                else
                {
                    /* Record placement of offset until where the input buffer was handled
                     *
                     * If this is changed, then the message has used this feature to indicate multi-message parsing support 
                     * Setting it to 0 means that everything is handled. Leaving it as is will indicate the same, but tells that it was not used.
                     * Changing the bytes_commited will tell where the message parsing left off and needs to require re-entry.
                     * If bytes_commited is equal or exceeds the num_bytes, then the process stops and the message is concidered to be done
                     */
                    do
                    {
                        /* record currently commited bytes */
                        read_offset_commited = read_buffer_state -> bytes_commited;

                        /* Fire up read function */
                        net_thread_state = (* net_thread_pool_node -> net_thread_parameters.net_thread_active_io_func)(read_buffer_state, write_buffer_state, &read_return_state);
                        if (net_thread_state == NET_RC_DISCONNECT)
                        {
                            scar_log (1, "Connection with host \"%s\" closed.\n", net_thread_pool_node -> net_thread_parameters.hostname);
                            goto net_disconnect;
                        }
                        else if (net_thread_state == NET_RC_MUST_WRITE)
                        {
                            /* Writing - write until amount of commited bytes is equal to the number of bytes in the buffer */
                            while (write_buffer_state -> bytes_commited < write_buffer_state -> num_bytes)
                            {
                                scar_log (1, "%d: >> %s", client_socket, write_buffer_state -> buffer);
                                rc = write (client_socket, write_buffer_state -> buffer, write_buffer_state -> num_bytes);
                                if (rc < 0)
                                {
                                    /* Something is wrong */
                                    scar_log (1, "Error in write: \"%s\". Connection with host \"%s\" closed.\n", strerror(errno), net_thread_pool_node -> net_thread_parameters.hostname);
                                    goto net_disconnect;
                                }
                                else
                                {
                                    /* Write was ok, register amount of commited Bytes */
                                    write_buffer_state -> bytes_commited += rc;
                                }
                            }
                            /* Done writing */
                            write_buffer_state -> bytes_commited = 0;
                            bzero (write_buffer_state -> buffer, write_buffer_state -> buffer_size);
                            write_buffer_state -> num_bytes = 0;
                        }
                    }
                    /* Check if the reparsing feature has been used and if the conditions are met based on the bytes_commited */
                    while ((read_offset_commited != read_buffer_state -> bytes_commited) && 
                           (read_buffer_state -> bytes_commited < read_buffer_state -> num_bytes) &&
                           (read_buffer_state -> bytes_commited != 0));
                }
            }
        }
        else
        {
            /* No activity - idle handling */
            if (net_thread_pool_node -> net_thread_parameters.net_thread_idle_io_func != NULL)
            {
                /* Fire up write function */
                net_thread_state = (* net_thread_pool_node -> net_thread_parameters.net_thread_idle_io_func)(write_buffer_state, &read_return_state);
                if (net_thread_state == NET_RC_DISCONNECT)
                {
                    scar_log (1, "Connection with host \"%s\" closed.\n", net_thread_pool_node -> net_thread_parameters.hostname);
                    goto net_disconnect;
                }
                else if (net_thread_state == NET_RC_MUST_WRITE)
                {
                    /* Writing - write until amount of commited bytes is equal to the number of bytes in the buffer */
                    while (write_buffer_state -> bytes_commited < write_buffer_state -> num_bytes)
                    {
                        scar_log (1, "%d: >> %s", client_socket, write_buffer_state -> buffer);
                        rc = write (client_socket, write_buffer_state -> buffer, write_buffer_state -> num_bytes);
                        if (rc < 0)
                        {
                            /* Something is wrong */
                            scar_log (1, "Error in write: \"%s\". Connection with host \"%s\" closed.\n", strerror(errno), net_thread_pool_node -> net_thread_parameters.hostname);
                            goto net_disconnect;
                        }
                        else
                        {
                            /* Write was ok, register amount of commited Bytes */
                            write_buffer_state -> bytes_commited += rc;
                        }
                    }
                    /* Done writing */
                    write_buffer_state -> bytes_commited = 0;
                    bzero (write_buffer_state -> buffer, write_buffer_state -> buffer_size);
                    write_buffer_state -> num_bytes = 0;
                }
            }
        }
    }

net_disconnect:
    scar_log (1, "Shutting down %d...\n", client_socket);
    shutdown (client_socket, SHUT_RDWR);
    scar_log (1, "Closing %d...\n", client_socket);
    close(client_socket);
    scar_log (1, "Socket is done.\n", client_socket);

    net_thread_pool_node -> net_thread_parameters.client_fd = -1;

    free_buffer_state (&read_buffer_state);
    free_buffer_state (&write_buffer_state);

    if (net_thread_pool_node -> net_thread_parameters.net_thread_state_liberator_func != 0)
    {
         (* net_thread_pool_node -> net_thread_parameters.net_thread_state_liberator_func)(&read_return_state);
    }

    pthread_exit(NULL);
}




int threadingDaemonStart (const int listening_port, 
                          const int max_clients, 
                          int read_chunk_size,
                          int write_chunk_size,
                          int (* net_thread_active_io_func)(buffer_state_t *, buffer_state_t *, void **),
                          int (* net_thread_idle_io_func)(buffer_state_t *, void **),
                          int (* net_thread_state_initiator_func)(void **, void *),
                          void * net_thread_state_initiator_arg,
                          int (* net_thread_state_liberator_func)(void **))
{
    int                         active_thread_counter = 0;
    int                         master_socket         = -1;
    int                         connection_fd         = -1;
    pthread_attr_t              attr;
    size_t                      stacksize             = 0;
    char *                      remote_host           = NULL;
    int                         rc                    = 0;
    net_thread_pool_t *         net_thread_pool       = NULL;
    net_thread_pool_t *         net_thread_pool_node  = NULL;
    net_thread_pool_t *         net_thread_looper     = NULL;


    pthread_attr_init(&attr);
    pthread_attr_getstacksize (&attr, &stacksize);
    printf("Default stack size = %li\n", stacksize);
    stacksize = sizeof(double)*1000000+1000000;

    printf("Amount of stack needed per thread = %li\n",stacksize);
    pthread_attr_setstacksize (&attr, stacksize); 

    /* Create an IPv4/IPv6 service socket for the commander */
    master_socket = createAndSetUpATCPServerSocket (listening_port, max_clients);
    if (master_socket < 0)
    {
        return -1;
    }

    scar_log (1, "Listening on port %d\n", listening_port);


    /* Connection handler */
    while (1)
    {
        connection_fd = net_accept_new_client_socket (master_socket, &remote_host);
        if (connection_fd < 0)
        {
            scar_log (1, "%S: Error: failure in client accept.\n");
            continue;
        }

        net_thread_pool_node = malloc (sizeof (net_thread_pool_t));
        net_thread_pool_node -> next                                                      = NULL;
        net_thread_pool_node -> net_thread_parameters.client_fd                           = connection_fd;
        if (read_chunk_size <= 0)
            net_thread_pool_node -> net_thread_parameters.read_byte_size                  = 1024;
        else
            net_thread_pool_node -> net_thread_parameters.read_byte_size                  = read_chunk_size;
        if (write_chunk_size <= 0)
            net_thread_pool_node -> net_thread_parameters.write_byte_size                 = 1024;
        else
            net_thread_pool_node -> net_thread_parameters.write_byte_size                 = write_chunk_size;
        net_thread_pool_node -> net_thread_parameters.hostname                            = remote_host;
        net_thread_pool_node -> net_thread_parameters.net_thread_active_io_func           = net_thread_active_io_func;
        net_thread_pool_node -> net_thread_parameters.net_thread_idle_io_func             = net_thread_idle_io_func;
        net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_func     = net_thread_state_initiator_func;
        net_thread_pool_node -> net_thread_parameters.net_thread_state_initiator_arg      = net_thread_state_initiator_arg;
        net_thread_pool_node -> net_thread_parameters.net_thread_state_liberator_func     = net_thread_state_liberator_func; 

        scar_log (1, "Got connection from: %s\n", remote_host);

        /* start client thread */
        rc = pthread_create(&(net_thread_pool_node -> threadid), &attr, threadingDaemonClientHandler, (void*)(&net_thread_pool_node));
        if (rc != 0)
        {
            scar_log (1, "%s: Failed to spawn thread. To bad for this client.\n", __func__);
            /* Liberate the memory and close the file descriptor */
            liberate_net_thread_pool_node (net_thread_pool_node, 1);
        }


        /* All is well, client network thread launched  - register the Thread information in the pool */
        if (net_thread_pool == NULL)
            net_thread_pool = net_thread_pool_node;
        else
        {
            net_thread_looper = net_thread_pool;
            while (net_thread_looper -> next != NULL)
            {
                net_thread_looper = net_thread_looper -> next;
            }
            net_thread_looper -> next = net_thread_pool_node;
        }

        /* Count active threads */
        active_thread_counter++;
    }

    /* Shutting down master socket */
    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);
    master_socket = -1;

    pthread_exit(NULL);
    return 0;
}


int liberate_net_thread_pool_node (net_thread_pool_t * net_thread_pool_node, int close_the_fd)
{
    pthread_kill (net_thread_pool_node -> threadid, 0);

    if (close_the_fd)
    {
        if (net_thread_pool_node -> net_thread_parameters.client_fd >= 0)
        {
            shutdown(net_thread_pool_node -> net_thread_parameters.client_fd, SHUT_RDWR);
            close(net_thread_pool_node -> net_thread_parameters.client_fd);
            net_thread_pool_node -> net_thread_parameters.client_fd = -1;
        }
    }

    free(net_thread_pool_node -> net_thread_parameters.hostname);
    net_thread_pool_node -> net_thread_parameters.hostname                            = NULL;
    net_thread_pool_node -> net_thread_parameters.read_byte_size                      = 0;
    net_thread_pool_node -> net_thread_parameters.write_byte_size                     = 0;
    net_thread_pool_node -> net_thread_parameters.net_thread_active_io_func           = NULL;
    net_thread_pool_node -> net_thread_parameters.net_thread_idle_io_func             = NULL;

    free(net_thread_pool_node);

    return 0;
}

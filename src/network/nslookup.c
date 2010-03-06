/* vi: set sw=4 ts=4: */
/*
 * Mini nslookup implementation for busybox
 *
 * Copyright (C) 1999,2000 by Lineo, inc. and John Beppu
 * Copyright (C) 1999,2000,2001 by John Beppu <beppu@codepoet.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <arpa/inet.h>

/*
 |  I'm only implementing non-interactive mode;
 |  I totally forgot nslookup even had an interactive mode.
 |
 |  [ TODO ]
 |  + find out how to use non-default name servers
 */

/* only works for IPv4 */
static int addr_fprint(char *addr)
{
        u_int8_t split[4];
        u_int32_t ip;
        u_int32_t *x = (u_int32_t *) addr;

        ip = ntohl(*x);
        split[0] = (ip & 0xff000000) >> 24;
        split[1] = (ip & 0x00ff0000) >> 16;
        split[2] = (ip & 0x0000ff00) >> 8;
        split[3] = (ip & 0x000000ff);
        printf("%d.%d.%d.%d", split[0], split[1], split[2], split[3]);
        return 0;
}

/* takes the NULL-terminated array h_addr_list, and
 * prints its contents appropriately
 */
static int addr_list_fprint(char **h_addr_list)
{
        int i, j;
        char *addr_string = (h_addr_list[1])
                ? "Addresses: " : "Address:   ";

        printf("%s ", addr_string);
        for (i = 0, j = 0; h_addr_list[i]; i++, j++) {
                addr_fprint(h_addr_list[i]);

                /* real nslookup does this */
                if (j == 4) {
                        if (h_addr_list[i + 1]) {
                                printf("\n          ");
                        }
                        j = 0;
                } else {
                        if (h_addr_list[i + 1]) {
                                printf(", ");
                        }
                }

        }
        printf("\n");
        return 0;
}

/* print the results as nslookup would */
static struct hostent *hostent_fprint(struct hostent *host, int is_server)
{
        char *format;
        if (is_server) {
                format = "Server:     ";
        } else {
                format = "Name:       ";
        }
        if (host) {
                printf("%s%s\n", format, host->h_name);
                addr_list_fprint(host->h_addr_list);
        } else {
                printf("*** Unknown host\n");
        }
        return host;
}

/* changes a c-string matching the perl regex \d+\.\d+\.\d+\.\d+
 * into a u_int32_t
 */
static u_int32_t str_to_addr(const char *addr)
{
        u_int32_t split[4];
        u_int32_t ip;

        sscanf(addr, "%d.%d.%d.%d",
                   &split[0], &split[1], &split[2], &split[3]);

        /* assuming sscanf worked */
        ip = (split[0] << 24) |
                (split[1] << 16) | (split[2] << 8) | (split[3]);

        return htonl(ip);
}

/* gethostbyaddr wrapper */
static struct hostent *gethostbyaddr_wrapper(const char *address)
{
        struct in_addr addr;
        struct in6_addr addr6;
        int    rc = 0;

        /* addr.s_addr = str_to_addr(address); */
        printf ("Trying IPv6 address: %s\n", address);
        if ((rc = inet_pton (AF_INET6, address, &(addr6.s6_addr))) > 0)
        {
            return gethostbyaddr((char *) &addr6, 16, AF_INET6);       /* IPv4 only for now */
        }

        printf ("Trying IPv4 address: %s\n", address);
        if ((rc = inet_pton (AF_INET, address, &(addr.s_addr))) > 0)
        {
            return gethostbyaddr((char *) &addr, 4, AF_INET);       /* IPv4 only for now */
        }

        return NULL;
}

/* lookup the default nameserver and display it */
static inline void server_print(void)
{
        struct sockaddr_in def = _res.nsaddr_list[0];
        char *ip = inet_ntoa(def.sin_addr);

        hostent_fprint(gethostbyaddr_wrapper(ip), 1);
        printf("\n");
}

/* naive function to check whether char *s is an ip address */
static int is_ip_address(const char *s)
{
        while (*s) {
                if ((isdigit(*s)) || (*s == '.')) {
                        s++;
                        continue;
                }
                return 0;
        }
        return 1;
}

/* ________________________________________________________________________ */
int main(int argc, char **argv)
{
        struct hostent *host = NULL;

#if 0
        res_init();
        server_print();
#endif
        
        host = gethostbyaddr_wrapper(argv[1]);
        if (host) {
            printf("Server :::::::::>>   %s\n", host->h_name);
            /* addr_list_fprint(host->h_addr_list); */
        }
        else
            printf("Server :::::::::>>   Host not found\n");
        return EXIT_SUCCESS;
#if 0
        if (is_ip_address(argv[1])) 
        {
            host = gethostbyaddr_wrapper(argv[1]);
        }
        else 
        {
            host = gethostbyname(argv[1]);
        }
        hostent_fprint(host, 0);
        return EXIT_SUCCESS;
#endif
}


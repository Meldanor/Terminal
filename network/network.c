/*
 * Copyright (C) 2012 Kilian Gärtner
 * 
 * This file is part of Terminal.
 * 
 * Terminal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * Terminal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Terminal.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "network.h"

#define OUT_BUFFER_SIZE 4096

int transferFile(FILE *source, int clientSocket) {

    char buffer[OUT_BUFFER_SIZE];
    char *p = buffer;
    int bytes_read;
    int bytes_send;
    while((bytes_read = read(source, buffer, OUT_BUFFER_SIZE)) > 0) {
        p = buffer;
        do {
            bytes_send = write(clientSocket, p, bytes_read);
            if (bytes_send >= 0) {
                bytes_read -= bytes_send;
                p += bytes_send;
            }
            if (errno != EINTR)
                return EXIT_FAILURE;
        }
        while (bytes_read > 0);
    }

    return EXIT_SUCCESS;
}

int createSocket(void) {
    return socket(SOCKET_FAMILY, SOCKET_TYPE, SOCKET_PROTOCOL);
}

int getAddress(char *address, struct sockaddr_in *sockAddr) {
    // IS THE ADDRESS IN THE FORMAT XXX.YYYY.ZZZZ.WWW LIKE 127.0.0.1?
    // IF RESULT IS ZERO, IT ISN'T
    if (inet_aton(address, &(sockAddr->sin_addr)) == 0) {
        // TRY TO RESOLVE THE DOMAIN
        struct hostent *host;
        // RESOLVE THE DOMAIN...
        host = gethostbyname(address);
        // UNABLE TO RESOLVE
        if (host == NULL) {
            printf("Unknown address %s!\n", address);
            return EXIT_FAILURE;
        }
        sockAddr->sin_addr = *(struct in_addr*)host->h_addr;
    }
    // FOUND AN ADDRESS
    return EXIT_SUCCESS;    
}

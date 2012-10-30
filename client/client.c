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
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../network/network.h"
#include "client.h"

int clientSocket;
bool clientIsRunning = true;

int main(int argc, char **args) {
    printf("Starting Terminal Client...\n");
    
    // REGISTERING THE STOP SIGNAL(CTRL+C)
    signal(SIGINT, stopClient);

    /* READ ARGUMENTS*/
    if (argc < 5) {
        printf("%s -p Port -h Host\n", args[0]);
        return EXIT_FAILURE;
    }
    int i;
    char *cur;

    char *address;
    long int port;
    for (i = 1 ; i < argc ; ++i) {
        cur = args[i];
        // READ PORT
        if (strcmp(cur, "-p") == 0) {
            // TODO: Implement check if this is a number!            
            port = strtol(args[++i], (char **) NULL, 10);
            if (port == LONG_MIN || port == LONG_MAX) {
                printf("Port %s is an invalid number!\n", args[i]);
                return EXIT_FAILURE;
            }
        }
        // READ HOST
        else if (strcmp(cur, "-h") == 0) {
            address = args[++i];
        }
        // UNKNOWN OPTION
        else {
            printf("Unknown option %s\n", cur);
            return EXIT_FAILURE;
        }
    }
    printf("Connect to %s on Port %ld...\n", address, port);

    // CREATE THE SOCKET AND TRY A CONNECT
    if (createConnection(address, port) == EXIT_FAILURE) {        
        return EXIT_FAILURE;
    }

    printf("Terminal Client started!\n");
    
    // THE CLIENT LOGIC
    clientLoop();

    return EXIT_SUCCESS;
}

int createConnection(char *address, int port) {
    // CREATE A SOCKET
    clientSocket = createSocket();
    // SOCKET CREATION FAILED 
    if(clientSocket < 0 ) {
        perror("Unable to create socket!\n");
        return EXIT_FAILURE;
    }

    // STRUCT FOR THE CONNECT INFORMATION
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = SOCKET_FAMILY;
    // CONVERT THE PORT TO A NETWORK FRIENDLY BYTE FORMAT
    serv_addr.sin_port = htons(port);
    
    // CONVERT THE ADDRESS TO A NETWORK FRIENDLY ADDRESS
    // ADDRESS CAN BE A IP ADDRESS OR A DOMAIN NAME
    if (getAddress(address, &serv_addr) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    // TRY TO CONNECT VIA THE SOCKET TO THE ADDRESS
    if (connect(clientSocket, (struct sockaddr*)(&serv_addr), sizeof(struct sockaddr)) == -1) {
        perror ("Unable to connect to the server!\n");
        return EXIT_FAILURE;
    }
}

#define OUT_BUFFER_SIZE 4096
#define IN_BUFFER_SIZE 4096

void clientLoop(void) {

    // BUFFER FOR OUTCOMING DATA
    char outBuffer[OUT_BUFFER_SIZE];
    // BUFFER FOR INCOMING DATA
    char inBuffer[IN_BUFFER_SIZE];
    // RESET THE BUFFER(WE DON'T WANT TO SEND TRASH!)
    memset(outBuffer, 0, OUT_BUFFER_SIZE);
    memset(inBuffer, 0, IN_BUFFER_SIZE);

    int bytes_read;
    int bytes_send;

    // WAIT FOR CONSOLE INPUT
    fgets(outBuffer, OUT_BUFFER_SIZE, stdin);
    
    bytes_send = write(clientSocket, outBuffer, OUT_BUFFER_SIZE - 1);
    if (bytes_read == -1 ) {
        perror("Something went wrong while receiving...\n");
    }
    else {
        printf("Send %d Bytes. %s", bytes_send, outBuffer);
        memcpy(inBuffer, outBuffer, bytes_send);
        bytes_read = read(clientSocket, inBuffer, bytes_send);
        if (bytes_read == -1) {
            perror("Something went wrong while receiving...\n");
        }
        else {
            printf("Received %d Bytes. %s", bytes_read, outBuffer);
        }
    }
    stopClient(EXIT_SUCCESS);
}

// CLEAN UP THE SERVER
void stopClient(int signal) {
    printf("Shutting down the client...\n");
    printf("Close client socket...\n");
    // CLOSE THE SOCKET
    close(clientSocket);
    
    // TERMINATE THE PROGRAM   
    exit(signal);
}

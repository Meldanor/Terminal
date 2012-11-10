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

#include "server.h"
#include "clientData.h"
#include "../network/network.h"
#include <pthread.h>

int serverSocket;
bool serverIsRunning = true;

// CURRENT CONNECTED CLIENTS
struct clientData *clients[64];
int clientCount;

int main(int argc, char **args) {

    printf("Starting Terminal Server...\n");

    // REGISTERING THE STOP SIGNAL
    signal(SIGINT, stopServer);

    /* READ ARGUMENTS*/
    if (argc < 3) {
        printf("%s -p Port\n", args[0]);
        return EXIT_FAILURE;
    }
    int i;
    char *cur;
    long int port;
    for (i = 1 ; i < argc ; ++i) {
        cur = args[i];
        // READ PORT
        if (strcmp(cur, "-p") == 0) {
            // Check if the user has written a number
            if (!isNumber(args[++i])) {
                printf("Port '%s' is an invalid number!\n", args[i]);
                return EXIT_FAILURE;
            }            
            port = strtol(args[i], (char **) NULL, 10);
            // DISALLOW KERNEL PORTS
            if (port <= 1024) {
                printf("Port %ld must be higher than 1024!\n", port);
                return EXIT_FAILURE;
            }
        }
        // UNKNOWN OPTION
        else {
            printf("Unknown option %s\n", cur);
            return EXIT_FAILURE;
        }
    }

    printf("Trying to listen to the port %ld...\n", port);
    // CREATE A SOCKET THE SERVER WILL LISTEN TO
    if (createConnection(port) == EXIT_FAILURE) {
        // SOMETHING FAILED
        return EXIT_FAILURE;
    }

    printf("Terminal Server started!\n");

    // HANDLE ALL INCOMING CLIENTS
    serverLoop();

    return EXIT_SUCCESS;
}

int createConnection(int port) {

    // CREATE A SOCKET USING IP PROTOCOL AND TCP PROTOCOL
    serverSocket = createSocket();
    if (serverSocket < 0) {
        perror("Unable to create socket!\n");
        return EXIT_FAILURE;
    }

    // INIT PORT AND ACCEPT CONNECTIONS FROM ALL IPs
    struct sockaddr_in server_addr;
    // IP PROTOCOL
    server_addr.sin_family = SOCKET_FAMILY;
    // ACCEPT CONNECTIONS FROM EVERY IP
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // LISTEN TO PORT
    server_addr.sin_port = htons( port );
    // BIND THE SOCKET TO THE PARAMETER
    int result = bind(serverSocket, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr_in));
    if (result < 0 ) {
        perror("Unable to bind server to socket!\n");
        return EXIT_FAILURE;
    }

    // START LISTENING TO SOCKET
    listen(serverSocket, SERVER_QUEUE_SIZE);

    return EXIT_SUCCESS;
}

void serverLoop(void) {
    // INFORMATION ABOUT THE CLIENT
    struct sockaddr_in client;
    int clientSocket;
    socklen_t len = sizeof(client);

    // SERVER LOOP
    while (serverIsRunning) {
        // BLOCKS UNTIL A CONNECTION IS INSIDE THE QUEUE
        clientSocket = accept( serverSocket, (struct sockaddr*)(&client), &len);
        if (clientSocket < 0 ) {
            perror("Can't accept a new client!\n");
            continue;
        }
        // TODO: ONLY ACCEPT A MAXIMUM
        // HANDLE THE CLIENT
        addClient(clientSocket, &client);
    }

    stopServer(EXIT_SUCCESS);
}

int addClient(int clientSocket, struct sockaddr_in *clientInformation) {
    struct clientData *clientData;
    clientData = malloc(sizeof(struct clientData));
    // not enought memory
    if (clientData == NULL) { 
        perror("Can't allocate memory for the clientData struct!\n");
        return EXIT_FAILURE;
    }

    pthread_t *thread;
    int result = 0;
    // create data for the thread
    getClientData(clientData, clientSocket, clientInformation);
    // Create thread
    result = pthread_create(thread, NULL, &handleClient, clientData);
    if (result != 0) {
        perror("Can't create new thread for client!\n");
        return EXIT_FAILURE;
    }
    clientData->thread = thread;

    // TODO: Need to rewrite! Incredible danger by multithreading system
    clients[clientCount++] = clientData; 

    return EXIT_SUCCESS;
}

static void *handleClient(void *arg) {

    printf("Client connected");
    struct clientData *clientData = (struct clientData *)(arg);

    int bytes_read;
    int bytes_sent;

    // client loop
    while (1) {
        // Wait for input from client
        bytes_read = read(clientData->clientSocket, clientData->inBuffer, sizeof(clientData->inBuffer));
    }

    // CLOSE CONNECTION
    close(clientData->clientSocket);
    clearClient(clientData);

    // TODO: Need to rewrite! Incredible danger by multithreading system
    clients[--clientCount] = NULL;

    printf("Client disconnected");
}

void stopServer(int signal) {
    // TODO: Reimplement everything with new structure
    printf("Shutting down the server...\n");
    // FUNCTION TO CLEAN UP
    printf("Close %d client sockets...\n", clientCount);
    // CLOSE CLIENT SOCKETS 
    int i;
    for (i = 0 ; i < clientCount; ++i) {
        close(clients[i]->clientSocket);
        clearClient(clients[i]);
    } 
    printf("Close server socket...\n");
    // CLOSE SERVER SOCKET    
    close(serverSocket);
    
    exit(signal);
}

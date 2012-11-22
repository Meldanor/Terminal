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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>

#include <pthread.h>

#include "server.h"
#include "clientData.h"
#include "../network/network.h"
#include "../common/linkedList.h"
#include "../common/regexHelper.h"
#include "HTTPHelper.h"

int serverSocket;
bool serverIsRunning = true;

// CURRENT CONNECTED CLIENTS
#define MAX_CLIENTS 64

static struct clientData *clients[MAX_CLIENTS] = {NULL};

int main(int argc, char **args) {

    // Not enough arguments
    if (argc < 3) {
        printf("Usage: %s -p Port\n", args[0]);
        return EXIT_FAILURE;
    }

    long int port;

    // Parse arguments
    int opt;
    char *endptr;
	while ((opt = getopt(argc, args, "p:")) != -1) {
		switch (opt) {
			case 'p':
			    // Convert the string to a integer
                port = strtol(optarg, &endptr, 10);
                // Invalid input - Contains no number
                if (optarg == endptr) {
                    printf("Port '%s' is an invalid number!\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            default:
                fprintf(stderr, "Unknown paramater %c", opt);
                return EXIT_FAILURE;
		}
	}
    // REGISTERING THE STOP SIGNAL
    signal(SIGINT, stopServer);

    // CREATE A SOCKET THE SERVER WILL LISTEN TO
    if (createConnection(port) == EXIT_FAILURE) {
        // SOMETHING FAILED
        return EXIT_FAILURE;
    }

    printf("Terminal Server started at port %ld.\n", port);

    // HANDLE ALL INCOMING CLIENTS
    serverLoop();

    return EXIT_SUCCESS;
}

int createConnection(int port) {

    // CREATE A SOCKET USING IP PROTOCOL AND TCP PROTOCOL
    serverSocket = createSocket();
    if (serverSocket < 0) {
        perror("Unable to create socket!");
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
        perror("Unable to bind server to socket!");
        return EXIT_FAILURE;
    }

    // START LISTENING TO SOCKET
    listen(serverSocket, SERVER_QUEUE_SIZE);

    return EXIT_SUCCESS;
}

void serverLoop(void) {
    // INFORMATION ABOUT THE CLIENT
    // Selber speicher
    int clientSocket;
    socklen_t len = sizeof(struct sockaddr_in);

    // SERVER LOOP
    while (serverIsRunning) {
        struct sockaddr_in *clientInformation = malloc(sizeof (struct sockaddr_in));
        if (clientInformation == NULL) {
            perror("Not enough memory!");
            break;
        }
        // BLOCKS UNTIL A CONNECTION IS INSIDE THE QUEUE
        clientSocket = accept( serverSocket, (struct sockaddr*)(clientInformation), &len);
        if (clientSocket < 0 ) {
            perror("Can't accept a new client!");
            continue;
        }
        // Create new client and start handeling it
        addClient(clientSocket, clientInformation);
    }

    stopServer(EXIT_SUCCESS);
}

int addClient(int clientSocket, struct sockaddr_in *clientInformation) {
    struct clientData *clientData = malloc(sizeof (struct clientData));
    // not enought memory
    if (clientData == NULL) {
        perror("Can't allocate memory for the clientData struct!");
        return EXIT_FAILURE;
    }
    int result = 0;
    // create data for the thread
    getClientData(clientData, clientSocket, clientInformation);

    // Create thread
    pthread_t thread;
    result = pthread_create(&thread, NULL, &handleClient, clientData);
    if (result != 0) {
        perror("Can't create new thread for client!");
        return EXIT_FAILURE;
    }
    clientData->thread = &thread;
    
    // Add client to list
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            clients[i] = clientData;
            clientData->position = i;
            break;
        }
    }

    return EXIT_SUCCESS;
}

void *handleClient(void *arg) {

    puts("New Client connected!");
    struct clientData *clientData = (struct clientData *)(arg);

    int bytes_read;
    int bytes_read_offset = 0;

    // client loop
    while (clientData->isConnected) {

        if (bytes_read_offset >= OUT_BUFFER_SIZE) {
            fprintf(stderr,"Offset: %d, Size: %d", bytes_read_offset, OUT_BUFFER_SIZE);
            memset((clientData->inBuffer), 0 , OUT_BUFFER_SIZE);
            bytes_read_offset = 0;
            sendError(413, clientData->clientSocket, clientData->outBuffer);
            perror("Error 413 Request Entity Too Large!");
        }

        // Read the client requests
        // Because of the request needn't to be in one flush
        // We read with an offset
        bytes_read = read(clientData->clientSocket, clientData->inBuffer + bytes_read_offset, OUT_BUFFER_SIZE - bytes_read_offset);

        // Error while reading
        if (bytes_read == -1) {
            perror("Can't read from input stream! Disconnect the client!");
            break;
        }
        if (bytes_read == 0)
            break;
        bytes_read_offset += bytes_read;
        clientData->inBuffer[bytes_read_offset] = '\0';

        // HTTP Request must end with an \r\n\r\n
        if (!isHTTPRequest(clientData->inBuffer, bytes_read_offset)) {
           continue;
        }

        // Check if it is a get request
        if (isGETRequest(clientData->inBuffer, bytes_read_offset)) {
            // Check if it is valid
            if (isValidGET(clientData->inBuffer)) {
                // open file
                char fileBuffer[255] = {0};
                extractFileFromGET(fileBuffer, clientData->inBuffer);
                int file = open(fileBuffer, O_RDONLY);
                // Send Error 404 - File Not Found
                if (file < 0) {
                    sendError(404, clientData->clientSocket, clientData->outBuffer);
                    fprintf(stderr, "Error 404 - File Not Found: %s\n", fileBuffer);
                }
                // Transfer file
                else {
                    printf("Sending file %s\n", clientData->outBuffer);
                    // Get information about the file
                    struct stat *fStat = (struct stat*)(malloc(sizeof(struct stat)));
                    if (fStat == NULL) {
                        perror("Can't allocate memory for file stat!");
                        break;
                    }
                    if (stat(fileBuffer, fStat) == -1) {
                        perror("Error while getting file stat!");
                        break;
                    }
                    memset(clientData->outBuffer, 0, OUT_BUFFER_SIZE);
                    // Create response
                    GETResponseHead(clientData->outBuffer, fStat->st_size);
                    // Send response
                    if (sendAll(clientData->clientSocket, clientData->outBuffer, strlen(clientData->outBuffer)) == -1) {
                        perror("Error while sending file to client!");
                        break;
                    }
                    if (transferFile(file, clientData->clientSocket, clientData->inBuffer) == -1) {
                        perror("Error while sending file to client!");
                        break;
                    }
                    close(file);
                    free(fStat);
                    printf("Finished sending file %s\n",fileBuffer);
                    // Disconnect client
                 
                }
                clientData->isConnected = false;
            }
            // Send Error 400 - Bad request
            else {
                sendError(400, clientData->clientSocket, clientData->outBuffer);
                fprintf(stderr, "Error 400 - Bad request: %s\n", clientData->inBuffer);
            }
        }
        // Send Error 501 - Not implemented request
        else {
            sendError(501, clientData->clientSocket, clientData->outBuffer);
            fprintf(stderr, "Error 501 - Not implemented request : %s\n", clientData->inBuffer);
        }
    }

    // CLOSE CONNECTION
    close(clientData->clientSocket);
    // Free Memory
    clearClient(clientData);
    clients[clientData->position] = NULL;

    puts("Client disconnected");

    return NULL;
}

void stopServer(int signal) {

    puts("Start server shutdown!");

    int i;
    int counter= 0;
    for (i = 0 ; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            close(clients[i]->clientSocket);
            clearClient(clients[i]);
            ++counter;
        }
    }
    printf("%d Clients disconnected!\n", counter);
    // CLOSE SERVER SOCKET
    close(serverSocket);
    puts("Closed server socket!");
    puts("Finished server shutdown!");
    exit(signal);
}

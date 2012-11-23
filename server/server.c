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
#include <signal.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "server.h"
#include "clientData.h"
#include "../network/network.h"
#include "HTTPHelper.h"

#define MAX_CLIENTS 64

int serverSocket;
bool serverIsRunning = true;

static struct clientData *clients[MAX_CLIENTS] = {NULL};

int main(int argc, char **args) {

    // Not enough arguments
    if (argc < 3) {
        printf("Usage: %s -p Port\n", args[0]);
        return EXIT_FAILURE;
    }

    char *port;

    // Parse arguments
    int opt;
	while ((opt = getopt(argc, args, "p:")) != -1) {
		switch (opt) {
			case 'p':
                port = optarg;
                break;
            default:
                fprintf(stderr, "Unknown paramater %c", opt);
                return EXIT_FAILURE;
		}
	}
    // REGISTERING THE STOP SIGNAL (CTRL+C)
    signal(SIGINT, stopServer);
    
    // Create a ServerSocket the programm is listening to
    if (initConnection(port) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    printf("Terminal Server started at port %s.\n", port);

    serverLoop();

    return EXIT_SUCCESS;
}

int initConnection(char *port) {

    // Information about the connection type
    struct addrinfo hints;
    memset(&hints, 0 , sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Listen to socket -> Allow every ip
    hints.ai_protocol = 0; // Any protocol of TCP is allowed
    
    // Store information in struct "res"
    struct addrinfo *res;
    if (getaddrinfo(NULL, port, &hints, &res) == -1) {
        perror("Can't parse information for socket creation");
        return EXIT_FAILURE;
    }
    
    // Create Socket with the stored information
    serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (serverSocket < 0 ) {
        perror("Can't create new socket!");
        return EXIT_FAILURE;
    }
    // Bind the Server to the Socket
    if (bind(serverSocket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Can't bind the server to the socket!");
        return EXIT_FAILURE;
    }
    // Free Memory
    freeaddrinfo(res);

    // Start Listening to the Socket    
    listen(serverSocket, SOMAXCONN);
    
    return EXIT_SUCCESS;
}

void serverLoop(void) {

    int clientSocket;
    socklen_t len = sizeof(struct sockaddr_in);

    // Main loop
    while (serverIsRunning) {
        // Struct for storing information about the client
        struct sockaddr_in *connectionInformation = malloc(sizeof (struct sockaddr_in));
        if (connectionInformation == NULL) {
            perror("Not enough memory!");
            break;
        }
        // BLOCKS UNTIL A CONNECTION IS INSIDE THE QUEUE
        clientSocket = accept( serverSocket, (struct sockaddr*)(connectionInformation), &len);
        if (clientSocket < 0 ) {
            perror("Can't accept a new client!");
            continue;
        }
        // Create new client and start handeling it
        addClient(clientSocket, connectionInformation);
    }

    stopServer(EXIT_SUCCESS);
}

int addClient(int clientSocket, struct sockaddr_in *connectionInformation) {
    struct clientData *clientData = malloc(sizeof (struct clientData));
    // not enought memory
    if (clientData == NULL) {
        perror("Can't allocate memory for the clientData struct!");
        return EXIT_FAILURE;
    }
    int result = 0;
    // create data for the thread
    getClientData(clientData, clientSocket, connectionInformation);

    // Create thread
    pthread_t *thread = malloc(sizeof(pthread_t));
    if (thread == NULL) {
        perror("Can't allocate memory for thread!");
        return EXIT_FAILURE;
    }
    result = pthread_create(thread, NULL, &handleClient, clientData);
    if (result != 0) {
        perror("Can't create new thread for client!");
        return EXIT_FAILURE;
    }
    clientData->thread = thread;
    
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
            memset((clientData->inBuffer), 0, OUT_BUFFER_SIZE);
            bytes_read_offset = 0;
            sendError(HTTP_ERROR_REQUEST_ENTITY_TOO_LARGE, clientData->clientSocket, clientData->outBuffer);
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
                    sendError(HTTP_ERROR_FILE_NOT_FOUND, clientData->clientSocket, clientData->outBuffer);
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
                sendError(HTTP_ERROR_BAD_REQUEST, clientData->clientSocket, clientData->outBuffer);
                fprintf(stderr, "Error 400 - Bad request: %s\n", clientData->inBuffer);
            }
        }
        // Send Error 501 - Not implemented request
        else {
            sendError(HTTP_ERROR_NOT_IMPLEMENTED, clientData->clientSocket, clientData->outBuffer);
            fprintf(stderr, "Error 501 - Not implemented request : %s\n", clientData->inBuffer);
        }
    }
    printf("Threadaddress: %p\n", clientData->thread);
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

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
#include <sys/socket.h>
#include <sys/types.h>
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
static struct LinkedList *clientList;

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

    clientList = newLinkedList();
   /* if (!initRegex()){
        perror("Can't init regex!");
        return EXIT_FAILURE;
    }*/

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
        puts("Client incoming...");
        if (clientSocket < 0 ) {
            perror("Can't accept a new client!\n");
            continue;
        }
        // Create new client and start handeling it
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
    int result = 0;
    // create data for the thread
    getClientData(clientData, clientSocket, clientInformation);    
    
    // Create thread
    pthread_t thread;
    result = pthread_create(&thread, NULL, &handleClient, clientData);
    if (result != 0) {
        perror("Can't create new thread for client!\n");
        return EXIT_FAILURE;
    }
    clientData->thread = &thread;
    add(clientList, clientData);     

    return EXIT_SUCCESS;
}

void *handleClient(void *arg) {

    puts("Client connected");
    struct clientData *clientData = (struct clientData *)(arg);

    int bytes_read;
    int bytes_read_offset = 0;

    // client loop
    while (clientData->isConnected) {

        if (bytes_read_offset >= sizeof(clientData->inBuffer)) {
            memset((clientData->inBuffer), 0 , sizeof(clientData->inBuffer));
            bytes_read_offset = 0;
            sendError(413, clientData->clientSocket, clientData->outBuffer);
            perror("Error 413 Request Entity Too Large!\n");
        }

        // Read the client requests
        // Because of the request needn't to be in one flush
        // We read with an offset
        bytes_read = read(clientData->clientSocket, clientData->inBuffer + bytes_read_offset, sizeof(clientData->inBuffer) - bytes_read_offset);
        
        // Error while reading
        if (bytes_read == -1) {
            perror("Can't read from input stream! Disconnect the client !");
            break;
        }
        bytes_read_offset += bytes_read;
        
        // HTTP Request must end with an \r\n\r\n
        if (!isHTTPRequest(clientData->inBuffer, bytes_read_offset))
            continue;

        // Delete the \r\n\r\n in the request
        memset((clientData->inBuffer) + bytes_read_offset -4, 0 , 4);

        // Check if it is a get request
        if (isGETRequest(clientData->inBuffer, bytes_read_offset)) {
            // Check if it is valid
            if (isValidGET(clientData->inBuffer, bytes_read_offset)) {
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
                    printf("Sending file %s\n", fileBuffer);
                    // Get information about the file
                    struct stat *fStat = (struct stat*)(malloc(sizeof(struct stat)));
                    if (fStat == NULL) {
                        perror("Can't allocate memory for file stat!");                                            
                        break;
                    }
                    stat(fileBuffer, fStat);

                    // Create response
                    GETResponseHead(clientData->outBuffer, "content/data", fStat->st_size);
                    // Send response
                    write(clientData->clientSocket, clientData->outBuffer, sizeof(clientData->outBuffer));
                    sendfile(clientData->clientSocket, file, 0, fStat->st_size);
                    // Finish response
                    write(clientData->clientSocket, "\n\n", strlen("\n\n"));
                }
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
        memset(clientData->outBuffer, 0, sizeof(clientData->outBuffer));
        memset(clientData->inBuffer, 0, sizeof(clientData->inBuffer));
        bytes_read_offset = 0;
    }

    // CLOSE CONNECTION
    close(clientData->clientSocket);
    clearClient(clientData);

    //removeElement(clientList, clientData);
 //   free(clientData);
    puts("Client disconnected");
    
    return NULL;
}

void stopServer(int signal) {

    puts("Start server shutdown!");
    puts("Clean up server...");

    printf("Close %d client sockets...\n", clientList->size);
    // CLOSE CLIENT SOCKETS 
    int i;
    struct clientData **clients = (struct clientData**)(toArray(clientList)); 
    if (clients != NULL) {   
        for (i = 0 ; i < clientList->size; ++i) {
            close(clients[i]->clientSocket);
            //clearClient(clients[i]);
        }
    }
    //clearList(clientList);
    // Closer server socket
    puts("Close server socket...");
    // CLOSE SERVER SOCKET    
    close(serverSocket);
    puts("Finished server shutdown!");
    exit(signal);
}

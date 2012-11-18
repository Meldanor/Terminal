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

#include "server.h"
#include "../network/network.h"
#include "../common/regexHelper.h"
#include "HTTPHelper.h"

int serverSocket;
bool serverIsRunning = true;

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
    socklen_t len = sizeof(client);

    // SERVER LOOP
    while (serverIsRunning) {
        // BLOCKS UNTIL A CONNECTION IS INSIDE THE QUEUE
        int clientSocket = accept(serverSocket, (struct sockaddr*)(&client), &len);
        puts("Client incoming...");
        if (clientSocket < 0 ) {
            perror("Can't accept a new client!\n");
            continue;
        }
        switch (fork()) {
            case -1:
                perror("Error while creating a new process!\n");
                break;
            case 0 :
                handleClient(clientSocket);
                break;
        }
    }

    stopServer(EXIT_SUCCESS);
}

#define IN_BUFFER_SIZE 4096
#define OUT_BUFFER_SIZE 4096

void handleClient(int clientSocket) {

    puts("Client connected");
    bool isConnected = true;

    char *inBuffer = malloc(IN_BUFFER_SIZE);
    if (inBuffer == NULL) {
        perror("Can't allocate memory for inBuffer!");
        isConnected = false;
    }
    char *outBuffer = malloc(OUT_BUFFER_SIZE);
    if (outBuffer == NULL) {
        perror("Can't allocate memory for outBuffer!");
        isConnected = false;
    }
    int bytes_read = 0;
    int bytes_read_offset = 0;
    
    // client loop
    while (isConnected) {

        if (bytes_read_offset >= OUT_BUFFER_SIZE) {
            fprintf(stderr,"Offset: %d, Size: %d", bytes_read_offset, OUT_BUFFER_SIZE);
            memset(inBuffer, 0 , OUT_BUFFER_SIZE);
            bytes_read_offset = 0;
            sendError(413, clientSocket, outBuffer);
            perror("Error 413 Request Entity Too Large!\n");
        }

        // Read the client requests
        // Because of the request needn't to be in one flush
        // We read with an offset
        bytes_read = read(clientSocket, inBuffer + bytes_read_offset, IN_BUFFER_SIZE - bytes_read_offset);

        // Error while reading
        if (bytes_read == -1) {
            perror("Can't read from input stream! Disconnect the client !");
            break;
        }
        if (bytes_read == 0)
            break;
        bytes_read_offset += bytes_read;

        // HTTP Request must end with an \r\n\r\n
        if (!isHTTPRequest(inBuffer, bytes_read_offset))
            continue;

        // Delete the \r\n\r\n in the request
        memset((inBuffer) + bytes_read_offset -4, 0 , 4);

        // Check if it is a get request
        if (isGETRequest(inBuffer, bytes_read_offset)) {
            // Check if it is valid
            if (isValidGET(inBuffer, bytes_read_offset)) {
                // open file
                char fileBuffer[255] = {0};
                extractFileFromGET(fileBuffer, inBuffer);
                int file = open(fileBuffer, O_RDONLY);
                // Send Error 404 - File Not Found
                if (file < 0) {
                    sendError(404, clientSocket, outBuffer);
                    fprintf(stderr, "Error 404 - File Not Found: %s\n", fileBuffer);
                }
                // Transfer file
                else {
                    printf("Sending file %s\n", outBuffer);
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
                    memset(outBuffer, 0, OUT_BUFFER_SIZE);
                    puts("Prepare head");
                    // Create response
                    GETResponseHead(outBuffer, "content/data", fStat->st_size);
                    puts("Finished head");
                    // Send response
                    puts("Send head");
                    if (sendAll(clientSocket, outBuffer, OUT_BUFFER_SIZE) == -1) {
                        perror("Error while sending file to client!");
                        break;
                    }
                    puts("Finished sending head");
                    puts("Start sendfile...");
                    if (sendfile(clientSocket, file, NULL, fStat->st_size) == -1) {
                        perror("Error while sending file to client!");
                        break;
                    }
                    puts("Finished sendfile...");
                    close(file);
                    // Finish response
                    if (sendAll(clientSocket, "\r\n\r\n", strlen("\r\n\r\n")) == -1) {
                        perror("Error while sending file to client!\n");
                        break;
                    }
                    printf("Finished sending file %s\n",fileBuffer);
                    // Disconnect client
                    isConnected = false;
                    fprintf(stderr, "Successfull: %d\n", close(clientSocket));
                }
            }
            // Send Error 400 - Bad request
            else {
                sendError(400, clientSocket, outBuffer);
                fprintf(stderr, "Error 400 - Bad request: %s\n", inBuffer);
            }
        }
        // Send Error 501 - Not implemented request
        else {
            sendError(501, clientSocket, outBuffer);
            fprintf(stderr, "Error 501 - Not implemented request : %s\n", inBuffer);
        }
    }

    // CLOSE CONNECTION
    close(clientSocket);

    puts("Client disconnected");
    exit(EXIT_SUCCESS);
}

void stopServer(int signal) {

    puts("Start server shutdown!");
    puts("Clean up server...");
    // Closer server socket
    puts("Close server socket...");
    // CLOSE SERVER SOCKET
    close(serverSocket);
    puts("Finished server shutdown!");
    exit(signal);
}

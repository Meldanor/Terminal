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

#include "connectedClient.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define IN_BUFFER_SIZE 4096
#define OUT_BUFFER_SIZE 4096

/* function to encapsulate the necessary multithreaded information */
int createClient(struct connectedClient *createClient, int clientSocket, struct sockaddr_in *clientInformation, pthread_t thread) {
    struct connectedClient *client;
    client = malloc(sizeof (struct connectedClient));
    // not enought memory
    if (client == NULL) { 
        perror("Can't allocate memory for the connectedClient struct!\n");
        return EXIT_FAILURE;
    }
    // assign values
    client->clientSocket = clientSocket;
    client->thread = thread;
    client->clientInformation = clientInformation;

    // create buffer and assign values
    char *bufferPointer;

    // in buffer
    bufferPointer = malloc(sizeof (char) * IN_BUFFER_SIZE);
    if (bufferPointer == NULL) {
        free(client);
        perror("Can't allocate memory for the terminalClient inBuffer!\n");
        return EXIT_FAILURE;
    }
    client->inBuffer = bufferPointer;

    // out buffer
    bufferPointer = malloc(sizeof (char) * OUT_BUFFER_SIZE);
    if (bufferPointer == NULL) {
        free(client->inBuffer);
        free(client);
        perror("Can't allocate memory for the terminalClient outBuffer!\n");
        return EXIT_FAILURE;
    }
    client->outBuffer = bufferPointer;

    return EXIT_SUCCESS;
}

/* Function to free memory and clear up the struct */
void clearClient(struct connectedClient *client) {
    // Nothing to do
    if (client == NULL) {
        return;
    }
    // free inBuffer
    if (client->inBuffer != NULL) {
        free(client->inBuffer);
    }
    // free outBuffer
    if (client->outBuffer != NULL) {
        free(client->outBuffer);
    }   
    // free the struct itself
    free(client);
}

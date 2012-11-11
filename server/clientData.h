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

#include <pthread.h>
#include <stdbool.h>

struct clientData {
    int clientSocket;
    bool isConnected;
    struct sockaddr_in *clientInformation;
    pthread_t *thread;
    char *inBuffer;
    char *outBuffer;
};

int getClientData(struct clientData *clientData, int clientSocket, struct sockaddr_in *clientInformation);

void clearClient(struct clientData *clientData);

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

#include "HTTPHelper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <time.h>

/* Regex Pointer*/
regex_t GET_REGEX;

/* Compile regexes */
bool init(void) {
    int regRes = regcomp(&GET_REGEX, "GET /\\S\\S* HTTP/1.0[\r\n]", 0);
    if (regRes != 0) {
        fprintf(stderr, "Error while compiling GET Regex!\n");
        return false;
    }
}

// Error buffer
char regexErrorBuffer[64];

/* --- Functions for the GET Request --- */

#define GET_HEAD "GET "

bool isGETRequest(char *request, int length) {
    // Must start with a GET
    return length >= 4 && strncmp(request, GET_HEAD, 4) == 0;
}

bool isValidGET(char *request, int length) {
    int regRes = 0;
    // Execute the regex
    regRes = regexec(&GET_REGEX, request, 0, NULL, 0);
    // Is a GET request
    if (regRes)
        return true;
    // Is not a GET request
    if (regRes == REG_NOMATCH)
        return false;
    // Something failed
    else {
        regerror(regRes, &GET_REGEX, regexErrorBuffer, sizeof(regexErrorBuffer));
        fprintf(stderr, "Regex match failed : %s \n", regexErrorBuffer);
        return false;
    }
}

int extractFileFromGET(char *fileBuffer, char *request) {
    // Start of the file part
    char *start = request + 4;

    // Search for the end of the file part
    char *end = NULL;
    end = strchr(start, ' ');
    // No SPACE found (impossible)
    if (end == NULL)
        return EXIT_FAILURE;
    // Copy file part from request to fileBuffer
    int size = (end - start) - 1;
    for(; size >= 0; --size) {
        fileBuffer[size] = start[size];
    }

    return EXIT_SUCCESS;
}

void getFormattedTime(char *buffer, int bufferSize) {
    // Get current time
    time_t curTime = time(NULL);
    // Format Time
    // Format Example: Sun, 04 Nov 2012 22:32:58 CET
    strftime(buffer, bufferSize, "Date: %a, %d %b %Y %H:%M:%S %Z", localtime(&curTime));
}

void GETResponseHead(char *headBuffer, char *contentType, int contentLength) {
    // Clear buffer
    memset(headBuffer,0, sizeof(headBuffer));

    // Add head
    strcat(headBuffer, "HTTP/1.0 200 OK\n");

    // Add date
    char date[64];
    getFormattedTime(date, sizeof(date));
    strcat(headBuffer, date);
    strcat(headBuffer, "\n");

    // Add Content Type
    strcat(headBuffer, "Content-Type: ");
    strcat(headBuffer, contentType);
    strcat(headBuffer, "\n");

    // Add Content Length
    strcat(headBuffer, "Content-Length: ");
    char contentLengthBuffer[32];
    // Convert int to string(unsafe)
    sprintf(contentLengthBuffer, "%d", contentLength);
    strcat(headBuffer, contentLengthBuffer);
    strcat(headBuffer, "\n");
    
    // Finish head information
    strcat(headBuffer, "\n");
}

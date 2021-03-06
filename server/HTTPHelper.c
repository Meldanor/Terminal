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

#include "../network/network.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <time.h>

bool isHTTPRequest(char *request, int length) {
    return (length > 4 && strstr(request, "\r\n\r\n") != NULL);
}

/* --- Functions for the GET Request --- */

#define GET_HEAD "GET "

bool isGETRequest(char *request, int length) {
    // Must start with a GET
    return length >= 4 && strncmp(request, GET_HEAD, 4) == 0;
}

bool isValidGET(char *request) {

    regex_t regex;
    int regRes = regcomp(&regex, "GET /\\S* HTTP/1\\.[01].*", 0);
    if (regRes != 0) {
        perror("Error while compiling GET Regex!");
        return false;
    }
    // Execute the regex
    regRes = regexec(&regex, request, 0, NULL, 0);
    // Is a GET request
    if (regRes == 0)
        return true;
    // Is not a GET request
    else if (regRes == REG_NOMATCH)
        return false;
    // Something failed
    else {
        char regexErrorBuffer[64];
        regerror(regRes, &regex, regexErrorBuffer, sizeof(regexErrorBuffer));
        fprintf(stderr, "Regex match failed : %s \n", regexErrorBuffer);
        return false;
    }
}

#define DOC_FOLDER "htdocs/"

bool extractFileFromGET(char *fileBuffer, char *request) {
    // Start of the file part
    char *start = request + 5;

    // Search for the end of the file part
    char *end = NULL;
    end = strchr(start, ' ');
    // No SPACE found (impossible)
    if (end == NULL)
        return false;
    // Copy file part from request to fileBuffer
    int size = (end - start) - 1;
    strcat(fileBuffer, DOC_FOLDER);
    if (size <= 0) {
        strcat(fileBuffer, "index.html");
        return true;
    }

    fileBuffer= fileBuffer + strlen(fileBuffer);
    for(; size >= 0; --size) {
        fileBuffer[size] = start[size];
    }

    return true;
}

void getFormattedTime(char *buffer, int bufferSize) {
    // Get current time
    time_t curTime = time(NULL);
    // Format Time
    // Format Example: Sun, 04 Nov 2012 22:32:58 CET
    strftime(buffer, bufferSize, "Date: %a, %d %b %Y %H:%M:%S %Z", localtime(&curTime));
}

void GETResponseHead(char *headBuffer, int contentLength) {
    // Clear buffer
    memset(headBuffer, 0, sizeof(headBuffer));

    // Add head
    strcat(headBuffer, "HTTP/1.0 200 OK\r\n");

    // Add date
    char date[64];
    getFormattedTime(date, sizeof(date));
    strcat(headBuffer, date);
    strcat(headBuffer, "\r\n");

    // Add Content Length
    strcat(headBuffer, "Content-Length: ");
    char contentLengthBuffer[32];
    // Convert int to string(unsafe)
    sprintf(contentLengthBuffer, "%d", contentLength);
    strcat(headBuffer, contentLengthBuffer);
    strcat(headBuffer, "\r\n\r\n");
}

/* ---- Misc functions ---- */

void sendError(int errorCode, int dest, char *buffer) {
    memset(buffer, 0 , sizeof(buffer));
    switch(errorCode) {
        case HTTP_ERROR_BAD_REQUEST:
            strcat(buffer, "HTTP/1.0 400 - Bad Request\r\n\r\n");
            break;
        case HTTP_ERROR_FILE_NOT_FOUND:
            strcat(buffer, "HTTP/1.0 404 - File Not Found\r\n\r\n");
            break;
        case HTTP_ERROR_REQUEST_ENTITY_TOO_LARGE:
            strcat(buffer, "HTTP/1.0 413 - Request Entity Too Large!\r\n\r\n");
            break;
        case HTTP_ERROR_NOT_IMPLEMENTED:
            strcat(buffer, "HTTP/1.0 501 - Not Implemented\r\n\r\n");
            break;
        default:
            fprintf(stderr, "Unknown error code %d!\n", errorCode);
    }
    sendAll(dest, buffer, strlen(buffer));
}

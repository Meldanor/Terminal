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

/* Regex Pointer*/
regex_t GET_REGEX;

/* Compile regexes */
bool init(void) {
    int regRes = regcomp(&regex, "GET /\\S\\S* HTTP/1.0[\r\n]", 0);
    if (regRes != 0) {
        fprintf(stderr, "Error while compiling GET Regex!\n");
        return false;
    }
}

// Error buffer
char regexErrorBuffer[64];

/* --- GET FUNCTION --- */

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

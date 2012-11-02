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
#include <stdbool.h>
#include <string.h>

#define GET_HEAD "GET "

bool isGETRequest(char *signs, int length) {
    return length >= 4 && strncmp(signs, GET_HEAD, 4) == 0;
}

bool isValidGET(char *signs, int length) {
    // Wrong head    
    if (!isGETRequest(signs, length))
        return false;
    //TODO: Check if file attribute is given and protocoll is given and terminated by \r\n
    char *p = signs + 4;
    // File attribute is wrong
    if (strncmp(p, "/", 1) != 0) {
        return false;
    }
}

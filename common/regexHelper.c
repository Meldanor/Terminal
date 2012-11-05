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
 
#include "regexHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

bool isNumber(char *string) {
    // Compile regex
    regex_t numberRegex;
    int regRes = regcomp(&numberRegex, "[0-9][0-9]*", 0);
    if (regRes != 0) {
        fprintf(stderr, "Error while compiling GET Regex!\n");
        return false;
    }
    // Execute regex
    regRes = regexec(&numberRegex, string, 0, NULL, 0);
    if (regRes != 0 && regRes != REG_NOMATCH) {
        char buffer[64];
        regerror(regRes, &numberRegex, buffer, sizeof(buffer));
        fprintf(stderr, "Regex match failed : %s \n", buffer);     
    }
    // free memory
    regfree(&numberRegex);
    return regRes == 0;
}

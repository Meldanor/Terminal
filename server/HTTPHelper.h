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
#include <stdbool.h>

bool isHTTPRequest(char *request, int length);

bool isGETRequest(char *request, int length);

bool isValidGET(char *request, int length);

bool extractFileFromGET(char *fileBuffer, char *request);

void getFormattedTime(char *buffer, int bufferSize);

void GETResponseHead(char *headBuffer, char *contentType, int contentLength);

void sendError(int errorCode, int dest, char *buffer);

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

#include "linkedList.h"

#include <stdlib.h>
#include <stdio.h>

struct LinkedList *create(void) {
    struct LinkedList *list = malloc(sizeof(struct LinkedList));
    if (list == NULL) {
        perror("Can't allocate memory for new LinkedList");
        return NULL;
    }
    struct Node *node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("Can't allocate memory for new LinkedList");
        return NULL;
    }
    list->first = node;
    node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("Can't allocate memory for new LinkedList");
        return NULL;
    }
    list->last = node;
    list->size = 0;
}

bool isEmpty(struct LinkedList *list) {
    if (list == NULL) {
        perror("LinkedList.c:isEmpty : List is null!");
        return true;
    }
    return list->size == 0;
}

bool add(struct LinkedList *list, void *value) {
    if (list == NULL) {
        perror("LinkedList.c:add : List is null!");
        return false;
    }
    list->size++;
    if (isEmpty(list)) {
        list->first->value = value;
        list->last->value = value;       
    }
    else {
        // TODO: Implement adding elements when the list has a first element
    }
}

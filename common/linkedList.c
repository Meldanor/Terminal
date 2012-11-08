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
#include <string.h>

struct LinkedList *create(void) {
    struct LinkedList *list = malloc(sizeof(struct LinkedList));
    if (list == NULL) {
        perror("Can't allocate memory for new LinkedList");
        return NULL;
    }
    list->first = NULL;
    list->last = NULL;
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
    struct Node *node = malloc(sizeof(struct Node));
    if (node == NULL) {
        perror("Can't allocate memory for new element!");
        return false;
    }
    node->value = value;
    
    if (isEmpty(list)) {
        node->next = NULL;
        node->prev = NULL;
        
        list->first = node;
        list->last = node;
        
        list->size++;
        return true;
    }
    else {
        node->next = NULL;
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
        
        list->size++;
        return true;
    }
}

void *removeFirst(struct LinkedList *list) {
    if (list == NULL) {
        perror("LinkedList.c:removeFirst : List is null!");
        return NULL;
    }
    if (isEmpty(list)) {
        perror("LinkedList.c:removeFirst : List is empty!");
        return NULL;
    }
    void *value = list->first->value;
    if (list->size == 1) {
        free(list->first);
        list->first = NULL;
        list->last = NULL;
    }
    else {
        struct Node *first = list->first;
        first->next->prev = NULL;
        list->first = first->next;
        free(first);
    }
    list->size--;
    return value;
}

void *removeLast(struct LinkedList *list) {
    if (list == NULL) {
        perror("LinkedList.c:removeFirst : List is null!");
        return NULL;
    }
    if (isEmpty(list)) {
        perror("LinkedList.c:removeFirst : List is empty!");
        return NULL;
    }
    void *value = list->last->value;
    if (list->size == 1) {
        free(list->first);
        list->first = NULL;
        list->last = NULL;
    }
    else {
        struct Node *last = list->last;
        last->prev->next = NULL;
        list->last = last->prev;
        free(last);
    }
    list->size--;
    return value;
}

bool removeElement(struct LinkedList *list, void *element) {
    if (list == NULL) {
        perror("LinkedList.c:removeElement : List is null!");
        return false;
    }
    if (isEmpty(list)) {
        perror("LinkedList.c:removeElement : List is empty!");
        return false;
    }
    
    struct Node *cur = list->first;
    while(cur != NULL) {
        if (element == cur->value) {
            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            
            list->size--;
            free(cur);
            return true;
        }
        cur = cur->next;
    }    
    return false;
}

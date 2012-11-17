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

struct LinkedList *newLinkedList(void) {
    struct LinkedList *list = malloc(sizeof(struct LinkedList));
    if (list == NULL) {
        perror("Can't allocate memory for new LinkedList");
        return NULL;
    }
    list->first = NULL;
    list->last = NULL;
    list->size = 0;

    return list;
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

void *removeAt(struct LinkedList *list, int index) {
    if (list == NULL) {
        perror("LinkedList.c:removeAt : List is null!");
        return NULL;
    }
    if (index < 0 || index >= list->size) {
        perror("LinkedList.c:removeAt : Index outside the list!");
        return NULL;
    }
    if (index == 0)
        return removeFirst(list);
    if (index == list->size - 1)
        return removeLast(list);
    
    struct Node *cur;
    bool forward = (index < (list->size / 2 ));    
    printf("Forwad: %s\n\n", forward ? "true" : "false");
    if (forward) {
        cur = list->first;
    }
    else {
        cur = list->last;
        index = list->size - index;
    }
    if (forward) {
        while(index != 0) {
            cur = cur->next;
            --index;
        }
    }
    else {
        while(index-- != 0) {
            cur = cur->prev;
            --index;
        }
    }
    
    if (cur->next != NULL)
        cur->next->prev = cur->prev;
    if (cur->prev != NULL)
        cur->prev->next = cur->next;
    void *value = cur->value;
    free(cur);
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

bool clearList(struct LinkedList *list) {
    if (list == NULL) {
        perror("LinkedList.c:clearList : List is null!");
        return false;
    }
    if (isEmpty(list))
        return true;
    struct Node *cur = list->first;
    struct Node *cur2 = list->first;
    while (cur != NULL) {
        cur2 = cur->next;
        free(cur);
        cur = cur2;
    }

    list->first = NULL;
    list->last = NULL;
    list->size = 0;
    return true;
}

void **toArray(struct LinkedList *list) {
    if (list == NULL) {
        perror("LinkedList.c:toArray : List is null!");
        return NULL;
    }
    if (isEmpty(list)) {
        return NULL;
    }
    void **ptr = malloc(sizeof(void*) * list->size);
    if (ptr == NULL) {
        perror("LinkedList.c:toArray : Can't allocate memory for array!");
        return NULL;
    }
    struct Node *cur = list->first;
    int i = 0;    
    while (cur != NULL) {
        ptr[i++] = cur->value;
        cur = cur->next;
    }
    return ptr;
}

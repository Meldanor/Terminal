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

struct Node {
    void *value;
    struct Node *next;
    struct Node *prev;
};

struct LinkedList {
    struct Node *first;
    struct Node *last;
    int size;
};

struct LinkedList *create(void);

bool isEmpty(struct LinkedList *list);

bool add(struct LinkedList *list, void *value);

void *removeFirst(struct LinkedList *list);

void *removeLast(struct LinkedList *list);

void *removeElement(struct LinkedList *list, int index);

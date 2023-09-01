/*
 *	single-linked list
 * 
 *	Copyright (C) 2017-2021 Nicholas Christopoulos.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#ifndef NDC_GSL_LIST_H_
#define NDC_GSL_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdbool.h>

typedef struct list_node_s { void *data; size_t size; struct list_node_s *next; } list_node_t;
typedef struct list_s { list_node_t *head, *tail; } list_t;

// create a new list and returns the pointer
list_t *list_create();

// for non-dynamic (and non-static) allocated lists you need to initialize them first
// for a non-dynamic allocated list use list_clear() instead of list_destroy()
// to free their contents.
void list_init(list_t *list);

// deletes all elements of the list
void list_clear(list_t *list);

// destroy a list, returns always NULL
list_t *list_destroy(list_t *list);

// adds a node at the end of the list; returns the pointer to the new node
// use size = 0 to store only a pointer
list_node_t *list_add(list_t *list, void *data, size_t size);
list_node_t *list_addstr(list_t *list, const char *str);
list_node_t *list_addptr(list_t *list, void *ptr);

// delete node
bool list_delete(list_t *list, list_node_t *node);

// returns the number of the nodes
size_t list_count(const list_t *list);

// create an index table of node pointers;
// the last element of the array is NULL.
list_node_t **list_to_index(list_t *list);

// create an table of data pointers;
// the last element of the array is NULL.
void **list_to_table(list_t *list);

// find and return note by comparing 'data' as string
list_node_t *list_findstr(list_t *list, const char *str);

// find and return note by memory address
list_node_t *list_findptr(list_t *list, const void *ptr);

#ifdef __cplusplus
}
#endif
	
#endif


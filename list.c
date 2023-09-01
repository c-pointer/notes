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

#include "errio.h"
#include "list.h"

// create a new list and returns the pointer
list_t *list_create() {
	list_t *list = (list_t *) m_alloc(sizeof(list_t));
	list->head = list->tail = NULL;
	return list;
	}

// initialize a list
void list_init(list_t *list) {
	list->head = list->tail = NULL;
	}

// deletes all elements of the list
void list_clear(list_t *list) {
	list_node_t *cur = list->head, *pre;
	while ( cur ) {
		pre = cur;
		cur = cur->next;
		if ( pre->size )
			m_free(pre->data);
		m_free(pre);
		}
	list->head = list->tail = NULL;
	}

// destroy a list, returns always NULL
list_t *list_destroy(list_t *list) {
	list_clear(list);
	m_free(list);
	return NULL;
	}

// adds a node at the end of the list; returns the pointer to the new node
// use size = 0 to store only a pointer
list_node_t *list_add(list_t *list, void *data, size_t size) {
	list_node_t *np = (list_node_t *) m_alloc(sizeof(list_node_t));

	// fill data
	np->size = size;
	if ( size ) {	// allocate space
		np->data = (void *) m_alloc(size);
		if ( data ) // copy data
			memcpy(np->data, data, size);
		}
	else
		np->data = data; // store pointer

	// connect new node
	np->next = NULL;
	if ( list->head ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->head = list->tail = np;
	
	return np;
	}

list_node_t *list_addstr(list_t *list, const char *str)
	{ return list_add(list, (void *) str, strlen(str) + 1); }
list_node_t *list_addptr(list_t *list, void *ptr)
	{ return list_add(list, ptr, 0); }

// delete node
bool list_delete(list_t *list, list_node_t *node) {
	list_node_t	*cur, *prev = NULL;
	for ( cur = list->head; cur; cur = cur->next ) {
		if ( cur == node ) {
			if ( prev )					prev->next = cur->next;
			if ( cur == list->head )	list->head = cur->next;
			if ( cur == list->tail )	list->tail = prev;
			if ( cur->size )
				m_free(cur->data);
			m_free(cur);
			return true;
			}
		prev = cur;
		}
	return false;
	}

// returns the number of the nodes
size_t list_count(const list_t *list) {
	size_t count = 0;
	for ( const list_node_t *cur = list->head; cur; cur = (const list_node_t *) cur->next ) count ++;
	return count;
	}

// create an index table of node pointers;
// the last element of the array is NULL.
list_node_t **list_to_index(list_t *list) {
	list_node_t	**table = NULL;
	size_t	i, count = list_count(list);
	list_node_t *cur;
	table = (list_node_t **) m_alloc(sizeof(list_node_t*) * (count + 1));
	for ( i = 0, cur = list->head; cur; cur = cur->next, i ++ )
		table[i] = cur;
	table[count] = NULL;
	return table;
	}

// create an table of data pointers;
// the last element of the array is NULL.
void **list_to_table(list_t *list) {
	void	**table = NULL;
	size_t	i, count = list_count(list);
	list_node_t *cur;
	table = (void **) m_alloc(sizeof(void*) * (count + 1));
	for ( i = 0, cur = list->head; cur; cur = cur->next, i ++ )
		table[i] = cur->data;
	table[i] = NULL;
	return table;
	}

// find and return note by comparing 'data' as string
list_node_t *list_findstr(list_t *list, const char *str) {
	for ( list_node_t *cur = list->head; cur; cur = cur->next )
		if ( strcmp((const char *) cur->data, str) == 0 )
			return cur;
	return NULL;
	}

// find and return note by memory address
list_node_t *list_findptr(list_t *list, const void *ptr) {
	for ( list_node_t *cur = list->head; cur; cur = cur->next )
		if ( cur->data == ptr )
			return cur;
	return NULL;
	}



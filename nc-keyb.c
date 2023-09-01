/*
 *	ncurses - keyboard
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

#include "list.h"
#include "nc-plus.h"

//
#define KEYMAPS_MAX		8
typedef struct { char name[32]; list_t map; } keymap_t;
static keymap_t keymaps[KEYMAPS_MAX];
static int kmap_count = 0;
typedef struct { int key, pid; } pkey_t;

// returns a pointer to keymap list
static list_t *getkeymap(const char *name) {
	for ( int i = 0; i < kmap_count; i ++ )
		if ( strncmp(keymaps[i].name, name, 32) == 0 )
			return &keymaps[i].map;
	strncpy(keymaps[kmap_count].name, name, 32);
	return &keymaps[kmap_count++].map;
	}

//
void nc_addkey(const char *map_name, int pkey, int key) {
	list_t *map = getkeymap(map_name);
	pkey_t	pk = { key, KEY_PRG(pkey) };
	list_add(map, &pk, sizeof(pkey_t));
	}

//
void nc_delkey(const char *map_name, int key) {
	list_t *map = getkeymap(map_name);
	list_node_t	*cur = map->head, *next;
	
	while ( cur ) {
		int c = ((pkey_t *) cur->data)->key;
		next = cur->next;
		if ( c == key )
			list_delete(map, cur); // delete this
		cur = next;
		}
	}

// assigns additional keys to procedural key pkey
void nc_setkey(const char *map_name, int pkey, ...) {
	va_list	ap;
	pkey_t	pk = { pkey, KEY_PRG(pkey) };
	int		c;
	
	list_t *map = getkeymap(map_name);
	list_add(map, &pk, sizeof(pkey_t));
	
	va_start(ap, pkey);
	while ( (c = va_arg(ap, int)) != 0 ) {
		pk.key = c;
		list_add(map, &pk, sizeof(pkey_t));
		}
	va_end(ap);
	}

// setup default keymap
void nc_use_default_keymap() {
	nc_setkey("input", KEY_BACKSPACE, '', '', 127, 0);
	nc_setkey("input", KEY_ENTER,  '\n', '\r', 0);
	nc_setkey("input", KEY_CANCEL, 27, '', '', 0);
	nc_setkey("input", KEY_EXIT, 0);
	
	nc_setkey("input", KEY_UP,     '', 0);
	nc_setkey("input", KEY_DOWN,   '', 0);
	nc_setkey("input", KEY_LEFT,   '', 0);
	nc_setkey("input", KEY_RIGHT,  '', 0);

	nc_setkey("input", KEY_HOME,      1, 0);
	nc_setkey("input", KEY_END,    '', 0);
	}

// returns the last defined KEY_PRG code of the 'key'
int	nc_getprg(const char *map_name, int key) {
	list_t *map = getkeymap(map_name);
	list_node_t *cur = map->head;
	int       pid = KEY_PRG(key); // default: return the same key
	pkey_t    *pk;
	
	while ( cur ) {
		pk = (pkey_t *) cur->data;
		if ( pk->key == key )
			pid = pk->pid;	// get the last defined
		cur = cur->next;
		}
	return pid;
	}

// returns the key-code from a string or <0 on error
// note: keycode of ^@ = 0
int nc_getkeycode(const char *name) {
	const char *p = name;
	int   flags = 0, c = 0;
	char	str[4];

	if ( *p == '^' ) {
		flags |= 0x2;
		p ++;
		}
	else {
		while ( p[1] == '-' ) {
			switch ( toupper(*p) ) {
			case 'S': flags |= 0x1; break;
			case 'C': flags |= 0x2; break;
			case 'A': case 'M': flags |= 0x4; break;
			default: // error
//				fprintf(stderr, "unknown key combination '%s'\n", name);
				return -1;
				}
			p += 2;
			}
		}
	
	if ( toupper(*p) == 'F' && isdigit(*(p+1)) ) {
		str[0] = p[1];
		str[2] = ( isdigit(p[2]) ) ? p[2] : '\0';
		str[3] = '\0';
		// flags does not works with ncurses... man -s 3 curs_getch
		return KEY_F(atoi(str));
		}
	
	if ( strcasecmp(p, "enter") == 0 )	return KEY_ENTER;
	else if ( strcasecmp(p, "esc") == 0 )	return 27;
	else if ( strcasecmp(p, "tab") == 0 )	return 9;
	else if ( strcasecmp(p, "home") == 0 )	return (flags & 0x1) ? KEY_SHOME : KEY_HOME;
	else if ( strcasecmp(p, "end") == 0 )	return (flags & 0x1) ? KEY_SEND : KEY_END;
	else if ( strcasecmp(p, "pgup") == 0 )	return KEY_PGUP;
	else if ( strcasecmp(p, "pageup") == 0 )	return KEY_PGUP;
	else if ( strcasecmp(p, "pgdn") == 0 )	return KEY_PGDN;
	else if ( strcasecmp(p, "pagedn") == 0 )	return KEY_PGDN;
	else if ( strcasecmp(p, "pagedown") == 0 )	return KEY_PGDN;
	else if ( strcasecmp(p, "up") == 0 )		return KEY_UP;
	else if ( strcasecmp(p, "down") == 0 )	return KEY_DOWN;
	else if ( strcasecmp(p, "left") == 0 )	return (flags & 0x1) ? KEY_SLEFT : KEY_LEFT;
	else if ( strcasecmp(p, "right") == 0 )	return (flags & 0x1) ? KEY_SRIGHT : KEY_RIGHT;
	else if ( strcasecmp(p, "insert") == 0 )	return (flags & 0x1) ? KEY_SIC : KEY_IC;
	else if ( strcasecmp(p, "delete") == 0 )	return (flags & 0x1) ? KEY_SDC : KEY_DC;
	else if ( strcasecmp(p, "ins") == 0 )	return (flags & 0x1) ? KEY_SIC : KEY_IC;
	else if ( strcasecmp(p, "del") == 0 )	return (flags & 0x1) ? KEY_SDC : KEY_DC;
	else {
		c = ( flags & 0x3 ) ? toupper(*p) : *p;
		if ( flags & 0x2 )	c = (c >= '@') ? c - '@' : c;
		if ( flags & 0x4 )	c |= KEY_ALT_BIT;
		return c;
		}
//	fprintf(stderr, "unknown key combination '%s'\n", name);
	return -2;
	}


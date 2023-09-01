/*
 *	ncurses - additional and abilities tools
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

#ifndef _NC_ADV_LIB_H_
#define _NC_ADV_LIB_H_

#include "errio.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "str.h"
#include <stdlib.h>
#include <ncurses.h>

#ifndef MAX
	#define MAX(a,b)	((a>b)?a:b)
	#define MIN(a,b)	((a<b)?a:b)
#endif

// mask 0x100 allocated by ncurses
#define KEY_ALT_BIT		0x200
#define KEY_USR_BIT		0x400
#define KEY_PRG_BIT		0x800

#define KEY_CTRL(n)		(toupper(n)-'@')
#define KEY_ALT(n)		(KEY_ALT_BIT | n)
#define KEY_USR(n)		(KEY_USR_BIT | n)
#define KEY_PRG(n)		(KEY_PRG_BIT | n)
#define KPRG_KEY(n)		(n & 0x3ff)

#define KEY_PGUP	KEY_PPAGE
#define KEY_PGDN	KEY_NPAGE

// init/close
void nc_init ();
void nc_close();

// keyboard
void nc_use_default_keymap();
void nc_addkey(const char *map_name, int pkey, int key);
void nc_delkey(const char *map_name, int key);
void nc_setkey(const char *map_name, int pkey, ...);
int  nc_getprg(const char *map_name, int key);
int  nc_getkeycode(const char *key_name);

// colors
void nc_setcolor  (WINDOW *win, int fg, int bg);
void nc_unsetcolor(WINDOW *win, int fg, int bg);
void nc_setvgacolor  (WINDOW *win, int fg, int bg); // note: VGA colors
void nc_unsetvgacolor(WINDOW *win, int fg, int bg); // note: VGA colors
#define nc_setvgac(w,a)		nc_setvgacolor(w, (a & 0xF), (a & 0xF0) >> 4)
#define nc_unsetvgac(w,a)	nc_unsetvgacolor(w, (a & 0xF), (a & 0xF0) >> 4)

int nc_createpair(int fg, int bg);
int nc_createvgapair(int fg, int bg);
void nc_setpair   (WINDOW *win, int pair);
void nc_unsetpair (WINDOW *win, int pair);

// utilities
void nc_wtitle(WINDOW *win, const char * title, int align);

// printf with "escape-codes"
void nc_mvwprintf(WINDOW *win, int y, int x, const char *fmt, ...);
#define nc_wprintf(w,f,...)	nc_mvwprintf(w,-1,-1,f,__VA_ARGS__)
#define nc_printf(f,...)	nc_mvwprintf(stdscr,-1,-1,f,__VA_ARGS__)

// input string 
// *editstr functions = edit contents of str; str must be a null terminated string
// *readstr functions = the typical gets, str does not need to initialized
bool nc_mvweditstr(WINDOW *w, int y, int x, char *str, int maxlen);
bool nc_mvwreadstr(WINDOW *w, int y, int x, char *str, int maxlen);
bool nc_mveditstr(int y, int x, char *s, int m);
bool nc_mvreadstr(int y, int x, char *s, int m);
bool nc_editstr(char *s, int m);
bool nc_readstr(char *s, int m);
bool nc_weditstr(WINDOW *w, char *s, int m);
bool nc_wreadstr(WINDOW *w, char *s, int m);

// tools
void nc_view(const char *title, const char *body);
int  nc_listbox(const char *title, const char **items, int default_index /* = 0 */);

#ifdef __cplusplus
}
#endif

#endif


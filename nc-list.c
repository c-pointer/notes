/*
 *	ncurses listbox
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

#include <stdbool.h>
#include <ctype.h>
#include <wctype.h>
#include "nc-plus.h"

/*
 * returns the selected item index or -1
 */
int nc_listbox(const char *title, const char **items, int defidx) {
	int		x, y, dx, dy, c, exf = 0;
	int		i, lines, wlines, offset;
	int		maxlen = 0, maxcols = 0, pos = 0;
	bool	found;
	
	for ( lines = 0; items[lines]; lines ++ ) {
		maxlen = MAX(maxlen, strlen(items[lines])); 
		maxcols = MAX(maxcols, u8width(items[lines])); 
		}
	if ( title ) {
		maxlen = MAX(maxlen, strlen(title)); 
		maxcols = MAX(maxcols, u8width(title));
		}
	if ( defidx < 0 || defidx >= lines )
		defidx = 0;
	
	// outer window
	dx = maxcols + 6;
	dy = MIN(LINES - 4, lines + 2);
	x = COLS / 2 - dx / 2;
	y = LINES / 2 - dy / 2;
	WINDOW	*wout = newwin(dy, dx, y, x);
	keypad(wout, TRUE);
	
	WINDOW	*w = subwin(wout, dy - 2, dx - 4, y + 1, x + 2);
	keypad(w, TRUE);
	wlines = dy - 4;

	offset = 0;
	pos = defidx;
	do {
		if ( offset > pos )	offset = pos;
		if ( offset + lines < pos )	offset = pos - lines;
		if ( offset < 0 ) offset = 0;
		werase(wout);
		box(wout, 0, 0);
		if ( title ) nc_wtitle(wout, title, 2);
		wrefresh(wout);
		werase(w);
		for ( i = offset, y = 0; i < lines; i ++, y ++ ) {
			if ( i == pos ) wattron(w, A_REVERSE);
			mvwhline(w, y, 0, ' ', getmaxx(w));
			mvwprintw(w, y, 1, "%s", items[i]);
			if ( i == pos ) wattroff(w, A_REVERSE);
			}
		wrefresh(w);
		
		c = wgetch(w);
		switch ( c ) {
		case 'q':		pos = -1; exf ++; break;
		case '': case '':
		case '\033':	pos = -1; exf ++; break;
		case '\r': case '\n': exf ++; break;
		case KEY_UP:
			if ( pos )
				pos --;
			break;
		case KEY_DOWN:
			if ( pos < (lines - 1) )
				pos ++;
			break;
		case KEY_PPAGE:
			pos -= wlines;
			if ( pos < 0 )
				pos = 0;
			break;
		case KEY_NPAGE:
			pos += wlines;
			if ( pos > (lines - 1) )
				pos = lines - 1;
			break;
		case '\001':
		case KEY_HOME:	offset = pos = 0; break;
		case '\005':
		case KEY_END:	pos = lines - 1; break;
		default: {
				found = false;
				wchar_t	wi, wc;
				int		clen = u8csize(c);
				if ( clen > 1 ) {
					char mbs[7];
					mbs[0] = c;
					for ( i = 1; i < clen; i ++ )
						mbs[i] = wgetch(w);
					mbs[clen] = '\0';
					wc = u8towc(mbs);
					}
				else
					wc = (wchar_t) c;
				wi = u8towc(items[pos]);
				if ( towupper(wi) >= towupper(wc) ) { // find next
					for ( i = pos + 1; i < lines; i ++ ) {
						wi = u8towc(items[i]);
						if ( towupper(wi) == towupper(wc) ) {
							pos = i;
							found = true;
							break;
							}
						}
					}
				if ( !found ) { // search from the beginning
					for ( i = 0; i < lines; i ++ ) {
						wi = u8towc(items[i]);
						if ( towupper(wi) == towupper(wc) ) {
							pos = i;
							found = true;
							break;
							}
						}
					}
				}
			}
		} while ( !exf );

	// cleanup
	delwin(w);
	delwin(wout);
	refresh();
	return pos;
	}


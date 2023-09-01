/*
 *	ncurses view text window
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

#include "nc-plus.h"

//
void nc_view(const char *title, const char *body) {
	int		x, y, dx, dy, c, exf = 0, i, lines, wlines, offset;
	char	**text = text_to_lines(body);
	
	for ( lines = 0; text[lines]; lines ++ ); // count lines in the text
	
	// outer window
	x = 10; y = 5;
	dx = COLS - 20;
	dy = LINES - 10;
	WINDOW	*wout = newwin(dy, dx, y, x);
	refresh();
	keypad(wout, TRUE);
	box(wout, 0, 0);
	if ( title ) nc_wtitle(wout, title, 0);
	
	WINDOW	*w = subwin(wout, dy - 4, dx - 6, y + 2, x + 3);
	keypad(w, TRUE);
	wlines = dy - 4;
	wrefresh(wout);

	offset = 0;
	do {
		// EOT
		bool eot = (lines - offset <= getmaxy(w));
		if ( eot ) {
			offset = lines - getmaxy(w);
			if ( offset < 0 ) offset = 0;
			// ␄
			nc_mvwprintf(wout, getmaxy(wout) - 2, getmaxx(wout) - 3, "$C20 $c");
			}
		else
			nc_mvwprintf(wout, getmaxy(wout) - 2, getmaxx(wout) - 3, "$C20↓$c");
		wrefresh(wout);

		//
		werase(w);
		for ( i = offset, y = 0; i < lines; i ++, y ++ )
			mvwprintw(w, y, 0, "%s", text[i]);
		wrefresh(w);
		
		c = wgetch(w);
		switch ( c ) {
		case 'q': exf ++; break;
		case '': case '': case '\033':
			exf ++; break;
		case '\r': case '\n': exf ++; break;
		case 'k': case KEY_UP:
			if ( offset )
				offset --;
			break;
		case 'j': case KEY_DOWN:
			if ( offset < (lines - 1) )
				offset ++;
			break;
		case KEY_PPAGE:
			offset -= wlines;
			if ( offset < 0 )
				offset = 0;
			break;
		case KEY_NPAGE:
			offset += wlines;
			if ( offset > (lines - 1) )
				offset = lines - 1;
			break;
		case 'g': case KEY_HOME:	offset = 0; break;
		case 'G': case KEY_END:		offset = lines - 1; break;
			};
		} while ( !exf );

	// cleanup
	delwin(w);
	delwin(wout);
	refresh();
	free(text);
	}



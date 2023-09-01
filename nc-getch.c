/*
 * ncurses - test getch
 */
#include <stdio.h>
#include <ncurses.h>

#define ALT_BIT		0x400

int main(int argc, char *argv[]) {
	char buf[16];
	initscr();
	noecho(); nonl(); cbreak();
	keypad(stdscr, TRUE);
	keypad(curscr, TRUE);
	for ( int i = 1; i < 127; i ++ ) {
		sprintf(buf, "\033%c", i);
		define_key(buf, ALT_BIT | i);
		}
	int ch = getch();
	endwin();
	printf("KEY NAME : %s - %d (0x%4X) %c %d\n", keyname(ch), ch, ch, ch & 0xFF, ch & 0xFF);
	}

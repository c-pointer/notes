/*
 * ncurses - test colors
 */
#include <ncurses.h>

static int ncvgac(int fg, int bg) {
	int B, bbb, ffff;

	B = 1 << 7;
	bbb = (7 & bg) << 4;
	ffff = 7 & fg;
	return (B | bbb | ffff);
	}

static short ncvga8(int c) {
	switch (7 & c)	{
	case 0: return (COLOR_BLACK);
	case 1: return (COLOR_BLUE);
	case 2: return (COLOR_GREEN);
	case 3: return (COLOR_CYAN);
	case 4: return (COLOR_RED);
	case 5: return (COLOR_MAGENTA);
	case 6: return (COLOR_YELLOW);
	case 7: return (COLOR_WHITE);
		}
	return 0;
	}

static void nc_clrpairs() {
	int fg, bg;
	int colorpair;

	for ( bg = 0; bg < 8; bg ++ ) {
		for ( fg = 0; fg < 8; fg ++ ) {
			colorpair = ncvgac(fg, bg);
			init_pair(colorpair, ncvga8(fg), ncvga8(bg));
			}
		}
	}

int main(int argc, char *argv[]) {
	int i;
	
	initscr();
	noecho(); nonl(); cbreak();
	keypad(curscr, TRUE);

    if ( has_colors() ) {
		start_color();
		use_default_colors();
		for ( i = 1; i < 256; i ++ )
			init_pair(i, COLOR_WHITE, i);
//		nc_clrpairs();
		}
	for ( i = 0; i < 256; i ++ ) {
		attron(COLOR_PAIR(i));
		printw(" %2X ", i);
		attroff(COLOR_PAIR(i));
		if ( ((i+1) % 16) == 0 )
			printw("\n");
		else if ( ((i+1) % 8) == 0 )
			printw(" - ");
		
		}
	getch();
	endwin();
	printf("tabsize %d, cols %d, lines %d\n", TABSIZE, COLS, LINES);
	printf("colors %d, pairs %d\n", COLORS, COLOR_PAIRS);
	}

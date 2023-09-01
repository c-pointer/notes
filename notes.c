/*
 *	notes
 *
 *	command-line notebook application
 * 
 *	Copyright (C) 2017-2022 Free Software Foundation, Inc.
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

#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <wordexp.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include <features.h>
#include <fnmatch.h>

#include "list.h"
#include "str.h"
#include "nc-plus.h"
#if defined(__GNU_GLIBC__)
	#define FNM_GLIBC_EXTRA FNM_EXTMATCH
#else
	#define FNM_GLIBC_EXTRA 0
#endif

#define OPT_AUTO	0x0001
#define OPT_LIST	0x0002
#define OPT_EDIT	0x0004
#define OPT_VIEW	0x0008
#define OPT_ADD		0x0010
#define OPT_APPD	0x0020
#define OPT_DEL		0x0040
#define OPT_MOVE	0x0080
#define OPT_FILES	0x0100
#define OPT_ALL		0x0200
#define OPT_COMPL	0x0400
#define OPT_STDIN	0x0800
#define OPT_PRINT	0x1000
#define OPT_NOCLOB	0x2000

int		opt_flags = OPT_AUTO;
int		opt_pv_filestat = 1;

int clr_normal = 0x07;
int clr_select = 0x70;
int clr_status = 0x70;
int clr_status_key = 0x74;
int clr_code = 0x02;
int clr_text = 0x07;
int clr_bold = 0x0f;
int clr_hide = 0x08;

// === configuration & interpreter ==========================================

static char home[PATH_MAX];	// user's home directory
static char ndir[PATH_MAX];	// app data directory (per user)
static char bdir[PATH_MAX];	// backup directory
static char conf[PATH_MAX];	// app configuration file
static bool g_globber = true;
static char sclob[64];
static char current_section[NAME_MAX];
static char current_filter[NAME_MAX];
static char default_ftype[NAME_MAX];
static char onstart_cmd[LINE_MAX];
static char onexit_cmd[LINE_MAX];
static list_t *exclude;

// returns true if the string 'str' is value of true
bool istrue(const char *str) {
	const char *p = str;
	while ( isblank(*p) ) p ++;
	if ( tolower(*p) == 'o' )
		if ( strncasecmp(p, "on", 2) == 0 ) return true;
	if ( isdigit(*p) )
		if ( *p != '0' ) return true;
	return (strchr("YT+", toupper(*p)) != NULL);
	}

// add pattern to exclude list
void excl_add_pat(const char *pars) {
	char *string = strdup(pars);
	const char *delim = " \t";
	char *ptr = strtok(string, delim);
	while ( ptr ) {
		list_addstr(exclude, ptr);
		ptr = strtok(NULL, delim);
		}
	m_free(string);
	}

// user menu
typedef struct { char label[256]; char cmd[LINE_MAX]; } umenu_item_t;
static list_t *umenu;

void umenu_add(const char *pars) {
	char	*src = strdup(pars), *p;
	umenu_item_t u;
	
	if ( (p = strchr(src, ';')) != NULL ) {
		*p ++ = '\0';
		while ( isblank(*p) )	p ++;
		strcpy(u.cmd, p);
		p = src;
		while ( isblank(*p) )	p ++;
		rtrim(p);
		strcpy(u.label, p);
		list_add(umenu, &u, sizeof(umenu_item_t));
		}
	m_free(src);
	}

// keymap

// name def-key user-key internal-key
typedef struct { const char *keyword; int id; } keyfunc_t;
static keyfunc_t keyfunc[] = {
{ "backspace",	KEY_PRG(KEY_BACKSPACE) },
{ "enter",		KEY_PRG(KEY_ENTER) },
{ "cancel",		KEY_PRG(KEY_CANCEL) },
{ "quit",		KEY_PRG(KEY_EXIT) },
//
{ "up",			KEY_PRG(KEY_UP) },
{ "down",		KEY_PRG(KEY_DOWN) },
{ "left",		KEY_PRG(KEY_LEFT) },
{ "right",		KEY_PRG(KEY_RIGHT) },
//
{ "home",		KEY_PRG(KEY_HOME) },
{ "end",		KEY_PRG(KEY_END) },
{ "pgup",		KEY_PRG(KEY_PGUP) },
{ "pgdn",		KEY_PRG(KEY_PGDN) },
{ "insmode",	KEY_PRG(KEY_IC) },
{ "delchar",	KEY_PRG(KEY_DC) },
//
{ "new",		KEY_PRG('n') },
{ "add",		KEY_PRG('a') },
{ "view",		KEY_PRG('v') },
{ "edit",		KEY_PRG('e') },
{ "delete",		KEY_PRG(KEY_DC) },
{ "rename",		KEY_PRG('r') },
{ "help",		KEY_PRG(KEY_HELP) },
{ "tag",		KEY_PRG(KEY_MARK) },
{ "shell",		KEY_PRG('!') },
{ "execute",		KEY_PRG('!') },
{ "untag-all",	KEY_PRG('u') },
{ "change-section",		KEY_PRG('c') },
{ "select-section",		KEY_PRG('s') },
{ "umenu",		KEY_PRG('m') },
{ "user-menu",		KEY_PRG('m') },
{ "search",		KEY_PRG(KEY_FIND) },
{ NULL, 0 } };

// setup default keymap
void set_default_keymap() {
	nc_use_default_keymap();
	nc_addkey("input", KEY_CANCEL, 3);
	
	nc_setkey("nav", KEY_BACKSPACE, 4, 8, 127, 0);
	nc_setkey("nav", KEY_ENTER, '\n', '\r', 0);
	nc_setkey("nav", KEY_CANCEL, 3, 27, '', 0);
	nc_setkey("nav", KEY_EXIT, 3, 'q', '', 0);
	nc_setkey("nav", KEY_REFRESH, '', 0);
	
	nc_setkey("nav", KEY_UP, '', 'k', 0);
	nc_setkey("nav", KEY_DOWN, '', 'j', 0);
	nc_setkey("nav", KEY_LEFT, '', 'h', 0);
	nc_setkey("nav", KEY_RIGHT, '', 'l', 0);
	nc_setkey("nav", KEY_HOME, 1, 'g', '^', 0);
	nc_setkey("nav", KEY_END, '', 'G', '$', 0);

	nc_setkey("nav", KEY_HELP, '?', KEY_F(1), 0); // help
	nc_setkey("nav", KEY_DC, 'd', KEY_F(8), 0);	// delete
	nc_setkey("nav", 'v', KEY_F(3), 0); // view
	nc_setkey("nav", 'e', KEY_F(4), 0); // edit
	nc_setkey("nav", 'n', 0);	// new
	nc_setkey("nav", 'a', 0);	// add
	nc_setkey("nav", KEY_MARK, 't', KEY_IC, 0);	// tag/untag
	nc_setkey("nav", 'u', KEY_F(9), 0);	// untag all
	nc_setkey("nav", 'r', KEY_F(6), 0);	// rename
	nc_setkey("nav", KEY_FIND, '/', KEY_F(7), 0); // search
	nc_setkey("nav", 'c', 0); // change section (move-to)
	nc_setkey("nav", 's', 0);	// select section (filter)
	nc_setkey("nav", 'm', KEY_F(2), 0);	// user-defined menu
	nc_setkey("nav", '!', KEY_F(10), 0);	// execute
//	nc_setkey("nav", 'f', 0);	// set filter test
	nc_setkey("nav", 'f', 0);	// file manager
	}

// map key to command
void keymap_add(const char *pars) {
	char	*src = strdup(pars), *p = src;
	char	dest[64], *d = dest;
	char	keycode[64], keycmd[64], kmap[64];
	bool	sq = false, dq = false;
	int		wc = 0, c;
	char str[16];
	
	while ( isblank(*p) )	p ++;
	while ( *p ) {
		if ( *p == '\'' )
			sq = !sq;
		else if ( !sq ) {
			if ( *p == '"' )
				dq = !dq;
			else if ( *p == '\\' ) {
				p ++;
				switch ( *p ) {
				case 't': *d ++ = '\t'; p ++; break;
				case 'r': *d ++ = '\r'; p ++; break;
				case 'n': *d ++ = '\n'; p ++; break;
				case 'v': *d ++ = '\v'; p ++; break;
				case 'e': *d ++ = '\033'; p ++; break;
				case 'a': *d ++ = '\a'; p ++; break;
				case 'b': *d ++ = '\b'; p ++; break;
				case 'f': *d ++ = '\f'; p ++; break;
				case 'x':
					p ++;
					str[0] = (isxdigit(*p)) ? *p ++ : 0;
					str[1] = (isxdigit(*p)) ? *p ++ : 0;
					str[2] = 0;
					*d ++ = strtol(str, NULL, 16);
					break;
				default:
					if ( *p >= '0' && *p < '8') {
						c = 0;
						if ( *p >= '0' && *p < '8' ) { c = (c << 3) | (*p - '0'); p ++; }
						if ( *p >= '0' && *p < '8' ) { c = (c << 3) | (*p - '0'); p ++; }
						if ( *p >= '0' && *p < '8' ) { c = (c << 3) | (*p - '0'); p ++; }
						*d ++ = c;
						}
					else
						*d ++ = *p ++;
					}
				continue;
				}
			else if ( !dq ) {
				if ( isblank(*p) ) {
					*d = '\0';
					d = dest;
					while ( isblank(*p) )	p ++;
					wc ++;
					if ( wc == 1 )
						strcpy(kmap, dest);
					else if ( wc == 2 )
						strcpy(keycode, dest);
					else if ( wc == 3 ) {
						strcpy(keycmd, dest);
						break;
						}
					}
				}
			}
		*d ++ = *p ++;
		}
	if ( d != dest && wc == 2 ) {
		*d = '\0';
		strcpy(keycmd, dest);
		wc ++;
		}

	if ( wc > 1 ) {
		// convert keynames from C-X or ^X to CTRL, A-X or M-X to ALT, S-X to shift,
		// S-C-A combinations are allowed but not by ncurses (man curs_getch)
		int key = nc_getkeycode(keycode);

		// if wc == 2 delete nodes of the keycode
		if ( wc == 2 )
			nc_delkey(kmap, key);

		// if wc == 3 assign keycode to keycmd
		if ( wc == 3 ) {
			for ( int i = 0; keyfunc[i].keyword; i ++ ) {
				if ( strcasecmp(keyfunc[i].keyword, keycmd) == 0 ) {
					nc_addkey(kmap, keyfunc[i].id, key);
					break;
					}
				}
			}
		}
	m_free(src);
	}

// === configuration & interpreter ==========================================
// === color ================================================================
//
int digit2i(char c) {
	if ( c >= 'a' )	return 10 + (c - 'a');
	if ( c >= 'A' )	return 10 + (c - 'A');
	return c - '0';
	}

//
int isodigit(char c) {
	return (c >= '0' && c <= '7');
	}

//
int getnum(const char *src) {
	const char *p = src;
	bool hex = false, oct = false;
	int n = 0;
	
	while ( isblank(*p) ) p ++;
	if ( *p == '0' ) {
		p ++;
		if ( *p == 'x' ) { p ++; hex = true; }
		else oct = true;
		}

	if ( hex ) {
		while ( isxdigit(*p) ) {
			n = (n << 4) | digit2i(*p);
			p ++;
			}
		}
	else if ( oct ) {
		while ( isodigit(*p) ) {
			n = (n << 3) | digit2i(*p);
			p ++;
			}
		}
	else {
		while ( isdigit(*p) ) {
			n = n * 10 + digit2i(*p);
			p ++;
			}
		}
	return n;
	}

// color normal
void color_setp(const char *pars) {
	const char *p = pars;
	static struct { const char *key; int *intptr; } ct[] = {
		{ "normal", &clr_normal },
		{ "text",   &clr_text },
		{ "status", &clr_status },
		{ "select", &clr_select },
		{ "key",    &clr_status_key },
		{ "code",   &clr_code },
		{ "bold",   &clr_bold },
		{ "hide",   &clr_hide },
		{ NULL,     NULL } };

	while ( isblank(*p) ) p ++;
	for ( int i = 0; ct[i].key; i ++ ) {
		if ( strncmp(p, ct[i].key, strlen(ct[i].key)) == 0 ) {
			*(ct[i].intptr) = getnum(p + strlen(ct[i].key));
			break;
			}
		}
	}

// === configuration & interpreter ==========================================
// === rules ================================================================

// rule view *.[0-9] man %f
// rule view *.man   man %f
// rule view *.md    bat %f
// rule view *.txt   less %f
// rule view *.pdf   okular %f
// rule edit *       $EDITOR %f
typedef struct { int code; char pattern[PATH_MAX], command[LINE_MAX]; } rule_t;
static list_t *rules;

// add rule to list
void rule_add(const char *pars) {
	char	*destp, pattern[PATH_MAX];
	int		action = 0;
	const char *p = pars;

	while ( isblank(*p) ) p ++;
	switch ( *p ) {
	case 'v': action = 'v'; break;
	case 'e': action = 'e'; break;
	default: return; }
	while ( isgraph(*p) ) p ++;
	if ( *p ) {
		while ( isblank(*p) ) p ++;
		if ( *p ) {
			destp = pattern;
			while ( isgraph(*p) ) *destp ++ = *p ++;
			*destp = '\0';
			if ( *p ) {
				while ( isblank(*p) ) p ++;
				if ( *p ) {
					rule_t	*rule = (rule_t *) m_alloc(sizeof(rule_t));
					rule->code = action;
					strcpy(rule->pattern, pattern);
					strcpy(rule->command, p);
					list_add(rules, rule, sizeof(rule_t));
					}
				}
			}
		}
	}

//
int note_shell(const char *precmd, const char *files) {
	const char *p = precmd, *s;
	char dest[LINE_MAX], *d;
	bool sq = false, dq = false;
	
	setenv("NOTESDIR", ndir, 1);
	setenv("NOTESFILES", files, 1);
	d = dest;
	while ( *p ) {
		if ( *p == '\'' )
			sq = !sq;
		else if ( !sq ) {
			if ( *p == '\"' )
				dq = !dq;
			else {
				if ( *p == '%' ) {
					switch ( *(p+1) ) {
					case '%': // %%
						*d ++ = '%';
						p += 2;
						continue;
					case 'f': // files
						for ( s = files; *s; *d ++ = *s ++ );
						p += 2;
						continue;
						}
					}
				}
			}
		*d ++ = *p ++;
		}
	*d = '\0';
	return system(dest);
	}

// execute rule for the file 'fn'
bool rule_exec(int action, const char *fn) {
	const char *base;
	size_t root_dir_len = strlen(ndir) + 1;

	list_node_t	*cur = rules->head;
	if ( (base = strrchr(fn, '/')) == NULL )
		return false;
	base ++;
	while ( cur ) {
		rule_t *rule = (rule_t *) cur->data;
		if ( rule->code == action && fnmatch(rule->pattern, base, FNM_PATHNAME | FNM_PERIOD | FNM_GLIBC_EXTRA) == 0 ) {
			char file[PATH_MAX];
			if ( fn[0] == '/' )
				snprintf(file, PATH_MAX, "'%s'", fn + root_dir_len);
			else
				snprintf(file, PATH_MAX, "'%s'", fn);
			note_shell(rule->command, file);
			return true;
			}
		cur = cur->next;
		}
	return false;
	}

// === configuration & interpreter ==========================================

// table of variables
typedef struct { const char *name; char type; void *value; } var_t;
var_t var_table[] = {
	{ "notebook", 's', ndir },
	{ "backupdir", 's', bdir },
	{ "clobber", 's', sclob },
	{ "deftype", 's', default_ftype },
	{ "onstart", 's', onstart_cmd },
	{ "onexit", 's', onexit_cmd },
	{ "pvhead", 'b', &opt_pv_filestat },
	{ NULL, '\0', NULL } };

// table of commands
typedef struct { const char *name; void (*func_p)(const char *); } cmd_t;
cmd_t cmd_table[] = {
	{ "exclude", excl_add_pat },
	{ "rule",  rule_add },
	{ "umenu", umenu_add },
	{ "map",   keymap_add },
	{ "color", color_setp },
	{ NULL, NULL } };

// assign variables
void command_set(int line, const char *variable, const char *value) {
	int n;
	for ( int i = 0; var_table[i].name; i ++ ) {
		if ( strcmp(var_table[i].name, variable) == 0 ) {
			switch ( var_table[i].type ) {
			case 's':
				strcpy((char *)(var_table[i].value), value);
				break;
			case 'i':
			case 'b':
				if ( isdigit(*value) )
					n = atoi(value);
				else {
					if      ( strcasecmp(value, "off") == 0 ) n = 0;
					else if ( strcasecmp(value, "on")  == 0 ) n = 1;
					else if ( strcasecmp(value, "false")  == 0 ) n = 0;
					else if ( strcasecmp(value, "true")  == 0 ) n = 1;
					else n = atoi(value);
					}
				*((int *)(var_table[i].value)) = n;
				break;
			case 'f':
				*((double *)(var_table[i].value)) = atof(value);
				break;
				}
			return;
			}
		}
	fprintf(stderr, "rc(%d): uknown variable [%s]\n", line, variable);
	}

// execute commands
void command_exec(int line, const char *command, const char *parameters) {
	for ( int i = 0; cmd_table[i].name; i ++ ) {
		if ( strcmp(cmd_table[i].name, command) == 0 ) {
			if ( cmd_table[i].func_p )
				cmd_table[i].func_p(parameters);
			return;
			}
		}
	fprintf(stderr, "rc(%d): uknown command [%s]\n", line, command);
	}

// parse string line
void parse(int line, const char *source) {
	char name[LINE_MAX], *d = name;
	const char *p = source;
	while ( isblank(*p) ) p ++;
	if ( *p && *p != '#' ) {
		while ( *p == '_' || isalnum(*p) )
			*d ++ = *p ++;
		*d = '\0';
		while ( isblank(*p) ) p ++;
		if ( *p == '=' ) {
			p ++;
			while ( isblank(*p) ) p ++;
			command_set(line, name, p);
			}
		else
			command_exec(line, name, p);
		}
	}

// read configuration file
void read_conf(const char *rc) {
	int line = 0;
	
	if ( access(rc, R_OK) == 0 ) {
		char buf[LINE_MAX];
		FILE *fp = fopen(rc, "r");
		if ( fp ) {
			while ( fgets(buf, LINE_MAX, fp) ) {
				line ++;
				rtrim(buf);
				parse(line, buf);
				}
			fclose(fp);
			}
		}
	}

// === notes ================================================================

typedef struct {
	char	file[PATH_MAX];		// full path filename
	char	name[NAME_MAX];		// name of note (basename)
	char	section[NAME_MAX];	// section
	char	ftype[NAME_MAX];	// file type
	struct stat st;				// just useless data for info
	} note_t;
list_t	*notes, *sections;

// copy file
bool copy_file(const char *src, const char *trg) {
	FILE	*inp, *outp, *logf = stderr;
	char	buf[LINE_MAX], *p;
	size_t	bytes;
	
//	logf = fopen("/tmp/notes.log", "a");
//	fprintf(logf, "copy_file(\"%s\", \"%s\")\n", src, trg);
	
	if ( (inp = fopen(src, "r")) == NULL ) {
		fprintf(logf, "%s: errno %d: %s\n", src, errno, strerror(errno));
		return false;
		}
	if ( (p = strrchr(trg, '/')) != NULL ) {
		char *dd = strdup(trg);
		dd[p - trg] = '\0';
		if ( access(dd, W_OK) != 0 ) { // new section ?
			if ( mkdir(dd, 0755) != 0 ) {
				fprintf(logf, "%s: errno %d: %s (mkdir [%s])\n", trg, errno, strerror(errno), dd);
				fclose(inp);
				m_free(dd);
				return false;
				}
			}
		m_free(dd);
		}
	if ( (outp = fopen(trg, "w")) == NULL ) {
		fprintf(logf, "%s: errno %d: %s\n", trg, errno, strerror(errno));
		fclose(inp);
		return false;
		}
	
	// copy
	while ( !feof(inp) ) {
		if ( (bytes = f_read(buf, 1, sizeof(buf), inp)) > 0 )
			f_write(buf, 1, bytes, outp);
		}
	
	// close
	fclose(outp);
	fclose(inp);
	return true;
	}

// backup note-file
bool note_backup(const note_t *note) {
	char	bckf[PATH_MAX];

	if ( strlen(bdir) ) {
		if ( strlen(note->section) )
			snprintf(bckf, PATH_MAX, "%s/%s/%s", bdir, note->section, note->name);
		else
			snprintf(bckf, PATH_MAX, "%s/%s", bdir, note->name);
		return copy_file(note->file, bckf);
		}
	return true;
	}

// prints information about the note
void note_pl(const note_t *note) {
	if ( opt_flags & OPT_FILES )
		printf("%s\n", note->file);
	else {
		size_t	seclen = 0;
		for ( list_node_t *cur = sections->head; cur; cur = cur->next )
			seclen = MAX(seclen, strlen((const char *) cur->data));
		printf("%-*s (%-3s) - %s\n", seclen, note->section, note->ftype, note->name);
		}
	}

// simple print (mode --print) of a note
void note_print(const note_t *note) {
	FILE *fp;
	char buf[LINE_MAX];
	
	printf("=== %s ===\n", note->name);
	if ( (fp = fopen(note->file, "rt")) != NULL ) {
		while ( fgets(buf, LINE_MAX, fp) )
			printf("%s", buf);
		fclose(fp);
		}
	else
		fprintf(stderr, "errno %d: %s\n", errno, strerror(errno));
	}

// delete a note
bool note_delete(const note_t *note) {
	note_backup(note);
	return (remove(note->file) == 0);
	}

// check filename to add in results list
bool dirwalk_checkfn(const char *fn) {
	list_node_t *cur;
	
    if ( strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0 )
		return false;
	for ( cur = exclude->head; cur; cur = cur->next ) {
		if ( fnmatch((const char *) cur->data, fn, FNM_PATHNAME | FNM_PERIOD | FNM_GLIBC_EXTRA | FNM_CASEFOLD) == 0 )
			return false;
		}
	return true;
	}

// walk throu subdirs to collect notes
void dirwalk(const char *name) {
	DIR *dir;
	struct dirent *entry;
    char path[PATH_MAX];
	size_t root_dir_len = strlen(ndir) + 1;

	if ( !(dir = opendir(name)) )	return;
	while ( (entry = readdir(dir)) != NULL ) {
		if ( !dirwalk_checkfn(entry->d_name) )
			continue;
		snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
		if ( entry->d_type == DT_DIR ) 
			dirwalk(path);
		else {
			note_t *note = (note_t *) m_alloc(sizeof(note_t));
			char	buf[PATH_MAX], *p, *e;
			strcpy(note->file, path);
			strcpy(buf, path + root_dir_len);
			if ( (e = strrchr(buf, '.')) != NULL ) {
				*e = '\0';
				strcpy(note->ftype, e + 1);
				}
			if ( (p = strrchr(buf, '/')) != NULL ) {
				*p = '\0';
				strcpy(note->section, buf);
				strcpy(note->name, p + 1);
				}
			else {
				note->section[0] = '\0';
				strcpy(note->name, buf);
				}
			if ( strlen(current_filter) == 0 || fnmatch(current_filter, note->name, FNM_PATHNAME | FNM_PERIOD | FNM_GLIBC_EXTRA | FNM_CASEFOLD) == 0 ) {
				stat(note->file, &note->st);
				list_add(notes, note, sizeof(note_t));
				if ( list_findstr(sections, note->section) == NULL )
					list_addstr(sections, note->section);
				}
			else
				m_free(note);
			}
		}
	closedir(dir);
	}

// copy contents of file to output
bool print_file_to(const char *file, FILE *output) {
	FILE	*input;
	char	buf[LINE_MAX];
	size_t	bytes;
	
	if ( (input = (file) ? fopen(file, "r") : stdin) != NULL ) {	
		while ( !feof(input) ) {
			if ( (bytes = f_read(buf, 1, sizeof(buf), input)) > 0 )
				f_write(buf, 1, bytes, output);
			}
		if ( file ) fclose(input);
		return true;
		}
	else 
		fprintf(stderr, "%s: errno %d: %s\n", file, errno, strerror(errno));
	return false;
	}

// expand envirnment variables
void vexpand(char *buf) {
	wordexp_t p;
	
	if ( wordexp(buf, &p, 0) == 0 ) {
		strcpy(buf, "");
		for ( int i = 0; i < p.we_wordc; i ++ )
			strcat(buf, p.we_wordv[i]);
		wordfree(&p);
		}
	}

//
void normalize_section_name(char *section) {
	list_node_t *cur;
	note_t		*note;

	for ( cur = notes->head; cur; cur = cur->next ) {
		note = (note_t *) cur->data;
		if ( strcasecmp(note->section, section) == 0 ) {
			strcpy(section, note->section);
			break;
			}
		}
	}

// if section does not exists, creates it
void make_section(const char *sec) {
	char dest[PATH_MAX];

	if ( strlen(sec) ) {
		snprintf(dest, PATH_MAX, "%s/%s", ndir, sec);
		if ( access(dest, X_OK) != 0 ) { // new section ?
			if ( mkdir(dest, 0700) != 0 ) {
				fprintf(stderr, "mkdir(%s): errno %d: %s\n", dest, errno, strerror(errno));
				exit(EXIT_FAILURE);
				}
			}
		}
	}

// create a note node
note_t*	make_note(const char *name, const char *defsec, int flags) {
	note_t *note = (note_t *) m_alloc(sizeof(note_t));
	FILE *fp;
	const char *p;

	if ( (p = strrchr(name, '/')) != NULL ) {
		strcpy(note->name, p+1);
		strncpy(note->section, name, p - name);
		note->section[p - name] = '\0';
		normalize_section_name(note->section);
		make_section(note->section);
		snprintf(note->file, PATH_MAX, "%s/%s/%s", ndir, note->section, note->name);
		}
	else {
		strcpy(note->name, name);
		if ( defsec && strlen(defsec) ) {
			strcpy(note->section, defsec);
			normalize_section_name(note->section);
			make_section(note->section);
			snprintf(note->file, PATH_MAX, "%s/%s/%s", ndir, note->section, note->name);
			}
		else {
			strcpy(note->section, "");
			snprintf(note->file, PATH_MAX, "%s/%s", ndir, note->name);
			}
		}
	
	if ( strrchr(name, '.') == NULL ) { // if no file extension specified
		strcat(note->file, ".");
		strcat(note->file, default_ftype);
		}
	
	// create the file
	if ( flags & 0x01 ) { // create file
		if ( (opt_flags & OPT_ADD) && !(opt_flags & OPT_NOCLOB) && !(opt_flags & OPT_APPD) ) {
			if ( access(note->file, F_OK) == 0 )
				return NULL;
			}
		if ( (fp = fopen(note->file, "wt")) != NULL )
			fclose(fp);
		else {
			m_free(note);
			note = NULL;
			}
		}
	return note;
	}

// === explorer =============================================================
static note_t **t_notes;
static int	t_notes_count;
static list_t	*tagged;
static WINDOW	*w_lst, *w_prv, *w_inf;
typedef enum { ex_nav, ex_search } ex_mode_t;

// short date
const char *sdate(const time_t *t, char *buf) {
	struct tm *tmp;
	
	tmp = localtime(t);
	if ( tmp ) 
		strftime(buf, 128, "%F %H:%M", tmp);
	else
		strcpy(buf, "* error *");
	return buf;
	}

// get a string value
bool ex_input(char *buf, const char *prompt_fmt, ...) {
	char	prompt[LINE_MAX];

	va_list ap;
	va_start(ap, prompt_fmt);
	vsnprintf(prompt, LINE_MAX, prompt_fmt, ap);
	va_end(ap);
	
	mvhline(getmaxy(stdscr) - 3, 0, ACS_HLINE, getmaxx(stdscr));
	move(getmaxy(stdscr) - 2, 0); clrtoeol();
	printw("  %s", prompt);
	move(getmaxy(stdscr) - 1, 0); clrtoeol();
	printw("> ");
	return nc_editstr(buf, getmaxx(stdscr)-4);
	}

// print status line
static const char *vrt_ln = "┃";
void ex_status_line(const char *fmt, ...) {
	char	msg[LINE_MAX];
	int short pair;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, LINE_MAX, fmt, ap);
	va_end(ap);
	
	nc_setvgacolor(w_inf, clr_status & 0xf, clr_status >> 4);
	wattr_get(w_inf, NULL, &pair, NULL);
	wbkgdset(w_inf, COLOR_PAIR(pair));
	werase(w_inf);
	mvwhline(w_inf, 0, 0, ' ', getmaxx(w_inf));
	nc_wprintf(w_inf, "%6d %s %s", list_count(notes), vrt_ln, msg);
	wrefresh(w_inf);
	}

// display list of notes (list window)
void ex_print_list(int offset, int pos) {
	int		y = 0, lines = getmaxy(w_lst);
	int short pair;
	
	nc_setvgacolor(w_lst, clr_normal & 0xf, clr_normal >> 4);
	wattr_get(w_lst, NULL, &pair, NULL);
	wbkgdset(w_lst, COLOR_PAIR(pair));
	werase(w_lst);
	if ( t_notes_count ) {
		for ( int i = offset; i < t_notes_count && i < offset + lines; i ++, y ++ ) {
			if ( pos == i )
				nc_setvgacolor(w_lst, clr_select & 0xf, clr_select >> 4);	
			mvwhline(w_lst, y, 0, ' ', getmaxx(w_lst));
			if ( strlen(t_notes[i]->section) ) {
				int l = u8width(t_notes[i]->section);
//				wattron(w_lst, A_DIM);
				mvwprintw(w_lst, y, getmaxx(w_lst)-l, "%s", t_notes[i]->section);
//				wattroff(w_lst, A_DIM);
				}
			mvwprintw(w_lst, y, 0, "%c%s ",
				((list_findptr(tagged, t_notes[i]) ) ? '+' : ' '), t_notes[i]->name);
			if ( pos == i )
				nc_setvgacolor(w_lst, clr_normal & 0xf, clr_normal >> 4);	
			}
		}
	else
		mvwprintw(w_lst, 0, 0, "* No notes found! *");
	wrefresh(w_lst);
	}

// display the contents of the note (preview window)
void ex_print_note(const note_t *note) {
	FILE	*fp;
	char	buf[LINE_MAX];
	bool	inside_code = false;
	int short pair;
	
	nc_setvgacolor(w_prv, clr_text & 0xf, clr_text >> 4);
	wattr_get(w_prv, NULL, &pair, NULL);
	wbkgdset(w_prv, COLOR_PAIR(pair));
	werase(w_prv);
	wmove(w_prv, 0, 0);
	if ( note ) {
		if ( opt_pv_filestat ) {
			nc_wprintf(w_prv, "Name: $B%s$b", note->name);
			if ( strlen(note->section) )
				nc_wprintf(w_prv, ", Section: $B%s$b", note->section);
			nc_wprintf(w_prv, "\nFile: $B%s$b\n", note->file);
			nc_wprintf(w_prv, "Date: $B%s$b\n", sdate(&note->st.st_mtime, buf));
			nc_wprintf(w_prv, "Stat: $B%6d$b bytes, mode $B0%o$b, owner $B%d$b:$B%d$b\n",
				note->st.st_size, note->st.st_mode & 0777, note->st.st_uid, note->st.st_gid);
			for ( int i = 0; i < getmaxx(w_prv); i ++ ) wprintw(w_prv, "─");
			}
		if ( (fp = fopen(note->file, "rt")) != NULL ) {
			while ( fgets(buf, LINE_MAX, fp) ) {
				if ( strcmp(note->ftype, "md") == 0 ) {
					int		i, color = clr_text;
					
					if ( buf[strlen(buf)-1] == '\n' )
						buf[strlen(buf)-1] = '\0';
					
					switch ( buf[0] ) {
					case '#': color = ( inside_code ) ? clr_code : clr_bold; break;
					case '`': if ( buf[1] == '`' && buf[2] == '`' ) { inside_code = !inside_code; color = clr_hide; } break;
					case '\t': color = clr_code; break;
					default: 
						color = ( inside_code ) ? clr_code : clr_text;
						}
//					nc_setpair(w_prv, color);
					nc_setvgacolor(w_prv, color & 0xf, color >> 4);
					wprintw(w_prv, "%s", buf);
					for ( i = getcurx(w_prv); i < getmaxx(w_prv) ; i ++ ) wprintw(w_prv, " ");
//					nc_unsetpair(w_prv, color);
					nc_setvgacolor(w_prv, clr_text & 0xf, clr_text >> 4);
					}
				else
//					nc_wprintf(w_prv, "%s", buf);
					wprintw(w_prv, "%s", buf);
				if ( getcury(w_prv) >= (getmaxy(w_prv)-1) )
					break;
				}
			fclose(fp);
			}
		}
	wrefresh(w_prv);
	}

// qsort callback
static int t_notes_cmp(const void *va, const void *vb) {
	const note_t **a = (const note_t **) va;
	const note_t **b = (const note_t **) vb;
	return strcasecmp((*a)->name, (*b)->name);
	}

// qsort callback
static int t_str_cmp(const void *va, const void *vb) {
	const char **a = (const char **) va;
	const char **b = (const char **) vb;
	return strcasecmp(*a, *b);
	}

// help
static char *ex_help_s = "&? help, &quit, &view, &edit, &rename, &delete, &new, &/ search, &section, &tag, &untag all";
static char ex_help[LINE_MAX];
static char *ex_help_long = "\
?, F1  ... Help. This window.\n\
q, ^Q  ... Quit. Terminates the program.\n\
ENTER  ... Display the current note with $PAGER.\n\
v, F3  ... View. Display the current or the tagged notes[1] with $PAGER.\n\
e, F4  ... Edit. Edit the current or the tagged notes[1] with the $EDITOR.\n\
r  F6  ... Rename. Renames the current note.\n\
d, DEL ... Delete. Deletes the current or the tagged notes[1].\n\
n      ... New. Invokes the $EDITOR with a new file; you will have to save it.\n\
a      ... Add. Creates a new empty note.\n\
s      ... Select Section.\n\
c      ... Change section. Changes the section of the current or the tagged notes[1].\n\
t, INS ... Tag/Untag current note.\n\
u, F9  ... Untag all.\n\
/, F7  ... Search[2].\n\
m, F2  ... Menu. Open the user-defined menu.\n\
!, x, F10  Execute something with current/tagged notes[1].\n\
f      ... Open the notes directory with the file manager.\n\
F5     ... Rebuild & redraw the list.\n\
\n\
Notes:\n\
[1] The tagged notes if there are any, otherwise the current note.\n\
[2] The application uses the same pattern as the shell with KSH extensions.\n\
    (see `man fnmatch`)\n\
";
//f      ... Set Filter[1].\n

// build the table with notes
bool ex_build() {
	if ( notes )
		list_clear(notes);
	if ( strlen(current_section) ) {
		char path[PATH_MAX];
		snprintf(path, PATH_MAX, "%s/%s", ndir, current_section);
		dirwalk(path);
		}
	else	
		dirwalk(ndir);
	t_notes = (note_t **) list_to_table(notes);
	t_notes_count = list_count(notes);
	if ( t_notes_count == 0 )
		return false;
	qsort(t_notes, t_notes_count, sizeof(note_t*), t_notes_cmp);
	return true;
	}

// rebuild the table with notes
bool ex_rebuild() {
	m_free(t_notes);
	return ex_build();
	}

// (re)build explorer windows
void ex_build_windows() {
	int cols3 = getmaxx(stdscr) / 3;
	int lines = getmaxy(stdscr) - 1;
	int short pair;
	
	if ( w_lst ) delwin(w_lst);
	if ( w_prv ) delwin(w_prv);
	if ( w_inf ) delwin(w_inf);

	nc_setvgacolor(stdscr, clr_normal & 0xf, clr_normal >> 4);
	wattr_get(stdscr, NULL, &pair, NULL);
	wbkgdset(stdscr, COLOR_PAIR(pair));
	werase(stdscr);
//	mvvline(0, cols3, ' ', lines);	
	
	w_lst = newwin(lines, cols3, 0, 0);
	w_prv = newwin(lines, (getmaxx(stdscr) - cols3) - 1, 0, cols3+1);
	w_inf = newwin(1, getmaxx(stdscr), lines, 0);

	keypad(w_lst, TRUE);
	keypad(w_prv, TRUE);
	keypad(w_inf, TRUE);
	refresh();
	}

// find a note by name, returns the index in the t_notes
int	ex_find(const char *name) {
	int		i;
	
	for ( i = 0; i < t_notes_count; i ++ ) {
		if ( strcmp(t_notes[i]->name, name) == 0 ) 
			return i;
		}
	return -1;
	}

//
void ex_colorize(char *dest, const char *src) {
	const char *p = src, *e;
	char *d = dest;
	char cstart[16], cend[16];
	
	if ( has_colors() ) { 
		sprintf(cstart, "$p%02x", clr_status_key); 
		sprintf(cend, "$p%02x", clr_status); 
		}
	else { 
		strcpy(cstart, "$U");
		strcpy(cend, "$u"); 
		}
	
	for ( p = src; *p; p ++ ) {
		if ( *p == '&' ) {
			p ++;
			if ( *p != '&' ) {
				for ( e = cstart; *e; e ++ ) *d ++ = *e;
				*d ++ = *p;
				for ( e = cend; *e; e ++ ) *d ++ = *e;
				}
			else
				*d ++ = *p;
			}
		else
			*d ++ = *p;
		}
	*d = '\0';
	}

bool ex_select_section(char *result, const char *default_value) {
	int i = 0, r = false;
	char **table = (char **) list_to_table(sections);
	qsort(table, list_count(sections), sizeof(char*), t_str_cmp);

	if ( default_value ) {
		for ( i = 0; table[i]; i ++ )
			if ( strcasecmp(table[i], default_value) == 0 )
				break;
		}
	table[0] = "(all)";
	if ( (i = nc_listbox("Select Section", (const char **) table, i)) >= 0 ) {
		if ( i == 0 )
			result[0] = '\0';
		else
			strcpy(result, table[i]);
		r = true;
		}
	m_free(table);
	return r;
	}

//
void vstrcat(char *buf, ...) {
	va_list ap;
	const char *s;

	va_start(ap, buf);
	while ( (s = va_arg(ap, const char *)) != NULL )
		strcat(buf, s);
	va_end(ap);
	}

//
int ex_tagged_shell(const char *cmd, list_t *tagged) {
	char files[LINE_MAX];
	const char *p;
	size_t root_dir_len = strlen(ndir) + 1;

	files[0] = '\0';
	for ( list_node_t *cur = tagged->head; cur; cur = cur->next ) {
		p = ((note_t *) (cur->data))->file;
		if ( cur != tagged->head ) // add separator
			strcat(files, " ");
		vstrcat(files, "'", p + root_dir_len, "'", NULL);
		}
	return note_shell(cmd, files);
	}

//
#define ex_presh()		{ clear(); refresh(); def_prog_mode(); endwin(); }
#define ex_refresh()	{ keep_status = 1; clear(); ungetch(12); }
#define fix_offset()	{ \
	if ( pos < 0 ) pos = 0; \
	if ( pos >= t_notes_count ) pos = t_notes_count - 1; \
	if ( pos > offset + lines ) offset = pos - lines; \
	if ( offset > pos ) offset = pos; \
	if ( offset < 0 ) offset = 0; }
#define INF_PREFIX	10

// TUI
void explorer() {
	bool	exitf = false;
	int		ch, pf, offset = 0, pos = 0;
	int		lines, keep_status = 0;
	char	buf[LINE_MAX];
	char	prompt[LINE_MAX];
	char	status[LINE_MAX];
	char	search[NAME_MAX];
	wchar_t	wsearch[NAME_MAX];
	int		spos, slen, i, maxlen;
	bool	insert = true;
	ex_mode_t mode = ex_nav;
	const char *term;
	
	if ( strlen(onstart_cmd) )
		system(onstart_cmd);

	ex_build();
	tagged = list_create();

	nc_init();
	raw();
	set_default_keymap();
	nc_addkey("input", KEY_CANCEL, 3);
	if ( (term = getenv("TERM")) != NULL ) {
		if ( strncmp(term, "xterm", 5) == 0 ) {
			define_key("\033[1~", KEY_HOME);
			define_key("\033[4~", KEY_END);
			}
		}
	ex_build_windows();
	ex_colorize(ex_help, ex_help_s);
	
	status[0]  = '\0';
	search[0]  = '\0';
	wsearch[0] = L'\0';
	spos = slen = 0;
	maxlen = getmaxx(w_inf) - INF_PREFIX;
	do {
		lines = getmaxy(stdscr) - 2;
		fix_offset();
		ex_print_list(offset, pos);
		if ( t_notes_count )
			ex_print_note(t_notes[pos]);
		
		if ( mode == ex_search ) {
			ex_status_line("%s", search);
			wmove(w_inf, 0, spos+(INF_PREFIX-1));
			}
		else if ( status[0] == '\0' )
			ex_status_line("%s", ex_help);
		else {
			ex_status_line("%s", status);
			if ( keep_status )
				keep_status --;
			else
				status[0] = '\0';
			}
		
		// read key
		ch = wgetch(w_inf);

		// input string mode
		if ( mode == ex_search ) {
			pf = nc_getprg("input", ch);
			wchar_t wch = (wchar_t) ch;
			if ( u8ischar(ch) ) {
				char mbs[7];
				int keylen = u8csize(ch);
				mbs[0] = ch;
				for ( i = 1; i < keylen; i ++ )
					mbs[i] = wgetch(w_inf);
				mbs[keylen] = '\0';
				wch = u8towc(mbs);
				}
			switch ( KPRG_KEY(pf) ) {
			case KEY_CANCEL:
				search[0] = '\0';
				wsearch[0] = L'\0';
				spos = 0;
				mode = ex_nav;
				curs_set(0);
				strcpy(current_filter, "");
				ex_rebuild();
				continue;
			case KEY_ENTER:	// enter -> view current note
				mode = ex_nav;
				sprintf(current_filter, "*%s*", search);
				curs_set(0);
				ex_rebuild();
				ex_refresh();
				continue;
			case KEY_LEFT:	if ( spos ) spos --; break;
			case KEY_RIGHT:	if ( search[spos] ) spos ++; break;
			case '':
			case KEY_HOME:	spos = 0; break;
			case '':
			case KEY_END:	spos = slen; break;
			case KEY_BACKSPACE:
				if ( spos ) {
					spos --; slen --;
					for ( i = spos; wsearch[i]; i ++ )
						wsearch[i] = wsearch[i+1];
					wsearch[slen] = '\0';
					}
				break;
			case KEY_DC:
				if ( wsearch[spos] ) {
					for ( i = spos; wsearch[i]; i ++ )
						wsearch[i] = wsearch[i+1];
					slen --;
					}
				break;
			case KEY_IC:	
				insert = !insert;
				curs_set((insert)?1:2);
				break;
			default:
				if ( slen < maxlen ) {
					if ( insert ) {
						for ( i = slen; i >= spos; i -- )
							wsearch[i+1] = wsearch[i];
						wsearch[spos] = wch;
						wsearch[++ slen] = '\0';
						}
					else {
						wsearch[spos] = wch;
						if ( spos == slen )
							wsearch[++ slen] = '\0';
						}
					spos ++;
					}
				}
			
			// rebuild everything
			u8cpytostr(search, wsearch);
			sprintf(current_filter, "*%s*", search);
			ex_rebuild();
			}

		// navigation mode
		else if ( mode == ex_nav ) {
			pf = nc_getprg("nav", ch);
//			fprintf(stderr, "%04X %04X %d\n", pf, ch, ch);
			switch ( KPRG_KEY(pf) ) {
			case KEY_RESIZE:
			case KEY_REFRESH:
				ex_build_windows();
				mvvline(0, getmaxx(stdscr) / 3, ' ', getmaxy(stdscr) - 1);
				maxlen = getmaxx(w_inf) - INF_PREFIX;
				break;
			case KEY_EXIT:
				exitf = true;
				break;
			case KEY_HELP:
				nc_view("Help", ex_help_long);
				ex_refresh();
				break;
			case KEY_UP:
				if ( t_notes_count )
					{ if ( pos ) pos --; }
				else
					offset = pos = 0;
				break;
			case KEY_DOWN:
				if ( t_notes_count )
					{ if ( pos < t_notes_count - 1 ) pos ++; }
				else
					offset = pos = 0;
				break;
			case KEY_HOME:
				offset = pos = 0;
				break;
			case KEY_END:
				if ( t_notes_count )
					pos = t_notes_count - 1;
				break;
			case KEY_PGUP:
			case KEY_PGDN:
				if ( t_notes_count ) {
					int dir = (pf == KEY_PRG(KEY_PGDN)) ? 1 : -1;
					pos += lines * dir;
					offset += lines * dir;
					if ( pos >= t_notes_count )
						offset = pos = t_notes_count - 1;
					if ( pos < 0 )
						offset = pos = 0;
					}
				break;
			case KEY_ENTER:	// enter -> view current note
				if ( t_notes_count ) {
					ex_presh();
					rule_exec('v', t_notes[pos]->file);
					ex_refresh();
					}
				break;
			case 'u': // untag all
				list_clear(tagged);
				sprintf(status, "untag all.");
				ex_refresh();
				break;
			case KEY_MARK: // tag/untag
				if ( t_notes_count ) {
					list_node_t *node = list_findptr(tagged, t_notes[pos]);
					if ( node )
						list_delete(tagged, node);
					else {
						list_addptr(tagged, t_notes[pos]);
						ungetch(KEY_DOWN);
						}
					}
				break;
			case 'v': // view in pager
				if ( t_notes_count ) {
					ex_presh();
					if ( list_count(tagged) )
						ex_tagged_shell("$PAGER %f", tagged);
					else
						rule_exec('v', t_notes[pos]->file);
					ex_refresh();
					}
				break;
	//		case KEY_F(8): // filter
	//			strcpy(buf, current_filter);
	//			if ( ex_input(buf, "Set filter (current filter: '%s')", current_filter) ) {
	//				strcpy(current_filter, buf);
	//				ex_rebuild();
	//				offset = pos = 0;
	//				}
	//			ex_refresh();
	//			break;
			case KEY_FIND: // search
				search[0] = '\0';
				wsearch[0] = L'\0';
				spos = slen = 0;
				mode = ex_search;
				curs_set(1);
				break;
			case 'e': // edit
				if ( t_notes_count ) {
					ex_presh();
					if ( list_count(tagged) )
						ex_tagged_shell("$EDITOR %f", tagged);
					else
						rule_exec('e', t_notes[pos]->file);
					ex_refresh();
					}
				break;
			case 's': // select current section
				if ( ex_select_section(current_section, current_section) ) {
					offset = pos = 0;
					ex_rebuild();
					}
				ex_refresh();
				break;
			case 'c': // change section
				if ( t_notes_count ) {
					strcpy(buf, "");
					if ( ex_input(buf, "Enter the new section (enter ? for listbox)") && strlen(buf) ) {
						char *new_section = NULL, *p;
						if ( strchr(buf, '?') != NULL ) {
							if ( ex_select_section(buf, NULL) )
								new_section = strdup(buf);
							}
						else
							new_section = strdup(buf);
						
						// check file-name
						if ( new_section ) {
							bool err = false;
							if ( strcmp(new_section,  ".") == 0 ) err = true;
							if ( strcmp(new_section, "..") == 0 ) err = true;
							for ( p = new_section; *p; p ++ ) {
								if ( strchr("/?*\\<>|", *p) != NULL ) {
									sprintf(status, "Illegal character (%c)", *p);
									err = true;
									break;
									}
								}
							if ( err ) {
								m_free(new_section);
								new_section = NULL;
								}
							}
						
						if ( new_section ) { // name its ok, continue
							normalize_section_name(new_section);
							make_section(new_section);
								
							// add the current element to tagged list
							if ( !list_count(tagged) )
								list_addptr(tagged, t_notes[pos]);
							
							// move files
							int succ = 0, fail = 0;
							for ( list_node_t *cur = tagged->head; cur; cur = cur->next ) {
								note_t *cn = (note_t *) cur->data;
								note_backup(cn);
								note_t *nn = make_note(cn->name, new_section, 0);
								if ( rename(cn->file, nn->file) != 0 ) {
									sprintf(status, "move failed");
									fail ++;
									}
								else
									succ ++;
								m_free(nn);
								}

							// report
							if ( succ == 1 ) sprintf(status, "one note moved%c", ((fail)?';':'.'));
							else sprintf(status, "%d notes moved%c", succ, ((fail)?';':'.'));
							if ( fail ) sprintf(status+strlen(status), " %d failed.", fail);

							// cleanup
							list_clear(tagged);
							m_free(new_section);
							}
						}
					
					ex_rebuild();
					ex_refresh();
					}
				break;
	/*
			case KEY_REFRESH:
				ex_rebuild();
				sprintf(status, "rebuilded.");
				ex_refresh();
				break;
	*/
			case 'f': // show in filemanager
				{
				char *fmans[] = { "xdg-open", "mc", "thunar", "dolphin", NULL };
				int idx = nc_listbox("File Manager", (const char **) fmans, 0);
				if ( idx > -1 ) {
					ex_presh();
					sprintf(buf, "%s '%s'", fmans[idx], ndir);
					system(buf);
					}
				ex_refresh();
				break;
				}
			case KEY_DC: // delete
				if ( t_notes_count ) {
					strcpy(buf, "");
					if ( list_count(tagged) )
						sprintf(prompt, "Delete all tagged notes ?");
					else
						sprintf(prompt, "Do you want to delete '%s' ?", t_notes[pos]->name);
					
					if ( ex_input(buf, "%s", prompt) && istrue(buf) ) {
						if ( !list_count(tagged) )
							list_addptr(tagged, t_notes[pos]);
						int succ = 0, fail = 0;
						for ( list_node_t *cur = tagged->head; cur; cur = cur->next )
							(note_delete(cur->data)) ? succ ++ : fail ++;
						if ( succ == 1 ) sprintf(status, "one note deleted%c", ((fail)?';':'.'));
						else sprintf(status, "%d notes deleted%c", succ, ((fail)?';':'.'));
						if ( fail ) sprintf(status+strlen(status), " %d failed.", fail);
						
						list_clear(tagged);
						ex_rebuild();
						if ( t_notes_count ) {
							if ( pos >= t_notes_count )
								pos = t_notes_count - 1;
							}
						else
							offset = pos = 0;
						}
					ex_refresh();
					}
				break;
			case 'r':	// rename
				if ( t_notes_count ) {
					strcpy(buf, t_notes[pos]->name);
					if ( ex_input(buf, "Enter the new name ([section/]new-name[.extension])", t_notes[pos]->name)
							&& strlen(buf)
							&& strcmp(buf, t_notes[pos]->name) != 0 ) {
						note_backup(t_notes[pos]);
						note_t *nn = make_note(buf, t_notes[pos]->section, 1);
						if ( !copy_file(t_notes[pos]->file, nn->file) )
							sprintf(status, "copy failed");
						else {
							if ( remove(t_notes[pos]->file) != 0 )
								sprintf(status, "delete old note failed");
							}
						m_free(nn);
						ex_rebuild();
						if ( (pos = ex_find(buf)) == -1 ) pos = 0;
						}
					ex_refresh();
					}
				break;
			case 'm':
				if ( t_notes_count && umenu->head ) {
					int idx;
					umenu_item_t **opts;
					
					opts = (umenu_item_t **) list_to_table(umenu);
					if ( (idx = nc_listbox("User Menu", (const char **) opts, 0)) > -1 ) {
						int tcnt = list_count(tagged);
						if ( !tcnt )
							list_addptr(tagged, t_notes[pos]);
						
						ex_presh();
						ex_tagged_shell(opts[idx]->cmd, tagged);
						printf("\nPress any key to return...\n");
						getch();
						if ( !tcnt )
							list_clear(tagged);
						}
					m_free(opts);
					ex_refresh();
					}
				break;
			case '!':
				if ( t_notes_count ) {
					char	cmd[LINE_MAX];
					strcpy(cmd, "");
					if ( ex_input(cmd, "Enter command (use '%%f' for files)") && strlen(cmd) ) {
						int tcnt = list_count(tagged);
						if ( !tcnt )
							list_addptr(tagged, t_notes[pos]);
						ex_presh();
						ex_tagged_shell(cmd, tagged);
						printf("\nPress any key to return...\n");
						getch();
						if ( !tcnt )
							list_clear(tagged);
						}
					ex_refresh();
					}
				break;
			case 'a':
			case 'n':
				strcpy(buf, "");
				if ( ex_input(buf, "Enter new name ([section/]new-name[.extension])") && strlen(buf) ) {
					note_t *note = make_note(buf, current_section, (ch == KEY_CREATE) ? 0 : 1);
					if ( note ) {
						sprintf(status, "'%s' created", note->name);
						ex_rebuild();
						if ( (pos = ex_find(note->name)) == -1 ) pos = 0;
						if ( ch == 'n' ) { // 'new' key invokes the editor, 'add' key do not
							ex_presh();
							rule_exec('e', note->file);
							}
						m_free(note);
						}
					else
						sprintf(status, "failed: errno (%d) %s", errno, strerror(errno));
					}
				ex_refresh();
				break;
				}
			}
		} while ( !exitf );
	nc_close();
	tagged = list_destroy(tagged);
	m_free(t_notes);
	if ( strlen(onexit_cmd) )
		system(onexit_cmd);
	}

// === main =================================================================

// set by env. variable, if $b exists then a=$b else a=c
#define setevar(a,b,c)	{ const char *e = getenv((b)); strcpy((a), (e)?e:(c)); }

// initialization
void init() {
	exclude = list_create();
	rules = list_create();
	umenu = list_create();
	notes = list_create();
	sections = list_create();
	
	// default values
	strcpy(default_ftype, "txt");
	setevar(home, "HOME", "/tmp");
	setevar(bdir, "BACKUPDIR", "");
	
	if ( getenv("NOTESDIR") )
		strcpy(ndir, getenv("NOTESDIR"));
	else {
		sprintf(ndir, "%s/Nextcloud/Notes", home);
		if ( access(conf, R_OK) != 0 )
			sprintf(ndir, "%s/.notes", home);
		}

	if ( strlen(conf) == 0 ) {
		if ( getenv("XDG_CONFIG_HOME") )
			sprintf(conf, "%s/notes/notesrc", getenv("XDG_CONFIG_HOME"));
		else
			sprintf(conf, "%s/.config/notes/notesrc", home);
		if ( access(conf, R_OK) != 0 )
			sprintf(conf, "%s/.notesrc", home);
		}

	// read config file
	read_conf(conf);
	
	// expand
	vexpand(ndir);
	vexpand(bdir);

	//
	if ( access(ndir, X_OK) != 0 )
		mkdir(ndir, 0700);
	chdir(ndir);
	
	//
	if ( strlen(sclob) ) {
		if ( istrue(sclob) )
			g_globber = true;
		else
			g_globber = false;
		}

	// setting up default pager and editor
	char editor[PATH_MAX], pager[PATH_MAX], buf[PATH_MAX];
	if ( getenv("NOTESPAGER") )
		strcpy(pager, getenv("NOTESPAGER"));
	else if ( getenv("PAGER") )
		strcpy(pager, getenv("PAGER"));
	else
		strcpy(pager, "less");
	
	if ( getenv("NOTESEDITOR") )
		strcpy(editor, getenv("NOTESEDITOR"));
	else if ( getenv("EDITOR") )
		strcpy(editor, getenv("EDITOR"));
	else
		strcpy(editor, "vi");

	snprintf(buf, PATH_MAX, "view * %s %%f", pager);
	rule_add(buf);
	snprintf(buf, PATH_MAX, "edit * %s %%f", editor);
	rule_add(buf);
	}

//
void cleanup() {
	exclude = list_destroy(exclude);
	rules = list_destroy(rules);
	umenu = list_destroy(umenu);
	notes = list_destroy(notes);
	sections = list_destroy(sections);
	}

#define APP_DESCR \
"notes - notes manager"

#define APP_VER "1.5"

static const char *usage = "\
"APP_DESCR"\n\
Usage: notes [mode] [options] [-s section] {note|pattern} [-|file(s)]\n\
\n\
Modes:\n\
    -a, --add      add a new note; use `!' to replace an existing note. \n\
    -a+, --append  append to a note; use `!' to create one if it does not exist.\n\
    -l, --list     list notes ('*' displays all)\n\
    -f, --files    same as list but displays only full pathnames (for scripts)\n\
    -v, --view     sends the note[s] to the $PAGER (see --all)\n\
    -p, --print    display the contents of a note[s] (see --all)\n\
    -e, --edit     load note[s] to $EDITOR (see --all)\n\
    -d, --delete   delete a note\n\
    -r, --rename   rename or move a note\n\
    -c, --rcfile   use this config file\n\
\n\
Options:\n\
    -s, --section  define section\n\
    -a, --all      displays all matching files; use it with -p, -v or -e\n\
    -              input from stdin\n\
\n\
Utilities:\n\
    --onstart      executes the 'onstart' command and returns its exit code\n\
    --onexit       executes the 'onexit' command and returns its exit code\n\
\n\
    -h, --help     this screen\n\
    --version      version and program information\n\
";
//    WIP --cleanup      removes empty sections\n
//    WIP --complete     list notes for scripts (completion code)\n

static const char *verss = "\
notes version "APP_VER"\n\
"APP_DESCR"\n\
\n\
Copyright (C) 2020-2023 Nicholas Christopoulos.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// main()
int main(int argc, char *argv[]) {
	int		i, j, exit_code = EXIT_FAILURE;
	char	*asw = NULL;
	list_t	*args;
	note_t	*note;
	list_node_t *cur_arg = NULL;
	bool	sectionf = false;
	char	tmp[LINE_MAX];

	setlocale(LC_ALL, "");

	// custom rcfile
	strcpy(conf, "");
	for ( i = 1; i < argc; i ++ ) {
		if ( strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--rcfile") == 0 ) {
			if ( i < argc - 1 )
				strcpy(conf, argv[i+1]);
			}
		}
	
	//
	init();
	args = list_create();
	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			if ( argv[i][1] == '\0' ) {     // one minus, read from stdin
				opt_flags |= OPT_STDIN;
				continue; // we finished with this argv
				}

			// check options
			for ( j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'l': opt_flags = OPT_LIST; break;
				case 'e': opt_flags = (opt_flags & OPT_AUTO) ? OPT_EDIT : opt_flags | OPT_EDIT; break;
				case 'v': opt_flags = OPT_VIEW; break;
				case 'p': opt_flags = OPT_VIEW|OPT_PRINT; break;
				case 'f': opt_flags |= OPT_FILES; break;
				case 'a': opt_flags = (opt_flags & OPT_AUTO) ? OPT_ADD : opt_flags | OPT_ALL; break;
				case 'n': opt_flags = OPT_ADD | OPT_EDIT; break;
				case '!': opt_flags |= OPT_NOCLOB; break;
				case 's': asw = current_section; sectionf = true; break;
				case 'r': opt_flags = OPT_MOVE; break;
				case 'd': opt_flags = OPT_DEL; break;
				case '+': opt_flags |= OPT_APPD; break;
				case 'c': break;
				case 'h': puts(usage); return exit_code;
//				case 'v': puts(verss); return exit_code;
				case '-': // -- double minus
					if ( strcmp(argv[i], "--all") == 0 )			{ opt_flags |= OPT_ALL; }
					else if ( strcmp(argv[i], "--rcfile") == 0 )	{ asw = tmp; }
					else if ( strcmp(argv[i], "--add") == 0 )		{ opt_flags = OPT_ADD; }
					else if ( strcmp(argv[i], "--add!") == 0 )		{ opt_flags = OPT_ADD | OPT_NOCLOB; }
					else if ( strcmp(argv[i], "--append") == 0 )	{ opt_flags = OPT_ADD | OPT_APPD; }
					else if ( strcmp(argv[i], "--append!") == 0 )	{ opt_flags = OPT_ADD | OPT_APPD | OPT_NOCLOB; }
					else if ( strcmp(argv[i], "--list") == 0 )		{ opt_flags = OPT_LIST; }
					else if ( strcmp(argv[i], "--view") == 0 )		{ opt_flags = OPT_VIEW; }
					else if ( strcmp(argv[i], "--print") == 0 )		{ opt_flags = OPT_VIEW|OPT_PRINT; }
					else if ( strcmp(argv[i], "--edit") == 0 )		{ opt_flags = (opt_flags & OPT_AUTO) ? OPT_EDIT : opt_flags | OPT_EDIT; }
					else if ( strcmp(argv[i], "--files") == 0 )		{ opt_flags |= OPT_FILES; }
					else if ( strcmp(argv[i], "--delete") == 0 )	{ opt_flags = OPT_DEL; }
					else if ( strcmp(argv[i], "--rename") == 0 )	{ opt_flags = OPT_MOVE; }
					else if ( strcmp(argv[i], "--complete") == 0 )	{ opt_flags = OPT_COMPL; }
					else if ( strcmp(argv[i], "--section") == 0 )	{ asw = current_section; sectionf = true; }
					else if ( strcmp(argv[i], "--help") == 0 )		{ puts(usage); return exit_code; }
					else if ( strcmp(argv[i], "--version") == 0 )	{ puts(verss); return exit_code; }
					else if ( strcmp(argv[i], "--onstart") == 0 )	{ if ( strlen(onstart_cmd) ) return system(onstart_cmd); }
					else if ( strcmp(argv[i], "--onexit") == 0 )	{ if ( strlen(onexit_cmd) ) return system(onexit_cmd); }
					return exit_code;
				default:
					fprintf(stderr, "unknown option [%c]\n", argv[i][j]);
					return exit_code;
					}
				}
			}
		else {
			if ( asw ) { // wait for string
				strcpy(asw, argv[i]);
				asw = NULL;
				}
			else 
				list_addstr(args, argv[i]);
			}
		}

	//
	if ( !g_globber )
		opt_flags |= OPT_NOCLOB;

	// no parameters
	if ( args->head == NULL ) {
		if ( opt_flags & OPT_LIST )
			list_addstr(args, "*");
		else {
			if ( opt_flags & OPT_ADD )		{ printf("usage: notes -a new-note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_APPD )		{ printf("usage: notes -a+ note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_DEL )		{ printf("usage: notes -d note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_MOVE )		{ printf("usage: notes -r note-name new-note-name\n"); exit(EXIT_FAILURE); }
			
			// default action with no parameters: run explorer
			explorer(); 
			return EXIT_SUCCESS; 
			}
		}
	cur_arg = args->head;

	if ( opt_flags & OPT_ADD ) {
		//
		//	create/append note, $1 is the name
		//
		char	*name = (char *) cur_arg->data;
		note_t	*note;
		FILE	*fp;
			
		note = make_note(name, current_section, 0);
		if ( !(opt_flags & OPT_NOCLOB ) ) {
			if ( opt_flags & OPT_APPD ) { // append and clobber
				if ( access(note->file, F_OK) != 0 ) {
					fprintf(stderr, "File '%s' does not exist.\nUse '!' option to create it.\n", note->file);
					return EXIT_FAILURE;
					}
				}
			else { // add and clobber
				if ( access(note->file, F_OK) == 0 ) {
					fprintf(stderr, "File '%s' already exist.\nUse '!' option to replace it.\n", note->file);
					return EXIT_FAILURE;
					}
				}
			}
		
		if ( note ) {
			// create / truncate / open-for-append file
			if ( (fp = fopen(note->file, ((opt_flags & OPT_APPD) ? "a" : "w"))) != NULL ) {
				exit_code = EXIT_SUCCESS;
				cur_arg = cur_arg->next;
				while ( cur_arg ) {
					if ( print_file_to((const char *) cur_arg->data, fp) )
						printf("* '%s' copied *\n", (const char *) cur_arg->data);
					cur_arg = cur_arg->next;
					}
				if ( opt_flags & OPT_STDIN ) // the '-' option used
					print_file_to(NULL, fp);
				fclose(fp);
				if ( opt_flags & OPT_EDIT )  // the '-e' option used
					rule_exec('e', note->file);
				}
			else
				fprintf(stderr, "%s: errno %d: %s\n", note->file, errno, strerror(errno));
			m_free(note);
			}
		else
			fprintf(stderr, "%s: errno %d: %s\n", name, errno, strerror(errno));
		}
	else {
		//
		//	$1 is the note pattern, find note and do .. whatever
		//	
		
		// create list of files
		if ( sectionf ) {
			char path[PATH_MAX];
			snprintf(path, PATH_MAX, "%s/%s", ndir, current_section);
			dirwalk(path);
			}
		else
			dirwalk(ndir);

		// get list of notes according the pattern (argv)
		const char *note_pat = (const char *) cur_arg->data;
		cur_arg = cur_arg->next;
		list_t *res = list_create(); // list of results
		for ( list_node_t *np = notes->head; np; np = np->next ) {
			note = (note_t *) np->data;
			if ( sectionf ) {
				if ( strcmp(current_section, note->section) != 0 )
					continue;
				}
			if ( fnmatch(note_pat, note->name, FNM_PERIOD | FNM_CASEFOLD | FNM_GLIBC_EXTRA) == 0 ) {
				if ( (opt_flags & OPT_LIST) || (opt_flags & OPT_AUTO) || (opt_flags & OPT_FILES) )
					note_pl(note);
				list_addptr(res, note);
				}
			}

		//
		//	'res' has the collected files, now do whatever with them
		//
		size_t res_count = list_count(res);
		if ( !(opt_flags & OPT_FILES) && res_count ) {
			list_node_t *cur = res->head;
			
			exit_code = EXIT_SUCCESS;
			if ( (opt_flags == OPT_AUTO) && res_count == 1 )
				opt_flags |= OPT_VIEW;
			
			while ( cur ) {
				note = (note_t *) cur->data;
				
				if ( (opt_flags & OPT_VIEW) || (opt_flags & OPT_EDIT) ) {
					int action = 'v';
					if ( opt_flags & OPT_EDIT ) {
						action = 'e';
						note_backup(note);
						}
					if ( opt_flags & OPT_PRINT )
						note_print(note);
					else 
						rule_exec(action, note->file);
					if ( (opt_flags & OPT_ALL) == 0 )
						break;
					}
				else if ( opt_flags & OPT_DEL ) {
					if ( note_delete(note) )
						printf("* '%s' deleted *\n", note->name);
					else
						fprintf(stderr, "errno %d: %s\n", errno, strerror(errno));
					}
				else if ( opt_flags & OPT_MOVE ) {
					if ( cur_arg == NULL )
						fprintf(stderr, "usage: notes -r old-name new-name\n");
					else {
						char	new_file[PATH_MAX], *p, *ext;
						char	*arg = (char *) cur_arg->data;
						size_t	root_dir_len = strlen(ndir) + 1;
						
						if ( (ext = strrchr(note->file, '.')) == NULL )
							ext = default_ftype;
						else 
							ext ++;
						strcpy(new_file, note->file);
						new_file[root_dir_len] = '\0';
						strcat(new_file, arg);
						if ( (p = strrchr(arg, '.')) == NULL ) {
							strcat(new_file, ".");
							strcat(new_file, ext);
							}
						if ( rename(note->file, new_file) == 0 )
							printf("* '%s' -> '%s' succeed *\n", note->name, arg);
						else
							fprintf(stderr, "rename failed:\n[%s] -> [%s]\nerrno %d: %s\n",
								note->file, new_file, errno, strerror(errno));
						}
					break; // only one file
					}

				// next note
				cur = cur->next;
				}
			}
		else {
			if ( !(opt_flags & OPT_FILES) )
				fprintf(stderr, "* no notes found *\n");
			}
		
		res = list_destroy(res);
		}

	// finish
	args = list_destroy(args);
	cleanup();
	return exit_code;
	}



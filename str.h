/*
 *	Additional C String library
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

#ifndef NDC_STR_H_
#define NDC_STR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>

/*
 *	constant words list
 */
typedef struct {
	const char **ptr;	// table of words
	int	count;			// number of items
	int	alloc;			// allocation size (used for realloc)
	} cwords_t;

// utf8
wchar_t *u8towcs(const char *u8str);
char *wcstou8(const wchar_t *wcs);
wchar_t u8towc(const char *u8char);
void	u8cpytostr(char *u8buf, const wchar_t *wcs);
void	u8cpytowcs(wchar_t *wcs, const char *u8str);
size_t	u8strlen(const char *str);
size_t	u8width(const char *str);
int		u8csize(unsigned char c);
bool	u8ischar(int c);

//
char *stradd(char *str, const char *source);

// pascaloids
char *concat(const char *s1, ...);
char *copy(const char *source, int index, int count);
int	pos(const char *source, char c);
int	strpos(const char *source, const char *what);
char *insert(const char *source, int pos, const char *string);
char *delete(const char *source, int pos, int count);

// right trim
#define rtrim(s)\
	{ size_t _=strlen((s)); while(_) { _--; if (isspace((s)[_])) (s)[_]='\0'; else break; } }

// replace char
#define strtotr(b,s,r)\
	{ for(char *_=(b); *_; _ ++) if (*_ == (s)) *_ = (r); }
#define strtomtr(b,s,r)\
	{ for(const char *__=(s); *__; __ ++) strtotr((b), *__, (r)[__-(s)]); }

// constant list of words
cwords_t *cwords_create();
cwords_t *cwords_clear(cwords_t *list);
const cwords_t *cwords_destroy(cwords_t *list);
int cwords_add(cwords_t *list, const char *src);
cwords_t *strtocwords(char *buf);

// regex
int res_match(const char *pattern, const char *source);
int rex_match(regex_t *r, const char *source);
int res_replace(const char *pattern, char *source, const char *repl, size_t max_matches);
int rex_replace(regex_t *r, char *source, const char *repl, size_t max_matches);

// parsing
const char *parse_num(const char *src, char *buf);
const char *parse_const(const char *src, const char *str);

// utilities
char **text_to_lines(const char *src);
char **free_text_lines(char **table);

#ifdef __cplusplus
}
#endif
	
#endif


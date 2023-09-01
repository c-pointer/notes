/*
 *	error handling routines
 * 
 *	Copyright (C) 2017-2022 Nicholas Christopoulos.
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

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "errio.h"

static char _panic_msg[LINE_MAX];

void _panic(const char *pf, size_t pl, const char *fmt, ...) {
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(_panic_msg, LINE_MAX, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\nPANIC (%s:%ld): ", pf, pl);
	fputs(_panic_msg, stderr);
	abort();	
	}

void *_e_alloc(size_t size, const char *pf, size_t pl) {
	void *p = malloc(size);
	if ( p == NULL )
		_panic(pf, pl, "Out of memory! asked for %ld bytes", size);
	return p;
	}

void *_e_realloc(void *ptr, size_t size, const char *pf, size_t pl) {
	void *p = realloc(ptr, size);
	if ( p == NULL )
		_panic(pf, pl, "Out of memory! asked for %ld bytes", size);
	return p;
	}

void _e_free(void *p, const char *pf, size_t pl) {
	if ( p )
		free(p);
	else
		_panic(pf, pl, "free NULL ponter!");
	}

// --------------------------------------------------------------------------------

size_t _e_write(void *ptr, size_t size, size_t count, FILE *fp, const char *pf, size_t pl) {
	size_t n = fwrite(ptr, size, count, fp);
	if ( n != count ) {
		int e = errno;
		const char *s = strerror(e);
		_panic(pf, pl, "Write error\n\terrno: (%d) %s", e, s);
		}
	return n;
	}

size_t _e_read(void *ptr, size_t size, size_t count, FILE *fp, const char *pf, size_t pl) {
	size_t n = fread(ptr, size, count, fp);
	if ( n == 0 ) {
		int e = errno;
		const char *s = strerror(e);
		_panic(pf, pl, "Read error\n\terrno: (%d) %s", e, s);
		}
	return n;
	}

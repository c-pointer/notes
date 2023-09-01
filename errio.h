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

#if !defined(__PANIC_H__)
#define __PANIC_H__

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

// --------------------------------------------------------------------------------

void _panic(const char *pf, size_t pl, const char *fmt, ...);
#define panic(fmt,...) _panic(__FILE__, __LINE__, fmt, ...)

// --------------------------------------------------------------------------------
	
void *_e_alloc(size_t size, const char *pf, size_t pl);
void *_e_realloc(void *ptr, size_t size, const char *pf, size_t pl);
void _e_free(void *p, const char *pf, size_t pl);

#define m_alloc(size) _e_alloc(size, __FILE__, __LINE__)
#define m_realloc(ptr,size) _e_realloc(ptr, size, __FILE__, __LINE__)
#define m_free(ptr) _e_free(ptr, __FILE__, __LINE__)

// --------------------------------------------------------------------------------

size_t _e_write(void *ptr, size_t size, size_t n, FILE *fp, const char *pf, size_t pl);
size_t _e_read(void *ptr, size_t size, size_t n, FILE *fp, const char *pf, size_t pl);

#define f_write(ptr,size,n,fp) _e_write(ptr, size, n, fp, __FILE__, __LINE__)
#define f_read(ptr,size,n,fp) _e_read(ptr, size, n, fp, __FILE__, __LINE__)

// --------------------------------------------------------------------------------

#if defined(__cplusplus)
	}
#endif

#endif

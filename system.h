/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * system.h
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define ARRAY_SIZE(a) ( (sizeof (a)) / (sizeof (a[0])) )

#define TRACE(s)				\
	do {					\
		fprintf(stderr,			\
			"error: %s:%d: %s\n",	\
			__FILE__,		\
			__LINE__,		\
			safe_strlen(s) ?	\
			(s) : "^");		\
	} while (0)

#define EXIT(s)					\
	do {					\
		TRACE((s));			\
		assert( 0 );			\
		exit(-1);			\
	} while (0)

#define FREE(p)					\
	do {					\
		if ((p)) {			\
			free((void *)(p));	\
			(p) = NULL;		\
		}				\
	} while (0)

void file_delete(const char *pathname);

void safe_sprintf(char *buf, size_t len, const char *format, ...);

size_t safe_strlen(const char *s);

#endif /* _SYSTEM_H_ */
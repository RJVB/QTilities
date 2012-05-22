/*
 *  wininix.h
 *
 *  Created by René J.V. Bertin on 20100719.
 *  Copyright 2010. All rights reserved.
 *
 */

#if (defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER)) && !defined(_WININIX_H)

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _MSC_VER

#	define	_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	1
#	include	<stdio.h>
#	include	<errno.h>

	static FILE *_fopen_(const char *name, const char *mode)
	{ FILE *fp = NULL;
		errno = fopen_s( &fp, name, mode );
		return fp;
	}
#	define fopen(n,m)	_fopen_((n),(m))
#endif


//#define	snprintf					_snprintf
// MSCV's snprintf does not append a nullbyte if it has to truncate the string to add to the destination.
// Crashes can result, so we proxy the function and add the nullbyte ourselves if the return value indicates
// truncation. NB: POSIX snprintf does not return -1 when it truncates (at least not the version on Mac OS X).
#include <stdarg.h>
static int snprintf( char *buffer, size_t count, const char *format, ... )
{ int n;
  va_list ap;
	va_start( ap, format );
	n = _vsnprintf( buffer, count, format, ap );
	if( n < 0 ){
		buffer[count-1] = '\0';
	}
	return n;
}
#define	strdup					_strdup

#include <strings.h>
#define	strncasecmp(a,b,n)			_strnicmp((a),(b),(n))


#define _WININIX_H

#ifdef __cplusplus
}
#endif

#endif

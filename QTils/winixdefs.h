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
#	include <errno.h>

	static FILE *_fopen_(const char *name, const char *mode)
	{ FILE *fp = NULL;
		errno = fopen_s( &fp, name, mode );
		return fp;
	}
#	define fopen(n,m)	_fopen_((n),(m))
#endif


#define	snprintf					_snprintf

// #include <strings.h>

//#define strdup(s)			_strdup((s))
#define strncasecmp(a,b,n)	_strnicmp((a),(b),(n))
#define strcasecmp(a,b)		_stricmp((a),(b))
	

#define _WININIX_H

#ifdef __cplusplus
}
#endif

#endif
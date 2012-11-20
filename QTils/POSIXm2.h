/*!
 *  POSIXm2.h
 *  POSIXm2
 *
 *  Created by René J.V. Bertin on 20110218
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *  This file contains a number of useful POSIX/ISO C9x routines for use from Modula-2
 *
 */

#ifndef _POSIXM2_H

#ifdef __cplusplus
extern "C"
{
#endif
	
#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _POSIXm2_C
#		define PM2ext __declspec(dllexport)
#	else
#		define PM2ext __declspec(dllimport)
#	endif
#else
#	define PM2ext /**/
#endif
	
#ifdef __GNUC__
#	include <stdio.h>
#	include <sys/types.h>
#	define GCC_PACKED	__attribute__ ((packed))
#else
#	define GCC_PACKED	/**/
#endif

#if !defined(__QUICKTIME__) && !defined(__MOVIES__)

	typedef unsigned char		UInt8;
	typedef unsigned short		UInt16;
	typedef short				SInt16;
#if __LP64__
	typedef unsigned int		UInt32;
	typedef int				SInt32;
#else
	typedef unsigned long		UInt32;
	typedef long				SInt32;
#endif

#endif

#if !( defined(_INC_SETJMP) || defined(_BSD_SETJMP_H) )
	typedef void* jmp_buf;
#endif

PM2ext void LogLocation_Mod2( char *fName, int flen, int lineNr );
PM2ext size_t PM2_LogMsg_Mod2( const char *msg, int mlen );
PM2ext size_t PM2_LogMsgEx_Mod2( const char *msg, int mlen, va_list ap );

PM2ext jmp_buf *new_jmp_buf();
PM2ext void dispose_jmp_buf( jmp_buf **env );
PM2ext void *setjmp_adr();
PM2ext void longjmp_Mod2( jmp_buf *env, int val );

PM2ext int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap );
PM2ext int vsnprintf_Mod2( char *dest, int slen, const char *format, int flen, va_list ap );

PM2ext void *calloc_Mod2( size_t n, size_t s );
PM2ext void *realloc_Mod2( void* mem, size_t size );
PM2ext void free_Mod2( char **mem );
PM2ext void memset_Mod2( void *mem, char val, size_t len );

#pragma mark ----LibPOSIXm2Base----

typedef struct test23Struct{
	int a, b;
} test23Struct;

// Convenience facility for obtaining the basic function pointers when loading
// POSIXm2.dll dynamically, via dlopen() or LoadLibrary().
// After obtaining a handle to the library, obtain the address of initDMBasePOSIXm2() and
// then pass it a pointer to a LibPOSIXm2Base structure. initDMBasePOSIXm2() will copy the function addresses
// into the respective members of that structure, and returns the size of the LibPOSIXm2Base structure.
// Used for interfacing from Modula-2.
#ifdef _MSC_VER
#	pragma pack(show)
#	pragma pack(push,1)
#endif
typedef struct LibPOSIXm2Base {
	// members that are hidden in the public Modula-2 interface:
	size_t (*PM2_LogMsgEx)( const char *msg, int mlen, va_list ap );

	void* (*setjmp_adr)();
	int (*vsscanf)( const char *source, int slen, const char *format, int flen, va_list ap );
	int (*vsnprintf)( char *dest, int slen, const char *format, int flen, va_list ap );

	void (*test23)( test23Struct *data, int dlen, int *Nret );

	// public Modula-2 interface:
	unsigned short argc;
	char **argv;
	char* (*CLArgNr)( unsigned short arg );

	void (*LogLocation)( char *fName, int flen, int lineNr );
	size_t (*PM2_LogMsg)(const char *msg, int mlen);
	char *lastSSLogMsg;

	jmp_buf* (*new_jmp_buf)();
	void (*dispose_jmp_buf)( jmp_buf **env );
	void (*longjmp)( jmp_buf *env, int val );

	void* (*calloc)( size_t n, size_t s );
	void* (*realloc)( void* mem, size_t size );
	void (*free)( char **mem );
	void (*memset)( void *mem, char val, size_t len );

	char* (*strstr)( const char *a, int alen, const char *b, int blen );
	char* (*strrstr)( const char *a, int alen, const char *b, int blen );

	// NB!! Modula-2 defines additional members in its interface that are not initialised in C!
} GCC_PACKED LibPOSIXm2Base;
#ifdef _MSC_VER
#	pragma pack(pop)
#	pragma pack(show)
#endif
	
PM2ext size_t initDMBasePOSIXm2( LibPOSIXm2Base *dmbase );
extern short InitPOSIXm2( void *hInst );

#ifdef __cplusplus
}
#endif

#define _POSIXM2_H
#endif

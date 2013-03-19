/*!
 *  POSIXm2.c
 *  POSIXm2
 *
 *  Created by René J.V. Bertin on 20110218
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *  This file contains a number of useful POSIX/ISO C9x routines for use from Modula-2
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("POSIXm2: POSIX/ISO C9x routines for Modula-2" );

#define HAS_LOG_INIT
#include "Logging.h"

#define _POSIXm2_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#if ! defined(_WINDOWS) && !defined(WIN32) && !defined(_MSC_VER)
#	include <unistd.h>
#endif

#ifdef _MSC_VER
#	include "mswin/vsscanf.h"
#endif

#include "POSIXm2.h"

#ifndef MIN
#	define	MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */

#undef POSIX_PATHNAMES

#if !(defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32) || defined(QTMOVIESINK)
char lastSSLogMsg[2048] = "";
#endif // not on MSWin

jmp_buf *new_jmp_buf()
{
	return (jmp_buf*) calloc( 1, sizeof(jmp_buf) );
}

void dispose_jmp_buf( jmp_buf **env )
{
	if( env && *env ){
		free(*env);
		*env = NULL;
	}
}

void *setjmp_adr()
{ void* sjadr = setjmp;
	return sjadr;
}

void longjmp_Mod2( jmp_buf *env, int val )
{
	if( env && *env ){
		longjmp( *env, val );
	}
	return;
}

static char *parse_format_opcodes( char *Dest, const char *Src )
{
	if( Src && Dest ){
	  char *src = Src, *dest = Dest;
		while( *src ){
			if( *src == '\\' ){
				switch( src[1] ){
					case 'a':
						*dest++ = '\a';
						break;
					case 'b':
						*dest++ = '\b';
						break;
					case '2':
						*dest++ = '\e';
						break;
					case 'f':
						*dest++ = '\f';
						break;
					case 'n':
						*dest++ = '\n';
						break;
					case 'r':
						*dest++ = '\r';
						break;
					case 't':
						*dest++ = '\t';
						break;
					case '"':
						*dest++ = '"';
						break;
					default:
						*dest++ = '\\';
						*dest++ = src[1];
						break;
				}
				src += 2;
			}
			else{
				*dest++ = *src++;
			}
		}
		*dest = '\0';
	}
	return Dest;
}

int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap )
{ char *fmt;
  int ret = -1;
	if( strchr( format, '\\' ) ){
		if( (fmt = parse_format_opcodes( strdup(format), format )) ){
			ret = vsscanf( source, fmt, ap );
			free(fmt);
		}
	}
	else{
		ret = vsscanf( source, format, ap );
	}
	return ret;
}

int vsnprintf_Mod2( char *dest, int dlen, const char *format, int flen, va_list ap )
{ char *fmt;
  int ret = -1;
	if( strchr( format, '\\' ) ){
		if( (fmt = parse_format_opcodes( strdup(format), format )) ){
			ret = vsnprintf( dest, dlen, fmt, ap );
			free(fmt);
		}
	}
	else{
		ret = vsnprintf( dest, dlen, format, ap );
	}
	return ret;
}

char M2LogEntity[512] = "";

static void Init_PM2_Log()
{
#ifdef _SS_LOG_ACTIVE
	if( !qtLogPtr ){
		// obtain programme name: cf. http://support.microsoft.com/kb/126571
		if( __argv ){
		  char *pName = strrchr(__argv[0], '\\'), *ext = strrchr(__argv[0], '.'), *c, *end;
			c = M2LogEntity;
			if( pName ){
				pName++;
			}
			else{
				pName = __argv[0];
			}
			end = &M2LogEntity[sizeof(M2LogEntity)-1];
			while( pName < ext && c < end ){
				*c++ = *pName++;
			}
			*c++ = '\0';
		}
		qtLogPtr = Initialise_Log( "POSIXm2 Log", M2LogEntity, 0 );
		strcpy( M2LogEntity, "Modula-2" );
		qtLog_Initialised = 1;
	}
#endif // _SS_LOG_ACTIVE
}

static char LogLoc[64];
static int LogLine;
static char LogLocSet = 0;

void LogLocation_Mod2( char *fName, int flen, int lineNr )
{
	Init_PM2_Log();
	cLogStoreFileLine( qtLogPtr, fName, lineNr );
	strncpy( LogLoc, fName, sizeof(LogLoc) );
	LogLoc[sizeof(LogLoc)-1] = '\0';
	LogLine = lineNr;
	LogLocSet = 1;
}

size_t PM2_LogMsg_Mod2( const char *msg, int mlen )
{ char *fmt = msg;
  size_t ret;
	if( strchr( msg, '\\' ) ){
		if( !(fmt = parse_format_opcodes( strdup(msg), msg )) ){
			fmt = (char*) msg;
		}
	}
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	Init_PM2_Log();
	if( qtLog_Initialised ){
		if( LogLocSet ){
			cLogStoreFileLine( qtLogPtr, LogLoc, LogLine );
			LogLocSet = 0;
		}
		else{
			cLogStoreFileLine( qtLogPtr, M2LogEntity, -1 );
		}
#	ifdef _SS_LOG_ACTIVE
		cWriteLog( qtLogPtr, (char*) fmt );
#else
		strncpy( lastSSLogMsg, msg, sizeof(lastSSLogMsg) );
		lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
#	endif // _SS_LOG_ACTIVE
		if( fmt != msg ){
			free(fmt);
		}
		return strlen(lastSSLogMsg);
	}
	else{
		strncpy( lastSSLogMsg, fmt, sizeof(lastSSLogMsg) );
		lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
		if( fmt != msg ){
			free(fmt);
		}
		return 0;
	}
#else
	strncpy( lastSSLogMsg, fmt, sizeof(lastSSLogMsg) );
	lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
	ret = Log( qtLogPtr, fmt );
	if( fmt != msg ){
		free(fmt);
	}
	return ret;
#endif
}

size_t PM2_LogMsgEx_Mod2( const char *msg, int mlen, va_list ap )
{ char *fmt = msg;
	if( strchr( msg, '\\' ) ){
		if( !(fmt = parse_format_opcodes( strdup(msg), msg )) ){
			fmt = (char*) msg;
		}
	}
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	Init_PM2_Log();
	if( qtLog_Initialised ){
		if( LogLocSet ){
			cLogStoreFileLine( qtLogPtr, LogLoc, LogLine );
			LogLocSet = 0;
		}
		else{
			cLogStoreFileLine( qtLogPtr, M2LogEntity, -1 );
		}
#	ifdef _SS_LOG_ACTIVE
		cWriteLogEx( qtLogPtr, (char*) fmt, ap );
#else
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
#	endif // _SS_LOG_ACTIVE
		if( fmt != msg ){
			free(fmt);
		}
		return strlen(lastSSLogMsg);
	}
	else{
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
		if( fmt != msg ){
			free(fmt);
		}
		return 0;
	}
#else
	vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
	if( fmt != msg ){
		free(fmt);
	}
	return Log( qtLogPtr, lastSSLogMsg );
#endif
}

void *malloc_Mod2( size_t s )
{
	return malloc( s );
}

void *calloc_Mod2( size_t n, size_t s )
{
	return calloc( n, s );
}

void *realloc_Mod2( void* mem, size_t size )
{
	return (mem)? realloc( mem, size ) : calloc( 1, size );
}

void POSIXm2_free_Mod2( char **mem )
{
	if( mem && *mem ){
		free(*mem);
		*mem = NULL;
	}
}

void memset_Mod2( void *mem, char val, size_t len )
{
	if( mem ){
		memset( mem, (int) val, len );
	}
}

char *strstr_Mod2( const char *a, int alen, const char *b, int blen )
{
	return strstr(a, b);
}

/*!
 * finds the last occurrence of string b in string a, and returns a pointer to that substring
 * or NULL in case the pattern isn't found.
 */
char *strrstr( const char *a, register const char *b )
{ unsigned int lena, lenb;
  register unsigned int l=0;
  register char *c;
  register int success= 0;
  	
	lena = strlen(a);
	lenb = strlen(b);
	l = lena - lenb;
	c = &a[l];
	do{
		while( l > 0 && *c!= *b){
			c--;
			l -= 1;
		}
		if( c == a && *c != *b ){
			return NULL;
		}
		if( !(success= !strncmp( c, b, lenb)) ){
			if( l > 0 ){
				c--;
				l -= 1;
			}
		}
	} while( !success && l > 0 );
	if( !success ){
		return NULL;
	}
	else{
		return c;
	}
}

char *strrstr_Mod2( const char *a, int alen, const char *b, int blen )
{
	return strrstr(a, b);
}

char *CLArgNr( unsigned short arg )
{
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	return ((arg < __argc)? __argv[arg] : "");
#else
	return "";
#endif
}

void test23_Mod2( test23Struct *data, int dmax, int *Nret )
{ int i;
#ifdef DEBUG
	Init_PM2_Log();
	if( qtLog_Initialised ){
		Log( qtLogPtr, "test23_Mod2(%p,dmax=%d,Nret=%p->%d)\n",
		    data, dmax, Nret, *Nret
		);
	}
#endif
	*Nret = 0;
	for( i = 0 ; i <= dmax ; i++ ){
	  int a = data[i].a;
		*Nret += a * data[i].b;
		data[i].a = data[i].b;
		data[i].b = a;
	}
}

size_t initDMBasePOSIXm2( LibPOSIXm2Base *dmbase )
{
	if( dmbase ){

#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
		dmbase->argc = __argc;
		dmbase->argv = __argv;
#else
		dmbase->argc = 0;
		dmbase->argv = NULL;
#endif
		dmbase->CLArgNr = CLArgNr;

		dmbase->PM2_LogMsgEx = PM2_LogMsgEx_Mod2;
		
		dmbase->new_jmp_buf = new_jmp_buf;
		dmbase->dispose_jmp_buf = dispose_jmp_buf;
		dmbase->setjmp_adr = setjmp_adr;
		dmbase->longjmp = longjmp_Mod2;

		dmbase->vsscanf = vsscanf_Mod2;
		dmbase->vsnprintf = vsnprintf_Mod2;

		dmbase->test23 = test23_Mod2;
		
		// public Modula-2 interface:
		dmbase->LogLocation = LogLocation_Mod2;
		dmbase->PM2_LogMsg = PM2_LogMsg_Mod2;
		dmbase->lastSSLogMsg = &lastSSLogMsg[0];

		dmbase->malloc = malloc_Mod2;
		dmbase->calloc = calloc_Mod2;
		dmbase->realloc = realloc_Mod2;
		dmbase->free = POSIXm2_free_Mod2;
		dmbase->memset = memset_Mod2;

		dmbase->strstr = strstr_Mod2;
		dmbase->strrstr = strrstr_Mod2;

		return sizeof(*dmbase);
	}
	else{
		return 0;
	}
}

short InitPOSIXm2( void *hInst )
{
//	Init_PM2_Log();
	return 1;
}
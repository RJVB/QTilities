/*!
 *  @file QTils.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20100723.
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *  This file contains a number of useful QuickTime routines.

 *  20101118: see Tech. Q&A 1262 for examples on determining static framerate/mpeg movies (http://developer.apple.com/library/mac/#qa/qa2001/qa1262.html)
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTils: Track clip/flip, add metadata, add TimeCode track, ..." );

#include "Logging.h"

#define _QTILS_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if ! defined(_WINDOWS) && !defined(WIN32) && !defined(_MSC_VER)
#	include <unistd.h>
#endif
#include <assert.h>

#ifdef _MSC_VER
#	include "mswin/vsscanf.h"
#endif

#if __APPLE_CC__
#	include <Carbon/Carbon.h>
#	include <QuickTime/QuickTime.h>
#else
#	include <ImageCodec.h>
#	include <TextUtils.h>
#	include <string.h>
#	include <Files.h>
#	include <Movies.h>
#	include <MediaHandlers.h>
#	include <QuickTimeComponents.h>
#	include <direct.h>
#endif

#include "QTilities.h"
#include "Lists.h"
#include "QTMovieWin.h"
#include "CritSectEx/CritSectEx.h"

#ifndef MIN
#	define	MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */

#undef POSIX_PATHNAMES

#if !(defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32) || defined(QTMOVIESINK)
	char lastSSLogMsg[2048] = "";
#endif // not on MSWin

CSEHandle *cseLogLock = NULL;

QTils_Allocators *QTils_Allocator = NULL;

static QTils_Allocators *__init_QTils_Allocator__( QTils_Allocators *qa,
							    void* (*mallocPtr)(size_t), void* (*callocPtr)(size_t,size_t),
							    void* (*reallocPtr)(void*, size_t), void (*freePtr)(void **) )
{
	assert(malloc);
	if( !qa ){
		qa = (*mallocPtr)( sizeof(QTils_Allocators) );
	}
	assert(qa);
	QTils_Allocator = qa;
	return init_QTils_Allocator( mallocPtr, callocPtr, reallocPtr, freePtr );
}

QTils_Allocators *init_QTils_Allocator( void* (*mallocPtr)(size_t), void* (*callocPtr)(size_t,size_t),
							    void* (*reallocPtr)(void*, size_t), void (*freePtr)(void **) )
{
	if( !QTils_Allocator ){
		return __init_QTils_Allocator__( NULL, mallocPtr, callocPtr, reallocPtr, freePtr );
	}
	else{
		QTils_LogMsgEx( "QTils_Allocator=%p { malloc=%p calloc=%p realloc=%p free=%p }\n",
				QTils_Allocator, mallocPtr, callocPtr, reallocPtr, freePtr );
		QTils_Allocator->malloc = mallocPtr;
		QTils_Allocator->calloc = callocPtr;
		QTils_Allocator->realloc = reallocPtr;
		QTils_Allocator->free = freePtr;
		return QTils_Allocator;
	}
}

static void __QTils_free__( void **mem )
{
	if( mem && *mem ){
		free(*mem);
		*mem = NULL;
	}
}

void *QTils_malloc( size_t s )
{
	if( !QTils_Allocator ){
		__init_QTils_Allocator__( NULL, malloc, calloc, realloc, __QTils_free__ );
	}
	return QTils_Allocator->malloc( s );
}

void *QTils_calloc( size_t n, size_t s )
{
	if( !QTils_Allocator ){
		__init_QTils_Allocator__( NULL, malloc, calloc, realloc, __QTils_free__ );
	}
	return QTils_Allocator->calloc( n, s );
}

void *QTils_realloc( void* mem, size_t size )
{
	if( !QTils_Allocator ){
		__init_QTils_Allocator__( NULL, malloc, calloc, realloc, __QTils_free__ );
	}
	return (mem)? QTils_Allocator->realloc( mem, size ) : QTils_Allocator->calloc( 1, size );
}

void QTils_freep( void **mem )
{
	if( !QTils_Allocator ){
		__init_QTils_Allocator__( NULL, malloc, calloc, realloc, __QTils_free__ );
	}
	if( mem ){
		QTils_Allocator->free(mem);
		*mem = NULL;
	}
}

char *QTils_strdup( const char *txt )
{ char *dup = NULL;
	if( txt ){
		if( (dup = QTils_malloc( (strlen(txt) + 1) * sizeof(char) )) ){
			strcpy( dup, txt );
		}
	}
	return dup;
}

static char *parse_format_opcodes( char *Dest, const char *Src )
{
	if( Src && Dest ){
	  const char *src = Src;
	  char *dest = Dest;
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
		if( (fmt = parse_format_opcodes( QTils_strdup(format), format )) ){
			ret = vsscanf( source, fmt, ap );
			QTils_free(&fmt);
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
		if( (fmt = parse_format_opcodes( QTils_strdup(format), format )) ){
			ret = vsnprintf( dest, dlen, fmt, ap );
			QTils_free(&fmt);
		}
	}
	else{
		ret = vsnprintf( dest, dlen, format, ap );
	}
	return ret;
}

int vssprintf( char **buffer, const char *format, va_list ap )
{ CFStringRef sRef;
  CFStringRef formatRef;
  int ret = -1;
  CFIndex len = -1;
  char *string;
  CFStringEncoding enc = CFStringGetSystemEncoding();
	if( buffer
	   && (formatRef = CFStringCreateWithCString( kCFAllocatorDefault, format, enc ))
	   && (sRef = CFStringCreateWithFormatAndArguments( kCFAllocatorDefault, NULL, formatRef, ap ))
	){
		CFAllocatorDeallocate( kCFAllocatorDefault, (void*) formatRef );
		string = (char*) CFStringGetCStringPtr( sRef, enc );
		if( !string ){
			// couldn't get a 'direct' string pointer; use the 'get a copy' approach:
			len = CFStringGetMaximumSizeForEncoding( CFStringGetLength(sRef), enc ) + 1;
			if( (string = QTils_calloc( len, sizeof(char) )) ){
				if( !CFStringGetCString( sRef, string, len - 1, enc ) ){
					QTils_free(&string);
					string = NULL;
				}
				else{
//					QTils_LogMsgEx( "vssprintf(): CFStringRef is length %d, copied to string of length %lu\n",
//								len-1, strlen(string) );
					string[len-1] = '\0';
				}
			}
		}
		if( string ){
		  size_t slen = strlen(string) + 1;
			if( *buffer ){
				*buffer = QTils_realloc( *buffer, slen * sizeof(char) );
//				QTils_LogMsgEx( "vssprintf(): resized destination string %p of length %lu to %lu\n",
//							*buffer, strlen(*buffer), slen-1 );
			}
			else{
				*buffer = QTils_calloc( slen, sizeof(char) );
//				QTils_LogMsgEx( "vssprintf(): created destination string %p of length %lu\n",
//							*buffer, slen-1 );
			}
			if( *buffer ){
				strncpy( *buffer, string, slen - 1 );
				(*buffer)[slen-1] = '\0';
				ret = (int) strlen(*buffer);
			}
			if( string && len >= 0 ){
				// string is a copy and not the result of CFStringGetCStringPtr(); free it now.
				QTils_free(&string);
			}
		}
		CFAllocatorDeallocate( kCFAllocatorDefault, (void*) sRef );
	}
	return ret;
}

int ssprintf( char **buffer, const char *format, ... )
{ va_list ap;
  int ret;
	va_start( ap, format );
	ret = vssprintf( buffer, format, ap );
	va_end(ap);
	return ret;
}

int vssprintf_Mod2( char **buffer, const char *format, int flen, va_list ap )
{ char *fmt = (char*) format;
  int ret = -1;
	if( strchr( format, '\\' ) ){
		if( !(fmt = parse_format_opcodes( QTils_strdup(format), format )) ){
			fmt = (char*) format;
		}
	}
	ret = vssprintf( buffer, fmt, ap );
	if( fmt != format ){
		QTils_free(&fmt);
	}
	return ret;
}

int vssprintfAppend( char **buffer, const char *format, va_list ap )
{ CFMutableStringRef sRef;
  CFStringRef formatRef;
  int ret = -1;
  CFIndex len = -1;
  char *string;
  CFStringEncoding enc = CFStringGetSystemEncoding();
	if( buffer
	   && (formatRef = CFStringCreateWithCString( kCFAllocatorDefault, format, enc ))
	   && (sRef = (CFMutableStringRef) CFStringCreateMutable( kCFAllocatorDefault, 0 ))
	){
		if( *buffer ){
			CFStringAppendCString( sRef, *buffer, enc );
		}
		CFStringAppendFormatAndArguments( sRef, NULL, formatRef, ap );
		CFAllocatorDeallocate( kCFAllocatorDefault, (void*) formatRef );
		string = (char*) CFStringGetCStringPtr( sRef, enc );
		if( !string ){
			len = CFStringGetMaximumSizeForEncoding( CFStringGetLength(sRef), enc ) + 1;
			if( (string = QTils_calloc( len, sizeof(char) )) ){
				if( !CFStringGetCString( sRef, string, len - 1, enc ) ){
					QTils_free(&string);
					string = NULL;
				}
				else{
					string[len-1] = '\0';
				}
			}
		}
		if( string ){
		  size_t slen = strlen(string) + 1, slen0;
			if( *buffer ){
				slen0 = strlen(*buffer);
				*buffer = QTils_realloc( *buffer, slen * sizeof(char) );
			}
			else{
				slen0 = 0;
				*buffer = QTils_calloc( slen, sizeof(char) );
			}
			if( *buffer ){
				strncpy( *buffer, string, slen - 1 );
				(*buffer)[slen-1] = '\0';
				ret = (int) strlen(*buffer) - slen0;
			}
			if( string && len >= 0 ){
				QTils_free(&string);
			}
		}
		CFAllocatorDeallocate( kCFAllocatorDefault, (void*) sRef );
	}
	return ret;
}

int ssprintfAppend( char **buffer, const char *format, ... )
{ va_list ap;
  int ret;
	va_start(ap, format);
	ret = vssprintfAppend( buffer, format, ap );
	va_end(ap);
	return ret;
}

int vssprintfAppend_Mod2( char **buffer, const char *format, int flen, va_list ap )
{ char *fmt = (char*) format;
  int ret = -1;
	if( strchr( format, '\\' ) ){
		if( !(fmt = parse_format_opcodes( QTils_strdup(format), format )) ){
			fmt = (char*) format;
		}
	}
	ret = vssprintfAppend( buffer, fmt, ap );
	if( fmt != format ){
		QTils_free(&fmt);
	}
	return ret;
}

void QTils_LogInit()
{
#if (defined(__APPLE_CC__) || defined(__MACH__)) && !defined(EMBEDDED_FRAMEWORK)
	PCLogAllocPool();
#endif
	if( !cseLogLock ){
		cseLogLock = CreateCSEHandle(4000);
	}
}

void QTils_LogFinish()
{
	DeleteCSEHandle(cseLogLock);
}

size_t QTils_LogMsg( const char *msg )
{ unsigned char unlockFlag;
	if( !msg || *msg == '\0' ){
		return -1;
	}
	unlockFlag = LockCSEHandle(cseLogLock);
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	if( qtLog_Initialised ){
		Log( qtLogPtr, (char*) msg );
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return strlen(msg);
	}
	else{
		strncpy( lastSSLogMsg, msg, sizeof(lastSSLogMsg) );
		lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return 0;
	}
#else
	strncpy( lastSSLogMsg, msg, sizeof(lastSSLogMsg) );
	lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
	UnlockCSEHandle(cseLogLock, unlockFlag);
	return Log( qtLogPtr, msg );
#endif
}

#ifdef _SS_LOG_ACTIVE
extern char M2LogEntity[];
#endif

size_t QTils_LogMsg_Mod2( const char *msg, int mlen )
{ char *fmt = (char*) msg;
  size_t ret;
  unsigned char unlockFlag;
	if( strchr( msg, '\\' ) ){
		if( !(fmt = parse_format_opcodes( QTils_strdup(msg), msg )) ){
			fmt = (char*) msg;
		}
	}
	unlockFlag = LockCSEHandle(cseLogLock);
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	if( qtLog_Initialised ){
#	ifdef _SS_LOG_ACTIVE
		cLogStoreFileLine( M2LogEntity, -1 ), cWriteLog( qtLogPtr, (char*) fmt );
#else
		strncpy( lastSSLogMsg, msg, sizeof(lastSSLogMsg) );
		lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
#	endif // _SS_LOG_ACTIVE
		if( fmt != msg ){
			QTils_free(&fmt);
		}
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return strlen(lastSSLogMsg);
	}
	else{
		strncpy( lastSSLogMsg, fmt, sizeof(lastSSLogMsg) );
		lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
		if( fmt != msg ){
			QTils_free(&fmt);
		}
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return 0;
	}
#else
	strncpy( lastSSLogMsg, fmt, sizeof(lastSSLogMsg) );
	lastSSLogMsg[sizeof(lastSSLogMsg)-1] = '\0';
	ret = Log( qtLogPtr, fmt );
	if( fmt != msg ){
		QTils_free(&fmt);
	}
	UnlockCSEHandle(cseLogLock, unlockFlag);
	return ret;
#endif
}

size_t QTils_vLogMsgEx( const char *msg, va_list ap )
{ unsigned char unlockFlag = LockCSEHandle(cseLogLock);
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	if( qtLog_Initialised ){
#	ifdef _SS_LOG_ACTIVE
		cLogStoreFileLine(__FILE__, __LINE__), cWriteLogEx( qtLogPtr, (char*) msg, ap );
#else
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), msg, ap );
#	endif // _SS_LOG_ACTIVE
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return strlen(msg);
	}
	else{
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), msg, ap );
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return 0;
	}
#else
	vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), msg, ap );
	UnlockCSEHandle(cseLogLock, unlockFlag);
	return Log( qtLogPtr, lastSSLogMsg );
#endif
}

size_t QTils_LogMsgEx( const char *msg, ... )
{ va_list ap;
  size_t ret;
	va_start( ap, msg );
	ret = QTils_vLogMsgEx( msg, ap );
	va_end(ap);
	return ret;
}

size_t QTils_LogMsgEx_Mod2( const char *msg, int mlen, va_list ap )
{ char *fmt = (char*) msg;
  unsigned char unlockFlag;
	if( strchr( msg, '\\' ) ){
		if( !(fmt = parse_format_opcodes( QTils_strdup(msg), msg )) ){
			fmt = (char*) msg;
		}
	}
	unlockFlag = LockCSEHandle(cseLogLock);
#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
	if( qtLog_Initialised ){
#	ifdef _SS_LOG_ACTIVE
		cLogStoreFileLine( M2LogEntity, -1 ), cWriteLogEx( qtLogPtr, (char*) fmt, ap );
#else
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
#	endif // _SS_LOG_ACTIVE
		if( fmt != msg ){
			QTils_free(&fmt);
		}
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return strlen(lastSSLogMsg);
	}
	else{
		vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
		if( fmt != msg ){
			QTils_free(&fmt);
		}
		UnlockCSEHandle(cseLogLock, unlockFlag);
		return 0;
	}
#else
	vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), fmt, ap );
	if( fmt != msg ){
		QTils_free(&fmt);
	}
	UnlockCSEHandle(cseLogLock, unlockFlag);
	return Log( qtLogPtr, lastSSLogMsg );
#endif
}

// convert a Pascal string to a C string using a static buffer internal to the function
char *P2Cstr( Str255 pstr )
{ static char cstr[257];
  int len = (int) pstr[0];

	strncpy( cstr, (char*) &pstr[1], len );
	cstr[len] = '\0';
	return(cstr);
}

// returns the textual representation of a Mac FOURCHARCODE (e.g. 'JPEG')
char *OSTStr( OSType type )
{ static union OSTStr {
		uint32_t four;
		char str[5];
  } ltype;
	ltype.four = EndianU32_BtoN(type);
	ltype.str[4] = '\0';
	return ltype.str;
}

const char *MacErrorString( ErrCode err, const char **errComment )
{ static const char *missing = "[undocumented]";
  const char *errString = GetMacOSStatusErrStrings(err, errComment);
	if( errComment && !*errComment ){
		*errComment = missing;
	}
	return (errString)? errString : missing;
}

UInt32 MacErrorString_Mod2( ErrCode err, char *errString, int slen, char *errComment, int clen )
{ const char *s, *c;
	if( (s = GetMacOSStatusErrStrings(err, &c)) ){
		strncpy( errString, s, slen );
		errString[slen-1] = '\0';
		if( c ){
			strncpy( errComment, c, clen );
			errComment[clen-1] = '\0';
		}
		else{
			errComment[0] = '\0';
		}
		return TRUE;
	}
	else{
		return FALSE;
	}
}

// sometimes it's a good idea to (re)anchor a Movie to a top-left corner at (0,0)
ErrCode AnchorMovie2TopLeft( Movie theMovie )
{ Rect mBox;
  ErrCode err;
	GetMovieBox( theMovie, &mBox );
	mBox.right -= mBox.left;
	mBox.bottom -= mBox.top;
	mBox.top = mBox.left = 0;
	SetMovieBox( theMovie, &mBox );
	err = GetMoviesError();
	UpdateMovie( theMovie );
	return err;
}

void GetMovieDimensions( Movie theMovie, Fixed *width, Fixed *height )
{ long w, h;
  Rect box;
	GetMovieNaturalBoundsRect( theMovie, &box );
	if( box.left > box.right ){
		w = box.left - box.right;
	}
	else{
		w = box.right - box.left;
	}
	if( box.bottom > box.top ){
		h = box.bottom - box.top;
	}
	else{
		h = box.top - box.bottom;
	}
	*width = Long2Fix(w);
	*height = Long2Fix(h);
}

/*!
	define and install a rectangular clipping region to the current track to mask off all but the requested
	quadrant (channel). If requested, also flip the track horizontally.
 */
void ClipFlipCurrentTrackToQuadrant( Movie theMovie, Track theTrack, short quadrant, short hflip, short withMetaData )
{ int clipped = FALSE;
  Rect trackBox;
  Fixed trackHeight, trackWidth;
  char mdInfo[128] = "";

	if( !theTrack ){
		return;
	}

	GetTrackDimensions( theTrack, &trackWidth, &trackHeight );
	trackBox.left = trackBox.top = 0;
	trackBox.right = FixRound( trackWidth );
	trackBox.bottom = FixRound( trackHeight );

	if( quadrant >= 0 && quadrant < 4 ){
	// From http://developer.apple.com/mac/library/documentation/QuickTime/RM/MovieInternals/MTTimeSpace/B-Chapter/2MovieTimeandSpace.html:
	  QDErr err;
	  short camWidth, camHeight;
	  Rect camBox;
	  RgnHandle trClip;
		camWidth = trackBox.right / 2;
		camHeight = trackBox.bottom / 2;
		switch( quadrant ){
			case 0:
				camBox.left = camBox.top = 0;
				camBox.right = camWidth;
				camBox.bottom = camHeight;
				break;
			case 1:
				camBox.left = camWidth;
				camBox.top = 0;
				camBox.right = trackBox.right;
				camBox.bottom = camHeight;
				break;
			case 2:
				camBox.left = 0;
				camBox.top = camHeight;
				camBox.right = camWidth;
				camBox.bottom = trackBox.bottom;
				break;
			case 3:
				camBox.left = camWidth;
				camBox.top = camHeight;
				camBox.right = trackBox.right;
				camBox.bottom = trackBox.bottom;
				break;
		}
		// the clipping region is initialised and defined using obsolescent functions, but I have
		// found no "new" way of doing the same thing.
		trClip = NewRgn();
		RectRgn(trClip, &camBox);
		SetTrackClipRgn( theTrack, trClip );
		err = GetMoviesError();
		DisposeRgn( trClip );
		UpdateMovie( theMovie );
		if( err == noErr ){
			clipped = TRUE;
			// the new track bounding box:
			trackBox = camBox;
			snprintf( mdInfo, sizeof(mdInfo), "quadrant #%d ", quadrant );
		}
	}
	// a Horizontal flip is obtained through a matrix operation:
	if( hflip ){
	  MatrixRecord m;
		GetTrackMatrix( theTrack, &m );
#ifdef DEBUG
		Log( qtLogPtr, "[[%6d %6d %6d]\n", FixRound(m.matrix[0][0]), FixRound(m.matrix[0][1]), FixRound(m.matrix[0][2]) );
		Log( qtLogPtr, " [%6d %6d %6d]\n", FixRound(m.matrix[1][0]), FixRound(m.matrix[1][1]), FixRound(m.matrix[1][2]) );
		Log( qtLogPtr, " [%6d %6d %6d]] type %d --> (hori.flip)\n",
			FixRound(m.matrix[2][0]), FixRound(m.matrix[2][1]), FixRound(m.matrix[2][2]),
			GetMatrixType(&m)
		);
#endif
#if 1
		{ Rect flippedBox;
			// construct the flipped version of the track's bounding box:
			flippedBox.right = trackBox.left;
			flippedBox.top = trackBox.top;
			flippedBox.left = trackBox.right;
			flippedBox.bottom = trackBox.bottom;
			MapMatrix( &m, &trackBox, &flippedBox );
		}
#else
		// different solution - this one implies a translation over -trackWidth which becomes
		// visible when adding a subsequent track. SetIdentityMatrix isn't required if one already
		// has obtained the track's matrix...
//		SetIdentityMatrix( &m );
		ScaleMatrix( &m, -fixed1, fixed1, 0, 0 );
#endif
#ifdef DEBUG
		Log( qtLogPtr, "[[%6d %6d %6d]\n", FixRound(m.matrix[0][0]), FixRound(m.matrix[0][1]), FixRound(m.matrix[0][2]) );
		Log( qtLogPtr, " [%6d %6d %6d]\n", FixRound(m.matrix[1][0]), FixRound(m.matrix[1][1]), FixRound(m.matrix[1][2]) );
		Log( qtLogPtr, " [%6d %6d %6d]] type %d\n",
			FixRound(m.matrix[2][0]), FixRound(m.matrix[2][1]), FixRound(m.matrix[2][2]),
			GetMatrixType(&m)
		);
#endif
		SetTrackMatrix( theTrack, &m );
		if( GetMoviesError() == noErr && strlen(mdInfo) < sizeof(mdInfo) - 9 ){
			strcat( mdInfo, "h-flipped" );
		}
	}
	if( clipped || hflip ){
		 AnchorMovie2TopLeft( theMovie );
	}
	// add some metadata about what we just did:
	if( withMetaData && strlen(mdInfo) ){
		AddMetaDataStringToTrack( theMovie, theTrack, akInfo, mdInfo, NULL );
	}
}

ErrCode AddUserDataToMovie( Movie theMovie, void *data, size_t dataSize, OSType udType, int replace )
{ ErrCode err = paramErr;		
  UserData userData;

	if( theMovie && data ){
	  Handle hData = NewHandle(dataSize);
		if( !hData ){
			return MemError();
		}
		memcpy( *hData, data, dataSize );
		userData = GetMovieUserData(theMovie);

		if( replace ){
			err = noErr;
			while( err == noErr && CountUserDataType( userData, udType ) ){
				err = RemoveUserData( userData, udType, 1 );
			}
		}
		err = AddUserData( userData, hData, udType );
		DisposeHandle(hData);
	}
	return err;
}	

ErrCode GetUserDataFromMovie( Movie theMovie, void **data, size_t *dataSize, OSType udType )
{ ErrCode err = paramErr;		
  UserData userData;
  long len;

	if( theMovie && data && dataSize ){
	  Handle hData = NewHandle(0);
		if( !hData ){
			return MemError();
		}
		userData = GetMovieUserData(theMovie);

		if( CountUserDataType( userData, udType ) ){
			if( (err = GetUserData( userData, hData, udType, 1 )) == noErr ){
				len = GetHandleSize(hData);
				if( len > 0 && (*data = QTils_calloc( len, 1 )) ){
					memcpy( *data, *hData, len );
					*dataSize = (size_t) len;
				}
				else if( len == 0 ){
					*data = NULL;
					*dataSize = 0;
				}
				else{
					err = MemError();
				}
			}
		}
		else{
			err = userDataItemNotFound;
		}
		DisposeHandle(hData);
	}
	return err;
}	

// for code interfacing to us from a non-C language that doesn't support FOURCHARCODEs
size_t ExportAnnotationKeys( AnnotationKeyList *list )
{
	if( list ){
		list->akAuthor = akAuthor;
		list->akComment = akComment;
		list->akCopyRight = akCopyRight;
		list->akDisplayName = akDisplayName;
		list->akInfo = akInfo;
		list->akKeywords = akKeywords;
		list->akDescr = akDescr;
		list->akFormat = akFormat;
		list->akSource = akSource;
		list->akSoftware = akSoftware;
		list->akWriter = akWriter;
		list->akYear = akYear;
		list->akCreationDate = akCreationDate;
		return sizeof(*list);
	}
	else{
		return 0;
	}
}

// internal routine that handles the getting and setting of QuickTime metadata.
ErrCode MetaDataHandler( Movie theMovie, Track toTrack,
						  AnnotationKeys key, char **Value, char **Lang, char *separator )
{ QTMetaDataRef theMetaData;
  ErrCode err;
  const char *value, *lang = NULL;
	if( theMovie && Value ){
	  OSType inkey;
	  QTMetaDataStorageFormat storageF;
	  QTMetaDataKeyFormat inkeyF;
	  UInt8 *inkeyPtr;
	  ByteCount inkeySize;

		// store the entry Value
		value = (const char*) *Value;
		if( Lang ){
			lang = (const char*) *Lang;
		}
		// 20110325:
		if( !separator ){
			separator = (key == akSource)? ", " : "\n";
		}

		// by default, we use QuickTime storage with Common Format keys that are OSTypes:
		storageF = kQTMetaDataStorageFormatQuickTime;
		inkeyF = /*kQTMetaDataKeyFormatQuickTime*/ kQTMetaDataKeyFormatCommon;
		inkeyPtr = (UInt8*) &inkey;
		inkeySize = sizeof(inkey);
		switch( key ){
			case akAuthor:
				inkey = kQTMetaDataCommonKeyAuthor;
				break;
			case akComment:
				inkey = kQTMetaDataCommonKeyComment;
				break;
			case akCopyRight:
				inkey = kQTMetaDataCommonKeyCopyright;
				break;
			case akDisplayName:
//				inkey = kUserDataTextFullName;
//				storageF = kQTMetaDataStorageFormatUserData;
//				inkeyF = kQTMetaDataKeyFormatUserData;
// with kQTMetaDataCommonKeyDisplayName, akDisplayName sets the track name
// as shown in the QT Player inspector.
				inkey = kQTMetaDataCommonKeyDisplayName;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akInfo:
				inkey = kQTMetaDataCommonKeyInformation;
				break;
			case akKeywords:
				inkey = kQTMetaDataCommonKeyKeywords;
				break;
			case akDescr:
				inkey = kUserDataTextDescription;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akFormat:
				inkey = kUserDataTextOriginalFormat;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akSource:
				inkey = kUserDataTextOriginalSource;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akSoftware:
				inkey = kQTMetaDataCommonKeySoftware;
				break;
			case akWriter:
				inkey = kUserDataTextWriter;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akCreationDate:
				inkey = kUserDataTextCreationDate;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			case akYear:
				// the Year key shown by QuickTime Player Pro doesn't have an OSType associated with it
				// but only an URI that serves as the key. Set up things appropriately:
				inkey = 'year';
				inkeyPtr = (UInt8*) "com.apple.quicktime.year";
				inkeySize = (ByteCount) strlen( (char*) inkeyPtr);
				inkeyF = kQTMetaDataKeyFormatQuickTime;
				break;
			case akTrack:
				inkey = kUserDataTextTrack;
				storageF = kQTMetaDataStorageFormatUserData;
				inkeyF = kQTMetaDataKeyFormatUserData;
				break;
			default:
				return kQTMetaDataInvalidKeyFormatErr;
				break;
		}
		// get a handle to the metadata storage (which can be empty, of course)
		if( toTrack ){
			err = (ErrCode) QTCopyTrackMetaData( toTrack, &theMetaData );
		}
		else{
			err = (ErrCode) QTCopyMovieMetaData( theMovie, &theMetaData );
		}
		if( err == noErr ){
		  char *newvalue = NULL;
		  QTMetaDataItem item = kQTMetaDataItemUninitialized;
		  OSStatus qmdErr;
			// check if the key is already present. If so, append the new value to the existing value,
			// separated by a newline.
			qmdErr = QTMetaDataGetNextItem(theMetaData, storageF, item, inkeyF, inkeyPtr, inkeySize, &item);
			if( qmdErr == noErr ){
				if( item != kQTMetaDataItemUninitialized ){
				  ByteCount size=0, nvlen;
					// the item exists already. Obtain its length:
					qmdErr = QTMetaDataGetItemValue( theMetaData, item, NULL, 0, &size );
					if( value ){
						// if we're setting a new value, determine the new length
						nvlen = size + strlen(value) + 2;
					}
					else{
						nvlen = size + 1;
					}
					// if no error occurred, get a new buffer of the correct size in which to
					// return the current metadata value or construct the new value.
					if( qmdErr == noErr && (newvalue = QTils_calloc( nvlen, sizeof(char) )) ){
						// get the <size> bytes of the value:
						if( (qmdErr = QTMetaDataGetItemValue( theMetaData, item,
										(UInt8*) newvalue, size, 0 )
							) == noErr
						){
							if( !value ){
							  char *lng=NULL;
							  ByteCount n = 0;
								// we're only reading the metadata value
								*Value = newvalue;
								if( Lang && (lng = QTils_calloc(17, sizeof(char))) ){
									// user also requested the language property, and we obtained
									// some memory to return it in:
									err = (ErrCode) QTMetaDataGetItemProperty( theMetaData, item,
											  kPropertyClass_MetaDataItem,
											  kQTMetaDataItemPropertyID_Locale,
											  16, lng, &n
									);
									if( n ){
										*Lang = lng;
									}
									else{
										QTils_free(&lng);
										*Lang = NULL;
									}
								}
								// we're done!
								err = (ErrCode) qmdErr;
								goto bail;
							}
							// construct the new value string
							{ size_t tlen = strlen(newvalue);
								snprintf( &newvalue[tlen], nvlen-tlen, "%s%s", separator, value );
							}
							value = newvalue;
							 // set the item to the new value:
							if( (qmdErr = QTMetaDataSetItem( theMetaData, item,
										(UInt8*) value, (ByteCount) strlen(value), kQTMetaDataTypeUTF8 )) != noErr
							){
								// error! let's hope QTMetaDataAddItem() will add the item correctly again
								QTMetaDataRemoveItem( theMetaData, item );
							}
							else{
								err = noErr;
								if( lang && *lang ){
									err = (ErrCode) QTMetaDataSetItemProperty( theMetaData, item,
											 kPropertyClass_MetaDataItem,
											 kQTMetaDataItemPropertyID_Locale,
											 (ByteCount) strlen(lang), lang
									);
								}
								UpdateMovie( theMovie );
							}
						}
					}
					else{
						qmdErr = -1;
					}
				}
			}
			if( qmdErr != noErr && value ){
				err = (ErrCode) QTMetaDataAddItem( theMetaData, storageF, inkeyF,
						inkeyPtr, inkeySize, (UInt8 *)value, (ByteCount) strlen(value), kQTMetaDataTypeUTF8, &item
				);
				if( err != noErr ){
					Log( qtLogPtr, "Error adding key %s=\"%s\" (%d)\n", OSTStr(inkey), value, err );
					// failure, in this case we're sure we can release the allocated memory.
					if( newvalue ){
						if( *Value == newvalue ){
							*Value = NULL;
						}
						QTils_free(&newvalue);
						newvalue = NULL;
					}
				}
				else{
					if( lang && *lang ){
						// item set and we have a language property to add:
						err = (ErrCode) QTMetaDataSetItemProperty( theMetaData, item,
								  kPropertyClass_MetaDataItem,
								  kQTMetaDataItemPropertyID_Locale,
								  (ByteCount) strlen(lang), lang
						);
					}
					UpdateMovie( theMovie );
				}
			}
bail:
			// 20110927; 20120608: leak analysis suggests that we are supposed to release allocated memory
			// also when the metadata has been updated successfully:
			// 20121115: but doing this would mean that we return nothing. And that's not what we want ... so
			// we check if value==NULL (which is the indication that we were reading the metadata)
			if( newvalue && value ){
				if( *Value == newvalue ){
					*Value = (value == newvalue)? NULL : value;
				}
				QTils_free(&newvalue);
				newvalue = NULL;
			}
			QTMetaDataRelease( theMetaData );
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode AddMetaDataStringToTrack( Movie theMovie, Track theTrack,
							    AnnotationKeys key, const char *Value, const char *Lang )
{
	if( !theMovie || !theTrack ){
		return paramErr;
	}
	else{
	  const char *value = Value, *lang = Lang;
	  ErrCode ret;
		ret = MetaDataHandler( theMovie, theTrack, key, (char**) &value, (char**) &lang, NULL );
		if( value && value != Value ){
			QTils_free(&value);
		}
		if( lang && lang != Lang ){
			QTils_free(&lang);
		}
		return ret;
	}
}

ErrCode AddMetaDataStringToMovie( Movie theMovie, AnnotationKeys key, const char *Value, const char *Lang )
{
	if( !theMovie ){
		return paramErr;
	}
	else{
	  const char *value = Value, *lang = Lang;
	  ErrCode ret;
		ret = MetaDataHandler( theMovie, NULL, key, (char**) &value, (char**) &lang, NULL );
		if( value && value != Value ){
			QTils_free(&value);
		}
		if( lang && lang != Lang ){
			QTils_free(&lang);
		}
		return ret;
	}
}

ErrCode GetMetaDataStringFromTrack( Movie theMovie, Track theTrack,
							    AnnotationKeys key, char **value, char **lang )
{
	if( !theMovie || !theTrack ){
		return paramErr;
	}
	else{
		return MetaDataHandler( theMovie, theTrack, key, value, lang, NULL );
	}
}

ErrCode GetMetaDataStringFromMovie( Movie theMovie, AnnotationKeys key, char **value, char **lang )
{
	if( !theMovie ){
		return paramErr;
	}
	else{
		return MetaDataHandler( theMovie, NULL, key, value, lang, NULL );
	}
}

// Modula-2 'methods' to handle metadata. The Track type isn't exported to M2, only the notion
// that a movie can have multiple things called tracks.
long GetMovieTrackCount_Mod2(Movie theMovie)
{
	return GetMovieTrackCount(theMovie);
}

ErrCode AddMetaDataStringToTrack_Mod2( Movie theMovie, long trackNr,
							    AnnotationKeys key,
							    const char *Value, int vlen, const char *Lang, int llen )
{
	if( !theMovie || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	else{
	  const char *value = Value, *lang = Lang;
	  ErrCode ret;
		ret = MetaDataHandler( theMovie, GetMovieIndTrack(theMovie,trackNr), key,
								 (char**) &value, (lang && *lang)? (char**) &lang : NULL, NULL );
		if( value && value != Value ){
			QTils_free(&value);
		}
		if( lang && lang != Lang ){
			QTils_free(&lang);
		}
		return ret;
	}
}

ErrCode AddMetaDataStringToMovie_Mod2( Movie theMovie, AnnotationKeys key,
								const char *Value, int vlen, const char *Lang, int llen )
{
	if( !theMovie ){
		return paramErr;
	}
	else{
	  const char *value = Value, *lang = Lang;
	  ErrCode ret;
		ret = MetaDataHandler( theMovie, NULL, key,
								 (char**) &value, (lang && *lang)? (char**) &lang : NULL, NULL );
		if( value && value != Value ){
			QTils_free(&value);
		}
		if( lang && lang != Lang ){
			QTils_free(&lang);
		}
		return ret;
	}
}

ErrCode GetMetaDataStringLengthFromTrack_Mod2( Movie theMovie, long trackNr,
							    AnnotationKeys key, size_t *len )
{
	if( !theMovie || !len || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	else{
	  char *value = NULL;
	  ErrCode err;
		err = MetaDataHandler( theMovie, GetMovieIndTrack(theMovie,trackNr), key, &value, NULL, NULL );
		if( err == noErr && value ){
			*len = strlen(value)+1;
			QTils_free(&value);
		}
		else{
			*len = 0;
		}
		return err;
	}
}

ErrCode GetMetaDataStringLengthFromMovie_Mod2( Movie theMovie, AnnotationKeys key, size_t *len )
{
	if( !theMovie || !len ){
		return paramErr;
	}
	else{
	  // initialise *value to NULL so that MetaDataHandler() will only return the <key> metadata
	  // (if it exists) rather than change/add it.
	  char *value = NULL;
	  ErrCode err;
		err = MetaDataHandler( theMovie, NULL, key, &value, NULL, NULL );
		if( err == noErr && value ){
			*len = strlen(value)+1;
			QTils_free(&value);
		}
		else{
			*len = 0;
		}
		return err;
	}
}

ErrCode GetMetaDataStringFromTrack_Mod2( Movie theMovie, long trackNr,
							    AnnotationKeys key,
							    char *rvalue, int vlen, char *rlang, int llen )
{
	if( !theMovie || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	else{
	 char *value = NULL;
	 char *lang = NULL;
	 ErrCode err;
		err = MetaDataHandler( theMovie, GetMovieIndTrack(theMovie,trackNr), key,
								 &value, &lang, NULL
		);
		if( err == noErr ){
			if( value ){
			  size_t len = strlen(value);
#ifdef DEBUG
				Log( qtLogPtr, "GetMetaDataStringFromTrack_Mod2(\"%s\") -> rvalue=%p vlen=%d",
				    value, rvalue, vlen
				);
#endif
				// Modula-2 passes the size of the "open" 'VAR ARRAY OF CHAR' value
				// argument in vlen, so we use that size indication:
				if( len >= vlen ){
					len = vlen-1;
				}
				strncpy( rvalue, value, len );
				rvalue[len] = '\0';
				QTils_free(&value);
			}
			else{
				rvalue[0] = '\0';
			}
			if( lang ){
			  size_t len = strlen(lang);
				// Modula-2 passes the size of the "open" 'VAR ARRAY OF CHAR' value
				// argument in llen, so we use that size indication:
				if( len >= llen ){
					len = llen-1;
				}
				strncpy( rlang, lang, len );
				rlang[len] = '\0';
				QTils_free(&lang);
			}
			else{
				rlang[0] = '\0';
			}
		}
		return err;
	}
}

ErrCode GetMetaDataStringFromMovie_Mod2( Movie theMovie, AnnotationKeys key,
							   char *rvalue, int vlen, char *rlang, int llen )
{
	if( !theMovie ){
		return paramErr;
	}
	else{
	 char *value = NULL;
	 char *lang = NULL;
	 ErrCode err;
		err = MetaDataHandler( theMovie, NULL, key, &value, &lang, NULL );
		if( err == noErr ){
			if( value ){
			  size_t len = strlen(value);
				// Modula-2 passes the size of the "open" 'VAR ARRAY OF CHAR' value
				// argument in vlen, so we use that size indication:
				if( len >= vlen ){
					len = vlen-1;
				}
				strncpy( rvalue, value, len );
				rvalue[len] = '\0';
				QTils_free(&value);
			}
			else{
				rvalue[0] = '\0';
			}
			if( lang ){
			  size_t len = strlen(lang);
				// Modula-2 passes the size of the "open" 'VAR ARRAY OF CHAR' value
				// argument in llen, so we use that size indication:
				if( len >= llen ){
					len = llen-1;
				}
				strncpy( rlang, lang, len );
				rlang[len] = '\0';
				QTils_free(&lang);
			}
			else{
				rlang[0] = '\0';
			}
		}
		return err;
	}
}

/*!
	Copy the metadata types we support from a source movie to a destination movie
 */
int CopyMovieMetaData( Movie dstMovie, Movie srcMovie )
{ char *mdstr = NULL, *lang = NULL;
  int n = 0, i;
  ErrCode err;
  AnnotationKeys key[] = { akAuthor, akComment, akCopyRight, akDisplayName,
		akInfo, akKeywords, akDescr, akFormat, akSource,
		akSoftware, akWriter, akYear, akCreationDate };
	for( i = 0 ; i < sizeof(key)/sizeof(AnnotationKeys) ; i++ ){
		err = MetaDataHandler( srcMovie, NULL, key[i], &mdstr, &lang, NULL );
		if( err == noErr ){
			if( mdstr ){
				MetaDataHandler( dstMovie, NULL, key[i], &mdstr, &lang, NULL );
				QTils_free(&mdstr); QTils_free(&lang);
				mdstr = NULL, lang = NULL;
				n += 1;
			}
			else if( lang ){
				QTils_free(&lang);
				lang = NULL;
			}
		}
	}
	if( n ){
		UpdateMovie(dstMovie);
	}
	return n;
}

/*!
	Copy the metadata types we support from a source track to a destination track
 */
int CopyTrackMetaData( Movie dstMovie, Track dstTrack, Movie srcMovie, Track srcTrack )
{ char *mdstr = NULL, *lang = NULL;
  int n = 0, i;
  ErrCode err;
  AnnotationKeys key[] = { akAuthor, akComment, akCopyRight, akDisplayName,
		akInfo, akKeywords, akDescr, akFormat, akSource,
		akSoftware, akWriter, akYear, akCreationDate };
	for( i = 0 ; i < sizeof(key)/sizeof(AnnotationKeys) ; i++ ){
		err = MetaDataHandler( srcMovie, srcTrack, key[i], &mdstr, &lang, NULL );
		if( err == noErr ){
			if( mdstr ){
				MetaDataHandler( dstMovie, dstTrack, key[i], &mdstr, &lang, NULL );
				QTils_free(&mdstr); QTils_free(&lang);
				mdstr = NULL, lang = NULL;
				n += 1;
			}
			else if( lang ){
				QTils_free(&lang);
				lang = NULL;
			}
		}
	}
	if( n ){
		UpdateMovie(dstMovie);
	}
	return n;
}


/*!
	from http://developer.apple.com/library/mac/#qa/qa2001/qa1262.html

	Get the identifier for the media that contains the first
	video track's sample data, and also get the media handler for
	this media.
	@n
	20110105: if no enabled video track found, try a TimeCode track...
 */
static ErrCode MovieGetVideoMediaAndMediaHandler( Movie inMovie, Media *outMedia, MediaHandler *outMediaHandler )
{ ErrCode err;
	if( inMovie && outMedia && outMediaHandler ){
	  Track videoTrack;
		*outMedia = NULL;
		*outMediaHandler = NULL;

		/* get first video track with a framefrate */
		videoTrack = GetMovieIndTrackType(inMovie, 1, FOUR_CHAR_CODE('vfrr'),
						movieTrackCharacteristic | movieTrackEnabledOnly);
		if( videoTrack ){
			/* get media ref. for track's sample data */
			*outMedia = GetTrackMedia(videoTrack);
			if( *outMedia ){
				/* get a reference to the media handler component */
				*outMediaHandler = GetMediaHandler(*outMedia);
			}
		}
		err = GetMoviesError();
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	Given a reference to the media that contains the sample data for a track,
	calculate the static frame rate. That is, the rate at which "normal" playback
	would be done. This is simply the number of samples in the media, divided by
	its duration.
 */
ErrCode GetMediaStaticFrameRate( Media inMovieMedia, double *outFPS )
{ ErrCode err;
	if( inMovieMedia && outFPS ){
	  /* get the number of samples in the media */
	  long sampleCount = GetMediaSampleCount(inMovieMedia);

		err = GetMoviesError();
		if( err == noErr && !sampleCount ){
			err = invalidMedia;
		}
		if( err == noErr ){
		  /* find the media duration expressed in the media's timescale */
		  TimeValue64 duration = GetMediaDisplayDuration(inMovieMedia);
			err = GetMoviesError();
			if( err == noErr ){
			  /* get the media time scale */
			  TimeValue64 timeScale = GetMediaTimeScale(inMovieMedia);
				err = GetMoviesError();
				if( err == noErr ){
					/* calculate the frame rate:
					 * frame rate	= (sample count) / (duration in seconds)
					 *			= (sample count) * (media time scale) / (media duration)
					 */
					*outFPS = (double)sampleCount * (double)timeScale / (double)duration;
				}
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	the static framerate of a movie, calculated from its first video track.
	@param	theMovie	the movie for the operation
	@param	outFPS	the best guessed frame rate: fpsTC if available and not too different from fpsMedia
	@param	fpsTC	frame rate determined from the (optional, first found) TimeCode track.
					TODO: check that the TimeCode track is associated with the video track?
	@param	fpsMedia	frame rate determined by GetMediaStaticFrameRate
 */
ErrCode GetMovieStaticFrameRate( Movie theMovie, double *outFPS, double *fpsTC, double *fpsMedia )
{ ErrCode err;
	if( theMovie ){
	  Media theMedia;
	  MediaHandler mh;
	  Track TCTrack;
	  double tcfps = -1, mfps = -1;
		TCTrack = GetMovieIndTrackType( theMovie, 1, TimeCodeMediaType,
								    movieTrackMediaType | movieTrackEnabledOnly
		);
		err = GetMoviesError();
		if( err == noErr && TCTrack ){
		  MediaHandler mh;
		  TimeCodeDef tcdef;
			mh = GetMediaHandler( GetTrackMedia(TCTrack) );
			err = (ErrCode) TCGetCurrentTimeCode( mh, NULL, &tcdef, NULL, NULL );
			if( err == noErr ){
				// the TimeCode frameRate must be the next highest integer if the fraction
				// is not integer to start with.
				tcfps = ceil( ((double)tcdef.fTimeScale) / ((double)tcdef.frameDuration) );
			}
		}
		err = MovieGetVideoMediaAndMediaHandler( theMovie, &theMedia, &mh );
		if( err == noErr && theMedia ){
			err = GetMediaStaticFrameRate( theMedia, &mfps );
		}
		if( outFPS ){
			if( tcfps >= 0 && fabs(tcfps - mfps) < 0.01 ){
				*outFPS = tcfps;
			}
			else if( mfps >= 0 ){
				*outFPS = mfps;
			}
		}
		if( fpsTC ){
			*fpsTC = tcfps;
		}
		if( fpsMedia ){
			*fpsMedia = mfps;
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

////////
//!
//! from Apple's qtframestepper.c :
//!
//! QTStep_GetStartTimeOfFirstVideoSample
//! Return, through the theTime parameter, the starting time of the first video sample of the
//! specified QuickTime movie.
//!
//! The "trick" here is to set the nextTimeEdgeOK flag, to indicate that you want to get the
//! starting time of the beginning of the movie.
//!
//! If this function encounters an error, it returns a (bogus) starting time of -1. Note that
//! GetMovieNextInterestingTime also returns -1 as a starting time if the search criteria
//! specified in the myFlags parameter are not matched by any interesting time in the movie.
//!
//////////

static OSErr QTStep_GetStartTimeOfFirstVideoSample (Movie theMovie, TimeValue *theTime)
{
	short			myFlags;
	OSType			myTypes[1];

	*theTime = -1;			// a bogus starting time
	if (theMovie == NULL)
		return(invalidMovie);

	myFlags = nextTimeMediaSample + nextTimeEdgeOK;			// we want the first sample in the movie
	myTypes[0] = VisualMediaCharacteristic;					// we want video samples

	GetMovieNextInterestingTime(theMovie, myFlags, 1, myTypes, (TimeValue)0, fixed1, theTime, NULL);
	return(GetMoviesError());
}

/*!
	Gets a movie's starting time, preferring absolute time if the movie has a TimeCode track
 */
ErrCode GetMovieStartTime( Movie theMovie, Track TCTrack, TimeRecord *tr, long *startFrameNr )
{ TimeRecord mTime;
  TimeValue t;
  ErrCode err;
	if( theMovie && tr ){
	  MediaHandler mh;
	  long frameNr;
	  TimeCodeDef tcdef;
	  TimeCodeRecord tcrec;
	  SInt64 st = -1;
	  TimeValue startTime;
		QTStep_GetStartTimeOfFirstVideoSample( theMovie, &startTime );
		if( TCTrack ){
			mh = GetMediaHandler( GetTrackMedia(TCTrack) );
			if( mh && GetMoviesError() == noErr ){
				err = (ErrCode) TCGetTimeCodeAtTime( mh, (TimeValue) startTime, &frameNr, &tcdef, &tcrec, NULL );
				if( err == noErr && !(tcdef.flags & tcCounter) ){
				  double TCframeRate = ((double)tcdef.fTimeScale) / ((double)tcdef.frameDuration);
					// starttime is the TimeCode value for TimeValue==0
					st = (SInt64)( FTTS(&tcrec.t, TCframeRate) + 0.5 );
					if( startFrameNr ){
						*startFrameNr = frameNr;
					}
				}
#ifdef DEBUG
				{ int i, n=3;
				  HandlerError e;
					for( i = 0 ; i < n ; i++ ){
						e = TCFrameNumberToTimeCode( mh, frameNr+i, &tcdef, &tcrec );
					}
				}
#endif
			}
		}
		mTime.base = GetMovieTimeBase(theMovie);
		t = GetTimeBaseStartTime(mTime.base, GetMovieTimeScale(theMovie), tr );
		if( st != -1 ){
		  SInt64 *trv = (SInt64*) &tr->value;
			tr->scale = tcdef.fTimeScale;
			*trv = st * tr->scale;
		}
		else{
			err = GetMoviesError();
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	specify a movie's starting time. Contrary to its "getter" counterpart GetMovieStartTime(),
	this function does not attempt to do anything with TimeCode track(s).
 */
void SetMovieStartTime( Movie theMovie, TimeValue startTime, int changeTimeBase )
{ TimeRecord mTime;
	GetMovieTime( theMovie, &mTime );
	if( changeTimeBase ){
	  TimeRecord tr;
		mTime.base = GetMovieTimeBase(theMovie);
		GetTimeBaseStartTime(mTime.base, GetMovieTimeScale(theMovie), &tr );
		*( (SInt64*)&(tr.value) ) = startTime;
		SetTimeBaseStartTime(mTime.base, &tr );
//		SetMovieMasterTimeBase(theMovie, mTime.base, nil );
	}
	mTime.value.hi = 0;
	mTime.value.lo = startTime;
	SetMovieTime( theMovie, &mTime );
}

OSErr MaybeShowMoviePoster( Movie theMovie )
{ QTVisualContextRef ctx;
  OSErr err;
	if( theMovie ){
		err = (OSErr) GetMovieVisualContext( theMovie, &ctx );
		if( err == noErr ){
			ShowMoviePoster( theMovie );
			err = GetMoviesError();
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode SlaveMovieToMasterMovie( Movie slave, Movie master )
{ TimeRecord slaveZero;
	if( slave && master ){
		GetTimeBaseStartTime( GetMovieTimeBase(master), GetMovieTimeScale(master), &slaveZero );
		SetTimeBaseMasterTimeBase( GetMovieTimeBase(slave),
							 GetMovieTimeBase(master),
							 &slaveZero );
		return GetMoviesError();
	}
	else{
		return paramErr;
	}
}

// 20101017: kudos to Jan Schotsman for giving the solution on how to create a "dref atom" that allows to
// store all the TimeCode information in theFile->theMovie. The second approach using PtrToHand comes from
// http://developer.apple.com/library/mac/#qa/qa1539/_index.html
ErrCode DataRefFromVoid( Handle *dataRef, OSType *dataRefType )
{ unsigned long drefAtomHeader[2];
  ErrCode err = paramErr;
	if( dataRef && dataRefType ){
#if 1
		*dataRef = NewHandleClear( sizeof(Handle) + sizeof(char) );

		drefAtomHeader[0] = EndianU32_NtoB(sizeof(drefAtomHeader));
		drefAtomHeader[1] = EndianU32_NtoB( kDataRefExtensionInitializationData );
		err = (ErrCode) PtrAndHand( drefAtomHeader, *dataRef, sizeof(drefAtomHeader) );
#else
	  Handle hMovieData;
		*dataRef = NULL;
		hMovieData = NewHandle(0);
		err = PtrToHand( &hMovieData, dataRef, sizeof(Handle) );
#endif
		if( err == noErr ){
			*dataRefType = HandleDataHandlerSubType;
		}
		else{
			*dataRef = NULL;
		}
	}
	return err;
}

/*!
	QuickTime uses a number of proprietary ways to access files, one of which consists of a "data handle"
	(not to be confounded with handler!), the data references, and its associated type.
	URLFromDataRef() takes such a reference/type pair, and attempts to determine the full path (URL) and
	name of the referenced file. The file name is returned as a Pascal string...
 */
static ErrCode URLFromDataRef( Handle dataRef, OSType dataRefType, char *theURL, int maxLen, Str255 fileName )
{ DataHandler dh;
  ErrCode err = invalidDataRef;
  ComponentResult result;
	if( dataRef ){
		if( theURL ){
		  CFStringRef outPath = NULL;
			// obtain the full path as a CFString, which is an abstract "text" type with associated encoding (etc).
			err = QTGetDataReferenceFullPathCFString(dataRef, dataRefType, (QTPathStyle)kQTNativeDefaultPathStyle, &outPath);
			if( err == noErr && outPath ){
				// convert the CFString to a C string:
#if __APPLE_CC__
				CFStringGetFileSystemRepresentation( outPath, theURL, maxLen );
#else
				CFStringGetCString( outPath, theURL, maxLen, CFStringGetSystemEncoding() );
#endif
#ifdef DEBUG
				if( *theURL ){
					Log( qtLogPtr, "dataRef theURL=\"%s\"\n", theURL );
				}
#endif
				CFAllocatorDeallocate( kCFAllocatorDefault, (void*) outPath );
			}
		}
		if( fileName ){
			// if the user requested to know the Pascal string fileName, we try to determine that name
			// via a complementary procedure, via the data handler for the file in question.
			fileName[0] = 0;
			result = OpenAComponent( GetDataHandler( dataRef, dataRefType, kDataHCanRead), &dh );
			if( result == noErr ){
				result = DataHSetDataRef( dh, dataRef );
				if( result == noErr ){
					result = DataHGetFileName( dh, fileName );
				}
			}
		}
	}
	return err;
}

#if TARGET_OS_MAC
static ErrCode CFURLFromDataRef( Handle dataRef, OSType dataRefType, CFURLRef *theURL )
{ char path[PATH_MAX];
  ErrCode err = paramErr;
	if( theURL && (err = URLFromDataRef( dataRef, dataRefType, path, sizeof(path), NULL ) ) == noErr ){
		*theURL = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8 *) path, strlen(path), false );
	}
	return err;
}
#endif

void DisposeDataRef(Handle dataRef)
{
	if( dataRef ){
		DisposeHandle(dataRef);
	}
}

void DisposeMemoryDataRef(MemoryDataRef *memRef)
{
	if( memRef ){
		if( memRef->dataRef ){
			DisposeHandle(memRef->dataRef);
			memRef->dataRef = NULL;
		}
		if( memRef->memory ){
			DisposeHandle(memRef->memory);
			memRef->memory = NULL;
		}
		if( memRef->virtURL ){
			QTils_free(&memRef->virtURL);
			memRef->virtURL = NULL;
		}
	}
}

/*!
	QuickTime uses a number of proprietary ways to access files, one of which consists of a "data handle"
	(not to be confounded with handler!), the data references, and its associated type.
	@n
	DataRefFromURL() tries to obtain
	a reference/type pair for a given file or http address. If the URL is relative (just a filename, or with
	a relative path specification), a full path is constructed by prepending the current working directory.
	@n
	Note that on MS Windows, paths referencing the parent directory are not supported, which appears to be a
	bug in QuickTime.
 */
ErrCode DataRefFromURL( const char **URL, Handle *dataRef, OSType *dataRefType )
{ ErrCode err;
  CFStringRef URLRef = (CFStringRef) nil;
  int hasFullPath, freeAnchor = FALSE, isHTTP;
  char *theURL_anchor, *theURL = NULL;
	if( !URL || !*URL || !**URL || !dataRef || !dataRefType ){
		return paramErr;
	}
	if( strncasecmp( *URL, "ftp://", 6 ) == 0 || strncasecmp( *URL, "http://", 7 ) == 0 ){
		isHTTP = TRUE;
	}
	else{
		isHTTP = FALSE;
	}
#if TARGET_OS_WIN32
	if( !isHTTP ){
		theURL = (char*) *URL;
		theURL++;
		while( *theURL ){
			if( *theURL == '.' && theURL[-1] == '.' ){
				Log( qtLogPtr, "DataRefFromURL(\"%s\"): URL contains an unsupported reference to the parent directory!\n",
				    *URL
				);
				return paramErr;
			}
			theURL++;
		}
	}
#endif
	// we make a local working copy as we will be modifying the string
	theURL = QTils_strdup(*URL);
	if( !theURL ){
		return MemError();
	}
	// we keep a copy for later...
	theURL_anchor = theURL;

	if( isHTTP ){
		goto getURLReference;
	}

	if( theURL[0] == '.' ){
		if(
#if TARGET_OS_WIN32
		   theURL[1] == '\\'
#else
		   theURL[1] == '/'
#endif
		){
			// 20101202: we don't want to get stuck on the '/' or '\' character,
			// so we have to increment TWICE, not once!
			theURL += 2;
		}
	}
#if TARGET_OS_WIN32
	hasFullPath = (theURL_anchor[1] == ':'
				|| (theURL_anchor[0]=='\\' && theURL_anchor[1]=='\\')
				|| (theURL_anchor[0]=='/' && theURL_anchor[1]=='/')
			);
#else
	hasFullPath = (theURL_anchor[0] == '/');
#endif
	// not a fully specified path, so we get the current working directory, and prepend it
	// in the appropriate way to our filename.
	if( !hasFullPath ){
	  char *c, cwd[1024];
#ifdef _MSC_VER
	  extern char* _getcwd(char*, int);
		c = _getcwd( cwd, sizeof(cwd) );
#else
		c = getwd( cwd );
#endif
		if( c ){
			if( (c = (char*) QTils_malloc( (strlen(cwd) + strlen(theURL) + 2) * sizeof(char) )) ){
				strcpy( c, cwd );
#if TARGET_OS_WIN32
				strcat( c, "\\" );
#else
				strcat( c, "/" );
#endif
				strcat( c, theURL );
				freeAnchor = TRUE;
				theURL = c;
				*URL = theURL;
			}
		}
	}
#if TARGET_OS_WIN32
	// 20101027: correct the pathname for MSWindows, if necessary (using either the DOS or the POSIX convention) :
#	ifndef POSIX_PATHNAMES
	if( strchr(theURL, '/') ){
	  char *c = theURL, *prev_c = NULL;
		if( *c == '/' ){
			*c++ = '\\';
			prev_c = c;
		}
		while( *c ){
			if( *c == '/' /* && (c[-1] != '\\' || &c[-1] == prev_c) */ ){
				*c = '\\';
				prev_c = c;
			}
			c++;
		}
	}
#	else
	if( strchr(theURL, '\\') ){
	  char *c = theURL;
		if( *c == '\\' ){
			*c++ = '/';
		}
		while( *c ){
			if( *c == '\\' && c[-1] != '\\' ){
				*c = '/';
			}
			c++;
		}
	}
#	endif // POSIX_PATHNAMES
#endif // TARGET_OS_WIN32

getURLReference:
	{ CFIndex n;
		// We first need to create a CFString from our C string:
		URLRef = CFStringCreateWithCString(kCFAllocatorDefault, theURL, CFStringGetSystemEncoding() );
#ifdef DEBUG
		CFShowStr(URLRef);
#endif
		n = CFStringGetLength(URLRef);
		if( URLRef ){
			if( n == strlen(theURL) ){
				if( isHTTP ){
					err = QTNewDataReferenceFromURLCFString( URLRef, 0, dataRef, dataRefType );
				}
				else{
					// upon success, we can call a function that does what we want:
					err = QTNewDataReferenceFromFullPathCFString( URLRef,
#ifdef POSIX_PATHNAMES
								kQTPOSIXPathStyle,
#else
								kQTNativeDefaultPathStyle,
#endif // POSIX_PATHNAMES
								0, dataRef, dataRefType
					);
				}
#ifdef DEBUG
				// debug: check if the reverse map yields our original URL!
				{ Str255 fname;
				  char path[1024];
					if( URLFromDataRef( *dataRef, *dataRefType, path, sizeof(path), fname ) != noErr ){
						fname[0] = 0;
						path[0] = '\0';
					}
					Log( qtLogPtr, "\"%s\" -> \"%s\" -> dref=%p,%.4s (-> \"%s\",\"%s\") err=%d\n",
						theURL_anchor, theURL, *dataRef, OSTStr(*dataRefType),
						path, P2Cstr(fname),
						err
					);
				}
#endif
			}
			CFAllocatorDeallocate( kCFAllocatorDefault, (void*) URLRef );
		}
		else{
			err = internalQuickTimeError;
			goto bail;
		}
	   }

bail:
	if( freeAnchor ){
		QTils_free(&theURL_anchor);
	}
	if( *URL != theURL && theURL ){
		QTils_free(&theURL);
	}
	return err;
}

/*!
	Initialises a structure containing a dataRef/dataRefType combination corresponding
	to the data passed in the string argument. If len==0 and string!=NULL, len will be
	set to strlen(string).
 */
ErrCode MemoryDataRefFromMemory( const char *string, size_t len, const char *virtURL, MemoryDataRef *memRef )
{ ErrCode err = paramErr;
	if( string && memRef ){
		if( !len ){
			len = strlen(string);
		}
		memRef->dataRef = NULL;
		memRef->memory = NewHandle(len);
		memcpy( *(memRef->memory), string, len );
		err = PtrToHand( &(memRef->memory), &(memRef->dataRef), sizeof(Handle) );
		if( err != noErr ){
			memRef->dataRef = NULL;
			DisposeHandle(memRef->memory);
			memRef->memory = NULL;
		}
		else{
			memRef->dataRefType = HandleDataHandlerSubType;
		}
		if( virtURL ){
			memRef->virtURL = QTils_strdup( (virtURL)? virtURL : "<in-memory>" );
		}
	}
	return err;
}

/*!
	Initialises a structure containing a dataRef/dataRefType combination corresponding
	to the data passed in the string argument. If len==0 and string!=NULL, len will be
	set to strlen(string).
 */
ErrCode MemoryDataRefFromString( const char *string, const char *virtURL, MemoryDataRef *memRef )
{ ErrCode err = paramErr;
	if( string && memRef ){
		err = MemoryDataRefFromMemory( string, strlen(string), virtURL, memRef );
	}
	return err;
}

ErrCode MemoryDataRefFromString_Mod2( const char *string, int len, const char *virtURL, int vlen, MemoryDataRef *memRef )
{ ErrCode err = paramErr;
	if( string && memRef ){
		err = MemoryDataRefFromMemory( string, strlen(string), virtURL, memRef );
	}
	return err;
}

/*!
	Adds media to a given track in a movie, of the specified type, and that will refer to the specified
	file (theURL).
 */
ErrCode NewImageMediaForURL( Movie theMovie, Track theTrack, OSType mediaType, char *theURL, MediaRefs *mref )
{ ErrCode err = noErr;
  Handle cDataRef = nil;
  OSType cDataRefType = 0;

	if( !mref ){
		return paramErr;
	}
	if( theURL && *theURL ){
	  char *orgURL = theURL;
		// Obtaining a dataRef to the input file first, as this is
		// how QuickTime knows where to look for the sample references in the sample table.
		err = DataRefFromURL( (const char**) &theURL, &cDataRef, &cDataRefType );
		if( theURL && theURL != orgURL ){
			QTils_free(&theURL);
			theURL = orgURL;
		}
	}
	if( err != noErr ){
	  char *c, cwd[1024];
#ifdef _MSC_VER
		c = _getcwd( cwd, sizeof(cwd) );
#else
		c = getwd( cwd );
#endif
		Log( qtLogPtr, "NewImageMediaForURL(): cannot get dataRef for \"%s\" (%d; cwd=%s)\n",
		    theURL, err,
		    (c)? c : "??"
		);
	}
	else{
		mref->dataRef = cDataRef;
		mref->dataRefType = cDataRefType;
		mref->theMedia = NewTrackMedia( theTrack, mediaType,
						    GetMovieTimeScale(theMovie), cDataRef, cDataRefType
		);
		if( !mref->theMedia ){
			err = GetMoviesError();
		}
	}
	return err;
}

/*!
	Copy all tracks in a source movie into a destination movie. The start and poster times are also
	copied, but not the meta data!
 */
ErrCode CopyMovieTracks( Movie dstMovie, Movie srcMovie )
{ long tracks = GetMovieTrackCount(dstMovie);
  long i, srcTracks = GetMovieTrackCount(srcMovie), n;
  Track srcTrack = (Track) -1, dstTrack = nil;
  Media srcMedia = nil, dstMedia = nil;
  Handle cdataRef;
  OSType cdataRefType;
  ErrCode err = noErr;

	if( srcTracks > 0 ){
		SetMovieTimeScale( dstMovie, GetMovieTimeScale(srcMovie) );
	}
	for( n = 0, i = 1; i <= srcTracks && srcTrack && err == noErr; i++ ){
	  OSType srcTrackType;
		srcTrack = GetMovieIndTrack(srcMovie, i);
		err = GetMoviesError();
		if( srcTrack && err == noErr ){
		  short refcnt;
		  long attr;
			srcMedia = GetTrackMedia(srcTrack);
			err = GetMoviesError();
			if( err != noErr ){
				srcMedia = nil;
			}
			else{
				GetMediaHandlerDescription(srcMedia, &srcTrackType, nil, nil);
				GetMediaDataRefCount( srcMedia, &refcnt );
				if( (err = GetMediaDataRef( srcMedia, MIN(1,refcnt), &cdataRef, &cdataRefType, &attr )) ){
					Log( qtLogPtr, "Error retrieving MediaDataRef for track %ld with %hd datarefs (%d)\n",
						i, refcnt, (int) err
					);
					cdataRef = nil, cdataRefType = 0;
					err = noErr;
				}
			}
		}
		if( srcTrack && srcTrack != ((Track)-1) && srcMedia ){
			{ Fixed w, h;
				GetTrackDimensions( srcTrack, &w, &h );
#if 1
				err = AddEmptyTrackToMovie( srcTrack, srcMovie, cdataRef, cdataRefType, &dstTrack );
				if( err == noErr && dstTrack ){
					dstMedia = GetTrackMedia(dstTrack);
					err = GetMoviesError();
				}
#else
				dstTrack = NewMovieTrack( dstMovie, w, h, GetTrackVolume(srcTrack) );
#endif
				err = GetMoviesError();
			}
			if( dstTrack && err == noErr ){
				if( !dstMedia ){
					// dataRef,dataRefType refer to srcMovie on disk, but passing them in gives
					// a couldNotResolveDataRef on QT 7.6.6/Win32?!
					dstMedia = NewTrackMedia( dstTrack, srcTrackType /*VideoMediaType*/,
								GetMovieTimeScale(dstMovie), cdataRef, cdataRefType
					);
					err = GetMoviesError();
				}
				if( dstMedia && err == noErr ){
					BeginMediaEdits(dstMedia);
					err = InsertTrackSegment( srcTrack, dstTrack, 0, GetTrackDuration(srcTrack), 0 );
					EndMediaEdits(dstMedia);
					// no need to call InsertMediaIntoTrack here.
				}
				else{
					Log( qtLogPtr, "error creating '%.4s' media for destination track %ld (%d)\n",
						OSTStr(cdataRefType), tracks + 1 + i, err
					);
				}
				if( cdataRef ){
					DisposeHandle( (Handle)cdataRef );
				}
			}
			else{
				err = GetMoviesError();
			}
			if( err != noErr ){
				DisposeMovieTrack( dstTrack );
				dstTrack = nil;
				Log( qtLogPtr, "error inserting track %ld into destination track %ld (%d)\n",
					i, tracks + 1 + i, err
				);
			}
			else{
				dstTrack = nil;
			}
			n += 1;
		}
	}
	if( n ){
		SetMoviePosterTime( dstMovie, GetMoviePosterTime(srcMovie) );
		{ TimeRecord mTime;
			GetMovieTime( dstMovie, &mTime );
			mTime.value.hi = 0;
			mTime.value.lo = GetMoviePosterTime(srcMovie);
			SetMovieTime( dstMovie, &mTime );
		}
		MaybeShowMoviePoster(dstMovie);
		UpdateMovie( dstMovie );
	}
	return err;
}

/*!
	Creates an empty movie in the given file (URL), with the specified creator tag. For the scriptTag,
	pass smCurrentScript. The function returns the reference/type pair, as well as the datahandler for
	storing data in the file, and the corresponding Movie object (for in-memory operations).
 */
ErrCode CreateMovieStorageFromURL( const char *URL, OSType creator, ScriptCode scriptTag, long flags,
						   Handle *dataRef, OSType *dataRefType,
						   DataHandler *outDataHandler, Movie *newMovie )
{ ErrCode err, wihErr;
  const char *orgURL = URL;
	err = DataRefFromURL( &URL, dataRef, dataRefType );
	if( err == noErr ){
		err = CreateMovieStorage( *dataRef, *dataRefType, creator,
					scriptTag, flags, outDataHandler, newMovie
		);
		if( err == noErr ){
#ifndef QTMOVIESINK
			InitQTMovieWindowHFromMovie( NewQTMovieWindowH(), URL, *newMovie,
								   *dataRef, *dataRefType, *outDataHandler, 0, &wihErr
			);
#endif
		}
		else{
			DisposeHandle( (Handle) *dataRef );
			*dataRef = NULL;
			*dataRefType = 0;
			*newMovie = NULL;
		}
	}
	if( URL && URL != orgURL ){
		QTils_free( (char**) &URL);
		URL = orgURL;
	}
	return err;
}

// *UNDOCUMENTED*
const char *lastMovieOpenedURL = NULL;

// Given the file <URL>, open it as a QuickTime movie. The function returns the Movie object (in-memory "handle")
// and the reference/type pair if requested. flags should be 0 or 1. <id> refers to the file's movie resource(s)
// and should be 0 or NULL.
ErrCode OpenMovieFromURL( Movie *newMovie, short flags, short *id,
					const char *URL, Handle *dataRef, OSType *dataRefType )
{ ErrCode err;
  const char *orgURL = URL;
  Handle ldataRef;
  OSType ldataRefType;
  DataHandler ldataHandler = NULL;
  short lid = 0, noFree = FALSE;
  ErrCode wihErr;

	if( !URL || !*URL ){
		lastMovieOpenedURL = URL = AskFileName( "Please choose a video or movie to open" );
		noFree = TRUE;
	}
	if( !dataRef ){
		dataRef = &ldataRef;
	}
	if( !dataRefType ){
		dataRefType = &ldataRefType;
	}
	if( !id ){
		id = &lid;
	}
	else{
		*id = 0;
	}
	err = DataRefFromURL( &URL, dataRef, dataRefType );
	if( err == noErr ){
// no real need for this:
//		err = OpenMovieStorage( *dataRef, *dataRefType, kDataHCanRead|kDataHCanWrite, &ldataHandler );
		err = NewMovieFromDataRef( newMovie, flags, id, *dataRef, *dataRefType );
#ifdef DEBUG
		if( err != noErr ){
			Log( qtLogPtr, "OpenMovieFromURL(\"%s\"): NewMovieFromDataRef(flags=%d,id=%d,dref=%p,type=%.4s) failed with error %d\n",
				URL, flags, (id)? *id : -1, *dataRef, OSTStr(*dataRefType), err
			);
		}
#endif
#ifndef QTMOVIESINK
		InitQTMovieWindowHFromMovie( NewQTMovieWindowH(), URL, *newMovie,
							   *dataRef, *dataRefType, ldataHandler, *id, &wihErr
		);
#endif
		if( URL && URL != orgURL && !noFree ){
			QTils_free( (char**) &URL);
			URL = orgURL;
		}
	}
	return err;
}

ErrCode OpenMovieFromURL_Mod2( Movie *newMovie, short flags, char *URL, int ulen )
{ ErrCode ret;
	ret = OpenMovieFromURL( newMovie, flags, NULL, URL, NULL, NULL );
	if( !*URL ){
		if( lastMovieOpenedURL ){
			strncpy( URL, lastMovieOpenedURL, ulen );
			URL[ulen-1] = '\0';
		}
	}
	return ret;
}

ErrCode OpenMovieFromMemoryDataRef( Movie *newMovie, MemoryDataRef *memRef, OSType contentType )
{ ErrCode err = couldntGetRequiredComponent;
  DataHandler ldataHandler = NULL;
  ErrCode wihErr;
  MovieImportComponent miComponent = OpenDefaultComponent( MovieImportType, contentType );
  Track usedTrack = nil;
  TimeValue addedDuration = 0;
  long outFlags = 0;
	if( miComponent ){
		*newMovie = NewMovie(0);
		err = (ErrCode) MovieImportDataRef( miComponent, memRef->dataRef, memRef->dataRefType, *newMovie, nil,
							   &usedTrack, 0, &addedDuration,
							   movieImportCreateTrack, &outFlags );
		if( err == noErr ){
			GoToBeginningOfMovie(*newMovie);
		}
	}
	else{
		err = GetMoviesError();
	}
#ifndef QTMOVIESINK
	if( err == noErr ){
	  QTMovieWindowH wih = NULL;
		wih = InitQTMovieWindowHFromMovie( NewQTMovieWindowH(), memRef->virtURL, *newMovie,
							   memRef->dataRef, memRef->dataRefType, ldataHandler, 1, &wihErr
		);
		if( wih ){
			(*wih)->memRef = memRef;
		}
	}
#endif
	return err;
}

// Saves the given movie in a file. If noDialog is False, or fname is NULL or points to an empty string,
// QuickTime posts a Save-As dialog that allows to choose the destination directory and final filename, as
// well as the export/transcoding settings.
ErrCode SaveMovieAs( char **fname, Movie theMovie, int noDialog )
{ Handle odataRef;
  OSType odataRefType;
  long flags = createMovieFileDeleteCurFile;
  char oname[1024] = "";
  ErrCode err;
#if TARGET_OS_MAC
  Boolean excl, exclPath;
  CFURLRef urlRef = NULL;
#endif

	if( !theMovie ){
		return paramErr;
	}

	if( !noDialog || !fname || !*fname ){
		flags |= showUserSettingsDialog;
	}

	if( fname && *fname && **fname ){
	  char *orgURL = *fname, *URL = *fname;
		// ConvertMovieToDataRef() wants a reference/type pair, evidently.
		err = DataRefFromURL( (const char**) &URL, &odataRef, &odataRefType );
		if( err == noErr ){
			flags |= movieFileSpecValid;
		}
		else{
			odataRef = nil;
			odataRefType = AliasDataHandlerSubType;
		}
#if TARGET_OS_MAC
		urlRef = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8*) URL, strlen(URL), false );
#endif
		if( URL && URL != orgURL ){
			QTils_free( (char**) &URL);
		}
	}
	else{
		odataRef = nil;
		odataRefType = AliasDataHandlerSubType;
	}

	{ char *c, cwd[1024];
#ifdef _MSC_VER
	  extern char* _getcwd(char*, int);
		c = _getcwd( cwd, sizeof(cwd) );
#else
		c = getwd( cwd );
#endif
		SetMovieProgressProc( theMovie, (MovieProgressUPP)-1L, 0);
#if TARGET_OS_MAC
		if( urlRef ){
			// exclude the file from being backed up by TimeMachine to work around
			// a bug on 10.7 (20111124):
			excl = CSBackupIsItemExcluded( urlRef, &exclPath );
			CSBackupSetItemExcluded( urlRef, false, true );
		}
#endif
		err = ConvertMovieToDataRef( theMovie,	/* identifies movie */
			nil,							/* all tracks */
			odataRef, odataRefType,			/* file specification; will be updated! */
			MovieFileType, 'TVOD',
			flags, 0
		);
#if TARGET_OS_MAC
		if( urlRef ){
			CSBackupSetItemExcluded( urlRef, excl, exclPath );
			CFRelease(urlRef);
		}
#endif
		URLFromDataRef( odataRef, odataRefType, oname, sizeof(oname)/sizeof(char), NULL );
		if( c ){
			// ConvertMovieToDataRef() may have changed our working directory: change back!
#ifdef _MSC_VER
			_chdir(c);
#else
			chdir(c);
#endif
		}
	}
	if( err == userCanceledErr ){
		Log( qtLogPtr, "SaveMovieAs(%p-?>\"%s\") cancelled\n", theMovie, oname );
	}
	else{
		Log( qtLogPtr, "SaveMovieAs(%p->\"%s\") returned %d\n", theMovie, oname, (int) err );
	}
	if( err == noErr && fname ){
		*fname = QTils_strdup(oname);
	}
	if( odataRef == nil && err == paramErr ){
		// ConvertMovieToDataRef() accepts a nil odataRef handle and will save to whatever file
		// the user selects - but it will not update odataRef, and returns paramErr. This
		// is not actually an error, IMHO.
		err = noErr;
	}
	return err;
}

ErrCode SaveMovieAs_Mod2( char *fname, int flen, Movie theMovie, int noDialog )
{ ErrCode err;
  // initialise a local string variable with the passed-in filename, or NULL if the string is empty:
  char *URL = (*fname)? fname : NULL;
	err = SaveMovieAs( &URL, theMovie, noDialog );
	if( err == noErr ){
		if( URL != fname ){
#if 0
			// SaveMovieAs() does not actually appear to change the filename string...
		  size_t len = strlen(URL);
			if( len >= flen ){
				len = flen - 1;
			}
			strncpy( fname, URL, len );
			fname[len] = '\0';
#endif
			QTils_free(&URL);
		}
	}
	return err;
}

ErrCode SaveMovie( Movie theMovie )
{ Handle odataRef = NULL;
  OSType odataRefType;
  DataHandler odataHandler = NULL;
  ErrCode err;
  Boolean closeDH;
#ifndef QTMOVIESINK
  QTMovieWindowH wih = QTMovieWindowH_from_Movie(theMovie);
	if( wih && (*wih)->self == (*wih) ){
		odataRef = (*wih)->dataRef;
		odataRefType = (*wih)->dataRefType;
		odataHandler = (*wih)->dataHandler;
		closeDH = FALSE;
		err = noErr;
	}
#endif
	if( !odataRef ){
		err = GetMovieDefaultDataRef( theMovie, &odataRef, &odataRefType );
	}
	if( err == noErr ){
#if TARGET_OS_MAC
	  Boolean excl, exclPath;
	  CFURLRef theURL = NULL;
		if( CFURLFromDataRef( odataRef, odataRefType, &theURL ) == noErr && theURL ){
			excl = CSBackupIsItemExcluded( theURL, &exclPath );
			CSBackupSetItemExcluded( theURL, false, true );
		}
#endif
		if( !odataHandler ){
			err = OpenMovieStorage( odataRef, odataRefType, kDataHCanRead|kDataHCanWrite, &odataHandler );
			closeDH = TRUE;
		}
		if( err == noErr ){
			err = UpdateMovieInStorage( theMovie, odataHandler );
#if TARGET_OS_MAC
			if( theURL ){
				CSBackupSetItemExcluded( theURL, excl, exclPath );
			}
#endif
			if( closeDH ){
				if( err == noErr ){
					err = CloseMovieStorage( odataHandler );
				}
				else{
					CloseMovieStorage(odataHandler);
				}
			}
		}
		// it's unclear whether one has to DisposeHandle(odataRef) here!
#if TARGET_OS_MAC
		if( theURL ){
			CFRelease(theURL);
		}
#endif
	}
	return err;
}

// save the given movie as a 'reference movie' to the specified file. No dialogs are posted.
// A reference movie is a sort of linked copy that can have its own metadata and edits, but
// obtains its media from the original file. NB: if <theMovie> is itself a reference movie,
// the new copy will refer to the original source material, not to <theMovie>!
ErrCode SaveMovieAsRefMov( const char *dstURL, Movie theMovie )
{ Handle odataRef;
  OSType odataRefType;
  DataHandler odataHandler;
  Movie oMovie;
  ErrCode err2;
#ifndef QTMOVIESINK
  QTMovieWindowH wih = QTMovieWindowH_from_Movie(theMovie), owih;
#endif

	if( !dstURL ){
		dstURL = AskSaveFileName( "Save as: please give/select a name for the reference movie" );
	}
    	err2 = CreateMovieStorageFromURL( dstURL, 'TVOD',
							   smCurrentScript, createMovieFileDeleteCurFile|createMovieFileDontCreateResFile,
							   &odataRef, &odataRefType,
							   &odataHandler, &oMovie
	);
	if( err2 == noErr ){
#ifndef QTMOVIESINK
		owih = QTMovieWindowH_from_Movie(oMovie);
#endif
		SetMovieProgressProc( oMovie, (MovieProgressUPP)-1L, 0);
		// stuff InsertMovieSegment() does NOT copy:
		SetMovieTimeScale( oMovie, GetMovieTimeScale(theMovie) );
		err2 = InsertMovieSegment( theMovie, oMovie, 0, GetMovieDuration(theMovie), 0 );
		if( err2 == noErr ){
			// more stuff InsertMovieSegment() does NOT copy:
			SetMovieStartTime( oMovie, GetMoviePosterTime(theMovie), TRUE );
			SetMoviePosterTime( oMovie, GetMoviePosterTime(theMovie) );
			CopyMovieSettings( theMovie, oMovie );
			CopyMovieUserData( theMovie, oMovie, kQTCopyUserDataReplace );
			CopyMovieMetaData( oMovie, theMovie );
			UpdateMovie(oMovie);
			MaybeShowMoviePoster(oMovie);
			UpdateMovie(oMovie);
			err2 = AddMovieToStorage( oMovie, odataHandler );
			if( err2 == noErr ){
				err2 = CloseMovieStorage( odataHandler );
			}
			else{
				CloseMovieStorage(odataHandler);
			}
#ifndef QTMOVIESINK
			if( owih ){
				(*owih)->dataHandler = NULL;
			}
#endif
		}
		CloseMovie(&oMovie);
	}
	else if( err2 == fBsyErr 
#ifndef QTMOVIESINK
		   && wih && (*wih)->theURL && strcmp((*wih)->theURL, dstURL) == 0
#endif
	){
		err2 = SaveMovie( theMovie );
	}
	return err2;
}

ErrCode SaveMovieAsRefMov_Mod2( const char *dstURL, int ulen, Movie theMovie )
{
	return SaveMovieAsRefMov( dstURL, theMovie );
}

ErrCode FlattenMovieToURL( const char *dstURL, Movie theMovie, Movie *theNewMovie )
{ Handle odataRef;
  OSType odataRefType;
  Movie oMovie = NULL;
  ErrCode err2 = 1;
  const char *orgURL = dstURL;
#if TARGET_OS_MAC
  Boolean excl, exclPath;
  CFURLRef urlRef = NULL;
#endif

	err2 = DataRefFromURL( &dstURL, &odataRef, &odataRefType );
	if( err2 == noErr ){
#if TARGET_OS_MAC
		urlRef = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8*) dstURL, strlen(dstURL), false );
		if( urlRef ){
			// exclude the file from being backed up by TimeMachine to work around
			// a bug on 10.7 (20111124):
			excl = CSBackupIsItemExcluded( urlRef, &exclPath );
			CSBackupSetItemExcluded( urlRef, false, true );
		}
#endif
		oMovie = FlattenMovieDataToDataRef( theMovie,
				flattenAddMovieToDataFork|flattenCompressMovieResource,
				odataRef, odataRefType, 'TVOD', smCurrentScript,
				createMovieFileDontOpenFile|createMovieFileDeleteCurFile|createMovieFileDontCreateResFile
		);
		err2 = GetMoviesError();
	}
	if( err2 == noErr && oMovie ){
		SetMovieProgressProc( oMovie, (MovieProgressUPP)-1L, 0);
		// stuff InsertMovieSegment() does NOT copy:
		SetMovieTimeScale( oMovie, GetMovieTimeScale(theMovie) );
		if( err2 == noErr ){
			// more stuff InsertMovieSegment() does NOT copy:
			SetMovieStartTime( oMovie, GetMoviePosterTime(theMovie), TRUE );
			SetMoviePosterTime( oMovie, GetMoviePosterTime(theMovie) );
			CopyMovieSettings( theMovie, oMovie );
			CopyMovieUserData( theMovie, oMovie, kQTCopyUserDataReplace );
			CopyMovieMetaData( oMovie, theMovie );
			UpdateMovie(oMovie);
			MaybeShowMoviePoster(oMovie);
			UpdateMovie(oMovie);
			if( theNewMovie ){
				*theNewMovie = oMovie;
			}
		}
	}
#if TARGET_OS_MAC
	if( urlRef ){
		CSBackupSetItemExcluded( urlRef, excl, exclPath );
		CFRelease(urlRef);
	}
#endif
	if( dstURL && dstURL != orgURL ){
		QTils_free( (char**) &dstURL);
	}
	return err2;
}

ErrCode CloseMovie( Movie *theMovie )
{
	if( theMovie && *theMovie ){
#ifndef QTMOVIESINK
	  QTMovieWindowH wih = QTMovieWindowH_from_Movie(*theMovie);
		if( wih ){
			if( (*wih)->theMovie ){
				// a movie is disposed off by CloseQTMovieWindow (called by DisposeQTMovieWindow)
				// so no need to do it again ourselves!
				DisposeQTMovieWindow(wih);
				*theMovie = nil;
				return noErr;
			}
			else{
				DisposeQTMovieWindow(wih);
				unregister_QTMovieWindowH_for_Movie(*theMovie);
			}
		}
#endif // QTMOVIESINK
		DisposeMovie(*theMovie);
		*theMovie = nil;
		return noErr;
	}
	else{
		return paramErr;
	}
}

unsigned short HasMovieChanged_Mod2( Movie theMovie )
{
	return (unsigned short) HasMovieChanged(theMovie);
}

void qtSetMoviePlayHints( Movie theMovie, unsigned long hints, int exclusive )
{
	if( theMovie ){
		/*hintsPlayingEveryFrame,  hintsHighQuality */;
		if( exclusive ){
			SetMoviePlayHints( theMovie, hints, 0xFFFFFFFF );
		}
		else{
			SetMoviePlayHints( theMovie, hints, hints );
		}
	}
}

double GetMovieDurationSeconds( Movie theMovie )
{
	if( theMovie ){
		return ((double) GetMovieDuration(theMovie) / ((double)GetMovieTimeScale(theMovie)));
	}
	else{
		return 0.0;
	}
}

double GetMovieTimeResolution( Movie theMovie )
{
	if( theMovie ){
		return (1.0 / ((double)GetMovieTimeScale(theMovie)));
	}
	else{
		return 0.0;
	}
}

// Add a TimeCode track to the current movie, which will allow to register wall time without
// adding a bogus frame that lasts from t=00:00:00 to t=<start time>.
// Code after Apple's QTKitTimecode example project, (C) Apple 2007

// a TimeCode track can have multiple segments, each of its own duration and with its own playback
// rate. the TCTrackInfo functions below provide an easy/simple interface to this feature.

//#define xfree(x)	if((x)){ free((x)); (x)=NULL; }
#define xfree(x)	QTils_free(&(x))

static void *dispose_TCTrackInfo( void *ptr )
{ TCTrackInfo *info = (TCTrackInfo*) ptr, *ret = NULL;
	if( info ){
		xfree( info->StartTimes );
		xfree( info->durations );
		xfree( info->frames );
		xfree( info->theURL );
		info->theTCTrack = nil;
		ret = info->cdr;
		QTils_free( (char**)&info );
	}
	return (void*) ret;
}

TCTrackInfo *dispose_TCTrackInfoList( TCTrackInfo *list )
{
	while( list ){
		list = dispose_TCTrackInfo(list);
	}
	return list;
}

static TCTrackInfo *new_TCTrackInfo( Track TCTrack, int N, double *StartTimes, double *durations, size_t *frames, const char *theURL )
{ TCTrackInfo *info = NULL;
  int i;
	if( N > 0 && theURL ){
		if( (info = (TCTrackInfo*) QTils_calloc( 1, sizeof(TCTrackInfo) ) ) ){
			if( (info->theURL = QTils_strdup(theURL))
			   && (info->StartTimes = (double*) QTils_malloc( N * sizeof(double) ))
			   && (info->durations = (double*) QTils_malloc( N * sizeof(double) ))
			   && (info->frames = (size_t*) QTils_malloc( N * sizeof(size_t) ))
			){
				for( i = 0 ; i < N ; i++ ){
					info->StartTimes[i] = StartTimes[i];
					info->durations[i] = durations[i];
					info->frames[i] = frames[i];
				}
				info->N = N;
				info->theTCTrack = TCTrack;
				info->dealloc = dispose_TCTrackInfo;
			}
			else{
				dispose_TCTrackInfo(info);
				info = NULL;
			}
		}
	}
	return info;
}

static int compare_TCTrackInfo( TCTrackInfo *info, int N, double *StartTimes, double *durations, size_t *frames )
{ int n = 0, ok = 1, i;
	if( info && N > 0 && info->N == N ){
		for( i = 0 ; i < N && ok ; i++ ){
			if( (StartTimes[i] == info->StartTimes[i])
			   && (durations[i] == info->durations[i])
			   && (frames[i] == info->frames[i])
			){
				n += 1;
			}
			else{
				ok = 0;
			}
		}
	}
	return n;
}

MovieFrameTime *secondsToFrameTime( double Time, double MovieFrameRate, MovieFrameTime *ft )
{
	if( ft ){
		ft->hours = (UInt8) (Time / 3600);
		Time -= ft->hours * 3600;
		ft->minutes = (UInt8) (Time / 60);
		Time -= ft->minutes * 60;
		ft->seconds = (UInt8) Time;
		Time -= ft->seconds;
		ft->frames = (UInt8) (Time * MovieFrameRate);
	}
	return ft;
}

#define kTimeCodeTrackMaxHeight (320 << 16)	// max height of timecode track
#define kTimeCodeTrackHeight (16)			// default hardcoded height for timecode track

/*!
	create a TimeCode track according to the <N> start times, durations and numbers of frames, adding it
	to the given movie & file (theURL), and associating it with the given track. The function returns
	a new TCTrackInfo description (a linked list), which will be used in subsequent invocations for the
	same movie and track to add segments to the track's TimeCode track.
 */
ErrCode AddTimeCodeTrack( Movie theMovie, Track theTrack, double trackStart, TCTrackInfo **TCInfoList,
				   int N, double *StartTimes, double *durations, size_t *frames,
				   Cartesian *translate, const char *theURL )
{ ErrCode err = noErr;
  Track theTCTrack = nil;
  TCTrackInfo *TCinfo;
  Media theTCMedia = nil;
  MediaHandler theTCMediaHandler = nil;
  TimeCodeDef theTCDef;
  TimeCodeRecord theTCRec;
  TimeCodeDescriptionHandle theTCSampleDescriptionH = NULL;
  long theMediaSample = 0L;
  int i, newtrack = 0;
  double totalDuration = 0;
  Handle dataRef = nil;
  OSType dataRefType;
  TimeValue timeScale;
  Fixed movieWidth, movieHeight;

	if( !theMovie || !theTrack || !TCInfoList
	   || !durations || !frames || !StartTimes
	){
		return (short) paramErr;
	}

	timeScale = GetMovieTimeScale(theMovie);

	if( (TCinfo = *TCInfoList) ){
		while( TCinfo && compare_TCTrackInfo(TCinfo, N, StartTimes, durations, frames)!=N ){
			TCinfo = TCinfo->cdr;
		}
		// if we find an entry for a TimeCode track with the exact same specifications, do not
		// create a new one. Instead, just add a reference for that track to the current image track.
		// this way, different concurrent views from the same movie will share a single TimeCode track.
		if( TCinfo ){
			theTCTrack = TCinfo->theTCTrack;
			goto add_track_reference;
		}
	}
	TCinfo = NULL;

	GetMovieDimensions( theMovie, &movieWidth, &movieHeight );
	if( !theTCTrack ){
		theTCTrack = NewMovieTrack( theMovie,
						movieWidth, Long2Fix(kTimeCodeTrackHeight), kNoVolume
		);
		newtrack = 1;
	}
	if( !theTCTrack ){
		err = GetMoviesError();
		goto bail;
	}

	if( DataRefFromVoid( &dataRef, &dataRefType ) == noErr ){
		// success, now create a track media based on the dataRef!
		theTCMedia = NewTrackMedia( theTCTrack, TimeCodeMediaType, timeScale,
						dataRef, dataRefType
		);
	}

	if( !theTCMedia ){
		if( err == noErr ){
			err = GetMoviesError();
		}
		goto bail;
	}

	theTCMediaHandler = GetMediaHandler(theTCMedia);
	if( !theTCMediaHandler ){
		err = GetMoviesError();
		goto bail;
	}

	// get default display style options and set them appropriately - not necessary!
	if( translate ){
	// TCTextOptions theTextOptions;
	  MatrixRecord theTrackMatrix;
//		TCGetDisplayOptions( theTCMediaHandler, &theTextOptions );
//		theTextOptions.txFont = kFontIDHelvetica;
//		TCSetDisplayOptions(theTCMediaHandler, &theTextOptions);
		GetTrackMatrix( theTCTrack, &theTrackMatrix );
// TimeCode above the filmed image:
//		TranslateMatrix( &theTrackMatrix, 0, Long2Fix(-kTimeCodeTrackHeight) );
// TimeCode below the filmed image:
		TranslateMatrix( &theTrackMatrix,
					 Long2Fix((long) translate->horizontal), Long2Fix((long) translate->vertical)
		);
		SetTrackMatrix( theTCTrack, &theTrackMatrix );
	}

	// enable the track and set the flags to show the timecode
	// the QT Controller will display the information ... but without these options
	// the TimeCode track is ignored for time control.
	SetTrackEnabled(theTCTrack, true);
//	TCSetTimeCodeFlags( theTCMediaHandler, tcdfShowTimeCode, ~tcdfShowTimeCode );
	TCSetTimeCodeFlags( theTCMediaHandler, tcdfShowTimeCode, tcdfShowTimeCode );

	theTCSampleDescriptionH = (TimeCodeDescriptionHandle) NewHandleClear(sizeof(TimeCodeDescription));
	if( !theTCSampleDescriptionH ){
		err = MemError();
		goto bail;
	}

	err = BeginMediaEdits(theTCMedia);
	if( err != noErr ){
		goto bail;
	}

	// record the filename
	if( theURL ){
	  UserData theSourceIDUserData;
		err = NewUserData(&theSourceIDUserData);
		if( noErr == err ){
		  Handle theDataHandle = NULL;

			PtrToHand( (ConstStringPtr) theURL, &theDataHandle, (long) strlen(theURL) );
			err = AddUserDataText( theSourceIDUserData, theDataHandle, TCSourceRefNameType, 1, langEnglish);
			if( noErr == err ){
				TCSetSourceRef( theTCMediaHandler, theTCSampleDescriptionH, theSourceIDUserData );
			}
			DisposeHandle(theDataHandle);
			DisposeUserData(theSourceIDUserData);
		}
		// not sure if errors here are blocking, or can be ignored!
		if( err != noErr ){
			Log( qtLogPtr, "AddTimeCodeTrack(): error %d recording URL \"%s\" in the TimeCode metadata!\n",
			    err, theURL
			);
		}
	}

	err = noErr;

	// convert the individual specified segments into something that can be added to the TC track (step 5):
	for( i = 0 ; i < N && err == noErr ; i++ ){
	  double frequency = frames[i] / durations[i], rfreq = frequency;
	  double startTime = StartTimes[i];
	  char Info[512], *info = Info, *Lng = "en_GB", *lng = Lng;
#define TCSCALE	1.0
	  int freq = (int) (frequency * TCSCALE + 0.5);
#ifdef DEBUG
		Log( qtLogPtr, "AddTimeCodeTrack(): adding %s TC track scale %gx; segment #%d start=%gs %gs/%lu frames -> %gHz",
		    (newtrack)? "to new" : "to", TCSCALE,
		    i, startTime/timeScale, durations[i], frames[i], frequency
		);
#endif
		if( fabs(frequency*TCSCALE - freq) > 0.1 / timeScale ){
			// the specified frequency is "sufficiently" non-integer to warrant a DropFrame timecode.
			theTCDef.flags = tc24HourMax|tcDropFrame;
			// 20110108: apparently the way to deal with non-integer frameRates is to select the
			// next higher integer number, but to let fTimeScale/frameDuration be the real frequency.
			freq = (int) ceil(frequency);
			// while Apple's documentation suggests the fTimeScale/frameDuration fraction should be
			// (approx.) the true frequency, it appears to work better if it's the next highest integer value:
			frequency = freq;
#ifdef DEBUG
			Log( qtLogPtr, "Frequency %gHz: abs. decimal part %g > %g so DropFrame timecode\n",
				frequency, fabs(rfreq*TCSCALE-freq), 0.1/timeScale
			);
#endif
		}
		else{
			theTCDef.flags = tc24HourMax;
		}
#if 0
		theTCDef.fTimeScale = (TimeValue)(timeScale * TCSCALE * frequency);
		theTCDef.frameDuration = (TimeValue) (timeScale /*/ frequency */);
		theTCDef.numFrames = (UInt8) (frequency * TCSCALE + 0.5);
#else
		theTCDef.fTimeScale = (TimeValue)(timeScale * TCSCALE );
		theTCDef.frameDuration = (TimeValue) (timeScale / frequency );
		theTCDef.numFrames = (UInt8) (freq);
#endif
		theTCDef.padding = 0;
//			theTCRec.t.hours = (UInt8) (startTime / 3600);
//			startTime -= theTCRec.t.hours * 3600;
//			theTCRec.t.minutes = (UInt8) (startTime / 60);
//			startTime -= theTCRec.t.minutes * 60;
//			theTCRec.t.seconds = (UInt8) startTime;
//			startTime -= theTCRec.t.seconds;
//			theTCRec.t.frames = (UInt8) (startTime * frequency * TCSCALE);
#if 0
		secondsToFrameTime( startTime, frequency * TCSCALE, &(theTCRec.t) );
#else
		secondsToFrameTime( startTime, freq, &(theTCRec.t) );
#endif
		snprintf( info, sizeof(Info), "TimeCode for \"%s\": %lu frames for %gs (%gHz) starting at %02u:%02u:%02u:%03u; timeScale=%d, frameDuration=%d\n",
		    ((theURL)? theURL : "<??>"), frames[i], durations[i], frequency,
		    theTCRec.t.hours, theTCRec.t.minutes, theTCRec.t.seconds, theTCRec.t.frames,
		    (int) theTCDef.fTimeScale, (int) theTCDef.frameDuration
		);
#ifndef DEBUG
		if( N > 1 )
#endif
		{
			Log( qtLogPtr, "AddTimeCodeTrack(): %s", info );
		}
		MetaDataHandler( theMovie, theTCTrack, akInfo, &info, &lng, NULL );
		if( info != Info ){
			xfree(info);
		}
		if( lng != Lng ){
			xfree(lng);
		}
		totalDuration += durations[i];

		(**theTCSampleDescriptionH).descSize = sizeof(TimeCodeDescription);
		(**theTCSampleDescriptionH).dataFormat = TimeCodeMediaType;
		(**theTCSampleDescriptionH).timeCodeDef = theTCDef;

		//**** Step 5b. - add a sample to the timecode track - each sample in a timecode track provides timecode information
		//                for a span of movie time; here, we add a single sample that spans the entire movie duration
		//	The sample data contains a single frame number that identifies one or more content frames that use the
		//  timecode; this value (a long integer) identifies the first frame that uses the timecode
		err = (ErrCode) TCTimeCodeToFrameNumber( theTCMediaHandler, &theTCDef, &theTCRec, &theMediaSample );
		if( err != noErr ){
			goto bail;
		}
		// the timecode media sample must be big-endian
		theMediaSample = EndianS32_NtoB(theMediaSample);

		// add the media sample with its description to the TimeCode media, using the latest (64bit)
		// function.
		err = AddMediaSample2( theTCMedia, (UInt8 *)(&theMediaSample), sizeof(long),
						  (TimeValue64)(durations[i] * timeScale), 0,
						  (SampleDescriptionHandle) theTCSampleDescriptionH, 1, 0, NULL
		);
	}
	if( err != noErr ){
		EndMediaEdits(theTCMedia);
		Log( qtLogPtr, "Failure adding timecode sample(s) to timecode media (%d)\n", (int) err );
		goto bail;
	}
	else{
		err = EndMediaEdits(theTCMedia);
	}
	if( err != noErr ){
		goto bail;
	}

	{ TimeValue refDur = (TimeValue)(totalDuration * timeScale);
	  TimeValue mediaDur = GetMediaDuration(theTCMedia);
		// 20110304: the culprit for a long-standing mysterious failure of adding the TC media
		// to its track: a round error causing the obtained duration to be very slightly shorter
		// (1 or 2 TimeValues) from the calculated duration. Telling InsertMediaIntoTrack to use
		// the real, total duration (instead of something ever so slightly too long) is the solution...
		// I should have used from the onset!
		if( refDur != mediaDur ){
			Log( qtLogPtr, "Media expected duration %ld != obtained duration %ld", refDur, mediaDur );
		}
		// 20101116: the TimeCode track has to be aligned temporally with the track it corresponds to!
		err = InsertMediaIntoTrack( theTCTrack, (TimeValue)(trackStart * timeScale), 0,
							  mediaDur, fixed1
		);
		if( err != noErr ){
			goto bail;
		}
	}

add_track_reference:
	//**** Step 6. - are we done yet? nope, create a track reference from the target track to the timecode track
	err = AddTrackReference( theTrack, theTCTrack, kTrackReferenceTimeCode, NULL );
	if( err == noErr ){
	  char *lng = "en_GB";
	  char *trackName = "Timecode Track";
		if( theURL ){
			MetaDataHandler( theMovie, theTCTrack, akSource, (char**) &theURL, &lng, NULL );
			MetaDataHandler( theMovie, theTCTrack, akTrack, (char**) &theURL, &lng, NULL );
		}
		// 20121115:
		MetaDataHandler( theMovie, theTCTrack, akDisplayName, (char**) &trackName, NULL, ", " );
		UpdateMovie( theMovie );

		if( newtrack ){
			TCinfo = new_TCTrackInfo( theTCTrack, N, StartTimes, durations, frames, theURL );
			if( TCinfo ){
				TCinfo->cdr = *TCInfoList;
				*TCInfoList = TCinfo;
			}
		}
	}
bail:
	if( err != noErr && dataRef ){
		DisposeHandle( dataRef );
	}
	if( theTCSampleDescriptionH ){
		DisposeHandle( (Handle) theTCSampleDescriptionH );
	}
	if( err != noErr ){
		if( theTCMedia ){
			DisposeTrackMedia( theTCMedia );
		}
		if( theTCTrack && newtrack ){
			DisposeMovieTrack( theTCTrack );
		}
	}
	return (short) err;
}

ErrCode GetTrackTextAtTime( Track _fTrack, TimeValue _fTime, char **foundText,
					  TimeValue *sampleStartTime, TimeValue *sampleDuration )
{ Handle textHandle = (Handle) -1;
  TimeValue textTime, sampleTime = TrackTimeToMediaTime(_fTime, _fTrack);
  long textHSize, textSamples = 0;
  ErrCode textErr;
	if( (textHandle = NewHandleClear(0)) ){
		textErr = GetMediaSample( GetTrackMedia(_fTrack), textHandle, 0, &textHSize,
					sampleTime, &textTime, NULL, NULL, NULL,
					0, &textSamples, NULL
		);
		if( textErr == noErr ){
		  char *text = (char*) *textHandle;
			// for text media samples, the returned handle is a 16-bit field followed
			// by the actual text data; textHSize is the full length (string + sizeof(short)
			// so allocating 1 byte extra we're safe.
			text = &text[sizeof(short)];
			if( foundText && *foundText == NULL ){
				if( (*foundText = QTils_calloc( textHSize + 1, sizeof(char) )) ){
					strncpy( *foundText, text, textHSize-sizeof(short) );
					// no need for the terminating nullbyte as we used QTils_calloc()
				}
				else{
					textErr = errno;
				}
			}
			// get the sample's "interesting" time value:
			GetTrackNextInterestingTime( _fTrack, nextTimeEdgeOK|nextTimeMediaSample,
								   sampleTime, -fixed1, &sampleTime, NULL
			);
			if( sampleStartTime ){
				*sampleStartTime = sampleTime;
				textErr = GetMoviesError();
			}
			if( sampleDuration ){
				// get the sample's "interesting duration":
				GetTrackNextInterestingTime( _fTrack, nextTimeEdgeOK|nextTimeMediaSample,
									   sampleTime, -fixed1, NULL, sampleDuration
				);
				textErr = GetMoviesError();
			}
		}
		DisposeHandle(textHandle);
	}
	else{
		textErr = (textHandle == (Handle)-1)? MemError() : paramErr;
	}
	return textErr;
}

ErrCode FindTextInMovie( Movie theMovie, char *text, int displayResult,
				    Track *inoutTrack, double *foundTime, long *foundOffset, char **foundText )
{ ErrCode err;
  Track _fTrack;
  TimeValue _fTime;
  long _fOffset, flags;
	if( !theMovie || !text ){
		err = paramErr;
	}
	else{
		flags = 0 /* searchTextEnabledTracksOnly */;
		if( !displayResult ){
			flags |= searchTextDontGoToFoundTime | searchTextDontHiliteFoundText;
		}
		_fTrack = (inoutTrack)? *inoutTrack : nil;
		_fTime = (foundTime)? (TimeValue) (*foundTime *GetMovieTimeScale(theMovie) + 0.5) : 0;
		_fOffset = (foundOffset)? *foundOffset : 0;
		if( (err = MovieSearchText( theMovie, text, strlen(text), flags,
							  &_fTrack, &_fTime, &_fOffset
			)) == noErr
		){
			if( inoutTrack ){
				*inoutTrack = _fTrack;
			}
			if( foundTime ){
				*foundTime = ((double)_fTime) / ((double)GetMovieTimeScale(theMovie));
			}
			if( foundOffset ){
				*foundOffset = _fOffset;
			}
			GetTrackTextAtTime( _fTrack, _fTime, foundText, NULL, NULL );
#ifdef DEBUG
			Log( qtLogPtr, "Found '%s' in track %p at time %d, offset in movie text string [%ld]'%s'\n",
			    text, _fTrack, _fTime, _fOffset, (foundText)? *foundText : "??"
			);
#endif
		}
	}
	return err;
}

ErrCode FindTextInMovie_Mod2( Movie theMovie, char *text, int tlen, int displayResult,
				    long *foundTrack, double *foundTime, long *foundOffset, char *foundText, int flen )
{ Track theTrack;
  ErrCode err;
  long i, tCount;
  char *ftext = NULL;
	if( theMovie ){
		tCount = GetMovieTrackCount(theMovie);
		theTrack = (foundTrack)? GetMovieIndTrack(theMovie, *foundTrack) : NULL;
		if( (err = FindTextInMovie( theMovie, text, displayResult, &theTrack,
							  foundTime, foundOffset, (foundText)? &ftext : NULL )) == noErr
		){
			if( foundTrack ){
				i = 1;
				while( i <= tCount && theTrack != GetMovieIndTrack( theMovie, i ) ){
					i += 1;
				}
				// a check (i <= tCount) would probably do just as well...
				if( GetMovieIndTrack( theMovie, i ) == theTrack ){
					*foundTrack = i;
				}
				else{
					err = trackNotInMovie;
					*foundTrack = 0;
				}
			}
			if( foundText && ftext ){
				strncpy( foundText, ftext, flen );
				foundText[flen-1] = '\0';
				QTils_free(&ftext);
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode FindTimeStampInMovieAtTime( Movie theMovie, double Time, char **foundText, double *foundTime )
{ TimeValue st;
  ErrCode err;
  QTMovieWindowH wih = QTMovieWindowH_from_Movie(theMovie);

	if( wih && (*wih)->theTimeStampTrack && foundText ){
		*foundText = NULL;
		st = (TimeValue) (Time * (*wih)->info->timeScale + 0.5);
		err = GetTrackTextAtTime( (*wih)->theTimeStampTrack, st, foundText, &st, NULL );
		if( foundTime ){
			*foundTime = ((double)st) / (*wih)->info->timeScale;
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode FindTimeStampInMovieAtTime_Mod2( Movie theMovie, double Time, char *foundText, int flen )
{ char *ftext = NULL;
  ErrCode err;
	if( (err = FindTimeStampInMovieAtTime( theMovie, Time, &ftext, NULL )) == noErr ){
		if( ftext ){
			if( foundText ){
				strncpy( foundText, ftext, flen );
				foundText[flen-1] = '\0';
			}
			QTils_free(&ftext);
		}
	}
	return err;
}

ErrCode SampleNumberAtMovieTime( Movie theMovie, Track theTrack, double t, long *sampleNum )
{ TimeValue st;
  ErrCode err;
  TimeValue sampleTime, obtainedTime;

	if( sampleNum ){
		st = (TimeValue) (t * GetMovieTimeScale(theMovie) + 0.5);
		if( !theTrack ){
			theTrack = GetMovieIndTrackType( theMovie, 1, VideoMediaType, movieTrackMediaType|movieTrackEnabledOnly );
		}
		if( theTrack ){
			sampleTime = TrackTimeToMediaTime( st, theTrack);
			MediaTimeToSampleNum( GetTrackMedia(theTrack), sampleTime, sampleNum, &obtainedTime, NULL );
		}
		err = GetMoviesError();
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode SampleNumberAtMovieTime_Mod2( Movie theMovie, long trackNr, double t, long *sampleNum )
{ ErrCode err = paramErr;
	if( !theMovie || trackNr < 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	if( trackNr ){
		err = SampleNumberAtMovieTime( theMovie, GetMovieIndTrack( theMovie, trackNr ), t, sampleNum );
	}
	else{
		err = SampleNumberAtMovieTime( theMovie, NULL, t, sampleNum );
	}
	return err;
}

Track GetMovieChapterTrack( Movie theMovie, QTMovieWindows ***rwih )
{ QTMovieWindowH wih;
  QTMovieWindows *wi = NULL;
	if( theMovie && (wih = QTMovieWindowH_from_Movie(theMovie)) && *wih ){
		wi = *wih;
		if( wi->theChapterTrack ){
			if( rwih ){
				*rwih = wih;
			}
			return wi->theChapterTrack;
		}
	}
	else{
		wih = NULL;
	}
	{ long idx, N = GetMovieTrackCount(theMovie);
	  Track track, theCTrack = NULL;
		for( idx = 1 ; idx <= N && !theCTrack ; idx++ ){
			track = GetMovieIndTrack( theMovie, idx );
			if( track ){
				theCTrack = GetTrackReference( track, kTrackReferenceChapterList, 1 );
				if( wi && theCTrack ){
					wi->theChapterRefTrack = track;
					wi->theChapterMedia = GetTrackMedia(theCTrack);
				}
			}
		}
		if( rwih ){
			*rwih = wih;
		}
		return theCTrack;
	}
}

TimeValue GetMovieIndChapterTime( Movie theMovie, Track theCTrack, long *ind )
{ TimeValue cTime = -1;
  long N;
	if( theMovie && theCTrack ){
		N = 0;
		// get the track's 1st sample (time) == the 1st chapter (time
		GetTrackNextInterestingTime( theCTrack, nextTimeMediaSample|nextTimeEdgeOK,
							   (TimeValue)0, fixed1, &cTime, NULL );
		while( cTime != -1 && (*ind < 0 || N < *ind) ){
			GetTrackNextInterestingTime( theCTrack, nextTimeMediaSample,
								   cTime, fixed1, &cTime, NULL );
			N += 1;
		}
		if( *ind < 0 ){
			*ind = N;
		}
	}
	return cTime;
}

long GetMovieChapterCount( Movie theMovie )
{ Track theCTrack = GetMovieChapterTrack( theMovie, NULL );
  long N = -1;

	GetMovieIndChapterTime( theMovie, theCTrack, &N );
	return N;
}

ErrCode GetMovieIndChapter( Movie theMovie, long ind, double *time, char **text )
{ TimeValue cTime;
  Track theCTrack = GetMovieChapterTrack( theMovie, NULL );
  ErrCode err;
	if( ind < 0 ){
		err = paramErr;
	}
	else	if( (cTime = GetMovieIndChapterTime(theMovie, theCTrack, &ind)) >= 0 ){
		if( time ){
			*time = ((double) cTime) / ((double) GetMovieTimeScale(theMovie));
		}
		if( text ){
			*text = NULL;
			err = GetTrackTextAtTime( theCTrack, cTime, text, NULL, NULL );
		}
		else{
			err = noErr;
		}
	}
	else{
		err = GetMoviesError();
	}
	return err;
}

ErrCode GetMovieIndChapter_Mod2( Movie theMovie, long ind, double *time, char *text, int tlen )
{ char *_text = NULL;
  ErrCode err;
	err = GetMovieIndChapter( theMovie, ind, time, &_text );
	if( err == noErr && _text ){
		strncpy( text, _text, tlen );
		text[tlen-1] = '\0';
		QTils_free(&_text);
	}
	return err;
}

static ErrCode initTextTrackWithMedia( QTMovieWindows *wi, Track *textTrack, Media *textMedia,
							 Fixed defWidth, Fixed defHeight, const char *trackName,
							 TextDescriptionHandle *textDesc )
{ Rect textRect;
  RGBColor colour = {0xffff, 0xffff, 0xffff};
  Handle dataRef = NULL;
  OSType dataRefType;
  Fixed movieWidth, movieHeight;
  ErrCode txtErr = noErr;
  Boolean newTTrack;

	if( !wi || !wi->theMovie || !textTrack || !textMedia || !textDesc ){
		return paramErr;
	}
	GetMovieDimensions( wi->theMovie, &movieWidth, &movieHeight );
	if( movieWidth == 0 ){
		movieWidth = defWidth;
	}
	textRect.top = textRect.left = 0;
	textRect.right = FixRound(movieWidth);
	textRect.bottom = 16;
	if( !*textTrack ){
		*textTrack = NewMovieTrack( wi->theMovie,
							movieWidth, defHeight, kNoVolume
		);
		newTTrack = TRUE;
	}
	else{
		newTTrack = FALSE;
	}
	if( *textTrack && !*textMedia ){
		txtErr = DataRefFromVoid( &dataRef, &dataRefType );
	}
	if( dataRef ){
		*textMedia = NewTrackMedia( *textTrack, TextMediaType,
				(wi->info->timeScale > 0)? wi->info->timeScale : GetMovieTimeScale(wi->theMovie),
				dataRef, dataRefType
		);
	}
	if( !*textMedia ){
		Log( qtLogPtr, "Failure creating internal dataRef (%d) or textTrack (%d)\n", txtErr, GetMoviesError() );
		if( dataRef ){
			DisposeHandle(dataRef);
			dataRef = NULL;
		}
	}
	if( !*textDesc ){
		*textDesc = (TextDescriptionHandle)NewHandleClear(sizeof(TextDescription));
	}
	if( *textMedia && *textDesc ){
		(***textDesc).descSize = sizeof(TextDescription);
		(***textDesc).dataFormat = TextMediaType;
		(***textDesc).displayFlags = dfClipToTextBox|dfAntiAlias;
		(***textDesc).defaultTextBox = textRect;
		(***textDesc).textJustification = teCenter;
		(***textDesc).bgColor = colour;
	}
	else{
		Log( qtLogPtr, "Failure creating textMedia or SampleDescription (%d)\n", GetMoviesError() );
		txtErr = MemError();
	}
	if( newTTrack ){
		if( !*textMedia ){
			DisposeMovieTrack(*textTrack);
			*textTrack = nil;
		}
		else{
			// chapter tracks need not be enabled, they're not to be seen
			SetTrackEnabled( *textTrack, FALSE );
			MetaDataHandler( wi->theMovie, *textTrack, akSource,
						 (char**) &wi->theURL, NULL, ", "
			);
			if( trackName ){
				MetaDataHandler( wi->theMovie, *textTrack, akDisplayName, (char**) &trackName, NULL, ", " );
			}
		}
	}
	return txtErr;
}

ErrCode MovieAddChapter( Movie theMovie, Track refTrack, const char *name,
				  double time, double duration )
{ TimeValue chapTime, tvTime, tvDuration, tscale;
  unsigned long strLength;
  ErrCode txtErr;
  QTMovieWindowH wih = NULL;
  QTMovieWindows *wi = NULL;
  Track theChapTrack;
  TextDescriptionHandle theChapterDesc = NULL;
  Boolean newTTrack = FALSE, mediaOpen = FALSE;

	if( !theMovie || !name || duration < 0 ){
		return paramErr;
	}

	theChapTrack = GetMovieChapterTrack( theMovie, &wih );
	if( wih ){
		wi = *wih;
	}
	else{
		Log( qtLogPtr, "MovieAddChapter(): failure: movie has no associated QTMovieWindowH\n" );
		return paramErr;
	}

	// theChapTrack==NULL means there is no chapter track yet, so no theChapterRefTrack either.
	// a chapter track will be created, but the reference track actually has to be chosen by the user.
	// we fall back to using the TimeCode track if it exists.
	if( !theChapTrack && !refTrack ){
		if( !wi->theTCTrack ){
			Log( qtLogPtr,
			    "MovieAddChapter(\"%s\",\"%s\"): missing reference and no TimeCode track;"
			    " the new chapter track might be dysfunctional!\n",
			    wi->theURL, name
			);
		}
		else{
			refTrack = wi->theTCTrack;
		}
	}
#ifdef DEBUG
	{ Handle mdataRef;
	  OSType mdataRefType;
	  long mdataFlags;
		txtErr = GetMediaDataRef( wi->theChapterMedia, 1, &mdataRef, &mdataRefType, &mdataFlags );
		Log( qtLogPtr, "dr=%p drt='%s' flags=%lx\n", mdataRef, OSTStr(mdataRefType), mdataFlags );
	}
#endif
	// create a new track, media and TextDescription if/when necessary:
	txtErr = initTextTrackWithMedia( wi, &theChapTrack, &wi->theChapterMedia,
				  Long2Fix(0), Long2Fix(0), "Chapter Track", &theChapterDesc
	);
	if( txtErr != noErr ){
		Log( qtLogPtr, "MovieAddChapter(\"%s\",\"%s\"): error creating chapter title text track (%d)\n",
		    wi->theURL, name, txtErr
		);
		// just to be sure:
		theChapTrack = nil;
	}
	else if( !wi->theChapterTrack ){
		wi->theChapterTrack = theChapTrack;
		newTTrack = TRUE;
	}
	tscale = (wi->info->timeScale > 0)? wi->info->timeScale : GetMovieTimeScale(theMovie);
	tvTime = (TimeValue)( time * tscale + 0.5 );
	tvDuration = (duration == 0)? 1 : (TimeValue)( duration * tscale + 0.5 );
	{ char *_fText = NULL;
	  TimeValue iStartTime, iDuration;
	  ErrCode dErr;
		if( GetTrackTextAtTime( theChapTrack, tvTime, &_fText, &iStartTime, &iDuration ) == noErr && _fText ){
			if( (txtErr = BeginMediaEdits(wi->theChapterMedia)) == noErr ){
				mediaOpen = TRUE;
			}
// Despite what Apple's example qttext.c says, DeleteTrackSegment wants times on the movie's timescale!
//			dErr = DeleteTrackSegment( theChapTrack, iStartTime, iDuration );
			dErr = DeleteTrackSegment( theChapTrack, tvTime, tvDuration );
			Log( qtLogPtr, "MovieAddChapter(\"%s\",\"%s\") already has a chapter \"%s\" at  time=%gs\n"
			    " deleted from %d to %d (err=%d)\n",
			    wi->theURL, name, _fText, time,
			    (int) tvTime, (int) (tvTime + tvDuration), (int) dErr
			);
			xfree(_fText);
		}
		else if( tvTime < GetTrackDuration(theChapTrack) ){
			if( (txtErr = BeginMediaEdits(wi->theChapterMedia)) == noErr ){
				mediaOpen = TRUE;
			}
			dErr = DeleteTrackSegment( theChapTrack, tvTime, tvDuration );
			Log( qtLogPtr, "MovieAddChapter(\"%s\",\"%s\",%g,%g) 'make-place' deletion from %d to %d (err=%d)\n",
			    wi->theURL, name, time, duration,
			    (int) tvTime, (int) (tvTime + tvDuration), (int) dErr
			);
		}
	}
	if( theChapTrack && (mediaOpen || (txtErr = BeginMediaEdits(wi->theChapterMedia) == noErr)) ){
	  MediaHandler cmh = GetMediaHandler(wi->theChapterMedia);
		strLength = strlen(name);
		txtErr = TextMediaAddTextSample( cmh, (Ptr) name, strLength,
			   0, 0, 0, NULL, NULL,
			   (**(theChapterDesc)).textJustification,
			   &((**(theChapterDesc)).defaultTextBox),
			   (**(theChapterDesc)).displayFlags, 0, 0, 0, NULL,
			   tvDuration, &chapTime
		);
		if( txtErr == noErr ){
			if( (txtErr = EndMediaEdits(wi->theChapterMedia)) == noErr ){
				txtErr = InsertMediaIntoTrack( theChapTrack, tvTime, chapTime,
								tvDuration, fixed1
				);
				if( txtErr != noErr ){
					Log( qtLogPtr, "MovieAddChapter::InsertMediaIntoTrack(theChapTrack) -> %d\n", txtErr );
				}
				else{
					if( refTrack
					   && refTrack != wi->theChapterRefTrack
					   && GetTrackReference( theChapTrack, kTrackReferenceChapterList, 1 ) != refTrack
					){
						if( GetTrackReferenceCount( theChapTrack, kTrackReferenceChapterList ) ){
							txtErr = SetTrackReference( refTrack, theChapTrack,
												  kTrackReferenceChapterList, 1
							);
						}
						else{
							txtErr = AddTrackReference( refTrack, theChapTrack,
												  kTrackReferenceChapterList, NULL
							);
						}
						if( txtErr != noErr ){
							Log( qtLogPtr, "MovieAddChapter::SetTrackReference(theChapTrack) -> %d\n", txtErr );
						}
					}
					if( !txtErr ){
						Log( qtLogPtr, "Added chapter \"%s\" at t=%gs for %d frames\n",
						    name, time, (int) tvDuration
						);
						UpdateMovie(theMovie);
					}
				}
			}
			else{
				Log( qtLogPtr, "MovieAddChapter::EndMediaEdits(theChapMedia) -> %d\n", txtErr );
			}
		}
		else{
			Log( qtLogPtr, "MovieAddChapter::TextMediaAddTextSample(theChapMediaHandler) -> %d\n", txtErr );
		}
	}
	if( theChapterDesc ){
		DisposeHandle( (Handle) theChapterDesc );
	}
	return txtErr;
}

ErrCode MovieAddChapter_Mod2( Movie theMovie, long trackNr, const char *name, int nlen,
				    double time, double duration )
{
	if( !theMovie || trackNr < 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	else{
		return MovieAddChapter( theMovie,
						   (trackNr)? GetMovieIndTrack(theMovie, trackNr) : NULL,
						   name, time, duration
		);
	}
}

ErrCode GetTrackName( Movie theMovie, Track theTrack, char **trackName )
{ ErrCode err = paramErr;
  UserData uData = NULL;

	if( theMovie && theTrack && trackName ){
		*trackName = NULL;
//		err = GetMetaDataStringFromTrack( theMovie, theTrack, akDisplayName, trackName, NULL );
		if( (uData = GetTrackUserData(theTrack)) ){
		  Handle h = NewHandle(0);
		  long len;
			if( (err = GetUserData( uData, h, kUserDataName, 1 )) == noErr ){
				len = GetHandleSize(h);
				if( len > 0 && (*trackName = QTils_calloc( len + 1, sizeof(char) )) ){
					memcpy( *trackName, *h, len );
					(*trackName)[len] = '\0';
				}
			}
			DisposeHandle(h);
		}
		else{
			err = GetMoviesError();
		}
	}
	return err;
}

ErrCode GetTrackName_Mod2( Movie theMovie, long trackNr, char *trackName, int tlen )
{ ErrCode err = paramErr;
  char *tn = NULL;
	if( !theMovie || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	if( (err = GetTrackName( theMovie, GetMovieIndTrack( theMovie, trackNr ), &tn )) == noErr ){
		if( tn ){
			strncpy( trackName, tn, tlen );
			trackName[tlen-1] = '\0';
		}
		else{
			trackName[0] = '\0';
		}
		QTils_free(&tn);
	}
	return err;
}

ErrCode GetTrackWithName( Movie theMovie, char *trackName, OSType type, long flags, Track *theTrack, long *trackNr )
{ ErrCode err = paramErr;
  long i = 1;
  char *tn;
  Track ret = NULL;
	if( theMovie && theTrack && trackName ){
		*theTrack = NULL;
		while( !*theTrack && ( (type && (ret = GetMovieIndTrackType( theMovie, i, type, flags )))
				|| (ret = GetMovieIndTrack( theMovie, i )) )
		){
			tn = NULL;
//			err = GetMetaDataStringFromTrack( theMovie, ret, akDisplayName, &tn, NULL );
			err = GetTrackName( theMovie, ret, &tn );
			if( tn ){
				if( strcmp( trackName, tn ) == 0 ){
					*theTrack = ret;
					if( trackNr ){
						*trackNr = i;
					}
				}
				QTils_free(&tn);
			}
			i += 1;
		}
	}
	return err;
}

ErrCode GetTrackWithName_Mod2( Movie theMovie, char *trackName, int tlen, long *trackNr )
{ Track theTrack;
	return GetTrackWithName( theMovie, trackName, 0, 0, &theTrack, trackNr );
}

ErrCode EnableTrack_Mod2( Movie theMovie, long trackNr )
{ Track theTrack;
	if( !theMovie || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	if( !GetTrackEnabled( (theTrack = GetMovieIndTrack(theMovie, trackNr)) ) ){
		SetTrackEnabled( theTrack, true );
		UpdateMovie(theMovie);
	}
	return GetMoviesError();
}

ErrCode DisableTrack_Mod2( Movie theMovie, long trackNr )
{ Track theTrack;
	if( !theMovie || trackNr <= 0 || trackNr > GetMovieTrackCount(theMovie) ){
		return paramErr;
	}
	if( GetTrackEnabled( (theTrack = GetMovieIndTrack(theMovie, trackNr)) ) ){
		SetTrackEnabled( theTrack, false );
		UpdateMovie(theMovie);
	}
	return GetMoviesError();
}

#if TARGET_OS_MAC
ErrCode testing( Movie theMovie )
{ ErrCode err = paramErr;
  UserData uData = NULL;

	if( theMovie ){
		if( (uData = GetMovieUserData(theMovie)) ){
		  Handle h = NewHandle(0);
		  long len;
			err = GetUserData( uData, h, '@inf', 1 );
			err = GetUserData( uData, h, '@wrt', 1 );
			err = GetUserData( uData, h, '@day', 1 );
			err = GetUserData( uData, h, '@swr', 1 );
			if( (err = GetUserData( uData, h, 'AllF', 1 )) == noErr ){
				len = GetHandleSize(h);
			}
			DisposeHandle(h);
		}
		else{
			err = GetMoviesError();
		}
	}
	return err;
}
#endif

#ifndef QTMOVIESINK

#include "QTMovieSink.h"

/*!
	a local free() function. This seems to be necessary because apparently under Win7
	memory has to be freed in the same dll as where it was allocated.
 */
static void QTils_free_Mod2( char **mem )
{
	if( mem && *mem ){
		QTils_LogMsgEx( "free_Mod2(): freeing %p and setting it to NULL; free()=%p\n", *mem, QTils_Allocator->free );
		QTils_free(mem);
		*mem = NULL;
	}
}

/*!
 initDMBaseQTils() initialises a structure with the addresses of the most important exported functions.
 This makes it less tedious to load the DLL dynamically at runtime.
 */
size_t initDMBaseQTils( LibQTilsBase *dmbase )
{ extern char lastSSLogMsg[2048];
	if( dmbase ){

		if( !QTils_Allocator ){
			__init_QTils_Allocator__( NULL, malloc, calloc, realloc, __QTils_free__ );
		}
		dmbase->QTils_Allocator = QTils_Allocator;

		dmbase->OpenQT = OpenQT;
		dmbase->CloseQT = CloseQT;

		dmbase->QTCompressionCodec = QTCompressionCodec;
		dmbase->QTCompressionQuality = QTCompressionQuality;

		dmbase->OpenQTMovieInWindow = OpenQTMovieInWindow;
		dmbase->_QTMovieWindowH_Check_ = _QTMovieWindowH_Check_;
		dmbase->DisposeQTMovieWindow = DisposeQTMovieWindow;
		dmbase->CloseQTMovieWindow = CloseQTMovieWindow;
		dmbase->QTMovieWindowHFromMovie = QTMovieWindowH_from_Movie;

		dmbase->QTMovieWindowToggleMCController = QTMovieWindowToggleMCController;
		dmbase->ActivateQTMovieWindow = ActivateQTMovieWindow;
		dmbase->QTMovieWindowSetPlayAllFrames = QTMovieWindowSetPlayAllFrames;
		dmbase->QTMovieWindowSetPreferredRate = QTMovieWindowSetPreferredRate;
		dmbase->QTMovieWindowPlay = QTMovieWindowPlay;
		dmbase->QTMovieWindowStop = QTMovieWindowStop;

		dmbase->QTMovieWindowGetTime = QTMovieWindowGetTime;
		dmbase->QTMovieWindowGetFrameTime = QTMovieWindowGetFrameTime;
		dmbase->QTMovieWindowSetTime = QTMovieWindowSetTime;
		dmbase->QTMovieWindowSetFrameTime = QTMovieWindowSetFrameTime;
		dmbase->QTMovieWindowStepNext = QTMovieWindowStepNext;
		dmbase->secondsToFrameTime = secondsToFrameTime;
		dmbase->QTMovieWindowSetGeometry = QTMovieWindowSetGeometry;
		dmbase->QTMovieWindowGetGeometry = QTMovieWindowGetGeometry;

		dmbase->MCAction = MCAction;
		dmbase->register_MCAction = register_MCAction;
		dmbase->get_MCAction = get_MCAction;
		dmbase->unregister_MCAction = unregister_MCAction;

		dmbase->DisposeMemoryDataRef = DisposeMemoryDataRef;
		dmbase->MemoryDataRefFromString = MemoryDataRefFromString;
		dmbase->OpenMovieFromMemoryDataRef = OpenMovieFromMemoryDataRef;
		dmbase->OpenQTMovieFromMemoryDataRefInWindow = OpenQTMovieFromMemoryDataRefInWindow;
		dmbase->OpenQTMovieWindowWithMovie = OpenQTMovieWindowWithMovie_Mod2;

		dmbase->OpenMovieFromURL = OpenMovieFromURL;
		dmbase->HasMovieChanged = HasMovieChanged_Mod2;
		dmbase->SaveMovie = SaveMovie;
		dmbase->SaveMovieAsRefMov = SaveMovieAsRefMov;
		dmbase->SaveMovieAs = SaveMovieAs;
		dmbase->CloseMovie = CloseMovie;
		dmbase->SetMoviePlayHints = qtSetMoviePlayHints;
		dmbase->GetMovieTrackCount = GetMovieTrackCount;
		dmbase->GetMovieDuration = GetMovieDurationSeconds;
		dmbase->GetMovieTimeResolution = GetMovieTimeResolution;

		dmbase->AddMetaDataStringToTrack = AddMetaDataStringToTrack_Mod2;
		dmbase->AddMetaDataStringToMovie = AddMetaDataStringToMovie_Mod2;
		dmbase->GetMetaDataStringLengthFromTrack = GetMetaDataStringLengthFromTrack_Mod2;
		dmbase->GetMetaDataStringLengthFromMovie = GetMetaDataStringLengthFromMovie_Mod2;
		dmbase->GetMetaDataStringFromTrack = GetMetaDataStringFromTrack_Mod2;
		dmbase->GetMetaDataStringFromMovie = GetMetaDataStringFromMovie_Mod2;
		dmbase->FindTextInMovie = FindTextInMovie;
		dmbase->FindTimeStampInMovieAtTime = FindTimeStampInMovieAtTime;
		dmbase->GetMovieChapterCount = GetMovieChapterCount;
		dmbase->GetMovieIndChapter = GetMovieIndChapter;
		dmbase->MovieAddChapter = MovieAddChapter;
		dmbase->GetTrackName = GetTrackName;
		dmbase->GetTrackWithName = GetTrackWithName;
		dmbase->EnableTrackNr = EnableTrack_Mod2;
		dmbase->DisableTrackNr = DisableTrack_Mod2;
		dmbase->SlaveMovieToMasterMovie = SlaveMovieToMasterMovie;
		dmbase->SampleNumberAtMovieTime = SampleNumberAtMovieTime;

		dmbase->Check4XMLError = Check4XMLError;
		dmbase->ParseXMLFile = ParseXMLFile;
		dmbase->XMLParserAddElement = XMLParserAddElement;
		dmbase->XMLParserAddElementAttribute = XMLParserAddElementAttribute;
		dmbase->DisposeXMLParser = DisposeXMLParser;
		dmbase->GetAttributeIndex = GetAttributeIndex;
		dmbase->GetStringAttribute = GetStringAttribute;
		dmbase->GetIntegerAttribute = GetIntegerAttribute;
		dmbase->GetShortAttribute = GetShortAttribute;
		dmbase->GetDoubleAttribute = GetDoubleAttribute;
		dmbase->GetBooleanAttribute = GetBooleanAttribute;

		dmbase->close_QTMovieSink = close_QTMovieSink;
		dmbase->QTMovieSink_AddFrame = QTMovieSink_AddFrame;
		dmbase->QTMovieSink_AddFrameWithTime = QTMovieSink_AddFrameWithTime;
		dmbase->get_QTMovieSink_Movie = get_QTMovieSink_Movie;
		dmbase->get_QTMovieSink_EncodingStats = get_QTMovieSink_EncodingStats;
		dmbase->QTMSFrameBuffer = QTMSFrameBuffer;
		dmbase->QTMSCurrentFrameBuffer = QTMSCurrentFrameBuffer;
		dmbase->QTMSPixelAddress = QTMSPixelAddress;
		dmbase->QTMSPixelAddressInCurrentFrameBuffer = QTMSPixelAddressInCurrentFrameBuffer;

		dmbase->LastQTError = LastQTError;

		dmbase->PumpMessages = PumpMessages;
		dmbase->QTils_LogMsg = QTils_LogMsg;
		dmbase->QTils_LogMsgEx = QTils_vLogMsgEx;
		dmbase->lastSSLogMsg = &lastSSLogMsg[0];
		dmbase->MacErrorString = MacErrorString_Mod2;

		dmbase->vsscanf = vsscanf_Mod2;
		dmbase->vsnprintf = vsnprintf_Mod2;
		dmbase->vssprintf = vssprintf_Mod2;
		dmbase->vssprintfAppend = vssprintfAppend_Mod2;
		dmbase->free = QTils_free_Mod2;

		return sizeof(*dmbase);
	}
	return 0;
}

LibQTilsBase *clientsDMBase;

/*!
 Slightly modified version of the above, replacing certain functions with the special-form
 required to be callable from (Stony Brook) Modula-2.
 This compiler adds an additional argument after ARRAY OF arguments, which specifies the array
 length. For the concerned functions, I have created wrapper functions (with the _Mod2 suffix)
 that expect that or those additional argument(s), and that handle them where necessary.
 Contrary to what the names given suggest, these additional arguments specify array length through
 the maximum index value. The actual length is thus 1 more.
 */
size_t initDMBaseQTils_Mod2( LibQTilsBase *dmbase )
{
	if( initDMBaseQTils(dmbase) ){
		// NB: these functions are variants for calling from Modula2, adding a length
		// argument after each 'open array' (string pointer) argument; therefore they do NOT conform
		// to the prototypes in dmbase.
		// NEVER To be used from C!!
		dmbase->MemoryDataRefFromString = (void*) MemoryDataRefFromString_Mod2;
		dmbase->OpenMovieFromURL = (void*) OpenMovieFromURL_Mod2;
		dmbase->HasMovieChanged = (void*) HasMovieChanged_Mod2;
		dmbase->SaveMovieAsRefMov = (void*) SaveMovieAsRefMov_Mod2;
		dmbase->SaveMovieAs = (void*) SaveMovieAs_Mod2;
		dmbase->OpenQTMovieInWindow = (void*) OpenQTMovieInWindow_Mod2;
		dmbase->DisposeQTMovieWindow = (void*) DisposeQTMovieWindow_Mod2;
//		dmbase->GetMovieTrackCount = GetMovieTrackCount;
		dmbase->FindTextInMovie = (void*) FindTextInMovie_Mod2;
		dmbase->FindTimeStampInMovieAtTime = (void*) FindTimeStampInMovieAtTime_Mod2;
		dmbase->GetMovieIndChapter = (void*) GetMovieIndChapter_Mod2;
		dmbase->MovieAddChapter = (void*) MovieAddChapter_Mod2;
		dmbase->GetTrackName = (void*) GetTrackName_Mod2;
		dmbase->GetTrackWithName = (void*) GetTrackWithName_Mod2;
		dmbase->EnableTrackNr = (void*) EnableTrack_Mod2;
		dmbase->DisableTrackNr = (void*) DisableTrack_Mod2;
		dmbase->SampleNumberAtMovieTime = (void*) SampleNumberAtMovieTime_Mod2;
		dmbase->QTils_LogMsg = (void*) QTils_LogMsg_Mod2;
		dmbase->QTils_LogMsgEx = (void*) QTils_LogMsgEx_Mod2;
		dmbase->Check4XMLError = (void*) Check4XMLError_Mod2;
		dmbase->ParseXMLFile = (void*) ParseXMLFile_Mod2;
		dmbase->XMLnameSpaceID = nameSpaceIDNone;
		dmbase->XMLParserAddElement = (void*) XMLParserAddElement_Mod2;
		dmbase->XMLParserAddElementAttribute = (void*) XMLParserAddElementAttribute_Mod2;
		dmbase->GetAttributeIndex = (void*) GetAttributeIndex_Mod2;
		dmbase->GetStringAttribute = (void*) GetStringAttribute_Mod2;
		dmbase->open_QTMovieSink = (void*) open_QTMovieSink_Mod2;
		dmbase->open_QTMovieSinkWithData = (void*) open_QTMovieSinkWithData_Mod2;
		dmbase->close_QTMovieSink = (void*) close_QTMovieSink_Mod2;
		return sizeof(*dmbase);
	}
	else{
		return 0;
	}
}
#endif // !QTMOVIESINK

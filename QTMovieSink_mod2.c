/*!
 *  @file QTMovieSink_mod2.c
 *  Modula-2 glue routines for QTMovieSink
 *
 *  Created by René J.V. Bertin on 20110307.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTMovieSink_mod2: Modula-2 glue routines for QTMovieSink");

#define _QTMOVIESINK_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#if ! defined(_WINDOWS) && !defined(WIN32) && !defined(_MSC_VER)
#	include <unistd.h>
#	include <libgen.h>
#	include <sys/param.h>
#endif

#if __APPLE_CC__
#	include <Carbon/Carbon.h>
#	include <QuickTime/QuickTime.h>
#else
#	include <qtml.h>
#	include <ImageCodec.h>
#	include <TextUtils.h>
#	include <string.h>
#	include <Files.h>
#	include <Movies.h>
#	include <MediaHandlers.h>
#	include <tchar.h>
#	include <wtypes.h>
#	include <FixMath.h>
#	include <Script.h>
#	include <NumberFormatting.h>
#	include <direct.h>

#	define strdup(s)			_strdup((s))
#	define strncasecmp(a,b,n)	_strnicmp((a),(b),(n))
#endif

#include "QTMovieSinkQTStuff.h"

#ifndef LOCAL_METADATA_HANDLER
#	include "QTils/QTilities.h"
#endif
#include "QTMovieSink.h"

// ####################################
#pragma mark --- Modula-2 interface ---

ErrCode open_QTMovieSink_Mod2( QTMovieSinks *qms, const char *theURL, int ulen,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality, int useICM,
						  int openQT )
{ ErrCode errReturn;
	if( !qms || frameBuffers > 256 ){
		errReturn = paramErr;
	}
	if( !(qms = open_QTMovieSink( qms, theURL, Width, Height, hasAlpha,
						  frameBuffers, codec, quality, useICM,openQT, &errReturn ))
		&& errReturn == noErr
	){
		errReturn = internalQuickTimeError;
	}
	return errReturn;
}

ErrCode open_QTMovieSinkWithData_Mod2( QTMovieSinks *qms, const char *theURL, int ulen,
						  QTAPixel **imageFrame,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality, int useICM,
						  int openQT )
{ ErrCode errReturn;
	if( !qms || frameBuffers > 256 ){
		errReturn = paramErr;
	}
	if( !(qms = open_QTMovieSinkWithData( qms, theURL, imageFrame, Width, Height, hasAlpha,
						  frameBuffers, codec,  quality, useICM, openQT, &errReturn ))
		&& errReturn == noErr
	){
		errReturn = internalQuickTimeError;
	}
	return errReturn;
}

QTAPixel *QTMSFrameBuffer( QTMovieSinks *qms, unsigned short frameBuffer )
{
	if( qms && frameBuffer < qms->frameBuffers ){
		return qms->imageFrame[frameBuffer];
	}
	else{
		return NULL;
	}
}

QTAPixel *QTMSCurrentFrameBuffer( QTMovieSinks *qms )
{
	if( qms ){
		return qms->imageFrame[qms->currentFrame];
	}
	else{
		return NULL;
	}
}

QTAPixel *QTMSPixelAddress( QTMovieSinks *qms, unsigned short frameBuffer, unsigned int pixnr )
{
	if( qms && frameBuffer < qms->frameBuffers
	   && pixnr < qms->privQT->numPixels
	){
		return &qms->imageFrame[frameBuffer][pixnr];
	}
	else{
		return NULL;
	}
}

QTAPixel *QTMSPixelAddressInCurrentFrameBuffer( QTMovieSinks *qms, unsigned int pixnr )
{
	if( qms && pixnr < qms->privQT->numPixels ){
		return &qms->imageFrame[qms->currentFrame][pixnr];
	}
	else{
		return NULL;
	}
}

Movie get_QTMovieSink_Movie( QTMovieSinks *qms )
{
	if( qms && qms->privQT ){
		return qms->privQT->theMovie;
	}
	else{
		return NULL;
	}
}

ErrCode close_QTMovieSink_Mod2( QTMovieSinks *qms, int addTCTrack, QTMSEncodingStats *stats, int closeQT )
{
	if( !qms ){
		return paramErr;
	}
	// just to be sure!
	qms->dealloc_qms = FALSE;
	return close_QTMovieSink( &qms, addTCTrack, stats, closeQT );
}

ErrCode QTMovieSink_AddTrackMetaDataString_Mod2( QTMovieSinks *qms,
							  AnnotationKeys key, const char *value, int vlen, const char *lang, int llen )
{
	return QTMovieSink_AddTrackMetaDataString( qms, key, value, lang );
}

ErrCode QTMovieSink_AddMovieMetaDataString_Mod2( QTMovieSinks *qms,
								  AnnotationKeys key, const char *value, int vlen, const char *lang, int llen )
{
	return QTMovieSink_AddMovieMetaDataString( qms, key, value, lang );
}
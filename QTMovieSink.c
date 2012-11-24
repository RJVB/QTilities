/*!
 *  @file QTMovieSink.c
 *	Very closely copied on John Stone's <jss@cs.utexas.edu> QTMovieFile C++ class for OSG
 *
 *  Created by René J.V. Bertin on 20100727.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTMovieSink: save bitmaps/screendumps into a QuickTime movie");

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

#if TARGET_OS_WIN32
static int set_priority_RT()
{ HANDLE us = GetCurrentThread();
  int current = GetThreadPriority(us);
	SetThreadPriority( us, THREAD_PRIORITY_TIME_CRITICAL );
	return current;
}

static int set_priority(int priority)
{
	return SetThreadPriority( GetCurrentThread(), priority );
}

#else

#	define set_priority_RT()	0
#	define set_priority(p)	/**/

#endif

// ########################
#pragma mark ----macros----

#define		kVideoTimeScale		1000
#define		kNoOffset 			0
#define		kMgrChoose			0
#define		kSyncSample			0
#define		kAddOneVideoSample		1
#define		kTrackStart			0
#define		kMediaStart			0

//#define VIDEO_CODEC_TYPE	kVideoCodecType
#ifndef VIDEO_CODEC_TYPE
#	define VIDEO_CODEC_TYPE			kJPEGCodecType
#endif
#ifndef VIDEO_CODEC_QUALITY
#	define VIDEO_CODEC_QUALITY		codecNormalQuality
#endif

// ##########################
#pragma mark ----typedefs----

#include "QTMovieSinkQTStuff.h"

#ifndef LOCAL_METADATA_HANDLER
#	include "QTils/QTilities.h"
#	define strdup(s)			QTils_strdup((s))
#else
#	define QTils_malloc(s)		malloc((s))
#	define QTils_calloc(n,s)		calloc((n),(s))
#	define QTils_realloc(p,s)	realloc((p),(s))
#	define QTils_free(p)		free(*(p))
#endif
#include "QTMovieSink.h"

// #######################################
#pragma mark ----constants=videoCodecs----
//   kMPEG4VisualCodecType

const unsigned long cVideoCodec = kVideoCodecType,
	cJPEGCodec = kJPEGCodecType,
	cMJPEGACodec = kMotionJPEGACodecType,
	cAnimationCodec = kAnimationCodecType,
	cRawCodec = kRawCodecType,
	cHD720pCodec = kDVCPROHD720pCodecType,
	cHD1080i60Codec = kDVCPROHD1080i60CodecType,
	cMPEG4Codec = kMPEG4VisualCodecType,
	cH264Codec = kH264CodecType,
	cCinepakCodec = kCinepakCodecType,
	cTIFFCodec = kTIFFCodecType;
#if defined(__APPLE_CC__) || defined(__MACH__)
	const unsigned long cApplePixletCodec = kPixletCodecType;
#endif

static QTCompressionCodecs _QTCompressionCodec_ = {
	kVideoCodecType,
	kJPEGCodecType,
	kMotionJPEGACodecType,
	kAnimationCodecType,
	kDVCPROHD720pCodecType,
	kDVCPROHD1080i60CodecType,
	kMPEG4VisualCodecType,
	kH264CodecType,
	kRawCodecType,
	kCinepakCodecType,
	kTIFFCodecType,
#if defined(__APPLE_CC__) || defined(__MACH__)
	kPixletCodecType,
#endif
};

QTCompressionCodecs *QTCompressionCodec()
{
	return &_QTCompressionCodec_;
}

const unsigned long cLossless = codecLosslessQuality,
	cMaxQuality = codecMaxQuality,
	cMinQuality = codecMinQuality,
	cLowQuality = codecLowQuality,
	cNormalQuality = codecNormalQuality,
	cHighQuality = codecHighQuality;

QTCompressionQualities _QTCompressionQuality_ = {
	codecLosslessQuality,
	codecMaxQuality,
	codecMinQuality,
	codecLowQuality,
	codecNormalQuality,
	codecHighQuality,
};

QTCompressionQualities *QTCompressionQuality()
{
	return &_QTCompressionQuality_;
}

// MacErrors.h

static const char *qualityString(const unsigned long quality)
{ const char *qs;
	switch( quality ){
		case codecLosslessQuality:
			qs = "(lossless quality)";
			break;
		case codecMinQuality:
			qs = "(minimal quality)";
			break;
		case codecMaxQuality:
			qs = "(maximal quality)";
			break;
		case codecLowQuality:
			qs = "(low quality)";
			break;
		case codecNormalQuality:
			qs = "(normal quality)";
			break;
		case codecHighQuality:
			qs = "(high quality)";
			break;
		default:
			qs = "(unknown quality)";
			break;
	}
	return qs;
}

#ifndef _QTILITIES_H
#warning "non maintained code!"
// obtain a data reference to the requested file. That means we first need to construct the full
// pathname to the file, which entails guessing whether or not theURL is already a full path.
OSErr CreateMovieStorageFromURL( const char **URL, OSType creator, ScriptCode scriptTag, long flags,
						   Handle *dataRef, OSType *dataRefType,
						   DataHandler *outDataHandler, Movie *newMovie, char **fullURL )
{ OSErr err;
  CFStringRef URLRef;
  int hasFullPath;
  char *theURL = (URL)? strdup(*URL) : NULL;
	if( !theURL ){
		return paramErr;
	}
	if( theURL[0] == '.' ){
		if( theURL[1] == '.' ){
			theURL++;
		}
		if(
#if TARGET_OS_WIN32
		   theURL[1] == '\\'
#else
		   theURL[1] == '/'
#endif
		){
			theURL++;
		}
	}
#if TARGET_OS_WIN32
	hasFullPath = (theURL[1] == ':');
#else
	hasFullPath = (theURL[0] == '/');
#endif
	if( !hasFullPath ){
	  char *c, cwd[1024];
#ifdef _MSC_VER
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
				if( fullURL ){
					*fullURL = theURL;
				}
				QTils_free(&theURL);
				theURL = c;
				*URL = theURL;
			}
		}
	}
#if TARGET_OS_WIN32
	// 20101027: correct the pathname for MSWindows, if necessary:
	if( strchr(theURL, '/') ){
	  char *c = theURL;
		if( *c == '/' ){
			*c++ = '\\';
		}
		while( *c ){
			if( *c == '/' && c[-1] != '\\' ){
				*c = '\\';
			}
			c++;
		}
	}
#endif
	URLRef = CFStringCreateWithCString(NULL, theURL, CFStringGetSystemEncoding() );
	if( URLRef ){
		err = QTNewDataReferenceFromFullPathCFString( URLRef, kQTNativeDefaultPathStyle, 0,
					dataRef, dataRefType
		);
	}
	else{
		err = internalQuickTimeError;
		goto bail;
	}
	if( err == noErr ){
		err = CreateMovieStorage( *dataRef, *dataRefType, creator,
					scriptTag, flags, outDataHandler, newMovie
		);
	}
bail:
	if( !fullURL ){
		QTils_free(&theURL);
	}
	return err;
}
#else

/*
	takes a filename, creates a data reference to it (returned in dataRef,dataRefType) and then
	creates a new movie storage (container) in that file, given the creator, script and flags parameters.
	If the filename had to be completed, the full name is returned in fullURL
 */
static OSErr CreateMovieStorageFromURL( const char **URL, OSType creator, ScriptCode scriptTag, long flags,
						   Handle *dataRef, OSType *dataRefType, CSBackupHook *csbHook,
						   DataHandler *outDataHandler, Movie *newMovie, char **fullURL )
{ OSErr err;
  const char *orgURL = (URL)? *URL : NULL;
	if( fullURL ){
		*fullURL = (char*) orgURL;
	}
	err = DataRefFromURL( URL, dataRef, dataRefType );
	if( err == noErr ){
#if TARGET_OS_MAC
		csbHook->urlRef = CFURLCreateFromFileSystemRepresentation( NULL, (const UInt8*) *URL,
													   strlen(*URL), false );
		if( csbHook->urlRef ){
			// exclude the file from being backed up by TimeMachine to work around
			// a bug on 10.7 (20111124):
			csbHook->bakExcl = CSBackupIsItemExcluded( csbHook->urlRef, &csbHook->bakExclPath );
			CSBackupSetItemExcluded( csbHook->urlRef, false, true );
		}
#endif
		err = CreateMovieStorage( *dataRef, *dataRefType, creator,
					scriptTag, flags, outDataHandler, newMovie
		);
		if( err == noErr ){
			if( !fullURL && URL && *URL != orgURL ){
				QTils_free( (void**) URL);
				*URL = orgURL;
			}
		}
		else{
			DisposeHandle( (Handle) *dataRef );
			*dataRef = NULL;
			*dataRefType = 0;
			*newMovie = NULL;
		}
	}
	return err;
}
#endif

#ifndef _QTILITIES_H
OSErr AnchorMovie2TopLeft( Movie theMovie )
{ Rect mBox;
  OSErr err;
	GetMovieBox( theMovie, &mBox );
	mBox.right -= mBox.left;
	mBox.bottom -= mBox.top;
	mBox.top = mBox.left = 0;
	SetMovieBox( theMovie, &mBox );
	err = GetMoviesError();
	UpdateMovie( theMovie );
	return err;
}
#endif

static ErrCode AddTCTrack( QTMovieSinks *qms )
{ QTMovieSinkQTStuff *qtPriv = (qms)? qms->privQT : NULL;
  ErrCode err;
	if( qtPriv && qtPriv->Frames ){
	  double startTime = 0, duration;
	  size_t frames = qtPriv->Frames;
	  // we need a bit of translation to get the TC track where we want it
	  // (under the image):
	  TCTrackInfo *list = NULL;
	  Cartesian translate = {0, 2};
		UpdateMovie( qtPriv->theMovie );
		duration = ((double) GetTrackDuration(qtPriv->theTrack)) /
				((double) GetMovieTimeScale(qtPriv->theMovie));
		err = AddTimeCodeTrack( qtPriv->theMovie, qtPriv->theTrack, 0, &list,
					  1, &startTime, &duration, &frames, &translate, qms->theURL
		);
		list = dispose_TCTrackInfoList(list);
	}
	else{
		err = paramErr;
	}
	return err;
}

// #################################################
#pragma mark ----QTMovieSink using CompressImage----

/*!
	the internal worker function for opening a QT Movie Sink that uses CompressImage
 */
static QTMovieSinks *_open_QTMovieSink_( QTMovieSinks *qms, const char *theURL,
						  QTAPixel **imageFrame,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality,
						  int openQT, ErrCode *errReturn )
{ OSErr err = noErr;
	if( frameBuffers <= 0 || frameBuffers > 1 ){
		if( errReturn ){
			*errReturn = (ErrCode) paramErr;
		}
		return( NULL );
	}
	if( theURL && *theURL ){
		if( !qms ){
			qms = (QTMovieSinks*) QTils_calloc( 1, sizeof(QTMovieSinks) );
			if( qms ){
				qms->dealloc_qms = 1;
			}
		}
		else{
			memset( qms, 0, sizeof(QTMovieSinks) );
		}
	}
	if( qms && theURL && *theURL ){
	  static QTMovieSinkQTStuff *qtPriv;
	  int i;
		qtPriv = (QTMovieSinkQTStuff*) QTils_malloc( sizeof(QTMovieSinkQTStuff) );
		if( qtPriv ){
			memset( qtPriv, 0, sizeof(QTMovieSinkQTStuff) );
			if( openQT ){
#if TARGET_OS_WIN32
				err = InitializeQTML(0);
#endif
				if( err == noErr ){
					err = EnterMovies();
				}
				if( err != noErr ){
					goto bail;
				}
			}
			err = CreateMovieStorageFromURL( &theURL, 'TVOD',
						smCurrentScript, createMovieFileDeleteCurFile|createMovieFileDontCreateResFile,
						&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->cbState,
						&qtPriv->dataHandler, &qtPriv->theMovie, (char**) &qtPriv->saved_theURL
			);
			if( err != noErr ){
				goto bail;
			}
			qms->theURL = theURL;
			qms->useICM = FALSE;

			qtPriv->numPixels = Width * Height;
			if( !(qms->imageFrame = imageFrame) ){
				qms->imageFrame = (QTAPixel**) NewPtrClear( frameBuffers * sizeof(QTAPixel*) );
				for( i =  0 ; i< frameBuffers && err == noErr && qms->imageFrame ; i++ ){
//					qms->imageFrame = (QTAPixel*) QTils_calloc( qtPriv->numPixels, sizeof(QTAPixel) );
					qms->imageFrame[i] = (QTAPixel*) NewPtrClear( qtPriv->numPixels * sizeof(QTAPixel) );
					err = errno = MemError();
				}
			}
			if( qms->imageFrame && !err ){
				qms->currentFrame = 0;
				qms->frameBuffers = frameBuffers;
				qms->dealloc_imageFrame = (imageFrame)? 0 : 1;
				qtPriv->trackFrame.left = 0;
				qtPriv->trackFrame.top = 0;
				qtPriv->trackFrame.right = Width;
				qtPriv->trackFrame.bottom = Height;
				if( hasAlpha ){
#if TARGET_OS_WIN32
					qtPriv->pixelFormat = k32RGBAPixelFormat;
#else
					qtPriv->pixelFormat = k32ARGBPixelFormat;
#endif
					err = QTNewGWorldFromPtr( &qtPriv->CI.theWorld, qtPriv->pixelFormat, &qtPriv->trackFrame,
								NULL, NULL, 0, qms->imageFrame[0], ((long)Width * 4)
					);
				}
				else{
				  long bytesPerRow;
#if TARGET_OS_WIN32
					qtPriv->pixelFormat = k24BGRPixelFormat;
					bytesPerRow = Width * 3;
#else
					// Mac OS X always gives you a k32ARGBPixelFormat when asking for a 24 bit depth...
					qtPriv->pixelFormat = k32ARGBPixelFormat /*k24RGBPixelFormat*/;
					bytesPerRow = Width * 4;
#endif
					err = QTNewGWorldFromPtr( &qtPriv->CI.theWorld, qtPriv->pixelFormat, &qtPriv->trackFrame,
								NULL, NULL, 0, qms->imageFrame[0], bytesPerRow
					);
				}
				if( err != noErr ){
					goto bail;
				}
			}
			else{
				err = errno;
				goto bail;
			}
#if TARGET_OS_WIN32
			qtPriv->CI.thePixMap = qtPriv->CI.theWorld->portPixMap;
#else
			qtPriv->CI.thePixMap = GetPortPixMap( qtPriv->CI.theWorld );
#endif

			SetMovieTimeScale( qtPriv->theMovie, kVideoTimeScale );
			qtPriv->theTrack = NewMovieTrack( qtPriv->theMovie, Long2Fix((long)Width), Long2Fix((long)Height), 0 );
			err = GetMoviesError();
			if( !qtPriv->theTrack || err != noErr ){
				goto bail;
			}
			qtPriv->theMedia = NewTrackMedia( qtPriv->theTrack, VideoMediaType, kVideoTimeScale, nil, 0 );
			err = GetMoviesError();
			if( !qtPriv->theTrack || err != noErr ){
				goto bail;
			}

			// shoud do verification here...
			{ long maxCompressedSize;
				err = GetMaxCompressionSize( qtPriv->CI.thePixMap, &qtPriv->trackFrame, kMgrChoose,
						(CodecQ) quality, (CodecType) codec, (CompressorComponent) anyCodec, &maxCompressedSize
				);
				if( err == noErr ){
					qtPriv->codecType = (CodecType) codec;
					qtPriv->quality = (CodecQ) quality;
					qtPriv->CI.compressedData = NewHandleClear(maxCompressedSize);
					err = MemError();
					if( !qtPriv->CI.compressedData || err != noErr ){
						// just to be sure we won't try to free an invalid pointer:
						qtPriv->CI.compressedData = NULL;
						goto bail;
					}
				}
				else{
					goto bail;
				}
			}
#if TARGET_OS_WIN32
			MoveHHi(qtPriv->CI.compressedData);
#endif
			HLock(qtPriv->CI.compressedData);
#if TARGET_OS_WIN32
			qtPriv->CI.compressedDataPtr = StripAddress(*qtPriv->CI.compressedData);
#else
			qtPriv->CI.compressedDataPtr = *qtPriv->CI.compressedData;
#endif

			qtPriv->CI.imgDesc = (ImageDescriptionHandle) NewHandleClear(sizeof(ImageDescription));
			err = MemError();
			if( !qtPriv->CI.imgDesc || err != noErr ){
				qtPriv->CI.imgDesc = NULL;
				goto bail;
			}

			err = BeginMediaEdits(qtPriv->theMedia);
			if( err != noErr ){
				goto bail;
			}

			qms->Width = Width;
			qms->Height = Height;
			qms->hasAlpha = hasAlpha;
			qms->currentFrame = 0;
			qtPriv->Frames = 0;
		}
bail:
		qms->privQT = qtPriv;
		if( err != noErr ){
			close_QTMovieSink( &qms, 0, NULL, openQT );
			qms = NULL;
		}
		else{
		  char infoStr[1024];
		  long gRes;
		  OSErr gErr = Gestalt(gestaltQuickTimeVersion, &gRes);
			if( gErr != noErr ){
				gRes = 0;
			}
			snprintf( infoStr, sizeof(infoStr), "QTMovieSink, %s - %s ; QT v%x.%x.%x %s",
				qualityString(quality), __DATE__,
				(gRes >> 24) & 0x00f, (gRes >> 20) & 0x00f, (gRes >> 16) & 0x00f,
#if defined(__MACH__) || defined(__APPLE_CC__)
				"Mac OS X"
#else
				"MS Windows"
#endif
			);
			QTMovieSink_AddTrackMetaDataString( qms, akSoftware, infoStr, NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akWriter, "RJVB", NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akCopyRight, "(C) 2010 RJVB", NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akSource, theURL, NULL );

			qms->lastErr = (unsigned long) err;
		}
	}
	if( errReturn ){
		*errReturn = (ErrCode) err;
	}
	return qms;
}

// forward:
static QTMovieSinks *_open_QTMovieSinkICM_( QTMovieSinks *qms, const char *theURL,
								    QTAPixel **imageFrame,
								    unsigned short Width, unsigned short Height, int hasAlpha,
								    unsigned short frameBuffers,
								    unsigned long codec, unsigned long quality,
								    int openQT, ErrCode *errReturn );

QTMovieSinks *open_QTMovieSink( QTMovieSinks *qms, const char *theURL,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality, int useICM,
						  int openQT, ErrCode *errReturn )
{
	if( useICM ){
		return _open_QTMovieSinkICM_( qms, theURL, NULL, Width, Height, hasAlpha, frameBuffers, codec, quality, openQT, errReturn );
	}
	else{
		if( frameBuffers >  1 ){
			frameBuffers = 1;
		}
		return _open_QTMovieSink_( qms, theURL, NULL, Width, Height, hasAlpha, frameBuffers, codec, quality, openQT, errReturn );
	}
}

QTMovieSinks *open_QTMovieSinkWithData( QTMovieSinks *qms, const char *theURL,
						  QTAPixel **imageFrame,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality, int useICM,
						  int openQT, ErrCode *errReturn )
{
	if( useICM ){
		return _open_QTMovieSinkICM_( qms, theURL, imageFrame, Width, Height, hasAlpha, frameBuffers, codec, quality, openQT, errReturn );
	}
	else{
		if( frameBuffers >  1 ){
			frameBuffers = 1;
		}
		return _open_QTMovieSink_( qms, theURL, imageFrame, Width, Height, hasAlpha, frameBuffers, codec, quality, openQT, errReturn );
	}
}

// forward:
static ErrCode close_QTMovieSinkICM( QTMovieSinks **qms, int addTCTrack, QTMSEncodingStats *stats, int closeQT );

ErrCode close_QTMovieSink( QTMovieSinks **QMS, int addTCTrack, QTMSEncodingStats *stats, int closeQT )
{ OSErr err = noErr;
  QTMovieSinks *qms;

	if( QMS ){
		qms = *QMS;
	}
	else{
		return (int) paramErr;
	}
	if( qms->useICM ){
		return close_QTMovieSinkICM( QMS, addTCTrack, stats, closeQT );
	}
	else if( qms && qms->privQT ){
	  QTMovieSinkQTStuff *qtPriv = qms->privQT;

		if( qtPriv->theMedia ){
			err = EndMediaEdits( qtPriv->theMedia );
			if( err == noErr && qtPriv->theTrack ){
				err = InsertMediaIntoTrack( qtPriv->theTrack, 0, 0,
						GetMediaDuration( qtPriv->theMedia ), fixed1
				);
				if( err == noErr && qtPriv->theMovie && qtPriv->dataHandler ){
					{ MatrixRecord m;
#if 1
						SetIdentityMatrix(&m);
						ScaleMatrix( &m, Long2Fix(1), Long2Fix(-1), Long2Fix(1), Long2Fix(1) );
#else
					  Rect trackBox, flippedBox;
					  Fixed w, h;
						GetTrackDimensions( qtPriv->theTrack, &w, &h );
						flippedBox.left = flippedBox.bottom = trackBox.left = trackBox.top = 0;
						flippedBox.right = trackBox.right = FixRound(w);
						flippedBox.top = trackBox.bottom = FixRound(h);
						MapMatrix( &m, &trackBox, &flippedBox );
#endif
						SetTrackMatrix( qtPriv->theTrack, &m );
					}
					if( addTCTrack ){
						AddTCTrack( qms );
					}
					AnchorMovie2TopLeft( qtPriv->theMovie );
					UpdateMovie( qtPriv->theMovie );
					err = AddMovieToStorage( qtPriv->theMovie, qtPriv->dataHandler );
					if( err == noErr ){
						err = CloseMovieStorage( qtPriv->dataHandler );
#if TARGET_OS_WIN32
#else
						if( qtPriv->cbState.urlRef ){
							CSBackupSetItemExcluded( qtPriv->cbState.urlRef,
											    qtPriv->cbState.bakExcl, qtPriv->cbState.bakExclPath );
							CFRelease(qtPriv->cbState.urlRef);
						}
						sync();
#endif
					}
				}
			}
		}
		if( err == noErr ){
			if( qtPriv->theTrack ){
				if( stats ){
					// at this point, get_QTMovieSink_EncodingStats will currently still have
					// access to all interesting data, and the movie is still open.
					get_QTMovieSink_EncodingStats( qms, stats );
				}
				DisposeMovieTrack( qtPriv->theTrack );
				qtPriv->theTrack = nil;
			}
			if( qtPriv->theMovie ){
				DisposeMovie( qtPriv->theMovie );
				qtPriv->theMovie = nil;
			}
			if( qtPriv->CI.imgDesc ){
				DisposeHandle( (Handle) qtPriv->CI.imgDesc );
				qtPriv->CI.imgDesc = nil;
			}
			if( qtPriv->CI.compressedData ){
				DisposeHandle( qtPriv->CI.compressedData );
				qtPriv->CI.compressedData = nil;
			}
			if( qtPriv->CI.savedPort ){
				SetGWorld( qtPriv->CI.savedPort, qtPriv->CI.savedGD );
				qtPriv->CI.savedPort = nil;
				qtPriv->CI.savedGD = nil;
			}
			if( qtPriv->CI.theWorld ){
				DisposeGWorld( qtPriv->CI.theWorld );
				qtPriv->CI.theWorld = nil;
			}
			if( qms->imageFrame && qms->dealloc_imageFrame ){
			  int i;
				for( i = 0 ; i < qms->frameBuffers ; i++ ){
					DisposePtr( (Ptr) qms->imageFrame[i] );
				}
				DisposePtr((Ptr)qms->imageFrame);
				qms->imageFrame = NULL;
			}
			if( qtPriv->saved_theURL ){
				QTils_free((void**)&qms->theURL);
				qms->theURL = qtPriv->saved_theURL;
			}
			if( qtPriv->dataRef ){
				DisposeHandle( (Handle) qtPriv->dataRef );
				qtPriv->dataRef = NULL;
			}
			QTils_free(&qtPriv);
			qms->privQT = NULL;
			if( closeQT ){
				ExitMovies();
#if TARGET_OS_WIN32
				TerminateQTML();
#endif
			}
		}
		qms->lastErr = err;
		if( qms->dealloc_qms ){
			QTils_free(&qms);
			*QMS = NULL;
		}
	}
	return (int) err;
}

// forward:
static ErrCode QTMovieSinkICM_AddFrame( QTMovieSinks *qms, double frameDuration );

ErrCode QTMovieSink_AddFrame( QTMovieSinks *qms, double frameDuration )
{
	if( qms->useICM ){
		return QTMovieSinkICM_AddFrame( qms, frameDuration );
	}
	else{
	  OSErr err;
	  register QTMovieSinkQTStuff *qtPriv = qms->privQT;
#if TARGET_OS_WIN32
	  int current;
		if( qms->AddFrame_RT ){
			current = set_priority_RT();
		}
#endif
		err = CompressImage( qtPriv->CI.thePixMap, &qtPriv->trackFrame,
				qtPriv->quality, qtPriv->codecType,
				qtPriv->CI.imgDesc, qtPriv->CI.compressedDataPtr
		);
		if( err == noErr ){
		  register TimeValue duration = (TimeValue) (frameDuration * kVideoTimeScale + 0.5);
			if( duration <= 0 ){
				duration = 1;
			}
			err = AddMediaSample( qtPriv->theMedia, qtPriv->CI.compressedData,
					 kNoOffset,			/* no offset in data */
					 (**qtPriv->CI.imgDesc).dataSize,
					 duration,
					 (SampleDescriptionHandle) qtPriv->CI.imgDesc,
					 kAddOneVideoSample,	/* one sample */
					 kSyncSample,			/* self-contained samples */
					 nil
			);
			if( err == noErr ){
				qtPriv->Frames += 1;
			}
		}
		qms->currentFrame = (qms->currentFrame + 1) % qms->frameBuffers;
#if TARGET_OS_WIN32
		if( qms->AddFrame_RT ){
			set_priority(current);
		}
#endif
		return err;
	}
}

// ############################################################
#pragma mark ----QTMovieSinkICM using ICMCompressionSession----

/*!
	Called after a frame has been encoded: takes the referenced frame and adds it to the movie track's Media.
 */
static OSStatus EncodedFrameOutputCallback( register void *encodedFrameOutputRefCon, ICMCompressionSessionRef session,
								    OSStatus error, ICMEncodedFrameRef frame, void *reserved)
{ OSStatus err = noErr;
  register Media theMedia = ((Media) encodedFrameOutputRefCon);
#ifdef DEBUG
  ImageDescriptionHandle imageDesc;
  MediaSampleFlags flags;
  OSStatus err2;
#endif

	err = AddMediaSampleFromEncodedFrame(theMedia, frame, NULL);

#ifdef DEBUG
	flags =  ICMEncodedFrameGetMediaSampleFlags(frame);
	if( (err = ICMEncodedFrameGetImageDescription(frame, &imageDesc)) != noErr ){
		fprintf( stderr, "EncodedFrameOutputCallback - ICMEncodedFrameGetImageDescription failed (%d)\n", err2 );
	}
#endif

	return err;
}

/*!
	function callback that is called multiple times during the encoding process,
	providing information on the encoding process. It updates statistics and the total frame counter
	and informes when the PixelBuffer is available for reuse.
 */
static void SourceTrackingCallback( void *sourceTrackingRefCon, register ICMSourceTrackingFlags sourceTrackingFlags,
							 void *sourceFrameRefCon, void *reserved)
{ register ICMEncodingFrame *frame = (ICMEncodingFrame*) sourceFrameRefCon;
	if( sourceTrackingFlags & kICMSourceTracking_FrameWasDropped ){
		frame->qtPriv->ICM.Dropped += 1;
		frame->OK = FALSE;
	}
	if( sourceTrackingFlags & kICMSourceTracking_FrameWasMerged ){
		frame->qtPriv->ICM.Merged += 1;
		frame->OK = FALSE;
	}
	if( sourceTrackingFlags & kICMSourceTracking_FrameTimeWasChanged ){
		frame->qtPriv->ICM.timeChanged += 1;
	}
#ifdef BUFFERCOPIED_STATS
	if( sourceTrackingFlags & kICMSourceTracking_CopiedPixelBuffer ){
		frame->qtPriv->ICM.bufferCopied += 1;
	}
#endif
	if( frame->OK && (sourceTrackingFlags & kICMSourceTracking_LastCall) ){
		frame->qtPriv->Frames += 1;
	}
	if( sourceTrackingFlags & kICMSourceTracking_ReleasedPixelBuffer ){
		frame->inUse = FALSE;
	}
}

static void ReleaseBytesCallback( void *releaseRefCon, const void *baseAddress )
{
    return;
}

/*!
	Utility to set an SInt32 value in a CFDictionary. From:
	file://localhost/Developer/ADC%20Reference%20Library/qa/qa2005/qa1443.html
	@n
	Used during the creation of the PixelBuffer pool.
 */
static OSStatus CFDict_SetNumberValue(CFMutableDictionaryRef inDict, CFStringRef inKey, SInt32 inValue)
{ CFNumberRef number;

	number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
	if( NULL == number ){
		return coreFoundationUnknownErr;
	}

	CFDictionarySetValue(inDict, inKey, number);

	CFRelease(number);

	return noErr;
}

/*!
	the internal worker function for opening a QT Movie Sink that uses ICM Compression Sessions
 */
static QTMovieSinks *_open_QTMovieSinkICM_( QTMovieSinks *qms, const char *theURL,
						  QTAPixel **imageFrame,
						  unsigned short Width, unsigned short Height, int hasAlpha,
						  unsigned short frameBuffers,
						  unsigned long codec, unsigned long quality,
						  int openQT, ErrCode *errReturn )
{ OSErr err = noErr;
  CVReturn err2;
	if( frameBuffers <= 0 ){
		if( errReturn ){
			*errReturn = paramErr;
		}
		return NULL;
	}
	if( theURL && *theURL ){
		if( !qms ){
			qms = (QTMovieSinks*) QTils_calloc( 1, sizeof(QTMovieSinks) );
			if( qms ){
				qms->dealloc_qms = 1;
			}
		}
		else{
			memset( qms, 0, sizeof(QTMovieSinks) );
		}
	}
	if( qms && theURL && *theURL ){
	  static QTMovieSinkQTStuff *qtPriv;
	  ICMCompressionSessionOptionsRef options_ref;
	  Boolean enable = TRUE;
		qtPriv = (QTMovieSinkQTStuff*) QTils_malloc( sizeof(QTMovieSinkQTStuff) );
		if( qtPriv ){
			memset( qtPriv, 0, sizeof(QTMovieSinkQTStuff) );
			if( openQT ){
#if TARGET_OS_WIN32
				err = InitializeQTML(0);
#endif
				if( err == noErr ){
					err = EnterMovies();
				}
				if( err != noErr ){
					goto bail;
				}
			}
			err = CreateMovieStorageFromURL( &theURL, 'TVOD',
						smCurrentScript, createMovieFileDeleteCurFile|createMovieFileDontCreateResFile,
						&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->cbState,
						&qtPriv->dataHandler, &qtPriv->theMovie, (char**) &qtPriv->saved_theURL
			);
			if( err != noErr ){
				goto bail;
			}
			qms->theURL = theURL;
			qms->useICM = TRUE;

			qtPriv->ICM.pixel_buffer_ref = (CVPixelBufferRef*) NewPtrClear( frameBuffers * sizeof(CVPixelBufferRef) );
			qtPriv->ICM.pixel_buffer_frames = (ICMEncodingFrame*) NewPtrClear( frameBuffers * sizeof(ICMEncodingFrame) );
			if( !imageFrame ){
				qms->imageFrame = (QTAPixel**) NewPtrClear( frameBuffers * sizeof(QTAPixel*) );
			}
			else{
				qms->imageFrame = imageFrame;
			}
			qtPriv->numPixels = Width * Height;
			if( qtPriv->ICM.pixel_buffer_ref && qtPriv->ICM.pixel_buffer_frames && qms->imageFrame ){
			 size_t bytesPerRow;
			 int i;
				qms->dealloc_imageFrame = (imageFrame)? 0 : 1;
				qms->currentFrame = 0;
				qms->frameBuffers = frameBuffers;
				qtPriv->trackFrame.left = 0;
				qtPriv->trackFrame.top = 0;
				qtPriv->trackFrame.right = Width;
				qtPriv->trackFrame.bottom = Height;
				if( hasAlpha ){
#if TARGET_OS_WIN32
					qtPriv->pixelFormat = k32BGRAPixelFormat /*k32RGBAPixelFormat*/;
#else
					qtPriv->pixelFormat = k32ARGBPixelFormat;
#endif
					bytesPerRow = Width * 4;
				}
				else{
#if TARGET_OS_WIN32
					qtPriv->pixelFormat = k24BGRPixelFormat;
					bytesPerRow = Width * 3;
#else
					// Mac OS X always gives you a k32ARGBPixelFormat when asking for a 24 bit depth...
					qtPriv->pixelFormat = k32ARGBPixelFormat /*k24RGBPixelFormat*/;
					bytesPerRow = Width * 4;
#endif
				}
				if( imageFrame ){
					err2 = kCVReturnSuccess;
					for( i = 0 ; i < frameBuffers && err2 == kCVReturnSuccess ; i++ ){
						err2 = CVPixelBufferCreateWithBytes( kCFAllocatorDefault,
									    Width, Height, qtPriv->pixelFormat,
									    qms->imageFrame[i], bytesPerRow,
									    ReleaseBytesCallback, (void*) qms,
									    NULL, // CFDictionaryRef pixelBufferAttributes,
									    &qtPriv->ICM.pixel_buffer_ref[i]
						);
						qtPriv->ICM.pixel_buffer_frames[i].qtPriv = qtPriv;
					}
				}
				else{
#if 0
					err2 = kCVReturnSuccess;
					for( i = 0 ; i < frameBuffers && err2 == kCVReturnSuccess ; i++ ){
						err2 = CVPixelBufferCreate( kCFAllocatorDefault,
										Width, Height, qtPriv->pixelFormat,
										NULL, // CFDictionaryRef pixelBufferAttributes,
										&qtPriv->ICM.pixel_buffer_ref[i]
						);
						qtPriv->ICM.pixel_buffer_frames[i].qtPriv = qtPriv;
					}
#else
					// theoretically, allocating a buffer from a pool should allow the best adapted kind of
					// memory allocation...
					{ CFMutableDictionaryRef dict;
						dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
						if( dict ){
							CFDict_SetNumberValue( dict, kCVPixelBufferPixelFormatTypeKey, qtPriv->pixelFormat );
							CFDict_SetNumberValue( dict, kCVPixelBufferWidthKey, Width );
							CFDict_SetNumberValue( dict, kCVPixelBufferHeightKey, Height );
							CFDict_SetNumberValue( dict, kCVPixelBufferBytesPerRowAlignmentKey, 16*2 );
							err2 = CVPixelBufferPoolCreate( kCFAllocatorDefault, NULL, dict,
										  &qtPriv->ICM.pixel_buffer_pool
							);
							CFDictionaryRemoveAllValues( dict );
							CFRelease(dict);
							for( i = 0 ; i < frameBuffers && err2 == kCVReturnSuccess ; i++ ){
								err2 = CVPixelBufferPoolCreatePixelBuffer( kCFAllocatorDefault,
										  qtPriv->ICM.pixel_buffer_pool, &qtPriv->ICM.pixel_buffer_ref[i]
								);
								qtPriv->ICM.pixel_buffer_frames[i].qtPriv = qtPriv;
							}
						}
						else{
							err2 = kCVReturnPoolAllocationFailed;
						}
					}
#endif
					for( i = 0 ; i < frameBuffers && err2 == kCVReturnSuccess ; i++ ){
						err2 = CVPixelBufferLockBaseAddress(qtPriv->ICM.pixel_buffer_ref[i], 0);
						if( err2 == kCVReturnSuccess ){
							qms->imageFrame[i] = (QTAPixel*) CVPixelBufferGetBaseAddress(qtPriv->ICM.pixel_buffer_ref[i]);
						}
						qtPriv->ICM.pixel_buffer_frames[i].qtPriv = qtPriv;
					}
				}
				if( err2 != kCVReturnSuccess || !qms->imageFrame ){
					err = (OSErr)err2;
					goto bail;
				}
				if( (err = ICMCompressionSessionOptionsCreate( kCFAllocatorDefault, &options_ref)) != noErr ){
					goto bail;
				}

				if( (err = ICMCompressionSessionOptionsSetDurationsNeeded( options_ref, TRUE)) != noErr ){
					goto bail;
				}
				ICMCompressionSessionOptionsSetAllowFrameTimeChanges( options_ref, FALSE );
				ICMCompressionSessionOptionsSetAllowTemporalCompression( options_ref, TRUE );
				ICMCompressionSessionOptionsSetProperty( options_ref,
							kQTPropertyClass_ICMCompressionSessionOptions,
							kICMCompressionSessionOptionsPropertyID_AllowAsyncCompletion,
							sizeof(Boolean), &enable
				);
				qtPriv->quality = (CodecQ) quality;
				ICMCompressionSessionOptionsSetProperty( options_ref,
							kQTPropertyClass_ICMCompressionSessionOptions,
							kICMCompressionSessionOptionsPropertyID_Quality,
							sizeof(CodecQ), &qtPriv->quality
				);
#ifdef DEBUG
				{ ByteCount n;
				  OSStatus _err;
				  ComponentInstance ci = NULL, cj = NULL;
				  CompressorComponent cc = bestSpeedCodec;
				  long m;
				  ICMCompressionSessionOptionsRef options;
				  QTAtomContainer container;
				  SCTemporalSettings temporalSettings;
				  SCSpatialSettings spatialSettings;
				  long codecFlags, flags = scAllowEncodingWithCompressionSession;
					_err = ICMCompressionSessionOptionsGetProperty( options_ref,
								kQTPropertyClass_ICMCompressionSessionOptions,
								kICMCompressionSessionOptionsPropertyID_Quality,
								sizeof(CodecQ), &quality, &n
					);
					if( codec == cTIFFCodec ){
					  Handle qtCompressionCompressorSettings = NewHandle(0);
					  Handle retSettings = NewHandle(0);
						_err = OpenADefaultComponent( GraphicsExporterComponentType, kQTFileTypeTIFF, &ci);
						_err = OpenADefaultComponent( StandardCompressionType, StandardCompressionSubType, &cj);
						_err = GraphicsExportSetCompressionMethod( ci, kQTTIFFCompression_PackBits );
						_err = GraphicsExportGetCompressionMethod( ci, &m );
						_err = ImageCodecGetSettings( ci, qtCompressionCompressorSettings);
						_err = SCSetInfo(cj, scPreferenceFlagsType, &flags);
							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCRequestSequenceSettings(cj);
							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCGetSettingsAsAtomContainer(cj, &container);
							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCCopyCompressionSessionOptions(cj, &options);
							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCGetInfo(cj, scSpatialSettingsType, &spatialSettings);
							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCGetInfo(cj, scTemporalSettingsType, &temporalSettings);
						_err = SCGetInfo(cj, scCodecFlagsType, &codecFlags);
						_err = ICMCompressionSessionOptionsSetProperty( options_ref,
									kQTPropertyClass_ICMCompressionSessionOptions,
									kICMCompressionSessionOptionsPropertyID_CompressorComponent,
									sizeof(&spatialSettings.codec), &spatialSettings.codec );
						DisposeHandle(qtCompressionCompressorSettings);
						DisposeHandle(retSettings);
						CloseComponent(ci);
						CloseComponent(cj);
					}
				}
#endif
			}
#if 0
			else{
				err = errno;
				goto bail;
			}
#endif

			SetMovieTimeScale( qtPriv->theMovie, kVideoTimeScale );
			qtPriv->theTrack = NewMovieTrack( qtPriv->theMovie, Long2Fix((long)Width), Long2Fix((long)Height), 0 );
			err = GetMoviesError();
			if( !qtPriv->theTrack || err != noErr ){
				goto bail;
			}
			qtPriv->theMedia = NewTrackMedia( qtPriv->theTrack, VideoMediaType, kVideoTimeScale, nil, 0 );
			err = GetMoviesError();
			if( !qtPriv->theTrack || err != noErr ){
				goto bail;
			}

			{ ICMEncodedFrameOutputRecord encoded_frame_output_record;
				encoded_frame_output_record.encodedFrameOutputCallback = EncodedFrameOutputCallback;
				encoded_frame_output_record.encodedFrameOutputRefCon = qtPriv->theMedia;
				encoded_frame_output_record.frameDataAllocator = NULL;

				if( (err = ICMCompressionSessionCreate( kCFAllocatorDefault,
						    Width, Height, codec, //kJPEGCodecType,
						    kVideoTimeScale, options_ref,
						    NULL /*source_pixel_buffer_attributes*/,
						    &encoded_frame_output_record,
						    &qtPriv->ICM.compression_session_ref
					)) != noErr
				){
					goto bail;
				}
			}
			ICMCompressionSessionOptionsRelease(options_ref);

			qtPriv->ICM.source_tracking_callback_record.sourceTrackingCallback = SourceTrackingCallback;
			qtPriv->ICM.source_tracking_callback_record.sourceTrackingRefCon = qtPriv;

#ifndef BUFFERCOPIED_STATS
			qtPriv->ICM.bufferCopied = -1;
#endif

			err = BeginMediaEdits(qtPriv->theMedia);
			if( err != noErr ){
				goto bail;
			}

			qms->Width = Width;
			qms->Height = Height;
			qms->hasAlpha = hasAlpha;
			qms->currentFrame = 0;
			qtPriv->Frames = 0;
		}
bail:
		qms->privQT = qtPriv;
		if( err != noErr ){
			close_QTMovieSinkICM( &qms, 0, NULL, openQT );
			qms = NULL;
		}
		else{
		  char infoStr[1024];
		  long gRes;
		  OSErr gErr = Gestalt(gestaltQuickTimeVersion, &gRes);
			if( gErr != noErr ){
				gRes = 0;
			}
			snprintf( infoStr, sizeof(infoStr), "QTMovieSink using ICM, %s - %s ; QT v%x.%x.%x %s",
				qualityString(quality), __DATE__,
				(gRes >> 24) & 0x00f, (gRes >> 20) & 0x00f, (gRes >> 16) & 0x00f,
#if defined(__MACH__) || defined(__APPLE_CC__)
				"Mac OS X"
#else
				"MS Windows"
#endif
			);
			QTMovieSink_AddTrackMetaDataString( qms, akSoftware, infoStr, NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akWriter, "RJVB", NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akCopyRight, "(C) 2010 RJVB", NULL );
			QTMovieSink_AddTrackMetaDataString( qms, akSource, theURL, NULL );

			qms->lastErr = (unsigned long) err;
		}
	}
	if( errReturn ){
		*errReturn = (ErrCode) err;
	}
	return qms;
}

int get_QTMovieSink_EncodingStats( QTMovieSinks *qms, QTMSEncodingStats *stats )
{
	if( qms && qms->privQT && stats ){
		stats->Total = qms->privQT->Frames;
		if( qms->privQT->theMovie ){
			stats->Duration = ((double)GetMovieDuration(qms->privQT->theMovie)) /
							((double)GetMovieTimeScale(qms->privQT->theMovie));
			GetMovieStaticFrameRate( qms->privQT->theMovie, &stats->frameRate, NULL, NULL );
		}
		else{
			stats->Duration = stats->frameRate = -1.0;
		}
		if( qms->privQT->ICM.pixel_buffer_ref ){
			stats->Dropped = qms->privQT->ICM.Dropped;
			stats->Merged = qms->privQT->ICM.Merged;
			stats->timeChanged = qms->privQT->ICM.timeChanged;
			stats->bufferCopied = qms->privQT->ICM.bufferCopied;
//			fprintf( stderr, "ICM encoding cycled over %ld buffers\n", qms->privQT->ICM.cycles );
		}
		else{
			stats->Dropped = stats->Merged = stats->timeChanged = stats->bufferCopied = -1;
		}
		return TRUE;
	}
	else{
		if( stats && qms && qms->privQT ){
			// we received a valid QTMovieSink, buf no ICM encoding stats are available; set
			// all fields to -1.
			stats->Total = stats->Dropped = stats->Merged = stats->timeChanged = stats->bufferCopied = -1;
		}
		return FALSE;
	}
}

/*!
	the internal worker function for closing a QT Movie Sink that uses ICM Compression Sessions
 */
static ErrCode close_QTMovieSinkICM( QTMovieSinks **QMS, int addTCTrack, QTMSEncodingStats *stats, int closeQT )
{ OSErr err = noErr;
  OSStatus err2 = noErr;
  QTMovieSinks *qms;

	if( QMS ){
		qms = *QMS;
	}
	else{
		return (int) paramErr;
	}
	if( qms && qms->privQT ){
	  QTMovieSinkQTStuff *qtPriv = qms->privQT;
		if( qtPriv->theMedia ){
			if( qtPriv->ICM.compression_session_ref ){
				err2 = ICMCompressionSessionCompleteFrames(qtPriv->ICM.compression_session_ref, TRUE, 0, 0);
			}
			err = EndMediaEdits( qtPriv->theMedia );
			if( err == noErr && err2 == noErr && qtPriv->theTrack ){
				err = InsertMediaIntoTrack( qtPriv->theTrack, 0, 0,
						GetMediaDuration( qtPriv->theMedia ), fixed1
				);
				if( err == noErr && qtPriv->theMovie && qtPriv->dataHandler ){
					{ MatrixRecord m;
						SetIdentityMatrix(&m);
						ScaleMatrix( &m, Long2Fix(1), Long2Fix(-1), Long2Fix(1), Long2Fix(1) );
						SetTrackMatrix( qtPriv->theTrack, &m );
//						SetMovieMatrix( qtPriv->theMovie, &m );
					}
					if( addTCTrack ){
						AddTCTrack( qms );
					}
					AnchorMovie2TopLeft( qtPriv->theMovie );
					UpdateMovie( qtPriv->theMovie );
					err = AddMovieToStorage( qtPriv->theMovie, qtPriv->dataHandler );
					if( err == noErr ){
						err = CloseMovieStorage( qtPriv->dataHandler );
#if TARGET_OS_WIN32
#else
						if( qtPriv->cbState.urlRef ){
							CSBackupSetItemExcluded( qtPriv->cbState.urlRef,
											    qtPriv->cbState.bakExcl, qtPriv->cbState.bakExclPath );
							CFRelease(qtPriv->cbState.urlRef);
						}
						sync();
#endif
					}
				}
			}
		}
		if( err == noErr ){
		  int i;
			if( qtPriv->theTrack ){
				DisposeMovieTrack( qtPriv->theTrack );
				qtPriv->theTrack = nil;
			}
			if( qtPriv->theMovie ){
				if( stats ){
					// at this point, get_QTMovieSink_EncodingStats will currently still have
					// access to all interesting data, and the movie is still open.
					get_QTMovieSink_EncodingStats( qms, stats );
				}
				DisposeMovie( qtPriv->theMovie );
				qtPriv->theMovie = nil;
			}
			if( qtPriv->ICM.compression_session_ref ){
				ICMCompressionSessionRelease( qtPriv->ICM.compression_session_ref );
				qtPriv->ICM.compression_session_ref = nil;
			}
			if( qtPriv->ICM.pixel_buffer_ref ){
				for( i = 0 ; i < qms->frameBuffers ; i++ ){
					if( qtPriv->ICM.pixel_buffer_ref[i] ){
						CVPixelBufferUnlockBaseAddress(qtPriv->ICM.pixel_buffer_ref[i], 0);
						CVPixelBufferRelease( qtPriv->ICM.pixel_buffer_ref[i] );
						qtPriv->ICM.pixel_buffer_ref[i] = nil;
					}
				}
				DisposePtr( (Ptr) qtPriv->ICM.pixel_buffer_ref );
			}
			if( qtPriv->ICM.pixel_buffer_frames ){
				DisposePtr( (Ptr) qtPriv->ICM.pixel_buffer_frames );
			}
			if( qtPriv->ICM.pixel_buffer_pool ){
				CVPixelBufferPoolRelease( qtPriv->ICM.pixel_buffer_pool );
				qtPriv->ICM.pixel_buffer_pool = nil;
			}
			if( qms->imageFrame && qms->dealloc_imageFrame ){
				DisposePtr((Ptr)qms->imageFrame);
				qms->imageFrame = NULL;
			}
			if( qtPriv->saved_theURL ){
				QTils_free((void**)&qms->theURL);
				qms->theURL = qtPriv->saved_theURL;
			}
			if( qtPriv->dataRef ){
				DisposeHandle( (Handle) qtPriv->dataRef );
				qtPriv->dataRef = NULL;
			}
			QTils_free(&qtPriv);
			qms->privQT = NULL;
			if( closeQT ){
				ExitMovies();
#if TARGET_OS_WIN32
				TerminateQTML();
#endif
			}
		}
		qms->lastErr = err;
		if( qms->dealloc_qms ){
			QTils_free(&qms);
			*QMS = NULL;
		}
	}
	return (int) err;
}

ErrCode QTMovieSink_AddFrameWithTime( QTMovieSinks *qms, double frameTime )
{ OSStatus err;
	if( qms->useICM ){
	  register QTMovieSinkQTStuff *qtPriv = qms->privQT;
	  register TimeValue64 time;
	  int i = 0;
#if TARGET_OS_WIN32
	  int current;
		if( qms->AddFrame_RT ){
			current = set_priority_RT();
		}
#endif
		time = (TimeValue64) (frameTime * kVideoTimeScale + 0.5);
		if( time <= 0 ){
			time = 1;
		}
		while( qtPriv->ICM.pixel_buffer_frames[qms->currentFrame].inUse && i < qms->frameBuffers ){
			qms->currentFrame = (qms->currentFrame + 1) % qms->frameBuffers;
			i += 1;
		}
//		qtPriv->ICM.cycles += i;
		if( !qtPriv->ICM.pixel_buffer_frames[qms->currentFrame].inUse ){
		  ICMEncodingFrame *frame = &qtPriv->ICM.pixel_buffer_frames[qms->currentFrame];
			frame->OK = TRUE;
			err = ICMCompressionSessionEncodeFrame( qtPriv->ICM.compression_session_ref,
									   qtPriv->ICM.pixel_buffer_ref[qms->currentFrame],
									   time, 1,
									   kICMValidTime_DisplayTimeStampIsValid,
									   NULL,
									   &qtPriv->ICM.source_tracking_callback_record, (void*)frame
			);
		}
		else{
			qtPriv->ICM.Dropped += 1;
			err = -codecCantQueueErr;
		}
#if TARGET_OS_WIN32
		if( qms->AddFrame_RT ){
			set_priority(current);
		}
#endif
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	the internal worker function for adding an image to a QT Movie Sink that uses ICM Compression Sessions
 */
static ErrCode QTMovieSinkICM_AddFrame( QTMovieSinks *qms, double frameDuration )
{ OSStatus err;
  register QTMovieSinkQTStuff *qtPriv = qms->privQT;
  register TimeValue64 duration;
  int i = 0;
#if TARGET_OS_WIN32
  int current;
	if( qms->AddFrame_RT ){
		current = set_priority_RT();
	}
#endif
	duration = (TimeValue64) (frameDuration * kVideoTimeScale + 0.5);
	if( duration <= 0 ){
		duration = 1;
	}
	while( qtPriv->ICM.pixel_buffer_frames[qms->currentFrame].inUse && i < qms->frameBuffers ){
		qms->currentFrame = (qms->currentFrame + 1) % qms->frameBuffers;
		i += 1;
	}
//	qtPriv->ICM.cycles += i;
	if( !qtPriv->ICM.pixel_buffer_frames[qms->currentFrame].inUse ){
	  ICMEncodingFrame *frame = &qtPriv->ICM.pixel_buffer_frames[qms->currentFrame];
		frame->OK = TRUE;
		err = ICMCompressionSessionEncodeFrame( qtPriv->ICM.compression_session_ref,
								   qtPriv->ICM.pixel_buffer_ref[qms->currentFrame],
								   0, duration,
								   kICMValidTime_DisplayDurationIsValid,
								   NULL,
								   &qtPriv->ICM.source_tracking_callback_record, (void*)frame
		);
	}
	else{
		qtPriv->ICM.Dropped += 1;
		err = -codecCantQueueErr;
	}
#if TARGET_OS_WIN32
	if( qms->AddFrame_RT ){
		set_priority(current);
	}
#endif
	return err;
}

// ###########################
#pragma mark ----Utilities----

#ifdef LOCAL_METADATA_HANDLER
static ErrCode AddMetaDataString( QTMovieSinks *qms, int toTrack,
						  AnnotationKeys key, const char *value, const char *lang )
{ QTMetaDataRef theMetaData;
  OSErr err;
	if( qms && qms->privQT ){
	  OSType inkey;
	  QTMetaDataStorageFormat storageF;
	  QTMetaDataKeyFormat inkeyF;
	  UInt8 *inkeyPtr;
	  ByteCount inkeySize;

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
				inkey = kQTMetaDataCommonKeyDisplayName;
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
				inkeySize = strlen( (char*) inkeyPtr);
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
		if( toTrack ){
			err = QTCopyTrackMetaData( qms->privQT->theTrack, &theMetaData );
		}
		else{
			err = QTCopyMovieMetaData( qms->privQT->theMovie, &theMetaData );
		}
		if( err == noErr ){
		  char *newvalue = NULL;
		  QTMetaDataItem item = kQTMetaDataItemUninitialized;
		  OSStatus qmdErr;
			// not sure if we have to make copies of the metadata values...
			value = strdup(value);
			// check if the key is already present. If so, append the new value to the existing value,
			// separated by a newline.
			qmdErr = QTMetaDataGetNextItem(theMetaData, storageF, item, inkeyF, inkeyPtr, inkeySize, &item);
			if( qmdErr == noErr ){
				if( item != kQTMetaDataItemUninitialized ){
				  ByteCount size=0, nvlen;
					qmdErr = QTMetaDataGetItemValue( theMetaData, item, NULL, 0, &size );
					nvlen=size+strlen(value)+2;
					if( qmdErr == noErr && (newvalue = QTils_calloc( nvlen, sizeof(char) )) ){
						// get the <size> bytes of the value:
						if( (qmdErr = QTMetaDataGetItemValue( theMetaData, item,
										(UInt8*) newvalue, size, 0 )) == noErr
						){ size_t len = strlen(newvalue);
							snprintf( &newvalue[len], nvlen-len, "\n%s", value );
							QTils_free((void**)&value);
							value = newvalue;
							newvalue = NULL;
							 // set the item to the new value:
							if( (qmdErr = QTMetaDataSetItem( theMetaData, item,
										(UInt8*) value, strlen(value), kQTMetaDataTypeUTF8 )) != noErr
							){
								// let's hope QTMetaDataAddItem() will add the item correctly again
								QTMetaDataRemoveItem( theMetaData, item );
							}
							else{
								err = noErr;
								if( lang && *lang ){
									err = QTMetaDataSetItemProperty( theMetaData, item,
											 kPropertyClass_MetaDataItem,
											 kQTMetaDataItemPropertyID_Locale,
											 strlen(lang), lang
									);
								}
								UpdateMovie( qms->privQT->theMovie );
							}
						}
					}
					else{
						qmdErr = -1;
					}
				}
			}
			if( qmdErr != noErr ){
				err = QTMetaDataAddItem( theMetaData, storageF, inkeyF,
						inkeyPtr, inkeySize, (UInt8 *)value, strlen(value), kQTMetaDataTypeUTF8, &item
				);
				if( err != noErr ){
					// failure, in this case we're sure we can release the allocated memory.
					if( value != newvalue ){
						QTils_free((void**)&value);
						value = NULL;
					}
					if( newvalue ){
						QTils_free((void**)&newvalue);
						newvalue = NULL;
					}
				}
				else{
					if( lang && *lang ){
						err = QTMetaDataSetItemProperty( theMetaData, item,
								  kPropertyClass_MetaDataItem,
								  kQTMetaDataItemPropertyID_Locale,
								  strlen(lang), lang
						);
					}
					UpdateMovie( qms->privQT->theMovie );
				}
			}
			QTMetaDataRelease( theMetaData );
		}
	}
	else{
		err = paramErr;
	}
	return (ErrCode) err;
}
#else
extern ErrCode MetaDataHandler( Movie theMovie, Track toTrack,
				    AnnotationKeys key, char **Value, char **Lang, char *separator );
#endif //LOCAL_METADATA_HANDLER

ErrCode QTMovieSink_AddTrackMetaDataString( QTMovieSinks *qms,
							  AnnotationKeys key, const char *value, const char *lang )
{
#ifdef LOCAL_METADATA_HANDLER
	return AddMetaDataString( qms, TRUE, key, value, lang );
#else
	if( qms && qms->privQT ){
		return MetaDataHandler( qms->privQT->theMovie, qms->privQT->theTrack, key,
						   (char**) &value, (lang && *lang)? (char**) &lang : NULL, NULL );
	}
	else{
		return paramErr;
	}
#endif
}

ErrCode QTMovieSink_AddMovieMetaDataString( QTMovieSinks *qms,
								  AnnotationKeys key, const char *value, const char *lang )
{
#ifdef LOCAL_METADATA_HANDLER
	return AddMetaDataString( qms, FALSE, key, value, lang );
#else
	if( qms && qms->privQT ){
		return MetaDataHandler( qms->privQT->theMovie, NULL, key,
						   (char**) &value, (lang && *lang)? (char**) &lang : NULL, NULL );
	}
	else{
		return paramErr;
	}
#endif
}


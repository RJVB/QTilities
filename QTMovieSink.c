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
#	define QTils_free(p)		free((p))
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
				QTils_free(theURL);
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
		QTils_free(theURL);
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
				QTils_free(*URL);
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

static OSErr CreateMovieStorageFromVoid( OSType creator, ScriptCode scriptTag, long flags,
						   Handle *dataRef, OSType *dataRefType,
						   DataHandler *outDataHandler, Movie *newMovie )
{ OSErr err, err2;
	err = DataRefFromVoid( dataRef, dataRefType );
	if( err == noErr ){
		err = CreateMovieStorage( *dataRef, *dataRefType, creator,
					scriptTag, flags, outDataHandler, newMovie
		);
		if( err != noErr ){
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
//	if( theURL && *theURL ){
		if( !qms ){
			qms = (QTMovieSinks*) QTils_calloc( 1, sizeof(QTMovieSinks) );
			if( qms ){
				qms->dealloc_qms = 1;
			}
		}
		else{
			memset( qms, 0, sizeof(QTMovieSinks) );
		}
//	}
	if( qms /*&& theURL && *theURL*/ ){
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
			if( theURL && *theURL ){
				err = CreateMovieStorageFromURL( &theURL, 'TVOD',
							smCurrentScript, createMovieFileDeleteCurFile|createMovieFileDontCreateResFile,
							&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->cbState,
							&qtPriv->dataHandler, &qtPriv->theMovie, (char**) &qtPriv->saved_theURL
				);
			}
			else{
				err = CreateMovieStorageFromVoid( 'TVOD', smCurrentScript, 0,
							&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->dataHandler, &qtPriv->theMovie
				);
			}
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
			close_QTMovieSink( &qms, 0, NULL, 1, openQT );
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
static ErrCode close_QTMovieSinkICM( QTMovieSinks **qms, int addTCTrack, QTMSEncodingStats *stats,
							 int closeMovie, int closeQT );

ErrCode close_QTMovieSink( QTMovieSinks **QMS, int addTCTrack, QTMSEncodingStats *stats,
					 int closeMovie, int closeQT )
{ OSErr err = noErr;
  QTMovieSinks *qms;

	if( QMS ){
		qms = *QMS;
	}
	else{
		return (int) paramErr;
	}
	if( qms->useICM ){
		return close_QTMovieSinkICM( QMS, addTCTrack, stats, closeMovie, closeQT );
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
			qtPriv->theMedia = NULL;
		}
		if( err == noErr ){
			if( qtPriv->theTrack ){
				if( stats ){
					// at this point, get_QTMovieSink_EncodingStats will currently still have
					// access to all interesting data, and the movie is still open.
					get_QTMovieSink_EncodingStats( qms, stats );
				}
				if( closeMovie ){
					DisposeMovieTrack( qtPriv->theTrack );
					qtPriv->theTrack = nil;
				}
			}
			if( qtPriv->theMovie && closeMovie ){
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
			if( qtPriv->CI.theWorld && closeMovie ){
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
				QTils_free(qms->theURL);
				qms->theURL = qtPriv->saved_theURL;
			}
			if( qtPriv->dataRef && closeMovie ){
				DisposeHandle( (Handle) qtPriv->dataRef );
				qtPriv->dataRef = NULL;
			}
			if( closeMovie ){
				QTils_free(qtPriv);
				qms->privQT = NULL;
			}
			if( closeMovie && closeQT ){
				ExitMovies();
#if TARGET_OS_WIN32
				TerminateQTML();
#endif
			}
		}
		qms->lastErr = err;
		if( qms->dealloc_qms && closeMovie ){
			QTils_free(qms);
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
//	if( theURL && *theURL ){
		if( !qms ){
			qms = (QTMovieSinks*) QTils_calloc( 1, sizeof(QTMovieSinks) );
			if( qms ){
				qms->dealloc_qms = 1;
			}
		}
		else{
			memset( qms, 0, sizeof(QTMovieSinks) );
		}
//	}
	if( qms /*&& theURL && *theURL*/ ){
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
			if( theURL && *theURL ){
				err = CreateMovieStorageFromURL( &theURL, 'TVOD',
							smCurrentScript, createMovieFileDeleteCurFile|createMovieFileDontCreateResFile,
							&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->cbState,
							&qtPriv->dataHandler, &qtPriv->theMovie, (char**) &qtPriv->saved_theURL
				);
			}
			else{
				err = CreateMovieStorageFromVoid( 'TVOD', smCurrentScript, 0,
							&qtPriv->dataRef, &qtPriv->dataRefType, &qtPriv->dataHandler, &qtPriv->theMovie
				);
			}
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
//							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCRequestSequenceSettings(cj);
//							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCGetSettingsAsAtomContainer(cj, &container);
//							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCCopyCompressionSessionOptions(cj, &options);
//							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
						_err = SCGetInfo(cj, scSpatialSettingsType, &spatialSettings);
//							fprintf( stderr, "%s:%d _err=%d\n", __FILE__, __LINE__, (int) _err );
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
			close_QTMovieSinkICM( &qms, 0, NULL, 1, openQT );
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
static ErrCode close_QTMovieSinkICM( QTMovieSinks **QMS, int addTCTrack, QTMSEncodingStats *stats,
							 int closeMovie, int closeQT )
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
			qtPriv->theMedia = NULL;
		}
		if( err == noErr ){
		  int i;
			if( qtPriv->theTrack && closeMovie ){
				DisposeMovieTrack( qtPriv->theTrack );
				qtPriv->theTrack = nil;
			}
			if( qtPriv->theMovie ){
				if( stats ){
					// at this point, get_QTMovieSink_EncodingStats will currently still have
					// access to all interesting data, and the movie is still open.
					get_QTMovieSink_EncodingStats( qms, stats );
				}
				if( closeMovie ){
					DisposeMovie( qtPriv->theMovie );
					qtPriv->theMovie = nil;
				}
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
				QTils_free(qms->theURL);
				qms->theURL = qtPriv->saved_theURL;
			}
			if( qtPriv->dataRef ){
				DisposeHandle( (Handle) qtPriv->dataRef );
				qtPriv->dataRef = NULL;
			}
			if( closeMovie ){
				QTils_free(qtPriv);
				qms->privQT = NULL;
			}
			if( closeMovie && closeQT ){
				ExitMovies();
#if TARGET_OS_WIN32
				TerminateQTML();
#endif
			}
		}
		qms->lastErr = err;
		if( qms->dealloc_qms && closeMovie ){
			QTils_free(qms);
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
							QTils_free(value);
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
						QTils_free(value);
						value = NULL;
					}
					if( newvalue ){
						QTils_free(newvalue);
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


#define QTILS_LOGO_WIDTH	128
#define QTILS_LOGO_HEIGHT 128
static char QTils_Logo[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251,
	248, 245, 241, 237, 232, 228, 224, 221, 218, 216, 214, 213, 213, 213, 214, 216, 218,
	221, 224, 227, 231, 236, 241, 245, 248, 251, 253, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 253, 248, 244, 237, 230, 224, 216, 210, 205, 202, 200, 199, 200,
	201, 203, 202, 203, 203, 202, 201, 202, 201, 200, 199, 199, 202, 206, 210, 216, 223,
	230, 237, 243, 248, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 246, 239, 231, 221, 212, 205,
	199, 195, 190, 184, 173, 166, 155, 141, 132, 121, 114, 104, 105, 108, 105, 104, 113,
	120, 130, 140, 153, 164, 171, 183, 190, 194, 198, 203, 211, 220, 230, 239, 246, 251,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253,
	247, 239, 228, 217, 206, 199, 194, 185, 169, 141, 113,  83,  55,  37,  21,  12,  10,
	 10,  11,  11,  11,  11,  11,  11,  11,  11,  11,  10,  10,  11,  19,  34,  55,  81,
	111, 139, 167, 184, 194, 198, 205, 216, 228, 239, 246, 253, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 251, 243, 232, 219, 206, 198, 191, 174, 144, 101,  60,
	 29,  11,  10,  10,  10,  10,  10,  10,  32,  61,  97, 131, 144, 158, 163, 152, 140,
	122,  89,  55,  28,  10,  10,  10,  10,  10,  10,  10,  26,  57,  98, 141, 173, 190,
	198, 206, 217, 231, 242, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 241, 227, 212,
	201, 193, 174, 136,  84,  37,  11,  10,  10,  10,  10,  10,  10,  10,  10,  75, 186,
	224, 243, 243, 243, 242, 239, 236, 239, 242, 243, 243, 241, 220, 184,  76,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  34,  80, 133, 173, 192, 200, 210, 226, 239, 248,
	253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 253, 249, 239, 224, 208, 200, 190, 155,  94,  37,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10, 117, 229, 217, 208, 203, 200, 198, 196, 196, 197,
	198, 200, 205, 208, 218, 233, 120,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  35,  89, 149, 189, 200, 207, 223, 239, 248, 253, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 241, 224, 208, 201, 188, 139,
	 66,  16,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10, 103,
	198, 188, 179, 163, 150, 142, 131, 127, 134, 144, 151, 166, 180, 188, 201, 106,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  13,  61, 135, 186,
	201, 207, 223, 239, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	253, 243, 228, 210, 202, 189, 128,  50,  10,  11,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  74, 112,  71,  43,  27,  12,  10,  10,  10,
	 10,  10,  13,  29,  44,  73, 115,  76,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  11,  10,  48, 125, 187, 202, 210, 227, 243, 251, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 253, 248, 234, 215, 204, 193, 134,  50,  10,  11,
	 13,  63, 137,  66,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  67, 135,  65,  16,
	 11,  10,  45, 129, 191, 204, 214, 233, 247, 253, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 241,
	223, 206, 199, 152,  57,  10,  10,  39, 121, 203, 242, 249, 172,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10, 173, 249, 244, 207, 130,  45,  10,  10,  52, 145, 197, 206,
	222, 241, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 248, 234, 213, 202, 174,  85,  10,  10,  46, 152, 234,
	249, 232, 214, 202, 200,  83,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  83, 200, 202, 213,
	232, 249, 239, 164,  55,  10,  10,  77, 169, 201, 212, 233, 248, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 245, 226, 206,
	192, 127,  31,  10,  31, 150, 239, 248, 225, 207, 198, 187, 157, 113,  53,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  55, 114, 156, 186, 197, 208, 226, 249, 243, 148,  27,  10,
	 27, 120, 190, 206, 225, 244, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 251, 241, 219, 202, 171,  74,  10,  10,  87, 223, 254, 230, 208,
	197, 176, 129,  67,  19,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  18,
	 64, 126, 176, 197, 209, 230, 254, 218,  79,  10,  10,  68, 166, 201, 218, 241, 251,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 236, 214, 196, 135,
	 33,  10,  10, 140, 252, 242, 215, 200, 177, 112,  40,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  41, 116, 177, 200, 216, 243,
	249, 133,   9,  10,  28, 127, 194, 212, 235, 249, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 249, 233, 210, 188, 100,  11,  10,  25, 178, 255, 232, 209, 191, 129,
	 41,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  42, 136, 193, 209, 233, 255, 173,  22,  10,  10,  93, 185,
	209, 231, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 230, 208, 177,  70,  10,
	 10,  31, 182, 253, 227, 205, 174,  80,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  85,
	176, 206, 227, 253, 181,  31,  10,  10,  63, 172, 207, 228, 247, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 247, 229, 207, 164,  50,  10,  10,  10, 114, 222, 216, 203, 155,  43,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  46, 155, 203, 216, 222, 112,  10,  10,
	 10,  43, 158, 206, 227, 247, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 228, 207, 152,  34,  10,  10,
	 10,  10,  40, 178, 195, 132,  23,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   9,
	  8,   8,   7,   7,   7,   7,   7,   8,   8,   9,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  24, 134, 195, 177,  40,  10,  10,  10,  10,  28, 146, 205, 226, 247, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 248, 229, 207, 147,  27,  10,  10,  10,  10,  10,  10,  98, 123,  13,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,   9,   7,   5,   5,  10,  16,  23,  30,  36,  40,  40,  40,  36,  30,
	 23,  16,  10,   5,   5,   7,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  14, 122,  96,  10,  10,
	 10,  10,  10,  10,  22, 140, 205, 227, 247, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 249, 230, 208, 145,  23,  10,  10,  10,
	 10,  10,  10,  10,  15,  13,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,   9,   7,   7,  14,  32,  53,  78,  97,
	110, 112, 120, 127, 130, 132, 128, 126, 118, 113, 110,  97,  78,  54,  32,  14,   7,
	  5,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  12,  14,  10,  10,  10,  10,  10,  10,  10,  19, 137, 207,
	229, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	251, 233, 210, 150,  24,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   7,
	  5,  17,  43,  76, 101, 117, 123, 147, 171, 198, 208, 226, 233, 233, 233, 232, 233,
	221, 206, 193, 167, 145, 123, 118, 102,  76,  43,  17,   5,   7,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  19, 143, 209, 231, 251, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 253, 237, 213, 158,  28,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,   7,   9,  32,  71, 103, 119, 121, 118, 120, 154, 237,
	243, 239, 230, 222, 218, 215, 214, 216, 218, 223, 232, 239, 243, 238, 161, 121, 118,
	121, 119, 103,  71,  34,   9,   7,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 23, 149, 212, 235, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253,
	241, 217, 171,  37,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   7,  10,  40,  81,
	123, 132, 116, 112, 112, 112, 116, 151, 211, 204, 201, 200, 200, 199, 194, 192, 195,
	200, 200, 201, 202, 205, 213, 157, 117, 112, 112, 112, 116, 133, 126,  83,  41,  11,
	  7,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  30, 163, 216, 239, 253, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 245, 222, 185,  53,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,   9,   9,  34,  87, 158, 206, 235, 205, 109, 110, 109, 109, 113, 146,
	199, 183, 171, 153, 144, 141, 133, 130, 135, 142, 145, 156, 172, 182, 202, 153, 114,
	109, 109, 110, 109, 207, 237, 211, 162,  87,  33,   9,   8,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  44, 180, 221, 244, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249,
	228, 201,  80,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   8,  20,  82, 179, 232, 248,
	238, 219, 213, 145, 108, 106, 106, 107, 117, 123, 106, 110, 113, 114, 115, 114, 114,
	114, 115, 114, 113, 110, 108, 124, 120, 108, 106, 106, 108, 145, 213, 218, 237, 249,
	237, 182,  80,  21,   8,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  70, 197, 227, 248,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 253, 235, 211, 114,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,   9,  12,  54, 173, 245, 249, 228, 209, 202, 199, 188, 161, 102, 105, 106, 107,
	107, 103, 114, 131, 149, 157, 167, 174, 174, 174, 167, 158, 150, 132, 115, 103, 107,
	107, 106, 105, 103, 161, 188, 198, 202, 208, 227, 249, 243, 172,  52,  12,   9,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10, 103, 209, 234, 251, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 242,
	220, 151,  14,  10,  20,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,   9,  15, 118, 237, 252, 227, 208, 200,
	188, 167, 135, 109, 102, 104, 102, 128, 163, 196, 226, 241, 251, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 251, 243, 227, 199, 166, 128, 102, 103, 102, 108, 134, 167,
	188, 200, 208, 228, 252, 233, 114,  15,   9,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  20,  10,  11, 141,
	218, 241, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 248, 227, 186,  43,  10,  29, 200, 175,  46,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	  9,  37, 183, 255, 237, 212, 201, 184, 154, 114,  97, 104,  97, 129, 180, 227, 253,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 253, 229, 184, 133,  95, 104,  97, 113, 153, 183, 201, 213, 238, 253, 176,
	 31,   9,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  47, 175, 202,  31,  10,  34, 181, 226, 247, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 235,
	210,  86,  10,  17, 188, 255, 236, 196,  61,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,   9,   9,  64, 218, 254, 225, 206, 191, 153, 111,
	 97, 100, 101, 157, 218, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 221, 161,
	103,  99,  96, 108, 152, 191, 207, 226, 255, 212,  59,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  61, 195, 236, 255, 194,  19,
	 10,  76, 206, 234, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 244, 223, 143,  10,  10, 156, 255, 233, 209, 131,
	 17,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   9,   9,
	 81, 234, 248, 219, 204, 173, 117,  93,  97, 101, 165, 235, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 235, 169, 100,  95,  92, 117, 175, 205,
	219, 249, 231,  75,  10,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  16, 129, 208, 232, 255, 164,   9,  10, 131, 221, 243, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 232,
	192,  41,  10, 110, 255, 239, 215, 145,  16,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,   9,   9,  63, 209, 232, 216, 201, 155,  94,  93,  88,
	147, 231, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 233, 149,  85,  92,  94, 159, 203, 216, 233, 207,  60,   9,   9,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  15, 144, 215, 239,
	255, 120,  10,  32, 184, 230, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 241, 219,  99,  10,  58, 243, 248, 222, 172,  32,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   9,   7,  39,
	 94, 179, 202, 195, 137,  83,  89, 112, 204, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 208, 116,  87,
	 80, 138, 197, 202, 179,  92,  39,   7,   9,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  30, 169, 222, 248, 245,  59,  10,  88, 215, 240, 253,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 230,
	169,  16,  10, 197, 255, 231, 196,  59,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,   9,  10,   8,  33,  80,  84, 120, 186, 123,  84,  82, 151, 244,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 247, 156,  80,  83, 124, 188, 119,  83,  79,  33,
	  7,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  55,
	195, 231, 255, 196,  12,  11, 158, 229, 249, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 241, 214,  74,  10, 111, 255, 242, 217, 105,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   9,  10,   8,  26,  74,
	 78,  78,  81, 104,  81,  86, 188, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 194,  89,  80, 104,  79,  77,  78,  73,  25,   7,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10, 103, 217, 241, 255, 111,  10,  61, 209,
	239, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249, 231,
	155,  10,  25, 221, 254, 231, 165,  16,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,   9,  10,  11,  10,  21,  67,  76,  74,  74,  75,  79,  95, 217, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 222, 101,  78,  74,  73,  73,
	 75,  66,  19,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  16, 164, 231, 254, 218,  22,  10, 143, 230, 249, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 242, 213,  66,  10, 116, 255, 243, 215,  69,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,   9,   9,  10,  11,  11,  88, 173,  90,
	 72,  70,  70,  73, 103, 229, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 233, 108,  72,  70,  70,  71,  89, 172,  91,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  70, 214, 243, 255, 106,  10,
	 55, 208, 240, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 234,
	155,  10,  14, 208, 255, 235, 157,   8,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	  9,  10,  10,  11,  11,  68, 241, 255, 202, 119,  67,  70, 108, 237, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 241, 113,  69,
	 67, 119, 202, 254, 244,  75,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,   9, 158, 236, 255, 197,  11,  10, 142, 233, 251, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 244, 219,  70,  10,  76, 254, 247, 222,  75,  10,
	 10,  10,  10,  10,  10,  10,  10,   9,  10,  10,  11,  11,  11,  43, 227, 252, 222,
	198, 112,  68,  97, 235, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 240, 102,  66, 113, 199, 222, 250, 233,  52,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  77, 223, 248, 251,
	 67,  10,  60, 214, 243, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 238,
	172,  14,  10, 148, 255, 241, 180,  17,  10,  10,  10,  10,  10,  10,  10,   9,   9,
	 11,  11,  10,  12,  19, 195, 255, 228, 203, 118,  65,  83, 224, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	230,  89,  64, 117, 203, 227, 255, 205,  22,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  19, 183, 241, 255, 143,  10,   8, 159, 237, 253, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 248, 229,  94,  10,  21, 184, 228, 226, 112,  10,
	 10,  10,  10,  10,  10,  10,   9,  10,  11,  10,  11,  12,  12, 148, 255, 235, 212,
	136,  61,  65, 211, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 216,  70,  59, 133, 212, 235, 255,
	152,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10, 116, 226,
	228, 182,  19,  10,  79, 225, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 243,
	197,  30,  10,  42, 179, 198, 192,  49,  10,  10,  10,  10,  10,  10,   9,  10,  10,
	 11,  11,  12,  12,  81, 253, 245, 221, 159,  57,  57, 177, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 188,  55,  54, 157, 221, 245, 253,  79,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  50, 193, 198, 179,  41,  10,  23, 188, 242, 255,
	255, 255, 255, 255, 255, 255, 255, 253, 238, 135,  10,  10,  10,  50, 139, 149,  13,
	 10,  10,  10,  10,   9,   9,  10,  11,  10,  11,  12,  12,  20, 208, 255, 230, 187,
	 73,  56, 126, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 136,  54,  70, 187,
	231, 255, 209,  18,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  13,
	150, 139,  51,  10,  10,  10, 121, 236, 251, 255, 255, 255, 255, 255, 255, 255, 248,
	223,  65,  10,  10,  10,  10,  12,  39,  10,  10,  10,  10,  10,   9,  10,  11,  10,
	 11,  12,  12,  12, 117, 255, 241, 216, 111,  53,  76, 235, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 242,  81,  50, 111, 217, 242, 255, 112,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  40,  12,  10,  10,  10,  10,  54, 218,
	247, 255, 255, 255, 255, 255, 255, 255, 244, 187,  19,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,   9,   9,  10,  11,  11,  11,  12,  11,  12,  25, 219, 255, 232, 166,
	 46,  48, 182, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 192,  45,
	 44, 167, 233, 255, 213,  19,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  13, 176, 244, 255, 255, 255, 255, 255, 255, 253,
	241, 128,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,   9,  10,  10,  11,  12,
	 12,  12,  11,  13, 104, 255, 244, 216,  91,  47, 105, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 114,  45,  91, 218, 245, 255,  92,  10,   9,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	115, 239, 253, 255, 255, 255, 255, 255, 250, 228,  68,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,   9,  10,  11,  11,  11,  12,  11,  12,  13,  11, 193, 255, 238, 169,
	 40,  44, 208, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	216,  46,  38, 171, 238, 255, 181,  10,  10,   9,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  57, 223, 249, 255, 255, 255, 255, 255,
	248, 201,  26,  10,  10,  10,  10,  10,  10,  10,   9,   9,  10,  11,  11,  11,  12,
	 11,  12,  12,  13,  56, 243, 249, 227, 101,  42, 115, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 125,  39, 105, 229, 249, 239,  48,
	 11,  10,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  20, 191, 247, 255, 255, 255, 255, 255, 246, 155,  10,  10,  10,  10,  10,  10,
	 10,  10,   9,  10,  10,  10,  11,  12,  11,  12,  12,  13,  14, 110, 236, 235, 192,
	 48,  36, 204, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 216,  38,  46, 195, 235, 236, 105,  11,  10,   9,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10, 141, 245, 255, 255, 255, 255,
	253, 241, 104,  10,  10,  10,  10,  10,  10,  10,   9,  10,  11,  10,  11,  12,  11,
	 12,  12,  13,  13,  14, 146, 205, 209, 135,  37,  94, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 105,  33, 137, 209, 205,
	144,  11,  11,  10,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  91, 239, 253, 255, 255, 255, 252, 229,  60,  10,  10,  10,  10,  10,
	 10,   9,  10,  10,  10,  11,  12,  11,  12,  12,  13,  13,  13,  14,  68, 151, 189,
	 79,  34, 175, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 185,  31,  78, 189, 150,  67,  11,  11,  11,  10,   9,  10,  10,
	 10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  50, 224, 251, 255, 255,
	255, 251, 207,  27,  10,  10,  10,  10,  10,   9,   9,  10,  11,  11,  11,  12,  11,
	 12,  13,  13,  13,  14,  15,  27,  34,  89,  39,  54, 234, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 241,  61,  34,  88,
	 29,  24,  12,  11,  11,  10,  10,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  20, 197, 250, 255, 255, 255, 250, 177,   9,  10,  10,  10,  10,
	 10,   9,  10,  11,  11,  11,  12,  11,  12,  13,  13,  14,  14,  14,  18,  27,  27,
	 28,  29, 115, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 125,  26,  25,  23,  23,  15,  11,  11,  10,  10,  10,
	  9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10, 164, 250, 255,
	255, 255, 250, 143,  10,  10,  10,  10,  10,   9,  10,  11,  10,  11,  12,  11,  12,
	 13,  13,  14,  14,  14,  14,  21,  25,  25,  26,  27, 180, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 188,  23,
	 23,  22,  23,  17,  11,  11,  11,  11,  10,   9,  10,  10,  10,  10,  10,  10,  10,
	 10,  10,  10,  10,  10,  10, 129, 247, 255, 255, 254, 246, 108,  10,  10,  10,  10,
	 10,   9,  10,  10,  11,  12,  11,  12,  12,  13,  13,  14,  14,  15,  16,  22,  24,
	 24,  25,  41, 225, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 232,  47,  22,  21,  21,  18,  12,  11,  11,  11,
	 10,  10,   9,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  92, 244,
	255, 255, 253, 240,  78,  10,  10,  14,  13,  13,   9,  11,  11,  11,  12,  11,  12,
	 13,  13,  13,  14,  15,  15,  16,  26,  26,  25,  24,  80, 250, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254,
	 88,  20,  22,  22,  22,  13,  11,  12,  11,  10,  10,   9,   9,  10,  10,  10,  10,
	 10,   9,  14,  14,  14,  10,  10,  65, 236, 253, 255, 252, 233,  54,  10,  64, 220,
	220, 186,  34,  11,  11,  12,  11,  12,  13,  13,  13,  14,  15,  15,  16, 148, 228,
	217, 132,  23, 125, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 137,  18, 130, 216, 227, 149,  12,  12,
	 12,  11,  11,  10,   9,  10,  10,  10,  10,  10,  32, 186, 220, 220,  67,  10,  45,
	223, 253, 255, 252, 219,  35,  10, 120, 255, 254, 188,  14,  11,  12,  11,  12,  12,
	 13,  14,  14,  14,  15,  16,  21, 210, 255, 247, 109,  22, 166, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 176,  17, 108, 246, 255, 212,  21,  12,  12,  11,  11,  10,  10,   9,  10,  10,
	 10,  10,  13, 187, 254, 255, 123,  10,  26, 211, 253, 255, 253, 205,  23,  10, 162,
	255, 249, 147,  11,  11,  12,  11,  12,  13,  13,  14,  14,  15,  15,  16,  47, 234,
	254, 232,  69,  20, 194, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 253, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 207,  15,  63, 233, 253, 239,  48,
	 12,  11,  12,  11,  11,  10,   9,  10,  10,  10,  10,  10, 145, 250, 255, 170,  10,
	 13, 196, 253, 255, 253, 193,  12,  10, 198, 255, 249, 111,  11,  12,  11,  12,  13,
	 13,  13,  14,  15,  15,  16,  17,  79, 250, 253, 219,  41,  30, 217, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 239, 223, 229, 247, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 223,  34,  35, 215, 253, 252,  81,  13,  11,  12,  11,  10,  10,  10,   9,
	 10,  10,  10,  10, 110, 246, 255, 202,  14,   8, 179, 254, 255, 253, 184,  10,  27,
	220, 255, 243,  84,  12,  12,  11,  12,  13,  14,  14,  14,  15,  16,  16,  17, 107,
	255, 253, 200,  25,  44, 232, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 251, 234, 197, 165, 178, 220, 247, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 235,  54,  18, 194, 254, 255,
	114,  13,  12,  11,  11,  11,  11,  10,   9,  10,  10,  10,  10,  79, 242, 255, 223,
	 31,  10, 167, 255, 255, 253, 176,  10,  46, 232, 255, 236,  62,  12,  11,  12,  13,
	 13,  14,  14,  15,  15,  15,  16,  18, 131, 255, 255, 181,  20,  60, 237, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 231, 191, 148, 139, 144, 176,
	220, 247, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 243,  67,  16, 180, 254, 255, 139,  13,  12,  12,  12,  11,  11,  10,
	 10,   9,  10,  10,  10,  56, 238, 254, 239,  47,  10, 158, 255, 255, 254, 170,  10,
	 59, 240, 255, 232,  48,  12,  12,  12,  13,  13,  14,  14,  15,  16,  16,  17,  18,
	150, 255, 255, 168,  21,  71, 240, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 251, 228, 188,  82,  32, 156, 175, 154, 176, 220, 248, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248,  79,  16, 165, 255,
	255, 153,  13,  13,  12,  11,  12,  11,  11,  10,   9,  10,  10,  10,  45, 231, 255,
	242,  62,  10, 155, 255, 255, 254, 164,  10,  67, 244, 255, 231,  38,  12,  12,  13,
	 13,  13,  14,  15,  15,  15,  16,  17,  18, 161, 255, 255, 161,  22,  77, 241, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 248, 225, 179,  71,   0,   0,  46, 198,
	186, 154, 176, 221, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 248,  83,  16, 157, 255, 255, 159,  14,  13,  12,  11,  12,  11,
	 11,  10,  10,   9,  10,  10,  40, 226, 255, 242,  70,  10, 150, 255, 255, 255, 163,
	 10,  69, 244, 254, 232,  37,  12,  12,  13,  13,  14,  14,  15,  15,  16,  17,  17,
	 18, 163, 254, 255, 161,  23,  78, 240, 254, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	246, 222, 171,  56,   0,  27, 100,   0,  49, 201, 186, 154, 176, 220, 248, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 247,  85,  17, 158,
	255, 254, 157,  14,  13,  12,  12,  11,  11,  11,  11,  10,   9,  10,  10,  40, 228,
	255, 240,  68,  10, 148, 255, 255, 255, 169,  10,  63, 238, 254, 236,  44,  12,  12,
	 13,  14,  14,  15,  15,  16,  16,  17,  18,  19, 153, 253, 255, 169,  23,  74, 237,
	254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 244, 218, 161,  44,   0,  15, 109, 185, 130,
	  0,  49, 201, 185, 154, 177, 221, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 245,  80,  17, 170, 255, 252, 146,  14,  13,  13,  12,  11,
	 12,  11,  11,  10,   9,  10,  10,  47, 236, 254, 235,  58,  10, 154, 255, 255, 255,
	176,  10,  51, 227, 254, 243,  59,  13,  13,  13,  14,  14,  15,  16,  16,  17,  17,
	 18,  19, 136, 250, 255, 185,  22,  67, 232, 254, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 242,
	214, 149,  32,   0,   0,  81, 116, 131, 183, 131,   0,  48, 201, 185, 154, 177, 221,
	247, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 240,  73,  17,
	191, 255, 250, 128,  14,  14,  13,  12,  12,  12,  11,  10,  10,  10,   9,  10,  59,
	247, 253, 227,  42,  10, 158, 255, 255, 255, 183,  10,  35, 213, 252, 251,  81,  13,
	 13,  13,  14,  15,  15,  16,  16,  17,  18,  18,  20, 115, 245, 255, 204,  25,  53,
	225, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 253, 241, 210, 137,  22,   0,   0,   0,  27,  98, 113,
	131, 183, 131,   0,  49, 202, 185, 154, 177, 221, 247, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 253, 230,  60,  19, 206, 255, 243, 105,  15,  13,  13,  13,
	 12,  11,  11,  11,  11,  10,   9,  10,  83, 253, 252, 208,  28,  10, 166, 255, 255,
	255, 194,  10,  16, 193, 249, 255, 111,  13,  13,  13,  14,  15,  16,  16,  16,  17,
	 18,  18,  20,  87, 238, 254, 228,  38,  38, 214, 252, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 237, 205,
	134,  23,   0,   0,   0,   0,   0,  25,  99, 113, 131, 184, 130,   0,  50, 202, 185,
	154, 177, 221, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 252, 217,  45,
	 38, 228, 254, 235,  76,  15,  14,  13,  13,  12,  12,  12,  11,  10,  10,   9,  10,
	117, 255, 250, 186,  13,  10, 179, 255, 255, 255, 206,  19,  11, 157, 236, 246, 142,
	 13,  13,  14,  14,  15,  16,  16,  17,  17,  18,  19,  20,  57, 213, 238, 231,  65,
	 25, 193, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 253, 235, 199, 122,  55,  42,   4,   0,   0,   0,   0,   0,
	 25,  99, 113, 131, 183, 130,   0,  49, 201, 185, 154, 177, 221, 247, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 251, 202,  23,  63, 234, 238, 212,  47,  15,  14,  13,
	 13,  12,  12,  11,  11,  11,  10,  10,  10, 146, 245, 236, 152,  10,  11, 195, 255,
	255, 255, 220,  30,  11, 110, 208, 210, 160,  12,  13,  14,  15,  15,  16,  16,  17,
	 18,  19,  19,  20,  30, 175, 206, 210,  91,  25, 167, 249, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 231, 193, 113,
	 52,  54,  53,  28,   0,   0,   0,   0,   0,   0,  25,  98, 112, 131, 183, 130,   0,
	 49, 202, 185, 154, 177, 220, 247, 255, 255, 255, 255, 255, 255, 255, 255, 249, 176,
	 19,  95, 209, 207, 171,  25,  15,  14,  14,  13,  13,  12,  11,  12,  11,  10,  10,
	 12, 161, 209, 208, 104,  10,  18, 213, 255, 255, 255, 237,  45,  11,  65, 175, 174,
	161,  32,  14,  14,  15,  16,  16,  17,  17,  18,  19,  19,  20,  21, 129, 177, 178,
	115,  26, 133, 244, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 249, 228, 186, 104,  55,  55,  53,  51,  50,  15,   0,   0,   0,
	  0,   0,   0,  25,  99, 113, 132, 184, 131,   0,  51, 202, 185, 154, 177, 221, 248,
	255, 255, 255, 255, 255, 255, 255, 246, 142,  20, 114, 179, 177, 127,  16,  15,  14,
	 14,  13,  13,  12,  11,  12,  11,  11,  10,  28, 162, 174, 175,  61,  10,  35, 227,
	255, 255, 255, 249,  67,  11,  14,  42,  47,  41,  19,  14,  14,  15,  15,  16,  17,
	 18,  18,  19,  20,  20,  21,  36,  50,  49,  42,  26,  94, 235, 251, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249, 226, 179,  94,  57,
	 55,  54,  53,  51,  51,  43,   5,   0,   0,   0,   0,   0,   0,  25,  99, 113, 131,
	183, 131,   0,  50, 200, 178, 147, 174, 221, 248, 255, 255, 255, 255, 255, 252, 237,
	101,  20,  38,  45,  45,  30,  16,  15,  15,  14,  14,  13,  12,  12,  12,  11,  10,
	 10,  16,  38,  45,  40,  13,  10,  56, 241, 255, 255, 255, 255,  98,  12,  11,  14,
	 26,  18,  15,  15,  15,  15,  16,  16,  17,  18,  19,  19,  20,  21,  21,  22,  24,
	 24,  24,  26,  57, 215, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 245, 191,  89,  59,  57,  55,  54,  53,  51,  50,  50,  32,   0,
	  0,   0,   0,   0,   0,   0,  24,  99, 112, 131, 183, 131,   0,  48, 166, 137, 131,
	173, 221, 248, 255, 255, 255, 255, 248, 221,  59,  20,  19,  18,  17,  17,  16,  16,
	 15,  14,  14,  13,  13,  12,  11,  11,  11,  11,  11,  13,  24,  13,   9,  10,  82,
	254, 255, 255, 255, 255, 138,  12,  11,  13,  26,  23,  15,  16,  15,  15,  16,  17,
	 17,  18,  19,  19,  20,  21,  21,  22,  23,  24,  24,  26,  29, 182, 243, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 230, 100,  60,  58,
	 61,  53,  54,  53,  51,  50,  49,  49,  20,   0,   0,   0,   0,   0,   0,   0,  25,
	 99, 113, 131, 184, 123,   4,  32, 109, 113, 129, 173, 221, 248, 255, 255, 255, 244,
	187,  28,  20,  19,  18,  18,  17,  16,  16,  15,  14,  14,  13,  13,  12,  11,  12,
	 11,  12,  11,  19,  24,  11,   9,  10, 119, 255, 255, 255, 255, 255, 175,  11,  11,
	 12,  26,  31,  17,  17,  15,  15,  16,  17,  17,  18,  19,  20,  20,  21,  22,  22,
	 23,  24,  25,  26,  27, 131, 236, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 229,  97,  61,  60,  64, 154, 134,  54,  54,  52,  51,  50,  50,
	 46,  11,   0,   0,   4,   4,   4,   4,   4,  25,  99, 113, 130, 161,  62,   5,  24,
	 97, 112, 129, 173, 221, 248, 255, 253, 238, 138,  21,  20,  19,  18,  18,  17,  16,
	 16,  15,  15,  14,  13,  13,  12,  11,  12,  11,  13,  13,  26,  24,  10,   9,  10,
	162, 255, 255, 255, 255, 255, 212,  19,  12,  11,  23,  35,  22,  17,  17,  15,  16,
	 17,  18,  18,  19,  20,  20,  21,  22,  23,  23,  24,  26,  72,  32,  72, 219, 246,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 223,  77,  63,  83,
	171, 175, 193, 134,  55,  53,  52,  51,  49,  50,  39,   5,   4,   4,   4,   4,   4,
	  5,   5,  28,  99, 115,  80,  10,   7,   5,  26,  97, 112, 129, 173, 222, 248, 246,
	223,  77,  24,  69,  20,  19,  18,  17,  16,  16,  15,  15,  14,  14,  13,  12,  12,
	 11,  12,  13,  18,  31,  21,   9,  10,  12, 197, 255, 255, 255, 255, 255, 241,  51,
	 12,  12,  20,  37,  31,  17,  17,  16,  16,  17,  18,  18,  19,  20,  21,  21,  22,
	 23,  24,  51, 164, 249,  86,  26, 178, 238, 253, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 216,  72,  99, 226, 195, 167, 192, 134,  53,  53,  51,
	 50,  49,  50,  31,   4,   5,   5,   5,   5,   5,   7,   5,  30,  67,   8,   7,   8,
	  8,   8,  26,  97, 112, 130, 173, 220, 233, 186,  25,  78, 248, 162,  46,  18,  17,
	 17,  16,  15,  15,  14,  14,  13,  12,  12,  11,  13,  13,  26,  34,  18,   9,  10,
	 35, 233, 255, 255, 255, 255, 255, 255,  95,  12,  12,  16,  37,  39,  23,  18,  17,
	 16,  17,  18,  19,  19,  20,  21,  21,  22,  23,  22, 166, 251, 255, 153,  30, 114,
	228, 247, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 210,
	 66, 106, 221, 193, 165, 192, 133,  53,  53,  51,  51,  50,  49,  22,   5,   7,   7,
	  8,   8,   8,   9,   8,   8,   9,  10,  10,  10,  10,  10,  28, 100, 120, 138, 172,
	200, 119,  23, 147, 255, 252, 166,  17,  18,  17,  16,  16,  15,  14,  14,  13,  12,
	 12,  12,  14,  18,  36,  34,  14,   9,  10,  78, 253, 255, 255, 255, 255, 255, 255,
	149,  12,  12,  13,  34,  43,  34,  18,  18,  17,  17,  18,  19,  19,  20,  21,  22,
	 23,  23,  25, 123, 235, 255, 221,  36,  49, 196, 238, 253, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 205,  62, 104, 220, 191, 163, 191, 133,
	 54,  53,  52,  51,  51,  47,  16,   7,   9,  10,   9,  10,  10,  10,  11,  11,  11,
	 11,  12,  12,  12,  12,  32, 136, 162, 145, 141,  45,  27, 216, 255, 236, 123,  19,
	 18,  17,  16,  16,  15,  14,  14,  13,  13,  12,  13,  14,  28,  40,  32,  10,   9,
	 10, 131, 255, 255, 255, 255, 255, 255, 255, 203,  15,  13,  12,  29,  45,  44,  23,
	 19,  17,  17,  18,  19,  20,  20,  21,  22,  23,  23,  25,  65, 214, 245, 255, 102,
	 31, 133, 227, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 198,  58, 107, 218, 189, 162, 190, 135,  54,  54,  52,  51,  52,  42,  13,
	 10,  11,  11,  12,  12,  12,  12,  13,  13,  13,  14,  14,  15,  14,  14,  52, 186,
	166,  86,  17,  84, 251, 246, 217,  65,  19,  18,  17,  16,  16,  15,  14,  14,  13,
	 13,  12,  14,  18,  40,  42,  27,   8,   9,  10, 191, 255, 255, 255, 255, 255, 255,
	255, 244,  53,  13,  12,  22,  46,  49,  37,  19,  19,  17,  18,  19,  20,  20,  21,
	 22,  23,  24,  24,  26, 169, 237, 255, 189,  30,  57, 198, 234, 251, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 193,  60, 107, 216, 186,
	160, 190, 134,  54,  54,  53,  52,  52,  36,  12,  13,  13,  14,  14,  14,  14,  15,
	 15,  15,  15,  16,  16,  15,  52,  14,  57, 156,  47,  15, 135, 233, 235, 172,  23,
	 19,  18,  17,  16,  15,  15,  15,  14,  13,  13,  14,  15,  31,  45,  44,  20,   8,
	 10,  40, 237, 255, 255, 255, 255, 255, 255, 255, 255, 113,  13,  13,  16,  44,  51,
	 51,  25,  20,  18,  18,  19,  20,  21,  21,  22,  23,  24,  24,  26, 101, 224, 247,
	251,  81,  32, 129, 221, 242, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 186,  60, 106, 215, 184, 159, 190, 134,  54,  55,  53,  53,
	 52,  30,  13,  15,  15,  16,  16,  16,  17,  17,  17,  17,  18,  17,  52, 168, 131,
	 15,  31,  20,  47, 153, 184, 205, 102,  20,  19,  18,  17,  17,  16,  15,  15,  14,
	 13,  13,  16,  19,  46,  47,  42,  13,   9,  10,  98, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 182,  13,  13,  13,  36,  53,  56,  43,  21,  20,  18,  19,  20,  21,
	 21,  22,  23,  24,  24,  26,  41, 185, 235, 255, 185,  31,  49, 185, 227, 248, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 179,  61,
	109, 214, 182, 158, 190, 135,  55,  55,  54,  54,  52,  26,  16,  18,  18,  18,  18,
	 19,  19,  19,  20,  19,  45, 111, 130, 164, 100,  17,  19, 118, 160, 145, 142,  35,
	 20,  19,  18,  17,  17,  16,  15,  15,  14,  13,  15,  16,  36,  53,  50,  35,   9,
	  9,  10, 169, 255, 255, 255, 255, 255, 255, 255, 255, 255, 239,  45,  14,  13,  25,
	 56,  57,  65,  30,  21,  19,  19,  20,  21,  21,  22,  23,  24,  24,  25,  27, 114,
	221, 244, 255, 102,  34, 100, 211, 234, 251, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 174,  62, 109, 212, 180, 156, 189, 133,  55,
	 56,  55,  54,  49,  23,  18,  20,  20,  20,  21,  21,  21,  22,  21,  39,  98, 104,
	110,  72,  20,  18,  74, 164, 144,  72,  15,  18,  19,  18,  17,  17,  16,  15,  15,
	 14,  14,  17,  24,  61,  54,  53,  24,   9,  10,  31, 230, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 117,  14,  13,  17,  61, 167, 192,  49,  23,  22,  19,  20,
	 21,  21,  22,  23,  24,  25,  25,  26,  42, 180, 228, 253, 224,  48,  32, 151, 219,
	239, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 167,  63, 108, 210, 177, 155, 181, 116,  55,  57,  55,  55,  46,  23,  21,  22,
	 22,  22,  23,  23,  23,  24,  23,  36,  84,  65,  13,  18,  20,  20,  76, 137,  27,
	 13,  14,  17,  17,  17,  17,  16,  15,  15,  14,  16,  18,  41, 191, 165,  60,  14,
	 10,  10,  99, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 201,  15,  14,
	 44, 197, 255, 245,  92,  40,  23,  21,  20,  21,  21,  22,  23,  24,  25,  25,  26,
	 28,  96, 209, 233, 255, 169,  33,  55, 184, 221, 242, 253, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 159,  64, 109, 209, 176, 151,
	109,  55,  59,  57,  56,  56,  42,  23,  23,  24,  24,  24,  25,  25,  25,  25,  25,
	 32,  22,  12,  13,  17,  20,  21,  35,  18,  14,  12,  13,  15,  17,  17,  16,  15,
	 14,  15,  18,  33,  86, 244, 255, 199,  40,  10,  10, 186, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 252,  71,  14,  33, 198, 244, 255, 148,  65,  28,  23,
	 20,  20,  21,  22,  23,  24,  25,  25,  26,  27,  28, 146, 216, 239, 255, 127,  35,
	 90, 199, 222, 245, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 154,  65, 111, 209, 129,  55,  60,  59,  59,  58,  57,  57,  39,
	 24,  25,  26,  26,  27,  27,  27,  27,  27,  26,  20,  11,  11,  12,  16,  20,  23,
	 21,  17,  14,  11,  11,  14,  15,  16,  15,  15,  18,  22,  59, 144, 255, 245, 200,
	 31,  10,  54, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 160,
	 14,  15, 140, 237, 255, 214,  73,  53,  24,  23,  20,  21,  22,  23,  24,  24,  25,
	 26,  27,  29,  50, 174, 217, 243, 242,  90,  36, 110, 201, 223, 245, 253, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 148,  65,  94,
	 69,  63,  61,  61,  59,  59,  58,  58,  56,  36,  26,  28,  28,  28,  28,  28,  29,
	 29,  29,  28,  21,  11,  10,  12,  15,  20,  24,  23,  17,  13,  10,  10,  12,  13,
	 15,  17,  19,  46,  68, 208, 255, 238, 144,  11,  10, 143, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 239,  46,  15,  75, 221, 246, 254, 117,  78,
	 41,  23,  22,  21,  22,  23,  24,  24,  25,  26,  27,  28,  30,  75, 189, 215, 227,
	196,  67,  35, 124, 203, 222, 244, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 143,  67,  65,  64,  63,  62,  61,  60,  59,  58,
	 58,  55,  34,  28,  29,  30,  30,  30,  31,  30,  31,  31,  29,  21,  11,  10,  12,
	 15,  20,  26,  25,  18,  12,   9,   9,  10,  13,  17,  33,  74, 113, 253, 246, 223,
	 79,  11,  33, 230, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 138,  15,  23, 179, 237, 255, 191,  79,  72,  33,  24,  21,  22,  23,  24,  24,
	 25,  26,  27,  28,  29,  31,  94, 189, 194, 189, 115,  35,  35, 132, 202, 220, 242,
	253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	139,  66,  65,  64,  64,  63,  62,  61,  60,  59,  58,  53,  34,  30,  31,  31,  31,
	 32,  32,  32,  32,  32,  30,  22,  11,  10,  11,  14,  20,  28,  28,  20,  11,   7,
	  7,  11,  17,  54,  70, 183, 255, 237, 182,  22,  11, 123, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 229,  37,  16, 108, 225, 247, 249,
	114,  87,  66,  28,  24,  22,  23,  24,  24,  25,  26,  27,  28,  28,  29,  31, 103,
	149,  75,  33,  34,  35,  38, 133, 205, 217, 239, 251, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 134,  66,  65,  65,  64,  63,  62,
	 60,  59,  58,  58,  52,  33,  31,  32,  33,  33,  33,  33,  34,  33,  33,  32,  22,
	 10,   9,  11,  13,  19,  27,  30,  23,  12,   8,   9,  28,  49,  77, 204, 227, 220,
	110,  12,  25, 220, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 137,  16,  36, 188, 234, 255, 200,  83,  88,  57,  27,  24,  23,  23,
	 24,  25,  26,  27,  28,  28,  29,  30,  31,  33,  33,  33,  34,  35,  36,  36, 128,
	204, 213, 233, 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 253, 130,  66,  65,  65,  64,  62,  96, 126,  57,  58,  58,  50,  33,  33,
	 34,  34,  34,  34,  33,  33,  33,  32,  31,  22,  10,   9,  10,  12,  17,  29,  31,
	 27,  18,  21,  36,  35,  86, 142, 162, 157,  33,  11, 119, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 234,  42,  16, 110, 219,
	241, 255, 147,  91,  89,  50,  26,  24,  23,  24,  25,  26,  27,  27,  28,  29,  30,
	 31,  31,  32,  33,  34,  35,  39,  57,  38, 113, 196, 208, 223, 241, 251, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 126,  66,  65,  63,
	 90, 145, 179, 122,  56,  58,  58,  47,  33,  32,  33,  33,  33,  33,  32,  32,  32,
	 31,  30,  21,   9,   8,   9,  17,  29,  31,  30,  28,  26,  30,  43,  83,  89, 102,
	 62,   9,  23, 216, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 154,  16,  30, 174, 224, 250, 236, 107,  94,  89,  44,  26,
	 24,  23,  25,  26,  26,  27,  28,  29,  30,  30,  31,  32,  33,  34,  36, 104, 229,
	 88,  40,  87, 178, 205, 212, 231, 245, 253, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 249, 122,  66,  76, 181, 152, 124, 146,  94,  55,  56,  56,
	 43,  31,  31,  32,  31,  31,  31,  31,  31,  31,  30,  28,  20,  10,  16,  26,  28,
	 29,  28,  29,  27,  25,  25,  44,  66,  68,  10,   8, 115, 249, 254, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 248,  63,  17,
	 77, 202, 228, 255, 208,  95,  98,  88,  42,  27,  24,  24,  25,  26,  27,  28,  29,
	 29,  30,  31,  32,  33,  34,  41, 185, 250, 239, 111,  40,  55, 144, 197, 205, 218,
	234, 246, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 244, 115,
	 62,  99, 156, 122,  78,  39,  37,  52,  54,  53,  40,  29,  30,  30,  30,  30,  29,
	 29,  29,  29,  28,  27,  23,  26,  27,  27,  27,  27,  27,  26,  26,  25,  24,  23,
	 23,   9,  42, 220, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 187,  15,  17, 121, 210, 232, 255, 181,  98, 101,
	 88,  40,  26,  24,  25,  26,  27,  28,  28,  29,  30,  31,  32,  32,  34,  75, 195,
	213, 240, 251, 148,  39,  40,  92, 171, 203, 206, 218, 233, 244, 251, 253, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 251, 244, 238, 210,  99,  60,  84,  70,  21,  20,  22,  36,
	 49,  51,  50,  37,  28,  29,  28,  28,  28,  28,  27,  27,  27,  27,  27,  26,  26,
	 26,  26,  25,  24,  21,  24,  24,  24,  23,  21,  16, 153, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	110,  17,  25, 152, 212, 229, 235, 147, 100, 103,  88,  40,  26,  25,  26,  26,  27,
	 28,  29,  30,  31,  31,  32,  33,  32, 102, 187, 209, 232, 255, 192,  68,  42,  46,
	122, 183, 205, 206, 214, 227, 239, 246, 251, 253, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 251, 246, 239, 228, 215, 207,
	210, 176,  65,  58,  45,  27,  20,  19,  21,  34,  47,  49,  48,  34,  26,  27,  26,
	 26,  26,  26,  25,  25,  25,  25,  24,  24,  24,  24,  22,  21,  41,  22,  21,  21,
	 19,  16,  93, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 233,  47,  17,  42, 166, 200, 200, 190,
	116, 107, 107,  91,  41,  27,  25,  26,  27,  28,  28,  29,  30,  31,  32,  32,  33,
	 35,  84, 174, 204, 222, 249, 234, 135,  47,  43,  54, 121, 182, 207, 206, 207, 216,
	227, 235, 241, 246, 248, 251, 253, 253, 253, 255, 255, 255, 253, 253, 253, 251, 249,
	246, 242, 235, 228, 217, 208, 207, 208, 187, 123,  54,  37,  46,  54,  42,  26,  20,
	 19,  21,  33,  45,  46,  44,  31,  24,  25,  24,  24,  24,  24,  23,  23,  23,  22,
	 22,  22,  21,  19,  48,  83,  60,  16,  15,  13,  33, 223, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 180,  16,  18,  51, 165, 164, 123, 104, 107, 107, 110,  95,  44,  27,  26,
	 27,  27,  28,  29,  30,  30,  31,  32,  33,  34,  36,  63, 154, 198, 210, 232, 252,
	219, 129,  49,  43,  48, 104, 155, 180, 191, 197, 202, 209, 216, 221, 226, 230, 234,
	235, 237, 239, 237, 235, 234, 231, 227, 221, 216, 209, 202, 197, 192, 182, 158, 106,
	 47,  38,  47, 128, 203,  93,  52,  41,  27,  22,  21,  23,  32,  42,  43,  41,  29,
	 22,  23,  23,  22,  22,  22,  21,  21,  20,  20,  20,  17,  52,  86,  91,  65,  11,
	 12,  12, 167, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 122,  18,  18,  42,  84,
	110, 109, 109, 110, 110, 113, 101,  51,  28,  26,  27,  28,  29,  29,  30,  31,  32,
	 32,  33,  34,  36,  44, 122, 184, 201, 214, 234, 249, 222, 155,  80,  42,  43,  62,
	101, 137, 166, 192, 207, 208, 206, 205, 204, 205, 205, 205, 205, 205, 204, 205, 206,
	208, 207, 193, 167, 139, 105,  62,  39,  37,  79, 157, 224, 249, 238, 198,  81,  49,
	 39,  28,  26,  25,  25,  31,  39,  40,  38,  27,  20,  20,  20,  20,  20,  19,  19,
	 19,  18,  15,  11,  35,  92,  75,  31,  13,  13, 105, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 245,  69,  18,  17,  34,  97, 110, 112, 113, 113, 113, 116, 110,
	 61,  29,  27,  27,  28,  29,  30,  31,  31,  32,  33,  33,  34,  35,  36,  77, 149,
	189, 199, 210, 227, 245, 239, 188, 116,  40,  44,  42,  52,  78, 110, 138, 160, 175,
	183, 185, 191, 190, 191, 184, 182, 174, 159, 139, 110,  79,  51,  38,  40,  37, 117,
	192, 239, 244, 225, 209, 199, 192, 139,  43,  41,  36,  28,  27,  27,  26,  29,  36,
	 37,  35,  25,  18,  18,  18,  17,  15,  13,  11,  15,  30,  56,  82,  94,  30,  11,
	 13,  55, 237, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 218,  37,  18,
	 15,  46, 107, 113, 116, 116, 117, 117, 118, 117,  75,  34,  27,  28,  29,  29,  30,
	 31,  31,  32,  33,  33,  34,  35,  37,  37,  90, 148, 183, 195, 202, 209, 213, 135,
	 41,  41,  41,  43,  50,  49,  44,  44,  42,  45,  49,  54,  58,  55,  48,  45,  40,
	 42,  42,  46,  49,  39,  38,  37,  37, 134, 213, 208, 201, 195, 181, 143,  82,  35,
	 30,  30,  32,  30,  28,  28,  26,  25,  28,  33,  32,  31,  20,  11,  12,  18,  30,
	 46,  66,  90, 108, 113, 109,  47,  11,  14,  27, 208, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 185,  19,  18,  17,  61, 114, 116, 118, 118, 118,
	119, 120, 123,  95,  44,  27,  28,  29,  30,  30,  31,  32,  32,  33,  34,  34,  35,
	 36,  37,  37,  72, 124, 163, 186, 169,  54,  40,  40,  40,  42, 131, 219, 189, 152,
	122,  96,  77,  64,  57,  61,  75,  90, 117, 148, 186, 220, 132,  39,  37,  37,  37,
	 52, 168, 185, 160, 118,  65,  30,  32,  31,  30,  29,  28,  28,  27,  27,  26,  26,
	 25,  25,  25,  24,  36,  72,  91, 101, 113, 118, 118, 118, 114, 111,  61,  11,  14,
	 14, 169, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	157,  18,  18,  18,  74, 119, 120, 121, 122, 121, 122, 123, 126, 112,  62,  29,  27,
	 29,  30,  30,  31,  32,  32,  33,  34,  34,  35,  35,  36,  38,  38,  47,  82,  78,
	 40,  39,  40,  40,  41, 123, 223, 231, 240, 244, 241, 236, 234, 234, 235, 237, 241,
	245, 242, 232, 225, 124,  39,  37,  36,  36,  36,  75,  77,  41,  34,  33,  32,  31,
	 30,  29,  29,  28,  27,  27,  26,  25,  24,  23,  22,  23,  53, 106, 125, 122, 121,
	119, 118, 118, 117, 116,  77,  15,  13,  14, 139, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 129,  19,  18,  21,  83, 122, 123,
	124, 124, 125, 125, 125, 128, 126,  89,  41,  27,  28,  30,  31,  31,  32,  32,  33,
	 34,  34,  35,  36,  35,  36,  38,  39,  38,  38,  38,  39,  39,  41, 113, 195, 195,
	198, 201, 206, 209, 212, 213, 212, 210, 207, 202, 198, 195, 197, 114,  38,  36,  36,
	 35,  34,  35,  35,  33,  32,  31,  31,  30,  30,  29,  28,  28,  27,  26,  25,  25,
	 23,  21,  34,  81, 122, 125, 123, 122, 122, 121, 121, 120, 120,  85,  19,  13,  14,
	113, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 115,  20,  18,  25,  91, 126, 126, 127, 127, 127, 128, 129, 129, 133,
	115,  67,  32,  27,  30,  31,  31,  32,  33,  33,  34,  34,  35,  35,  36,  36,  37,
	 37,  37,  37,  38,  38,  39,  56,  95, 124, 146, 162, 172, 179, 184, 188, 186, 181,
	174, 165, 149, 126,  97,  54,  36,  36,  35,  34,  34,  33,  33,  32,  32,  31,  30,
	 30,  29,  28,  28,  27,  26,  26,  25,  22,  25,  60, 109, 130, 127, 125, 126, 124,
	123, 125, 123, 123,  93,  22,  13,  15,  99, 252, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 106,  20,  18,  26,
	 94, 130, 129, 130, 130, 130, 132, 132, 132, 134, 134, 104,  54,  29,  28,  31,  31,
	 32,  33,  33,  33,  34,  35,  35,  36,  36,  36,  36,  37,  37,  38,  38,  39,  40,
	 39,  38,  45,  60,  69,  76,  81,  80,  71,  63,  47,  36,  37,  37,  36,  35,  35,
	 34,  34,  34,  33,  32,  32,  31,  30,  30,  29,  29,  28,  27,  27,  26,  23,  23,
	 46,  97, 130, 132, 128, 129, 129, 127, 128, 127, 127, 128,  95,  23,  13,  15,  90,
	251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 252, 108,  20,  18,  25,  91, 132, 133, 137, 165, 181, 135,
	135, 134, 134, 138, 132,  97,  49,  28,  28,  31,  32,  32,  33,  34,  34,  34,  35,
	 35,  35,  36,  36,  36,  36,  37,  37,  38,  38,  38,  38,  39,  39,  39,  39,  38,
	 38,  37,  36,  36,  36,  36,  35,  34,  34,  33,  33,  32,  32,  31,  31,  30,  30,
	 29,  29,  28,  27,  27,  24,  23,  42,  88, 128, 137, 132, 131, 133, 133, 176, 165,
	135, 130, 130,  93,  23,  13,  15,  95, 247, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 115,
	 20,  18,  23,  84, 136, 133, 215, 255, 188, 134, 140, 137, 137, 138, 141, 132,  95,
	 52,  29,  28,  30,  32,  33,  34,  34,  34,  35,  35,  36,  36,  36,  36,  36,  36,
	 36,  37,  37,  37,  37,  37,  37,  36,  36,  36,  36,  36,  35,  35,  35,  34,  34,
	 33,  33,  32,  32,  32,  31,  30,  30,  29,  29,  28,  27,  23,  24,  45,  89, 128,
	140, 136, 135, 134, 136, 132, 185, 255, 215, 130, 135,  87,  19,  14,  16, 102, 251,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 134,  20,  19,  20,  78, 150, 214, 231,
	255, 203, 140, 145, 140, 140, 140, 142, 145, 137, 105,  63,  34,  27,  29,  32,  33,
	 33,  34,  34,  34,  35,  35,  36,  36,  36,  36,  36,  36,  36,  36,  36,  36,  36,
	 36,  36,  35,  35,  34,  34,  34,  33,  33,  33,  32,  32,  31,  31,  30,  30,  29,
	 28,  26,  23,  30,  56,  98, 133, 142, 139, 137, 137, 137, 142, 135, 203, 255, 232,
	215, 149,  80,  17,  14,  16, 121, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 161,  24,  21,  20,  65, 170, 206, 225, 253, 225, 156, 146, 145, 142, 143,
	142, 144, 147, 144, 122,  85,  50,  31,  27,  30,  32,  33,  34,  34,  34,  34,  35,
	 35,  35,  35,  35,  36,  36,  35,  35,  35,  34,  35,  34,  34,  34,  34,  33,  33,
	 32,  32,  31,  31,  31,  30,  29,  27,  24,  27,  45,  79, 118, 141, 145, 142, 140,
	140, 140, 142, 144, 153, 222, 254, 226, 207, 173,  69,  16,  15,  14, 149, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 191,  44,  21,  22,  50,
	159, 202, 217, 244, 242, 187, 143, 151, 147, 146, 146, 146, 147, 148, 150, 142, 116,
	 84,  56,  37,  29,  29,  31,  33,  33,  34,  34,  34,  34,  34,  35,  35,  34,  34,
	 34,  34,  34,  34,  34,  33,  32,  32,  32,  31,  31,  30,  28,  25,  25,  33,  52,
	 79, 112, 139, 148, 147, 144, 143, 143, 143, 144, 147, 140, 185, 241, 245, 218, 203,
	162,  52,  18,  16,  33, 180, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 223,  80,  22,  22,  36, 136, 197, 208, 229, 252, 225, 175,
	144, 154, 150, 148, 148, 148, 148, 149, 153, 154, 147, 132, 107,  76,  52,  37,  32,
	 28,  28,  30,  31,  32,  32,  33,  33,  33,  33,  32,  32,  31,  31,  30,  29,  28,
	 27,  26,  29,  35,  49,  73, 103, 128, 144, 152, 151, 147, 146, 146, 146, 146, 149,
	151, 143, 178, 227, 252, 230, 208, 198, 140,  36,  18,  17,  68, 216, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 251, 137,
	 24,  23,  22, 101, 182, 202, 212, 235, 250, 225, 186, 154, 151, 156, 153, 150, 150,
	150, 150, 150, 152, 155, 155, 147, 135, 118, 100,  84,  68,  54,  47,  42,  39,  39,
	 35,  33,  35,  38,  38,  41,  46,  52,  66,  81,  97, 116, 133, 145, 153, 154, 152,
	150, 150, 149, 148, 149, 151, 154, 148, 153, 185, 227, 250, 235, 213, 201, 184, 104,
	 20,  18,  16, 129, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 194,  62,  23,  23,  57, 150, 194, 201,
	212, 231, 248, 234, 206, 169, 147, 153, 152, 153, 153, 153, 153, 153, 154, 155, 156,
	157, 159, 158, 155, 153, 147, 141, 136, 135, 131, 129, 131, 135, 136, 140, 147, 152,
	154, 157, 158, 157, 155, 153, 153, 152, 152, 151, 151, 150, 151, 151, 145, 167, 205,
	234, 247, 230, 210, 201, 194, 151,  59,  19,  19,  50, 185, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 243, 131,  26,  23,  24,  84, 155, 191, 199, 205, 218, 228, 220, 155, 157,
	154, 154, 153, 154, 154, 155, 155, 155, 155, 155, 155, 156, 156, 156, 157, 157, 158,
	158, 159, 159, 158, 158, 157, 157, 156, 155, 155, 154, 154, 154, 154, 153, 154, 154,
	154, 153, 154, 153, 152, 157, 155, 220, 228, 217, 204, 198, 189, 154,  83,  20,  19,
	 19, 121, 239, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 206,  85,  22,  24,
	 27,  81, 142, 179, 192, 196, 171, 149, 158, 156, 156, 156, 156, 156, 156, 156, 156,
	156, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 157, 156, 156,
	156, 156, 156, 156, 156, 156, 156, 155, 155, 154, 155, 155, 155, 155, 157, 147, 170,
	196, 191, 177, 140,  75,  23,  20,  18,  75, 199, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 178,  63,  23,  24,  27,  63, 121, 161, 154, 164,
	158, 157, 157, 158, 158, 159, 159, 159, 159, 158, 159, 159, 163, 165, 163, 161, 160,
	159, 159, 159, 159, 159, 159, 159, 159, 161, 163, 165, 163, 158, 157, 157, 158, 158,
	157, 157, 157, 156, 156, 156, 156, 163, 153, 162, 120,  61,  25,  20,  19,  54, 167,
	253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	247, 164,  61,  23,  23,  31,  57, 111, 152, 164, 163, 160, 159, 159, 160, 160, 160,
	160, 160, 160, 163, 163, 176, 168, 155, 159, 162, 164, 166, 167, 167, 165, 162, 160,
	155, 167, 177, 172, 163, 160, 160, 160, 159, 158, 159, 159, 159, 159, 162, 164, 152,
	112,  58,  28,  19,  20,  52, 152, 243, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 245, 169,  73,  22,  21,  21,
	 42,  89, 134, 160, 167, 166, 163, 162, 162, 162, 162, 162, 169, 172, 245, 236, 226,
	223, 200, 193, 197, 201, 199, 193, 199, 220, 227, 234, 247, 188, 169, 162, 162, 160,
	160, 161, 162, 164, 165, 159, 134,  91,  43,  18,  17,  19,  65, 163, 242, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 253, 192, 105,  37,  22,  19,  26,  54,  95, 129, 155, 166,
	168, 166, 165, 164, 170, 160, 207, 209, 219, 226, 234, 240, 243, 245, 244, 241, 236,
	228, 220, 210, 209, 175, 170, 163, 163, 165, 167, 166, 154, 129,  96,  54,  24,  16,
	 18,  30,  98, 186, 251, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	227, 156,  81,  30,  21,  19,  25,  41,  70, 105, 138, 158, 166, 175, 160, 174, 184,
	191, 197, 198, 199, 200, 200, 199, 199, 198, 198, 193, 186, 175, 172, 176, 166, 157,
	138, 106,  71,  40,  23,  17,  18,  25,  74, 149, 223, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 217, 156, 100,  51,  22,
	 19,  20,  27,  43,  62,  88, 106, 118, 133, 145, 160, 160, 168, 179, 187, 181, 171,
	161, 158, 148, 133, 119, 106,  90,  63,  42,  26,  18,  17,  18,  47,  94, 151, 214,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 242, 204, 152,  94,  53,  26,  20,  19,  20,  26,
	 32,  41,  49,  50,  49,  50,  57,  51,  48,  48,  47,  41,  32,  25,  19,  17,  18,
	 24,  50,  91, 147, 200, 241, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 242, 220, 186, 153, 126,  95,  70,  56,  46,  37,  37,  29,  24,  28,
	 36,  36,  44,  55,  68,  92, 121, 151, 182, 218, 241, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 247, 236, 227, 229, 220, 216, 219, 227, 227, 235, 244, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

QTMovieWindowH qtLogoWin = NULL;

QTMovieWindowH ShowQTilsLogo()
{ QTMovieSinks *qms;
  ErrCode err;
	if( QTMovieWindowH_isOpen(qtLogoWin) ){
		return qtLogoWin;
	}
	else{
		DisposeQTMovieWindow(qtLogoWin);
		qtLogoWin = NULL;
	}
	if( !qtLogoWin && (qms = open_QTMovieSink( NULL, NULL,
						   QTILS_LOGO_WIDTH, QTILS_LOGO_HEIGHT, TRUE, 1,
						   _QTCompressionCodec_.Raw, _QTCompressionQuality_.Normal,
						   FALSE, FALSE, &err ))
	){
	  int i, frames;
		for( frames = 0 ; frames < qms->frameBuffers ; frames++ ){
			for( i= 0; i < QTILS_LOGO_WIDTH * QTILS_LOGO_HEIGHT; i++ ){
				qms->imageFrame[frames][i].ciChannel.red = QTils_Logo[i];
				qms->imageFrame[frames][i].ciChannel.green = QTils_Logo[i];
				qms->imageFrame[frames][i].ciChannel.blue = QTils_Logo[i];
				qms->imageFrame[frames][i].ciChannel.alpha = 0xFF;
			}
		}
		QTMovieSink_AddFrame( qms, 1.0 );
		qms->theURL = QTils_strdup("QTils Logo");
		qtLogoWin = OpenQTMovieWindowWithQTMovieSink( qms, FALSE, FALSE );
	}
	return qtLogoWin;
}
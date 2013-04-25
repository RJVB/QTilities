/*!
 *  @file QTMovieWin.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101122.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the mostly platform-independent routines.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTMovieWin: QuickTime utilities: a simple toolkit for opening/controlling QuickTime windows");

#include "Logging.h"
#include "timing.h"

#define _QTILS_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>

#if (defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32)
#	include <Windows.h>

#	include <QTML.h>
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
#else
#	include <QuickTime/QuickTime.h>
#endif // TARGET_OS_WIN32


#include "QTilities.h"
#include "Lists.h"
#include "QTMovieWin.h"

int UseQTMLDoubleBuffering = 0;

/*!
	function interface to the QTMovieWindowH_Check macro
 */
int _QTMovieWindowH_Check_( QTMovieWindowH wih )
{
	return QTMovieWindowH_Check(wih);
}

/*!
	a QTMovieWindow handle can either be allocated through a specific QuickTime function,
	or via a very simple, generic mechanism.
 */
QTMovieWindowH NewQTMovieWindowH()
{ QTMovieWindowH wih;
  QTMovieWindows *wi;
#ifdef USE_QTHANDLES
	// Use QuickTime's own handles creation routines. This is legacy code on Mac OS X.
	if( (wih = (QTMovieWindowH) NewHandleClear( sizeof(QTMovieWindows) )) ){
		wi = *wih;
		wi->self = wi;
	}
#else
	// a simple approach to create a handle: a pointer to a member of the structure that
	// points to the structure itself (self).
	wi = (QTMovieWindows*) QTils_calloc(1, sizeof(QTMovieWindows));
	if( wi ){
		wi->self = wi;
		wih = &wi->self;
	}
	else{
		wih = NULL;
	}
#endif
	return wih;
}

QTMovieWindowH InitQTMovieWindowHFromMovie( QTMovieWindowH wih, const char *theURL, Movie theMovie,
								  Handle dataRef, OSType dataRefType, DataHandler dh, short resId, ErrCode *err )
{ QTMovieWindows *wi;
  extern int QTWMcounter;
	if( wih && *wih && (*wih)->self == *wih && theMovie ){
		wi = *wih;

		GetMovieBox( theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		GetMovieNaturalBoundsRect( theMovie, &wi->theInfo.naturalBounds );
		MacOffsetRect( &wi->theInfo.naturalBounds, -wi->theInfo.naturalBounds.left, -wi->theInfo.naturalBounds.top );
		wi->theMovie = theMovie;
		wi->dataRef = dataRef;
		wi->dataRefType = dataRefType;
		wi->dataHandler = dh;
		wi->resId = resId;
		wi->info = &wi->theInfo;
		QTWMcounter += 1;
		wi->idx = QTWMcounter;
		// intialise a number of useful tidbits about the movie: its duration, playback frequency,
		// timeScale, etc.
		wi->theInfo.timeScale = GetMovieTimeScale(wi->theMovie);
		wi->theInfo.duration = ((double)GetMovieDuration(wi->theMovie)) / ((double) wi->theInfo.timeScale);
		wi->theTCTrack = GetMovieIndTrackType( wi->theMovie, 1, TimeCodeMediaType,
									   movieTrackMediaType|movieTrackEnabledOnly
		);
		{ TimeRecord tr;
		  HandlerError tcerr;
			if( wi->theTCTrack ){
				wi->theTCMediaH = GetMediaHandler( GetTrackMedia(wi->theTCTrack) );
				if( wi->theTCMediaH && GetMoviesError() == noErr ){
					tcerr = TCGetTimeCodeAtTime( wi->theTCMediaH, (TimeValue) 0, NULL, &wi->tcdef, NULL, NULL );
				}
				else{
					tcerr = 1;
				}
				if( tcerr != noErr && (wi->tcdef.flags & tcCounter) ){
					wi->theTCTrack = nil;
				}
			}
			GetMovieStartTime( wi->theMovie, wi->theTCTrack, &tr, &(wi->info->startFrameNr) );
			wi->theInfo.startTime = ((double) *((SInt64*)&tr.value))/((double)tr.scale);
		}
		{ long i = 1;
		  Track tTrack;
		  char *trackName;
		  ErrCode err;
			wi->theTimeStampTrack = NULL;
			while( (tTrack = GetMovieIndTrackType( wi->theMovie, i, TextMediaType, movieTrackMediaType ))
				 && !wi->theTimeStampTrack
			){
				trackName = NULL;
//				err = GetMetaDataStringFromTrack( wi->theMovie, tTrack, akDisplayName, &trackName, NULL );
				err = GetTrackName( wi->theMovie, tTrack, &trackName );
				if( trackName ){
#ifdef DEBUG
					Log( qtLogPtr, "Movie '%s' track #%ld \"%s\"\n", theURL, i, trackName );
#endif
					if( strcmp( trackName, "timeStamp Track" ) == 0 ){
						wi->theTimeStampTrack = tTrack;
					}
					QTils_free(&trackName);
				}
				i += 1;
			}
		}
#ifdef DEBUG
		{ Track tcTrack;
		  long idx=-1;
			if( GetTrackWithName( wi->theMovie, "Timecode Track", 0, 0, &tcTrack, &idx ) == noErr ){
				Log( qtLogPtr, "Movie '%s' has Timecode Track #%d\n", theURL, idx );
			}
		}
#endif
		wi->theInfo.TCframeRate = -1;
		if( GetMovieStaticFrameRate( wi->theMovie, NULL, &wi->theInfo.TCframeRate, &wi->theInfo.frameRate ) ){
			wi->theInfo.frameRate = 1.0e308; // Inf
		}
		if( wi->theInfo.TCframeRate < 0 ){
			wi->theInfo.TCframeRate = wi->theInfo.frameRate;
		}
		if( theURL ){
			if( wi->theURL ){
				QTils_free(&wi->theURL);
			}
			wi->theURL = (const char*) QTils_strdup(theURL);
		}
		register_QTMovieWindowH_for_Movie( wi->theMovie, wih );
		// set theChapterTrack to NULL to be sure GetMovieChapterTrack won't return its value
		// We determine this *after* registering the wih as it allows GetMovieChapterTrack to
		// do part of our initialising work - setting wi->theChapterRefTrack.
		wi->theChapterTrack = NULL;
		wi->theChapterMedia = NULL;
		wi->theChapterTrack = GetMovieChapterTrack(wi->theMovie, NULL);
		// create a list into which to store movie controller action handlers:
		if( !wi->MCActionList ){
			wi->MCActionList = (void*) init_MCActionList();
		}
#if TARGET_OS_MAC
		if( !wi->NSMCActionList ){
			wi->NSMCActionList = (void*) init_NSMCActionList();
		}
#endif
		wi->loadState = GetMovieLoadState(wi->theMovie);
		*err = noErr;
	}
	else{
		*err = paramErr;
	}
	return wih;
}

/*!
	Dispose of a QTMovieWindow object completely, including of the handle.
	if the window is still open, it is closed first.
	It would not be wise to do this while events are being processed, so there are
	some protections against "going too far" should we be called from inside an event handler.
 */
void DisposeQTMovieWindow( QTMovieWindowH WI )
{
	if( WI && *WI && (*WI)->self == *WI && QTMovieWindowH_from_Movie((*WI)->theMovie) ){
	  QTMovieWindows *wi = *WI;
#ifndef QTMOVIESINK
		CloseQTMovieWindow(WI);
#endif //QTMOVIESINK
#if TARGET_OS_MAC
		DrainQTMovieWindowPool(WI);
		if( wi->handlingEvent || wi->performingClose ){
			return;
		}
#endif
		wi->self = NULL;
#ifdef USE_QTHANDLES
		DisposeHandle( (Handle) WI );
#else
		QTils_free(&wi);
#endif
	}
}

void DisposeQTMovieWindow_Mod2( QTMovieWindowH *WI )
{
	if( WI ){
		DisposeQTMovieWindow( *WI );
		*WI = NULL;
	}
}

#ifndef QTMOVIESINK

/*!
	the local table with known QuickTime actions for which we provide callbacks
 */
const MCActions _MCAction_ = {
	mcActionStep, mcActionPlay, mcActionActivate,
	mcActionDeactivate, mcActionGoToTime, mcActionMouseDown,
	mcActionMovieClick, mcActionKeyUp, mcActionSuspend, mcActionResume,
	mcActionMovieLoadStateChanged, mcActionMovieFinished, mcActionIdle,
	-1, -2, -3, -4
};

const MCActions *MCAction()
{
	return &_MCAction_;
}

//extern ErrCode OpenMovieFromURL( Movie *newMovie, short flags, short *id, const char *URL,
//					    Handle *dataRef, OSType *dataRefType );


#pragma mark ---- Movie Controller action handler ----

#if defined(__APPLE_CC__) || defined(_SS_LOG_ACTIVE)
#	if defined(_WINDOWS_)
#		define PROCANDTHREAD	GetCurrentProcessId(), GetCurrentThreadId()
#	else
#		define PROCANDTHREAD	getpid(), pthread_self()
#	endif
#	define mcFeedBack(mc,wi,actionStr)	if((wi)->theMovie){ TimeScale scale;	\
  TimeValue time;	\
  TimeRecord trec;	\
  const char *movieName = wi->theURL;	\
  char keystr[8]; \
	if( action != wi->lastAction || (at - wi->lastActionTime > 0.01) ){ \
		wi->lastAction = action; wi->lastActionTime = at; \
		time = MCGetCurrentTime( mc, &scale );	\
		GetMovieTime(theMovie, &trec );	\
		if( action == mcActionKeyUp ){	\
			strcpy( keystr, " key=X" );	\
			keystr[5]= (((EventRecord*) params)->message & charCodeMask);	\
		}	\
		else{	\
			keystr[0] = '\0';	\
		}	\
		Log( qtLogPtr, "%s: %s %g/%g of %g sec; dt=%g rate=%g fun=%p par=%p%s s,s=%d,%d process %u:%u\n", actionStr	\
			, (movieName && *movieName)? movieName : "??"	\
			, ((double)time) / ((double) scale), ((double) *((SInt64*)&trec.value))/((double)trec.scale)	\
			, wi->theInfo.duration, (double)(at - lAT), rate/65535.0, fun, params, keystr,	\
			wi->wasStepped, wi->wasScanned,	\
			PROCANDTHREAD \
		);	\
	}	\
}	\
else{	\
	Log( qtLogPtr, "%s : %s closed during action processing?!\n", actionStr	\
	    , (wi && wi->theURL && *wi->theURL)? wi->theURL : "??"	\
	);	\
}
#else
#	define mcFeedBack(mc,wi,actionStr )	/**/
#endif

#ifdef DEBUG
#	include <stdio.h>
#endif

/*!
	receives QuickTime "actions" when they are about to occur, and dispatches them to
	the user-specified callbacks
 */
Boolean QTMovieWindow_MCActionHandler( MovieController mc, short action, void *params, long refCon)
{ QTMovieWindowH wih = (QTMovieWindowH) refCon;
  QTMovieWindowPtr wi;
  Movie theMovie;
  static Boolean localEvents = FALSE;
  Boolean ret = FALSE;
	// NB: on MSVisualC, one has to store the MCActionCallback value into a variable of type
	// Boolean (*fun)(QTMovieWindowH)
	// before calling, for some obscure reason!
  int (*fun)(QTMovieWindowH wi, void *params) = NULL;
#if TARGET_OS_MAC
  void *target;
  void *selector;
  NSMCActionCallback nsfun = NULL;
#endif
  Fixed rate;
  float at = (float) HRTime_Time(), lAT;
  short lA, willPlay;

	if( !wih || !(wi = *wih) || wi->self != wi ){
		return ret;
	}

	theMovie = wi->theMovie;
	// the lastAction states are updated once in mcfeedback() to ensure that we do not end up there
	// (recursively). Therefore, we may need to cache the current values.
	lA = wi->lastAction; lAT = wi->lastActionTime;
	rate = GetMovieRate(theMovie);

	willPlay = (rate != 0);
	if( willPlay != wi->isPlaying ){
	  short _action;
		_action = (willPlay)? _MCAction_.Start : _MCAction_.Stop;
#if TARGET_OS_MAC
		if( (nsfun = get_NSMCAction( wih, _action, &target, &selector )) ){
			ret = (*nsfun)( target, selector, wih, NULL );
			QTWMflush();
		} else
#endif
		if( (fun = get_MCAction( wih, _action )) ){
			ret = (Boolean) (*fun)( wih, NULL );
			QTWMflush();
		}
	}
	wi->isPlaying = willPlay;

	// step/scan handling: user needs to be able to catch a finished action, which is the next following
	// Play event!
	// a Step event is (usually?) followed by a Play and then a GotoTime event
#ifdef DEBUG
	if( (wi->wasStepped > 0 || wi->wasScanned > 0 || !wi->isPlaying) && action != mcActionIdle ){
//		Log( qtLogPtr, "Stepped:%d Scanned:%d action=%d\n", wi->wasStepped, wi->wasScanned, action );
//		fprintf( stderr, "Stepped:%d Scanned:%d action=%d\n", wi->wasStepped, wi->wasScanned, action );
	}
#endif
	switch( action ){
		case mcActionControllerSizeChanged:
		case mcActionActivate:
		case mcActionDeactivate:
		case mcActionSetPlaySelection:
		case mcActionMouseDown:
		case mcActionMovieClick:
		case mcActionKey:
		case mcActionKeyUp:
		case mcActionSuspend:
		case mcActionResume:
		case mcActionMovieFinished:
			wi->wasStepped = FALSE;
			wi->wasScanned = FALSE;
			break;
	}
	if( wi->hasAnyMCAction ){
#if TARGET_OS_MAC
		if( (nsfun = get_NSMCAction( wih, _MCAction_.AnyAction, &target, &selector )) ){
			(*nsfun)( target, selector, wih, &action );
			QTWMflush();
		}
		else
#endif
		if( (fun = get_MCAction( wih, _MCAction_.AnyAction )) ){
			(Boolean) (*fun)( wih, &action );
		}
	}
	switch( action ){
		case mcActionMovieLoadStateChanged:
			// params==kMovieLoadStateComplete when streaming video has been received completely
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			wi->loadState = (long) params;
			mcFeedBack( mc, wi, "LoadStateChanged" );
			break;
		case mcActionControllerSizeChanged:{
		  Rect bounds;
		  WindowPtr w;
		  char msg[512];
			MCGetControllerBoundsRect(mc, &bounds);
#if TARGET_OS_WIN32
			w = GetNativeWindowPort(wi->theView);
			SizeWindow((WindowPtr)w, bounds.right, bounds.bottom, TRUE);
#endif
			GetMovieBox( wi->theMovie, &wi->theMovieRect );
			// call MacOffsetRect to translate the Rect to (0,0) such that (right,bottom)
			// become equal to (width,height):
			MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
#if TARGET_OS_MAC
//			{ extern void QTMovieWindowSetMovieSize( QTMovieWindowH wih, short width, short height, const char *caller );
// on Mac OS X, calling [movieView setControllerVisible:YES] will cause TWO mcActionControllerSizeChanged events, the first
// with the current MovieBox, the second with a MovieBox adapted to accomodate the controller height INSIDE the current height.
// Setting MovieSize to the dimensions from the MovieBox will thus cause the movie view to DIMINISH in height!
//				QTMovieWindowSetMovieSize( wih, wi->theMovieRect.right, wi->theMovieRect.bottom, "mcActionControllerSizeChanged" );
//			}
#endif
			snprintf( msg, sizeof(msg), "ControllerSizeChanged: %hdx%hd+%hd+%hd movieRect=%hdx%hd+%hd+%hd",
				bounds.right, bounds.bottom, bounds.left, bounds.top,
				wi->theMovieRect.right, wi->theMovieRect.bottom, wi->theMovieRect.left, wi->theMovieRect.top
			);
			mcFeedBack( mc, wi, msg );
			break;
		}
		case mcActionIdle:
			if( localEvents /* || wi->wasStepped > 0 || wi->wasScanned > 0 */ ){
				mcFeedBack( mc, wi, "Idle" );
			}
			// Call the Step callback if a step was pending and the movie time changed:
			if( wi->stepPending ){
			  Boolean ok = FALSE;
#if TARGET_OS_MAC
				if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
					ok = TRUE;
				} else
#endif
				if( (fun = get_MCAction( wih, action )) ){
					ok = TRUE;
				}
				if( ok ){
				  TimeRecord trec;
					GetMovieTime( wi->theMovie, &trec );
					if( wi->stepStartTime != *((SInt64*)&trec.value) ){
						wi->stepPending = FALSE;
						if( !localEvents ){
							mcFeedBack( mc, wi, "Idle->Step" );
						}
						// we discard the return value here!
#if TARGET_OS_MAC
						if( nsfun ){
							(*nsfun)( target, selector, wih, NULL );
						} else
#endif
						(*fun)( wih, NULL );
						QTWMflush();
					}
				}
			}
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
			}
			break;
		case mcActionStep:
			// the action sent when the user 'steps' the movie with the cursor keys. The action
			// is received *before* the step is executed!
			// NB: we know in which direction the movie is being stepped:
			// params==1 -> forward; params==-1 -> backward
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			if( !ret ){
				wi->wasStepped = TRUE;
				if( fun ){
				  TimeRecord trec;
					// remember we're starting a step here
					wi->stepPending = TRUE;
					GetMovieTime( wi->theMovie, &trec );
					wi->stepStartTime = *((SInt64*)&trec.value);
				}
			}
			mcFeedBack( mc, wi, "Step" );
			break;
		case mcActionGetPlayRate:
			Log( qtLogPtr, "GetPlayRate\n" );
			break;
		case mcActionPlay:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			if( wi->wasStepped > 0 ){
				// this is to prevent the upcoming GoToTime action to be taken for a user-scan event.
				wi->wasStepped = -1;
			}
//			else{
//				wi->wasStepped = FALSE;
//			}
			wi->wasScanned = FALSE;
			mcFeedBack( mc, wi, "Play" );
			break;
		case mcActionActivate:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "Activate" );
			wi->isActive = TRUE;
			break;
		case mcActionDeactivate:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "Deactivate" );
			wi->isActive = FALSE;
			break;
		case mcActionGoToTime:{
		  TimeRecord *trec = (TimeRecord*) params;
		  double t;
			if( params ){
				t = ((double) *((SInt64*)&(trec->value)))/((double)trec->scale);
				params = &t;
			}
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			if( !ret && !wi->wasStepped ){
				wi->wasScanned = TRUE;
			}
			if( wi->wasStepped < 0 ){
				wi->wasStepped = FALSE;
			}
			mcFeedBack( mc, wi, "GoToTime" );
			break;
		}
		case mcActionSetPlaySelection:
			mcFeedBack( mc, wi, "SetPlaySelection" );
			break;
		case mcActionMouseDown:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "MouseDown" );
			break;
		case mcActionMovieClick:{
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "MovieClick" );
			break;
		}
		case mcActionKeyUp:{
		  EventRecord *evt = (EventRecord*) params;
//		  unsigned long message = evt->message;
			evt->message &= charCodeMask;
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "KeyUp" );
			break;
		}
		case mcActionSuspend:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "Suspend" );
			break;
		case mcActionResume:
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "Resume" );
			break;
		case mcActionMovieFinished:{
#if TARGET_OS_MAC
			if( (nsfun = get_NSMCAction( wih, action, &target, &selector )) ){
				ret = (*nsfun)( target, selector, wih, params );
				QTWMflush();
			} else
#endif
			if( (fun = get_MCAction( wih, action )) ){
				ret = (Boolean) (*fun)( wih, params );
				QTWMflush();
			}
			mcFeedBack( mc, wi, "MovieFinished" );
			break;
		}
		case mcActionRestartAtTime:
			mcFeedBack( mc, wi, "RestartAtTime" );
			break;
		case mcActionPauseToBuffer:
			mcFeedBack( mc, wi, "PauseToBuffer" );
			break;
		default:
			break;
	}
	return ret;
}

/*!
	add a Movie Controller to the Movie in a QTMovieWindow. This takes a number of steps as shown below.
	A movie controller is what allows a user to control movie playback. Without it, the movie can only
	be controlled from software.
 */
void CreateNewMovieController( QTMovieWindowH wih, int controllerVisible )
{ Rect	bounds;
  long 	controllerFlags;
  QTMovieWindowPtr wi = *wih;

	// 0,0 Movie coordinates
	GetMovieBox( wi->theMovie, &wi->theMovieRect );
	MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );

	if( !wi->theMC ){
		// Attach a movie controller
		if( controllerVisible ){
			wi->theMC = NewMovieController( wi->theMovie, &wi->theMovieRect,
									 mcTopLeftMovie
			);
		}
		else{
			wi->theMC = NewMovieController( wi->theMovie, &wi->theMovieRect,
									 mcTopLeftMovie|mcNotVisible
			);
		}
	}
	if( wi->theMC ){
#if TARGET_OS_WIN32
	  WindowRef nativePort = GetNativeWindowPort(wi->theView);
		MCSetControllerPort( wi->theMC, (CGrafPtr) nativePort );
		MCSetControllerAttached( wi->theMC, TRUE );
#endif

		// Disable editing
		MCEnableEditing( wi->theMC, FALSE );
#if TARGET_OS_WIN32
		MCSetVisible( wi->theMC, (controllerVisible)? TRUE : FALSE );
#endif

		if( controllerVisible ){
			// Get the controller rect
			MCGetControllerBoundsRect( wi->theMC, &bounds );
		}
		else{
			GetMovieBox( wi->theMovie, &bounds );
			MacOffsetRect( &bounds, -bounds.left, -bounds.top );
		}

		// Tell the controller to attach a movie's CLUT to the window as appropriate.
		MCDoAction( wi->theMC, mcActionGetFlags, &controllerFlags );
		MCDoAction( wi->theMC, mcActionSetFlags, (void *)(controllerFlags | mcFlagsUseWindowPalette) );

		// Allow the controller to accept keyboard events
		MCDoAction( wi->theMC, mcActionSetKeysEnabled, (void *)TRUE );

		// Set the controller action filter
		MCSetActionFilterWithRefCon( wi->theMC, QTMovieWindow_MCActionHandler, (long) wih );

#if TARGET_OS_WIN32
		{ Rect maxBounds;
			// Set the grow box amount
			GetMaxBounds( &maxBounds );
			MCDoAction( wi->theMC, mcActionSetGrowBoxBounds, &maxBounds );
			MCActivate( wi->theMC, nativePort, TRUE );
			// Size our window
			SizeWindow( (WindowPtr)nativePort, bounds.right, bounds.bottom, FALSE );
			if( controllerVisible ){
				MCSetControllerBoundsRect(wi->theMC, &bounds);
			}
		}
#endif
	}
}

/*!
	returns the current (last) error that occurred.
	This is not a 'sticky' value!
 */
ErrCode LastQTError()
{
	return (ErrCode) GetMoviesError();
}

/*!
	obtain a handle to the current window (itself a handle on MSWin).
	Probably not of much interest.
 */
NativeWindowH NativeWindowHFromQTMovieWindowH( QTMovieWindowH wi )
{
	if( QTMovieWindowH_Check(wi) ){
		(*wi)->theViewPtr = &((*wi)->theView);
		return &((*wi)->theViewPtr);
	}
	else{
		return NULL;
	}
}

ErrCode QTMovieWindowSetPlayAllFrames( QTMovieWindowH WI, int onoff, int *curState )
{ QTMovieWindowPtr wi;
  ErrCode err = paramErr;
  unsigned char AllF;
	if( QTMovieWindowH_Check(WI) ){
		wi = *WI;
		if( curState ){
		  char *allf = NULL;
		  size_t allfSize = 0;
			if( (err = GetUserDataFromMovie( wi->theMovie, (void**) &allf, &allfSize, 'AllF' )) == noErr ){
				*curState = *allf;
				QTils_free(&allf);
			}
			else{
				QTils_free(&allf);
				if( err != userDataItemNotFound ){
					goto bail;
				}
				else{
					*curState = -1;
				}
			}
		}
		AllF = (onoff)? 1 : 0;
		err = AddUserDataToMovie( wi->theMovie, &AllF, 1, 'AllF', 1 );
		{ Track theTrack;
		  long tracks = GetMovieTrackCount(wi->theMovie), idx;
		  long trackHints;
		  long ignore1, ignore2, ignore3;
			for( idx = 0 ; idx < tracks ; idx++ ){
				if( (theTrack = GetMovieIndTrack( wi->theMovie, idx )) ){
					GetTrackLoadSettings( theTrack, &ignore1, &ignore2, &ignore3, &trackHints );
					if( onoff ){
						trackHints |= hintsPlayingEveryFrame;
					}
					else{
						trackHints &= ~hintsPlayingEveryFrame;
					}
					SetTrackLoadSettings( theTrack, ignore1, ignore2, ignore3, trackHints );
					SetMediaPlayHints( GetTrackMedia(theTrack), trackHints, hintsPlayingEveryFrame );
				}
			}
		}
		UpdateMovie(wi->theMovie);
	}
bail:
	return err;
}

ErrCode QTMovieWindowSetPreferredRate( QTMovieWindowH WI, int rate, int *curRate )
{ QTMovieWindowPtr wi;
  ErrCode err = paramErr;
	if( QTMovieWindowH_Check(WI) ){
		wi = *WI;
		if( curRate ){
			*curRate = Fix2Long( GetMoviePreferredRate(wi->theMovie) );
		}
		SetMoviePreferredRate( wi->theMovie, Long2Fix(rate) );
		err = GetMoviesError();
	}
bail:
	return err;
}

/*!
	returns the current time (in seconds) for the given open movie window
	in either relative mode (since movie start) or in absolute units (wall clock time).
	Absolute time is determined using the TimeCode track, if the movie has one.
 */
ErrCode QTMovieWindowGetTime( QTMovieWindowH wih, double *t, int absolute )
{ QTMovieWindowPtr wi;
  ErrCode err;
	if( QTMovieWindowH_Check(wih) ){
	  TimeRecord trec;
		wi = *wih;
		if( absolute && (*wih)->theTCTrack ){
		  TimeCodeRecord ft;
		  long frame;
			err = (ErrCode) TCGetCurrentTimeCode( wi->theTCMediaH, &frame, &(wi->tcdef), &ft, NULL );
			if( err == noErr ){
				wi->theInfo.TCframeRate = ((double)wi->tcdef.fTimeScale)
									/ ((double)wi->tcdef.frameDuration);
				*t = FTTS(&ft.t, wi->info->TCframeRate);
			}
		}
		else{
			GetMovieTime( wi->theMovie, &trec );
			err = GetMoviesError();
			if( err == noErr ){
				*t = ((double) *((SInt64*)&trec.value))/((double)trec.scale);
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	return the movie's current time broken down in hours, minutes, secondes and frames
 */
ErrCode QTMovieWindowGetFrameTime( QTMovieWindowH wih, MovieFrameTime *ft, int absolute )
{ ErrCode err;
  double t;
	if( !ft ){
		err = paramErr;
	}
	else{
		if( absolute && (*wih)->theTCTrack ){
		  // there's a TimeCode Track of the proper format:
		  long frame;
			err = (ErrCode) TCGetCurrentTimeCode( (*wih)->theTCMediaH, &frame, &((*wih)->tcdef),
										  (TimeCodeRecord*) ft, NULL
			);
			(*wih)->theInfo.TCframeRate = ((double)(*wih)->tcdef.fTimeScale)
								/ ((double)(*wih)->tcdef.frameDuration);
		}
		else{
			err = QTMovieWindowGetTime( wih, &t, absolute );
			if( err == noErr ){
				secondsToFrameTime( t, (*wih)->theInfo.frameRate, ft );
			}
		}
	}
	return err;
}

/*!
	set the time: displaces the cursor to the specified moment.
 */
static ErrCode QTMovieWindowSetTimeChecked( QTMovieWindowH wih, double t, int absolute, TimeValue *useThisTime )
{ QTMovieWindowPtr wi;
  ErrCode err;
  TimeRecord trec;
	wi = *wih;
	if( absolute && (*wih)->theTCTrack ){
	  // there's a TimeCode Track of the proper format:
	  long frame;
	  TimeCodeRecord tcrec;
#ifdef DEBUG
		secondsToFrameTime( wi->info->duration, wi->info->TCframeRate, &tcrec.t );
		err = (ErrCode) TCTimeCodeToFrameNumber( wi->theTCMediaH, &(wi->tcdef), &tcrec, &frame );
#endif
		secondsToFrameTime( t, wi->theInfo.TCframeRate, &tcrec.t );
		err = (ErrCode) TCTimeCodeToFrameNumber( wi->theTCMediaH, &(wi->tcdef), &tcrec, &frame );
		if( err == noErr ){
#ifdef DEBUG
		  double t2;
			frame -= wi->info->startFrameNr;
			// 20110107: should we use TCframeRate or frameRate?!
			wi->theInfo.TCframeRate = ((double)wi->tcdef.fTimeScale) / ((double)wi->tcdef.frameDuration);
			t2 = ((double) frame) / wi->theInfo.TCframeRate;
			t -= wi->info->startTime;
			t = t2;
#else
			wi->theInfo.TCframeRate = ((double)wi->tcdef.fTimeScale) / ((double)wi->tcdef.frameDuration);
			t = (double)(frame - wi->info->startFrameNr) / wi->theInfo.TCframeRate;
#endif
		}
	}
	// call GetMovieTime() to initialise the trec structure
	if( useThisTime ){
		err = noErr;
	}
	else{
		GetMovieTime( wi->theMovie, &trec );
		err = GetMoviesError();
	}
	if( err == noErr ){
		// trec.value is a 'wide', a structure containing a lo and a high int32 variable.
		// set it by casting to an int64 because that's the underlying intention ...
		// NB: we could use SetMovieTimeValue here and pass the time as a 32bit TimeValue
		// using a 64bit interface might have advantages.
#if TARGET_OS_WIN32
		if( useThisTime ){
			SetMovieTimeValue( wi->theMovie, *useThisTime );
//			SetTimeBaseValue( GetMovieTimeBase(wi->theMovie), *useThisTime, wi->theInfo.timeScale );
		}
		else{
			*( (SInt64*)&(trec.value) ) = (SInt64)( t * wi->theInfo.timeScale + 0.5 );
			SetMovieTime( wi->theMovie, &trec );
		}
		err = GetMoviesError();
		UpdateMovie( wi->theMovie );
		PortChanged( (GrafPtr) GetNativeWindowPort(wi->theView) );
//		if( wi->theMC ){
//			MCMovieChanged( wi->theMC, wi->theMovie );
//			if( !MCIdle(wi->theMC) ){
//				MCDraw(wi->theMC, (WindowRef) GetNativeWindowPort(wi->theView) );
//			}
//		}
#else
		{ extern void SetQTMovieTime( struct NSQTMovieWindow *theNSQTMovieWindow, TimeRecord *trec );
		  extern void SetQTMovieTimeValue( struct NSQTMovieWindow *theNSQTMovieWindow, TimeValue tVal, TimeValue tScale );
			if( useThisTime ){
				SetQTMovieTimeValue( wi->theNSQTMovieWindow, *useThisTime, wi->theInfo.timeScale );
			}
			else{
				*( (SInt64*)&(trec.value) ) = (SInt64)( t * wi->theInfo.timeScale + 0.5 );
				SetQTMovieTime( wi->theNSQTMovieWindow, &trec );
			}
			err = GetMoviesError();
			UpdateMovie(wi->theMovie);
		}
#endif
		MoviesTask( wi->theMovie, 0L );
		QTWMflush();
	}
	return err;
}

ErrCode QTMovieWindowSetTime( QTMovieWindowH wih, double t, int absolute )
{ ErrCode err;
	if( QTMovieWindowH_Check(wih) ){
		err = QTMovieWindowSetTimeChecked( wih, t, absolute, NULL );
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode QTMovieWindowSetFrameTime( QTMovieWindowH wih, MovieFrameTime *ft, int absolute )
{ ErrCode err;
  double t;
	if( !ft || !QTMovieWindowH_Check(wih) ){
		err = paramErr;
	}
	else{
		if( absolute && (*wih)->theTCTrack ){
		  // there's a TimeCode Track of the proper format:
		  long frame;
		  TimeCodeRecord tcrec;
		  QTMovieWindowPtr wi = *wih;
			tcrec.t = *ft;
			err = (ErrCode) TCTimeCodeToFrameNumber( wi->theTCMediaH, &(wi->tcdef), &tcrec, &frame );
			if( err == noErr ){
			  double t2;
				frame -= wi->info->startFrameNr;
				wi->theInfo.TCframeRate = ((double)wi->tcdef.fTimeScale) / ((double)wi->tcdef.frameDuration);
				t2 = ((double) frame) / wi->theInfo.TCframeRate;
#ifdef DEBUG
				t = FTTS(ft, wi->info->TCframeRate) - wi->info->startTime;
#endif
				err = QTMovieWindowSetTimeChecked( wih, t2, FALSE, NULL );
			}
		}
		else{
			t = FTTS(ft, (*wih)->theInfo.frameRate);
			err = QTMovieWindowSetTimeChecked( wih, t, absolute, NULL );
		}
	}
	return err;
}

ErrCode QTMovieWindowStepNext( QTMovieWindowH wih, int steps )
{ QTMovieWindowPtr wi;
  ErrCode err;
	if( QTMovieWindowH_Check(wih) ){
	  TimeValue nextTime;
	  static OSType types[1] = {VisualMediaCharacteristic};
		wi = *wih;
		GetMovieNextInterestingTime( wi->theMovie, nextTimeStep, 1, types,
							   GetMovieTime(wi->theMovie, NULL), Long2Fix(steps), &nextTime, NULL );
		if( (err = GetMoviesError()) == noErr ){
			if( nextTime >= 0 ){
				err = QTMovieWindowSetTimeChecked( wih, 0, 0, &nextTime );
			}
			else{
				err = invalidTime;
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

#endif // !QTMOVIESINK

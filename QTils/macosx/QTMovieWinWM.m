/*!
 *  @file QTMovieWinWM.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101130.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the Mac OSX-specific routines and those that depend
 * on them directly.
 *
 */

#include "copyright.h"
IDENTIFY("QTMovieWinWM: QuickTime utilities: Mac OS X part of the toolkit");

#import <AppKit/AppKit.h>
#import <AppKit/NSApplication.h>

#define HAS_LOG_INIT
#import "PCLogController.h"
#include "../Logging.h"
#include "../../timing.h"

#define _QTILS_C
#define _QTMW_M

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#import <Cocoa/Cocoa.h>
#import <QuickTime/QuickTime.h>
#import "QTKit/QTKit.h"
#ifndef _QTKITDEFINES_H
#	define _QTKITDEFINES_H
#endif

#ifndef TARGET_OS_MAC
#	define TARGET_OS_MAC
#endif

#include "../QTilities.h"
#include "../Lists.h"
#include "../QTMovieWin.h"
#import "NSQTMovieWindow.h"


static char errbuf[512];

NSAutoreleasePool *QTilsPool = NULL;
NSMutableArray *QTMovieWindowList = NULL;
Boolean QTMWInitialised = FALSE;
NSApplication *theApp = NULL;

void *qtLogName()
{
	if( !theApp ){
		theApp = [NSApplication sharedApplication];
	}
	return theApp;
}

void *nsString(char *s)
{
	return [NSString stringWithCString:s encoding:NSUTF8StringEncoding];
}

QTMovieWindowH QTMovieWindowHFromNativeWindow( NativeWindow hWnd );

// some redefinition(s) from the Carbon interface into QTKit:

WindowRef GetNativeWindowPort( NSWindow *theWindow )
{
	return (WindowRef) [theWindow windowRef];
}

void RectFromNSRect( Rect *bounds, NSRect *r )
{
	bounds->left = (short) (r->origin.x + 0.5);
	bounds->top = (short) (r->origin.y + 0.5);
	bounds->right = (short) (r->origin.x + r->size.width + 0.5);
	bounds->bottom = (short) (r->origin.y + r->size.height + 0.5);
}

void lGetMovieBox( Movie theMovie, Rect *boxRect )
{ NSRect rect;
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSValue *sizeObj = [[[QTMovie movieWithQuickTimeMovie:theMovie disposeWhenDone:NO error:nil] movieAttributes]
				  objectForKey:QTMovieNaturalSizeAttribute];
	if( sizeObj ){
		rect.size = [sizeObj sizeValue];
	}
	rect.origin.x = rect.origin.y = 0;
	RectFromNSRect( boxRect, &rect );
	[pool drain];
}

// envelope size: [NSWindow frame] -> NSRect / [NSWindow setFrame:NSRect display:YES]
// content size: [NSWindow contentRectForFrameRect:[NSWindow frame]] -> NSRect

// needs to be defined but do nothing:
void SizeWindow( WindowRef w, short width, short height, Boolean update )
{
//  NSWindow *nswin = (NSWindow*) w;
//  NSRect contentRect = [nswin iFrame];
//	contentRect.size = NSMakeSize( (float) width, (float) height );
//	[nswin setFrame:[nswin frameRectForContentRect:contentRect] display:YES animate:NO];
}

Boolean ConvertNSEventToEventRecord( NSEvent *event, EventRecord *er )
{
	return FALSE;
}

Boolean QTils_MessagePumpIsSecondary = FALSE, QTils_MessagePumpIsInActive = FALSE;

// a minimal message pump which can be called regularly to make sure MSWindows event messages
// get handled.
unsigned int PumpMessages(int force)
{ unsigned int n = 0;
  NSEvent *event = nil;
  NSDate *until, *prehistory;
  QTMovieWindowH wi = NULL;
  static unsigned short active = 0;

	if( QTils_MessagePumpIsInActive || (QTils_MessagePumpIsSecondary && !force) ){
		return 0;
	}

	active += 1;
	if( *errbuf && qtLogPtr ){
		Log( qtLogPtr, errbuf );
		errbuf[0] = '\0';
	}
	prehistory = [NSDate distantPast];
	if( force ){
		until = [NSDate distantFuture];
	}
	else{
		until = prehistory;
	}
	do{
		event = [NSApp nextEventMatchingMask:NSAnyEventMask
					untilDate:until
					inMode:NSDefaultRunLoopMode
					dequeue:YES
		];
		if( event ){
		  NativeWindow nswin = (NativeWindow) [event window];
		  EventRecord er;
			if( nswin
//			   && ConvertNSEventToEventRecord( event, &er )
			   && (wi = QTMovieWindowHFromNativeWindow(nswin))
			){
				if( wi ){
					(*wi)->handlingEvent += 1;
//					if( (*wi)->theMC ){
//#ifdef DEBUG
//						Log( qtLogPtr, "Forwarding nswin=%@ message to wi,*wi=%p,%p QT controller #%u\n", nswin, wi, *wi, (*wi)->idx );
//#endif
//						MCIsPlayerEvent( (*wi)->theMC, &er );
//					}
				}
			}
			else{
				wi = NULL;
			}
			[NSApp sendEvent:event];
			if( wi ){
				(*wi)->handlingEvent -= 1;
			}
		}
		// we should only block the 1st time we're calling nextEventMatchingMask, otherwise
		// we might never return should the last window be closed while processing an event!
		until = prehistory;
		n += 1;
	} while( event != nil );
	// check if any windows are around that ought to -and can- be closed.
	if( active == 1 && QTMovieWindowList ){
	  NSQTMovieWindow *NSqtwmh;
	  int i = 0;
		while( i < [QTMovieWindowList count] && (NSqtwmh = [QTMovieWindowList objectAtIndex:i]) ){
			if( (wi = [NSqtwmh theQTMovieWindowH]) ){
				if( (*wi)->shouldClose && !(*wi)->performingClose ){
					NSLog( @"Handling pending close for %@\n", NSqtwmh );
					CloseQTMovieWindow(wi);
					[QTMovieWindowList removeObjectAtIndex:i];
				}
				else{
					i += 1;
				}
			}
			else{
				[QTMovieWindowList removeObjectAtIndex:i];
			}
		}
	}

	active -= 1;
	return n;
}

int QTWMcounter = -1;

//////////
//
// QTApp_HandleEvent
// Perform any application-specific event loop actions.
//
// Return true to indicate that we've completely handled the event here, false otherwise.
//
//////////

static BOOL QTApp_HandleEvent( EventRecord *theEvent )
{
#pragma unused(theEvent)

	// ***insert application-specific event handling here***

	return FALSE;			// no-op for now
}

// retrieve the QTMovieWindowH corresponding to a native window. There are several ways
// this can be done; for now the one using the GWL_USERDATA slot (intended for this use?) appears
// to work reliably enough.
QTMovieWindowH QTMovieWindowHFromNativeWindow( NativeWindow hWnd )
{ QTMovieWindowH gwi = QTMovieWindowH_from_NativeWindow(hWnd);
	if( !QTMovieWindowH_Check(gwi) || (*gwi)->theView != hWnd )
	{
		gwi = NULL;
#ifdef DEBUG
//		Log( qtLogPtr, "QTMovieWindowHFromNativeWindow(%@): not a QTMovieWindow?!\n", hWnd );
#endif
	}
	return gwi;
}

void QTWMflush()
{
#ifdef AllowQTMLDoubleBuffering
//	QTMLFlushDirtyPorts();
#endif // AllowQTMLDoubleBuffering
}

// Close the window and movie of a QTMovieWindow, deallocating all depending objects
// the handle itself is kept around to avoid illegal access in user code.
// This function is typically called either through an internal event handler, or through
// DisposeQTMovieWindow().
ErrCode CloseQTMovieWindow( QTMovieWindowH WI )
{ ErrCode err = paramErr;
  QTMovieWindows *wi;
	if( WI && *WI && (*WI)->self == *WI && QTMovieWindowH_from_Movie((*WI)->theMovie) ){
		wi = *WI;
		if( wi->handlingEvent ){
			return noErr;
		}
		if( wi->theMovieView ){
			// 20110310: stop the idling, that is the movie should no longer be tasked!
			[[wi->theMovieView movie] setIdling:NO];
		}
		PumpMessages(FALSE);
		unregister_MCActionList( wi->MCActionList );
		wi->MCActionList = NULL;
		if( wi->theMovieView ){
			[wi->theMovieView setControllerVisible:NO];
		}
		if( wi->theMC /*&& !wi->performingClose*/ ){
			DisposeMovieController(wi->theMC);
			wi->theMC = nil;
		}
		if( wi->theURL ){
			QTils_free( &(void*) wi->theURL);
			wi->theURL = NULL;
		}
		if( wi->dataRef ){
			if( wi->memRef && wi->memRef->dataRef == wi->dataRef ){
				wi->memRef->dataRef = NULL;
				DisposeMemoryDataRef(wi->memRef);
			}
			DisposeHandle( wi->dataRef );
			wi->dataRef = nil;
			wi->dataRefType = 0;
		}
		wi->theViewPtr = NULL;
		if( !wi->performingClose ){
			unregister_QTMovieWindowH_for_Movie( wi->theMovie );
			if( wi->theMovieView ){
			  QTMovie *qm = [wi->theMovieView movie];
			  Movie m = [qm quickTimeMovie];
				// release the only thing we ever retained:
				[qm invalidate];
				DisposeMovie(m);
				if( m == wi->theMovie ){
					NSLog( @"%@ held wi->theMovie: setting wi->theMovie = NULL!", qm );
					wi->theMovie = NULL;
				}
				[qm release];
				// theMovieView ought to (i.e. will probably) be released through the pool drain below
				// (it was obtained through loading a NIB so shouldn't be released explicitly?)
				wi->theMovieView = NULL;
			}
			if( wi->theView ){
			  NSWindow *nswin = wi->theView;
				unregister_QTMovieWindowH( WI );
				// 20110502:
				wi->theView = NULL;
				[nswin autorelease];
				[nswin performClose:nswin];
				// 20120205: moved line upwards:
//				wi->theView = NULL;
			}
			else{
				unregister_QTMovieWindowH( WI );
			}
			if( wi->theNSQTMovieWindow ){
			  struct NSQTMovieWindow *w = wi->theNSQTMovieWindow;
				// theNSQTMovieWindow ought to be released through the pool drain below
				// (it's set to autorelease!):
				wi->theNSQTMovieWindow = NULL;
				[w prepareForRelease];
			}
			if( wi->theMovie ){
			  Movie m = wi->theMovie;
				// theMovie existed before the pool and our other ObjC objects were created, so we
				// only close it after all the other resources have been released.
				wi->theMovie = nil;
				DisposeMovie(m);
			}
			if( wi->dataHandler ){
			  DataHandler d = wi->dataHandler;
				wi->dataHandler = nil;
				CloseMovieStorage(d);
			}
		}
		wi->idx = -1;
		err = noErr;
	}
	return err;
}

int DrainQTMovieWindowPool( QTMovieWindowH WI )
{ QTMovieWindows *wi;
  int ret = 0;
	if( WI && (wi = *WI) && !wi->handlingEvent && !wi->performingClose ){
	}
	return ret;
}

QTMovieWindowH lastQTWMH = NULL;

QTMovieWindowH OpenQTMovieWindowWithMovie( Movie theMovie, const char *theURL, short resId,
									    Handle dataRef, OSType dataRefType, int controllerVisible )
{ QTMovieWindows *wi = NULL;
  QTMovieWindowH wih = NULL;
  ErrCode err;
	if( (wih = QTMovieWindowH_from_Movie(theMovie)) || (wih = NewQTMovieWindowH()) ){
		wi = *wih;
	}

	if( wih && *wih && (*wih)->self == *wih ){
		if( theURL && (!wi->theURL || strcmp(theURL, wi->theURL)) ){
			wi->theURL = (const char*) QTils_strdup(theURL);
		}
		else{
			theURL = wi->theURL;
		}
		wi->theMovie = theMovie;
		GetMovieBox( wi->theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		wi->theNSQTMovieWindow = [[NSQTMovieWindow alloc] initWithQTWMandPool:wih];
		if( wi->theNSQTMovieWindow ){
			wi->theView = [wi->theNSQTMovieWindow theWindow];
			wi->theMovieView = [wi->theNSQTMovieWindow theMovieView];
			PumpMessages(FALSE);
			// do what it takes to attach the movie to our new window:
			err = noErr;
			if( err != noErr ){
				Log( qtLogPtr, "OpenQTMovieInWindow(): error %d opening \"%s\"\n", err, theURL );
				CloseQTMovieWindow(wih);
				DisposeQTMovieWindow(wih);
				wi = NULL, wih = NULL;
			}
			else{
				// RJVB 20110316: prepare movie for fast starting. Do it here to let the Movie Toolbox
				// make best use of the time we spend initialising other things.
				{ TimeValue timeNow;
				  Fixed playRate;
				  OSErr pErr;
					timeNow = GetMovieTime(theMovie, NULL);
					playRate = GetMoviePreferredRate(theMovie);
					if( wi->dataRefType == 'URL ' || wi->dataRefType == ' LRU'
					   || wi->dataRefType == 'url ' || wi->dataRefType == ' lru'
					){
						Log( qtLogPtr, "OpenQTMovieInWindow('%s') - prerolling a streaming movie may take some time!\n",
						    theURL
						);
					}
					pErr = PrePrerollMovie( theMovie, timeNow, playRate, NULL, NULL );
					if( pErr != noErr ){
						Log( qtLogPtr, "OpenQTMovieInWindow('%s') - PrePrerollMovie returned %d\n",
						    theURL, pErr
						);
					}
					pErr = PrerollMovie( theMovie, timeNow, playRate );
					if( pErr != noErr ){
						Log( qtLogPtr, "OpenQTMovieInWindow('%s') - PrerollMovie returned %d\n",
						    theURL, pErr
						);
					}
				}
				if( dataRef && wi->dataRef != dataRef ){
					InitQTMovieWindowHFromMovie( wih, NULL, theMovie, dataRef, dataRefType, NULL, resId, &err );
				}
				wi->theViewPtr = &wi->theView;
				register_QTMovieWindowH( wih, wi->theView );
				[QTMovieWindowList addObject:wi->theNSQTMovieWindow];

				ShowMoviePoster( wi->theMovie );
				SetMovieActive( wi->theMovie, YES );
				wi->theMC = [[wi->theNSQTMovieWindow theQTMovie] quickTimeMovieController];
				if( (BOOL) controllerVisible != [wi->theMovieView isControllerVisible] ){
					[wi->theNSQTMovieWindow ToggleMCController];
					if( controllerVisible ){
						wi->info->controllerHeight = [[wi->theNSQTMovieWindow theMovieView] controllerBarHeight];
					}
					else{
						wi->info->controllerHeight = 0;
					}
				}
				CreateNewMovieController(wih, controllerVisible);
				[wi->theNSQTMovieWindow setTitleWithCString:wi->theURL];
				// create a list into which to store movie controller action handlers:
				if( !wi->MCActionList ){
					wi->MCActionList = (void*) init_MCActionList();
				}
				if( !wi->NSMCActionList ){
					wi->NSMCActionList = (void*) init_NSMCActionList();
				}
				wi->loadState = GetMovieLoadState(wi->theMovie);
				{ MovieFrameTime ft;
					secondsToFrameTime( wi->theInfo.startTime, wi->theInfo.TCframeRate, &ft );
					Log( qtLogPtr, "OpenQTMovieInWindow(\"%s\"): %gs @ %gHz/%gHz starts %02d:%02d:%02d;%d loadState=%d\n",
					    wi->theURL, wi->theInfo.duration, wi->theInfo.frameRate, wi->theInfo.TCframeRate,
					    ft.hours, ft.minutes, ft.seconds, ft.frames,
					    wi->loadState
					);
				}
				Log( qtLogPtr, "OpenQTMovieInWindow(): movie=%p MC=%p wih,wi=%p,%p(%p) registered with QTMovieView=%p in process %u:%u\n",
				    wi->theMovie, wi->theMC,
				    wih, wi, wi->theView, GetNativeWindowPort(wi->theView),
				    getpid(), (unsigned int) pthread_self()
				);
				lastQTWMH = wih;
			}
		}
		else{
			Log( qtLogPtr, "\"%s\": QTMovie and/or QTMovieView failed to open\n", theURL );
			errbuf[0] = '\0';
			CloseQTMovieWindow(wih);
			DisposeQTMovieWindow(wih);
			wi = NULL, wih = NULL;
		}
		PumpMessages(FALSE);
	}
	return wih;
}

QTMovieWindowH OpenQTMovieWindowWithMovie_Mod2( Movie theMovie, char *theURL, int ulen, int visibleController )
{ QTMovieWindowH wih = NULL;
	if( theMovie ){
		if( theURL && !*theURL ){
			theURL = NULL;
		}
		wih = OpenQTMovieWindowWithMovie( theMovie, theURL, 1, NULL, 0, visibleController );
	}
	return wih;
}

// Attempts to open <theURL> as a QuickTime movie, and upon success opens a window of our
// own class (QTMWClass) in which the movie will be displayed. If requested, a movie controller
// is also created.
QTMovieWindowH OpenQTMovieInWindow( const char *theURL, int controllerVisible )
{ QTMovieWindowH wih;
  ErrCode err;
  short resId;
  Movie theMovie;
  Handle dataRef;
  OSType dataRefType;

	if( !QTMovieWindowList || !QTMWInitialised ){
		return NULL;
	}

	if( !theURL || !*theURL ){
		theURL = AskFileName( "Please choose a video or movie to display" );
	}

	if( !theURL || !*theURL ){
		return NULL;
	}

	err = OpenMovieFromURL( &theMovie, newMovieActive, &resId, theURL, &dataRef, &dataRefType );
	if( err != noErr ){
		Log( qtLogPtr, "OpenMovieFromURL('%s') returned err=%d, dataRefType='%s'\n",
		    theURL, err, OSTStr(dataRefType)
		);
		return NULL;
	}

	wih = OpenQTMovieWindowWithMovie( theMovie, theURL, resId, dataRef, dataRefType, controllerVisible );
	if( !wih ){
		DisposeMovie(theMovie);
	}
	return wih;
}

QTMovieWindowH OpenQTMovieInWindow_Mod2( const char *theURL, int ulen, int controllerVisible )
{ QTMovieWindowH wih;
	wih = OpenQTMovieInWindow( theURL, controllerVisible );
	if( wih ){
	  char *URL;
		// the Modula-2 definition of QTMovieWindows calls theURL a POINTER TO ARRAY[0..1024] OF CHAR
		// ... make it one!
		if( strlen( (*wih)->theURL ) < 1024 ){
			URL = (char*) QTils_calloc( 1024, sizeof(char*) );
			if( URL ){
				strcpy( URL, (*wih)->theURL );
				QTils_free( &(*wih)->theURL );
				(*wih)->theURL = URL;
			}
		}
	}
	return wih;
}

QTMovieWindowH OpenQTMovieFromMemoryDataRefInWindow( MemoryDataRef *memRef, OSType contentType, int controllerVisible )
{ QTMovieWindowH wih;
  ErrCode err;
  Movie theMovie;

	if( !QTMovieWindowList || !QTMWInitialised ){
		return NULL;
	}

	if( !memRef || !memRef->dataRef ){
		return NULL;
	}

	err = OpenMovieFromMemoryDataRef( &theMovie, memRef, contentType );
	if( err != noErr ){
		Log( qtLogPtr, "OpenMovieFromMemoryDataRef((virtual) %s) returned err=%d, dataRefType='%s'\n",
		    memRef->virtURL, err, OSTStr(memRef->dataRefType)
		);
		return NULL;
	}

	wih = OpenQTMovieWindowWithMovie( theMovie, memRef->virtURL, 1, memRef->dataRef, memRef->dataRefType, controllerVisible );
	if( !wih ){
		DisposeMovie(theMovie);
	}
	else{
		(*wih)->memRef = memRef;
	}
	return wih;
}

ErrCode GetWindowRect( NativeWindow w, Rect *bounds )
{
	if( w ){
	  NSRect r = [w iFrame];
		RectFromNSRect( bounds, &r );
		return noErr;
	}
	else{
		return paramErr;
	}
}

void MSMoveWindow( NativeWindow w, short left, short top, short width, short height, int dummmy )
{ NSRect r;
	r.origin.x = (CGFloat) left;
	r.origin.y = (CGFloat) top;
	r.size.width = (CGFloat) width;
	r.size.height = (CGFloat) height;
	[w setIFrame:r display:YES animate:NO];
}

void QTMovieWindowSetMovieSize( QTMovieWindowH wih, short width, short height, const char *caller )
{
	if( QTMovieWindowH_Check(wih) ){
		if( caller ){
			[(*wih)->theNSQTMovieWindow setMovieSize:NSMakeSize(width,height) caller:caller];
		}
		else{
			[(*wih)->theNSQTMovieWindow setMovieSize:NSMakeSize(width,height)];
		}
	}
}

/*!
	toggle a QT Movie controller's visibility. The window size is adapted
	to accomodate for the increased/decreased dimensions.
 */
ErrCode QTMovieWindowToggleMCController( QTMovieWindowH wih )
{ ErrCode err;
	if( QTMovieWindowH_isOpen(wih) ){
	 QTMovieWindows *wi = *wih;
//	 BOOL vis = [(*wih)->theMovieView isControllerVisible];
//		[(*wih)->theMovieView setControllerVisible:!vis];
		[wi->theNSQTMovieWindow ToggleMCController];
		if( [[wi->theNSQTMovieWindow theMovieView] isControllerVisible] ){
			wi->info->controllerHeight = [[wi->theNSQTMovieWindow theMovieView] controllerBarHeight];
		}
		else{
			wi->info->controllerHeight = 0;
		}
		err = noErr;
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	change a movie window's size and/or position
	@param	pos	new position, or NULL to leave unchanged
	@param	size	new dimensions, or NULL to leave unchanged
	@param	sizeScale	scale factor specifying a magnification w.r.t. the current (or new!) dimensions
	@param	setEnvelope	if True, the dimension/position specification apply to the window outline instead of contents
 */
ErrCode QTMovieWindowSetGeometry( QTMovieWindowH wih, Cartesian *pos, Cartesian *size,
						   double sizeScale, int setEnvelope )
{
#ifdef DEBUG
  Rect bounds, mcbounds;
  short MCextraWidth = 0, MCextraHeight = 0;
  Cartesian lSize, *Size;
#endif
  NSRect ebounds, cbounds;
  QTMovieWindowPtr wi;
  ErrCode err = noErr;
	if( QTMovieWindowH_isOpen(wih) ){
		wi = *wih;
#ifdef DEBUG
		if( setEnvelope ){
		  Rect wRect;
			if( (err = GetWindowRect( wi->theView, &wRect )) != noErr ){
				return err;
			}
			// NB: In conformance with conventions for the RECT structure, the bottom-right coordinates
			// of the returned rectangle are exclusive. In other words, the pixel at (right, bottom)
			// lies immediately outside the rectangle.
			bounds.left = (short) wRect.left;
			bounds.top = (short) wRect.top;
			bounds.right = (short) abs(wRect.right - wRect.left);
			bounds.bottom = (short) abs(wRect.bottom - wRect.top);
			mcbounds = bounds;
		}
		else{
			// 20110204: we'll probably also want to know the movie bounds excluding the MC
			GetMovieBox( wi->theMovie, &wi->theMovieRect );
			MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
			bounds = wi->theMovieRect;
			// 20110204: if there's an MC, we need to tell it to resize even if it's invisible!
			// of course this requires a different bounds rect...
			if( wi->theMC ){
				MCGetControllerBoundsRect(wi->theMC, &mcbounds);
				MacOffsetRect( &mcbounds, -mcbounds.left, -mcbounds.top );
				MCextraWidth = mcbounds.right - bounds.right;
				MCextraHeight = mcbounds.bottom - bounds.bottom;
			}
		}
#endif // DEBUG
		ebounds = [wi->theView iFrame];
		cbounds = [wi->theView contentRectForFrameRect:[wi->theView frame]];
		cbounds.origin.x = ebounds.origin.x + (ebounds.size.width - cbounds.size.width);
		cbounds.origin.y = ebounds.origin.y + (ebounds.size.height - cbounds.size.height);
		// update both bounds rects!
		if( size ){
#ifdef DEBUG
			lSize = *size;
			Size = &lSize;
			if( wi->theMC ){
				// 20110908: content size specification is supposed to contain the MC dimensions,
				// so in order to preserve the functioning of the code below we'll have to
				// subtract those dimensions first...
				if( !setEnvelope && MCGetVisible(wi->theMC) ){
					Size->horizontal -= MCextraWidth;
					Size->vertical -= MCextraHeight;
				}
				mcbounds.right = Size->horizontal + MCextraWidth;
				mcbounds.bottom = Size->vertical + MCextraHeight;
			}
			bounds.right = Size->horizontal;
			bounds.bottom = Size->vertical;
#endif //DEBUG
			ebounds.size.width = cbounds.size.width = (CGFloat) size->horizontal;
			ebounds.size.height = cbounds.size.height = (CGFloat) size->vertical;
		}
#ifdef DEBUG
		if( wi->theMC ){
			// note that we do NOT want to scale the controller...
			mcbounds.right = (short) (bounds.right * sizeScale + 0.5) + MCextraWidth;
			mcbounds.bottom = (short) (bounds.bottom * sizeScale + 0.5) + MCextraHeight;
		}
		bounds.right = (short) (bounds.right * sizeScale + 0.5);
		bounds.bottom = (short) (bounds.bottom * sizeScale + 0.5);
#endif //DEBUG
		ebounds.size.width *= sizeScale;
		ebounds.size.height *= sizeScale; // we could scale the envelope height via the scaled content height!
		cbounds.size.width *= sizeScale;
		cbounds.size.height *= sizeScale;
		if( [[wi->theNSQTMovieWindow theMovieView] isControllerVisible] ){
			wi->info->controllerHeight = [[wi->theNSQTMovieWindow theMovieView] controllerBarHeight];
		}
		else{
			wi->info->controllerHeight = 0;
		}
		if( pos ){
#ifdef DEBUG
			if( setEnvelope ){
				mcbounds.left = pos->horizontal;
				mcbounds.top = pos->vertical;
				bounds.left = pos->horizontal;
				bounds.top = pos->vertical;
			}
#endif //DEBUG
			ebounds.origin.x = cbounds.origin.x = pos->horizontal;
			ebounds.origin.y = cbounds.origin.y = pos->vertical;
		}
		// define the new movie size: cbounds.size minus the controllerHeight
		{ NSSize msize = NSMakeSize(cbounds.size.width, cbounds.size.height - wi->info->controllerHeight);
			[wi->theNSQTMovieWindow setMovieSize:msize];
		}
		// we do NOT adapt cbounds.size to reflect the presence/absence of the MovieController:
		// user specified cbounds.size, and should get exactly that!
		if( !setEnvelope ){
			[wi->theView setIFrame:[wi->theView iFrameRectForContentIRect:cbounds] display:YES animate:NO];
		}
		else{
			[wi->theView setIFrame:ebounds display:YES animate:NO];
		}
		err = GetMoviesError();
		QTWMflush();
		PumpMessages(FALSE);
		GetMovieBox( wi->theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		ebounds = [wi->theView iFrame];
		Log( qtLogPtr, "QTMovieWindowSetGeometry(%s%s%gx): %gx%g+%g+%g movieRect=%hdx%hd+%hd+%hd\n",
			(pos)? "position," : "",
			(size)? "size," : "",
			sizeScale,
			ebounds.size.width, ebounds.size.height, ebounds.origin.x, ebounds.origin.y,
			wi->theMovieRect.right, wi->theMovieRect.bottom, wi->theMovieRect.left, wi->theMovieRect.top
		);
	}
	else{
		err = paramErr;
	}
	return err;
}

/*!
	return a movie window's size and/or position (pass NULL to ignore either or both); of the content region
	(getEnvelope==False) or of the window outline (getOutline==True)
 */
ErrCode QTMovieWindowGetGeometry( QTMovieWindowH wih, Cartesian *pos, Cartesian *size, int getEnvelope )
{ QTMovieWindowPtr wi;
  ErrCode err = noErr;
  NSRect ebounds, cbounds;
	if( QTMovieWindowH_isOpen(wih) ){
		wi = *wih;
		// update theMovieRect:
		GetMovieBox( wi->theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		ebounds = [wi->theView iFrame];
		cbounds = [wi->theView contentRectForFrameRect:[wi->theView frame]];
		cbounds.origin.x = ebounds.origin.x + (ebounds.size.width - cbounds.size.width);
		cbounds.origin.y = ebounds.origin.y + (ebounds.size.height - cbounds.size.height);
		Log( qtLogPtr, "GetWindowRect(%p)=%d: envelope=%gx%g+%g+%g content=%gx%g+%g+%g movieRect=%hdx%hd+%hd+%hd\n",
		    wi->theView, err,
		    ebounds.size.width, ebounds.size.height, ebounds.origin.x, ebounds.origin.y,
		    cbounds.size.width, cbounds.size.height, cbounds.origin.x, cbounds.origin.y,
		    abs(wi->theMovieRect.right - wi->theMovieRect.left), abs(wi->theMovieRect.bottom - wi->theMovieRect.top),
		    wi->theMovieRect.left, wi->theMovieRect.top
		);
		if( getEnvelope ){
			if( pos ){
				pos->horizontal = (short) (ebounds.origin.x + 0.5);
				pos->vertical = (short) (ebounds.origin.y + 0.5);
			}
			if( size ){
				size->horizontal = (short) (ebounds.size.width + 0.5);
				size->vertical = (short) (ebounds.size.height + 0.5);
			}
		}
		else{
			if( pos ){
				pos->horizontal = (short) (cbounds.origin.x + 0.5);
				pos->vertical = (short) (cbounds.origin.y + 0.5);
			}
			if( size ){
				size->horizontal = (short) (cbounds.size.width + 0.5);
				size->vertical = (short) (cbounds.size.height + 0.5);
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

void SetQTMovieTime( NSQTMovieWindow *theNSQTMovieWindow, TimeRecord *trec )
{
	if( theNSQTMovieWindow && trec ){
		[[theNSQTMovieWindow theQTMovie] setCurrentTime:QTMakeTimeWithTimeRecord(*trec)];
	}
}

ErrCode QTMovieWindowPlay( QTMovieWindowH wih )
{ ErrCode err;
 	if( !QTMovieWindowH_Check(wih) ){
		err = paramErr;
	}
	else{
		[(*wih)->theMovieView play:(*wih)->theNSQTMovieWindow];
		err = GetMoviesError();
		PumpMessages(FALSE);
	}
	return err;
}

ErrCode QTMovieWindowStop( QTMovieWindowH wih )
{ ErrCode err;
 	if( !QTMovieWindowH_Check(wih) ){
		err = paramErr;
	}
	else{
		[(*wih)->theMovieView pause:(*wih)->theNSQTMovieWindow];
		err = GetMoviesError();
		PumpMessages(FALSE);
	}
	return err;
}

#pragma mark ---- housekeeping

// QuickTime must be initialised before it can be used.
static ErrCode lOpenQT()
{ ErrCode err;
	if( !QTMWInitialised ){
		InitQTMovieWindows();
	}
	err = EnterMovies();
	return err;
}

static int QTOpened = FALSE;

ErrCode OpenQT()
{ ErrCode err;
	if( !QTOpened ){
		err = lOpenQT();
		if( !QTilsPool ){
			QTilsPool = [[NSAutoreleasePool alloc] init];
		}
		if( !QTMovieWindowList ){
			QTMovieWindowList = [[[NSMutableArray alloc] initWithCapacity:1] retain];
		}
		if( err == noErr ){
			QTOpened = TRUE;
		}
	}
	else{
		err = noErr;
	}
	return err;

}

// And it should be closed when done with it.
void CloseQT()
{
	if( QTMovieWindowList ){
		[QTMovieWindowList release];
	}
// draining our pool can cause us to crash? So be it, let the OS handle deallocating all remaining memory!!
//	if( QTilsPool ){
//		[QTilsPool drain];
//	}
	if( QTOpened ){
		ExitMovies();
		QTOpened = FALSE;
	}
}

void GetMaxBounds(Rect *maxRect)
{
	maxRect->left = -32767;
	maxRect->top = -32767;
	maxRect->right = 32766;
	maxRect->bottom = 32766;
}

void appInit()
{ NSApplication *app = [NSApplication sharedApplication];
//  theAppDelegate *delegate = [[theAppDelegate alloc] init];
//	[app setDelegate: delegate];
	[app run];
//	[delegate release];
}

short InitQTMovieWindows()
{ BOOL ret;

	// NSApp is nil as long as [NSApplication sharedApplication] hasn't been called,
	// and that probably means there's no autorelease pool? OTOH, simply calling
	// that selector does NOT create a global autorelease pool, apparently.
	if( !QTilsPool /* && !NSApp */ ){
		QTilsPool = [[NSAutoreleasePool alloc] init];
	}
#ifndef EMBEDDED_FRAMEWORK
	if( !theApp ){
		theApp = [NSApplication sharedApplication];
	}
#endif
	QTils_LogInit();

	init_QTMWlists();
	init_HRTime();

	QTWMcounter = -1;
	if( !QTMovieWindowList ){
		QTMovieWindowList = [[[NSMutableArray alloc] initWithCapacity:1] retain];
	}
#ifndef DEBUG
	PCLogSetActive(NO);
#endif
	QTMWInitialised = TRUE;
//	Log( qtLogPtr, "QTMovieWindows initialised!\n" );
	return ret;
}

__attribute__((constructor))
static void initialiser( int argc, char** argv, char** envp )
{
	if( !QTMWInitialised ){
		InitQTMovieWindows();
	}
}

ErrCode ActivateQTMovieWindow( QTMovieWindowH wih )
{ QTMovieWindowPtr wi;
  ErrCode err;
	if( QTMovieWindowH_Check(wih) ){
		wi = *wih;
		[[wi->theNSQTMovieWindow theWindow] orderFront:wi->theNSQTMovieWindow];
//		[[wi->theNSQTMovieWindow theWindow] orderFrontRegardless];
		if( wi->theMC ){
			err = (ErrCode) MCActivate( wi->theMC, GetNativeWindowPort(wi->theView), TRUE );
		}
		else{
			err = noErr;
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

BOOL loadQTilsMenu()
{ BOOL ret;
	if( !theApp ){
		theApp = [NSApplication sharedApplication];
	}
	if( !(ret = [NSBundle loadNibNamed:@"MainWindow" owner:theApp] ) ){
		NSLog( @"%@ failed to load 'MainWindow' nib!", theApp );
	}
	return ret;
}

BOOL QTils_LogActive()
{
	return PCLogActive();
}

BOOL QTils_LogSetActive(BOOL active)
{
	return PCLogSetActive(active);
}

int QTils_Log(const char *fileName, int lineNr, NSString *format, ... )
{ va_list ap;
  int ret;
	va_start(ap, format);
	ret = vPCLogStatus([NSApplication sharedApplication], basename((char*)fileName), lineNr, format, ap);
	va_end(ap);
	return ret;
}

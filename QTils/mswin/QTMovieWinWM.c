/*!
 *  @file QTMovieWinWM.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101130.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the MSWin32-specific routines and those that depend
 * on them directly.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTMovieWinWM: QuickTime utilities: MSWin32 part of the toolkit");

#define HAS_LOG_INIT
#include "../Logging.h"
#include "../../timing.h"

#define _QTILS_C

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <Windows.h>

#include <QTML.h>
#include <ImageCodec.h>
#include <TextUtils.h>
#include <string.h>
#include <Files.h>
#include <Movies.h>
#include <MediaHandlers.h>
#include <tchar.h>
#include <wtypes.h>
#include <FixMath.h>
#include <Script.h>
#include <NumberFormatting.h>
#include <direct.h>

#ifndef TARGET_OS_WIN32
#	define TARGET_OS_WIN32
#endif

#include "../QTilities.h"
#include "../Lists.h"
#include "../QTMovieWin.h"

#define strdup(s)			_strdup((s))
#define strncasecmp(a,b,n)	_strnicmp((a),(b),(n))
#define strcasecmp(a,b)		_stricmp((a),(b))

static char errbuf[512];

static HINSTANCE ghInst = NULL;

// a minimal message pump which can be called regularly to make sure MSWindows event messages
// get handled.
unsigned int PumpMessages(int force)
{ MSG msg;
  BOOL ok;
  unsigned int n = 0;
	if( *errbuf && qtLogPtr ){
		Log( qtLogPtr, errbuf );
		errbuf[0] = '\0';
	}
	FlushLog( qtLogPtr );
	if( force ){
		ok = GetMessage( &msg, NULL, 0, 0 );
	}
	else{
		ok = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
	}
	while( ok ){
		if( ok != -1 ){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			n += 1;
		}
		ok = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
	}
	if( *errbuf && qtLogPtr ){
		Log( qtLogPtr, errbuf );
		errbuf[0] = '\0';
	}
	else{
		FlushLog( qtLogPtr );
	}
	return n;
}

static char QTMWClass[] = QTMOVIEWINDOW;

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

/*!
	retrieve the QTMovieWindowH corresponding to a native window. There are several ways
	this can be done; for now the one using the GWL_USERDATA slot (intended for this use?) appears
	to work reliably enough.
 */
QTMovieWindowH QTMovieWindowHFromNativeWindow( NativeWindow hWnd )
{ QTMovieWindowH wi, gwi = NULL;
	gwi = (QTMovieWindowH) GetWindowLongPtr( hWnd, GWL_USERDATA );
//	wi = (QTMovieWindowH) GetProp( hWnd, (LPCWSTR) "QTMovieWindowH" );
//	if( !wi || wi != gwi || !*wi || *wi != *gwi || (*wi)->self != (*wi) || (*wi)->theView != hWnd )
//	if( !gwi || !*gwi || (*gwi)->self != (*gwi) || (*gwi)->theView != hWnd )
	if( !QTMovieWindowH_Check(gwi) || (*gwi)->theView != hWnd )
	{
		gwi = NULL;
#ifdef DEBUG
		Log( qtLogPtr, "QTMovieWindowHFromNativeWindow(%p): not a QTMovieWindow?!", hWnd );
#endif
	}
	return gwi;
}

/*!
	the event handler that is specific to our window class:
 */
static LRESULT CALLBACK QTMWProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ PAINTSTRUCT ps;
  HDC hdc;
  QTMovieWindowH wi = NULL;

	if( !IsIconic(hWnd) && GetNativeWindowPort(hWnd) ){
	  MSG msg;
	  EventRecord macEvent;
	  LONG thePoints = GetMessagePos();

		msg.hwnd = hWnd;
		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;
		msg.time = GetMessageTime();
		msg.pt.x = LOWORD(thePoints);
		msg.pt.y = HIWORD(thePoints);
		WinEventToMacEvent(&msg, &macEvent);  // Convert the message to a QTML event

		if( !QTApp_HandleEvent( &macEvent ) ){
			wi = QTMovieWindowHFromNativeWindow(hWnd);
			// if we have a Movie Controller, pass the QTML event
			if( wi ){
				(*wi)->handlingEvent += 1;
				if( (*wi)->theMC ){
#ifdef DEBUG
//					Log( qtLogPtr, "Forwarding hWND=%p message to wi,*wi=%p,%p QT controller #%u\n", hWnd, wi, *wi, (*wi)->idx );
#endif
					MCIsPlayerEvent( (*wi)->theMC, (const EventRecord *)&macEvent );
				}
			}
		}
	}

	switch( message ){
#ifdef DEBUG
		case WM_CREATE:
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( wi ){
				Log( qtLogPtr, "WM_CREATE QT window %p=#%u\n", hWnd, (*wi)->idx );
			}
			else{
				Log( qtLogPtr, "WM_CREATE QT window %p=???\n", hWnd );
			}
			break;
#endif
		case WM_PAINT:{
#ifdef AllowQTMLDoubleBuffering
		  HRGN winUpdateRgn = CreateRectRgn(0,0,0,0);
#endif //AllowQTMLDoubleBuffering
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( wi ){
#ifdef DEBUG
				Log( qtLogPtr, "Paint QT window %p=#%u\n", hWnd, (*wi)->idx );
#endif
			}
#ifdef AllowQTMLDoubleBuffering
			// Get the Windows update region
			GetUpdateRgn( hWnd, winUpdateRgn, FALSE );
#endif // AllowQTMLDoubleBuffering
			// Call BeginPaint/EndPaint to clear the update region
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );

#ifdef AllowQTMLDoubleBuffering
			if( wi ){
				// Add to the dirty region of the port any region
				// that Windows says needs updating. This allows the
				// union of the two to be copied from the back buffer
				// to the screen on the next flush.
				QTMLAddNativeRgnToDirtyRgn( GetNativeWindowPort(hWnd), winUpdateRgn );
				DeleteObject( winUpdateRgn );

				// Flush the entire dirty region to the screen
				QTMLFlushPortDirtyRgn( GetNativeWindowPort(hWnd) );
			}
#endif // AllowQTMLDoubleBuffering
			break;
		}
		case WM_CLOSE:{
			// Unregister the HWND with QTML
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( QTMovieWindowH_Check(wi) ){
			  int (*fun)(QTMovieWindowH wi, void *params) = NULL;
				Log( qtLogPtr, "Close QT window #%u\n", (*wi)->idx );
				(*wi)->shouldClose = 1;
				if( (fun = get_MCAction( wi, _MCAction_.Close )) ){
					(*fun)( wi, NULL );
				}
				CloseQTMovieWindow( wi );
				// care must be taken using wi hereafter!
			}
			break;
		}
		case WM_DESTROY:
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( QTMovieWindowH_Check(wi) ){
				Log( qtLogPtr, "Destroy QT window %p=#%u\n", hWnd, (*wi)->idx );
				(*wi)->shouldClose = 1;
				// at this point it wouldn't be wise to let CloseQTMovieWindow()
				// call DestroyWindow(); DefWindowProc() does that for us.
				(*wi)->theView = NULL;
				(*wi)->theViewPtr = NULL;
				CloseQTMovieWindow( wi );
				// care must be taken using wi hereafter!
			}
			else{
				Log( qtLogPtr, "Destroy QT window %p\n", hWnd );
			}
//			PostQuitMessage(0);
			break;

		case WM_ENTERSIZEMOVE:
			{ RECT clientRect;
				if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
					(*wi)->handlingEvent += 1;
				}
				if( wi ){
					Log( qtLogPtr, "EnterSizeMove QT window #%u\n", (*wi)->idx );
					GetClientRect( hWnd, &clientRect );
					(*wi)->gOldWindowPos.cx = clientRect.right - clientRect.left;
					(*wi)->gOldWindowPos.cy = clientRect.bottom - clientRect.top;
				}
			}
			break;
		case WM_EXITSIZEMOVE:
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if ( wi && (*wi)->theMovie ){
			  RECT clientRect;
			  long widthAdjust = 0, heightAdjust = 0;
				Log( qtLogPtr, "ExitSizeMove QT window #%u\n", (*wi)->idx );
				GetClientRect( hWnd, &clientRect );
				widthAdjust = (clientRect.right - clientRect.left) - (*wi)->gOldWindowPos.cx;
				heightAdjust = (clientRect.bottom - clientRect.top) - (*wi)->gOldWindowPos.cy;
				if( widthAdjust || heightAdjust ){
				  Rect controllerBox;
					MCGetControllerBoundsRect( (*wi)->theMC, &controllerBox);
					controllerBox.right += (short)widthAdjust;
					controllerBox.bottom += (short)heightAdjust;
					MCSetControllerBoundsRect( (*wi)->theMC, &controllerBox);
				}
			}
			break;
	}
	// we might have closed the wi at this point, so do an additional check
	// to see if we can really decrease the event handling counter!
	if( wi && *wi ){
		(*wi)->handlingEvent -= 1;
	}

	return DefWindowProc( hWnd, message, wParam, lParam );
}

void QTWMflush()
{
#ifdef AllowQTMLDoubleBuffering
	QTMLFlushDirtyPorts();
#endif // AllowQTMLDoubleBuffering
}

/*!
	Close the window and movie of a QTMovieWindow, deallocating all depending objects
	the handle itself is kept around to avoid illegal access in user code.
	This function is typically called either through an internal event handler, or through
	DisposeQTMovieWindow().
 */
ErrCode CloseQTMovieWindow( QTMovieWindowH WI )
{ ErrCode err = paramErr;
  QTMovieWindows *wi;
	if( WI && *WI && (*WI)->self == *WI && QTMovieWindowH_from_Movie((*WI)->theMovie) ){
		wi = *WI;
		// remove the associations between this WI and its native window, so that any
		// events to the latter do not get passed on by us to QT anymore.
		if( wi->theView ){
		  MSG msg;
			// flush any pending events for this window
			// while( PeekMessage( &msg, wi->theView, 0, 0, PM_REMOVE ) );
			SetWindowLongPtr( wi->theView, GWL_USERDATA, (LONG_PTR) NULL );
			SetProp( wi->theView, (LPCSTR) "QTMovieWindowH", NULL );
			unregister_QTMovieWindowH( WI );
		}
		unregister_MCActionList( wi->MCActionList );
		wi->MCActionList = NULL;
		unregister_QTMovieWindowH_for_Movie( wi->theMovie );
		if( wi->theMC ){
			DisposeMovieController(wi->theMC);
			wi->theMC = nil;
		}
		if( wi->theMovie ){
			DisposeMovie( wi->theMovie );
			wi->theMovie = nil;
		}
		if( wi->dataHandler ){
			CloseMovieStorage(wi->dataHandler);
			wi->dataHandler = nil;
		}
		if( wi->theURL ){
			QTils_free( (char**) &wi->theURL);
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
		if( wi->theView ){
//		  CGrafPtr port = (CGrafPtr) GetHWNDPort(wi->theView);
		  CGrafPtr port = (CGrafPtr) GetNativeWindowPort(wi->theView);
		  NativeWindow theView = wi->theView;
			if( port ){
				DestroyPortAssociation(port);
			}
			wi->theView = NULL;
			wi->theViewPtr = NULL;
#ifdef DEBUG
			Log( qtLogPtr, "CloseQTMovieWindow(%p): DestroyWindow(%p)", WI, theView );
#endif
			DestroyWindow( theView );
		}
		wi->idx = -1;
		err = noErr;
		PumpMessages(FALSE);
	}
	return (ErrCode) err;
}

QTMovieWindowH lastQTWMH = NULL;

static Boolean nothing( DialogPtr dialog, EventRecord *event, DialogItemIndex *itemhit )
{
	return 0;
}

QTMovieWindowH OpenQTMovieWindowWithMovie( Movie theMovie, const char *theURL, short resId,
									    Handle dataRef, OSType dataRefType, int visibleController )
{ QTMovieWindows *wi = NULL;
  QTMovieWindowH wih = NULL;
  ErrCode err;

	if( (wih = QTMovieWindowH_from_Movie(theMovie)) || (wih = NewQTMovieWindowH()) ){
		wi = *wih;
	}

	if( wih && *wih && (*wih)->self == *wih ){
		if( !theURL ){
			theURL = wi->theURL;
		}
		GetMovieBox( theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		wi->theView = CreateWindowEx(
				WS_EX_APPWINDOW|WS_EX_WINDOWEDGE, (LPCSTR) QTMWClass,
				(LPCSTR) theURL, WS_OVERLAPPED|WS_CAPTION|WS_VISIBLE,
				CW_USEDEFAULT, CW_USEDEFAULT, wi->theMovieRect.right, wi->theMovieRect.bottom,
				0, 0, ghInst, NULL
		);
		if( wi->theView ){
		 CGrafPtr currentPort;
		 GDHandle currentGDH;
			PumpMessages(FALSE);
			// do what it takes to attach the movie to our new window:
			GetGWorld( &currentPort, &currentGDH );
			// Create GrafPort <-> HWND association
			CreatePortAssociation( wi->theView, NULL, 0);
			// Set the port
			SetGWorld((CGrafPtr)GetNativeWindowPort(wi->theView), nil);
			err = noErr;
			if( err != noErr ){
				Log( qtLogPtr, "OpenQTMovieWindowWithMovie(): error %d opening \"%s\"\n", err, theURL );
				CloseQTMovieWindow(wih);
				DisposeQTMovieWindow(wih);
				wi = NULL, wih = NULL;
			}
			else{
				if( dataRef && wi->dataRef != dataRef ){
					InitQTMovieWindowHFromMovie( wih, theURL, theMovie, dataRef, dataRefType, NULL, resId, &err );
				}
				// final association between the new movie and its new window:
				SetMovieGWorld( wi->theMovie, (CGrafPtr)GetNativeWindowPort(wi->theView), nil );
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
						Log( qtLogPtr, "OpenQTMovieWindowWithMovie('%s') - prerolling a streaming movie may take some time!\n",
						    theURL
						);
					}
					pErr = PrePrerollMovie( theMovie, timeNow, playRate, NULL, NULL );
					if( pErr != noErr ){
						Log( qtLogPtr, "OpenQTMovieWindowWithMovie('%s') - PrePrerollMovie returned %d\n",
						    theURL, pErr
						);
					}
					pErr = PrerollMovie( theMovie, timeNow, playRate );
					if( pErr != noErr ){
						Log( qtLogPtr, "OpenQTMovieWindowWithMovie('%s') - PrerollMovie returned %d\n",
						    theURL, pErr
						);
					}
				}

				wi->theViewPtr = &wi->theView;
				register_QTMovieWindowH( wih, wi->theView );

				// register the QTMovieWindowPtr with the window we just opened. We use SetProp/Getprop for that
				// internally, as there doesn't seem to be a safe way to check the type of information returned
				// by GetWindowLongPtr() (i.e. we could get someone else's GLW_USERDATA??!)
				SetWindowLongPtr( wi->theView, GWL_USERDATA, (LONG_PTR) wih );
				SetProp( wi->theView, (LPCSTR) "QTMovieWindowH", wih );
				// internal registry based on a C++ hash_map (see list.cpp)

				ShowMoviePoster( wi->theMovie );
				SetMovieActive( wi->theMovie, TRUE );
				CreateNewMovieController(wih, visibleController);
				// call QTMovieWindowSetGeometry with scale==1 to update the controllerHeight:
				QTMovieWindowSetGeometry( wih, NULL, NULL, 1.0, 0 );

				SetWindowText( wi->theView, wi->theURL );
				ShowWindow(wi->theView, SW_SHOWDEFAULT);
				UpdateWindow(wi->theView);
				// create a list into which to store movie controller action handlers:
				if( !wi->MCActionList ){
					wi->MCActionList = (void*) init_MCActionList();
				}
				wi->loadState = GetMovieLoadState(wi->theMovie);
				{ MovieFrameTime ft;
					secondsToFrameTime( wi->theInfo.startTime, wi->theInfo.TCframeRate, &ft );
					Log( qtLogPtr, "OpenQTMovieWindowWithMovie(\"%s\"): %gs @ %gHz/%gHz starts %02d:%02d:%02d;%d loadState=%d\n",
					    wi->theURL, wi->theInfo.duration, wi->theInfo.frameRate, wi->theInfo.TCframeRate,
					    ft.hours, ft.minutes, ft.seconds, ft.frames,
					    wi->loadState
					);
				}
				Log( qtLogPtr, "OpenQTMovieWindowWithMovie(): movie=%p MC=%p wih,wi=%p,%p registered with HWND=%p in process %u:%u\n",
				    wi->theMovie, wi->theMC,
				    wih, wi, wi->theView,
				    GetCurrentProcessId(), GetCurrentThreadId()
				);
//				ShowMovieInformation( wi->theMovie, nothing, 0 );
				lastQTWMH = wih;
			}
			// leave our GWorld to what it was when we started:
			SetGWorld( currentPort, currentGDH );
		}
		else{
			FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
			);
			Log( qtLogPtr, "\"%s\": %s", theURL, errbuf );
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
// own class (QTMWClass) in which the movie will be displayed. A movie controller
// is also created, visible or not.
QTMovieWindowH OpenQTMovieInWindow( const char *theURL, int visibleController )
{ QTMovieWindowH wih = NULL;
  ErrCode err;
  short resId;
  Movie theMovie;
  Handle dataRef;
  OSType dataRefType;

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

	wih = OpenQTMovieWindowWithMovie( theMovie, theURL, resId, dataRef, dataRefType, visibleController );
	if( !wih ){
		DisposeMovie(theMovie);
	}
	return wih;
}

/*!
	Modula-2 wrapper for OpenQTMovieInWindow()
 */
QTMovieWindowH OpenQTMovieInWindow_Mod2( const char *theURL, int ulen, int visibleController )
{ QTMovieWindowH wih;
	wih = OpenQTMovieInWindow( theURL, visibleController );
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
		// Something is different when calling us from inside Stoney Brooks Modula2 that
		// causes the window not to refresh properly until one clicks inside the window (on the movie).
		// Attempt to simulate that ...
//		SendMessage( (*wih)->theView, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(1,1) );
//		PumpMessages(FALSE);
//		SendMessage( (*wih)->theView, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(1,1) );
//		PumpMessages(FALSE);
	}
	return wih;
}

/*!
	Opens a movie generated from the in-memory dataRef <memRef>.
 */
QTMovieWindowH OpenQTMovieFromMemoryDataRefInWindow( MemoryDataRef *memRef, OSType contentType, int controllerVisible )
{ QTMovieWindowH wih;
  ErrCode err;
  Movie theMovie;

	if( !memRef || !memRef->dataRef ){
		return NULL;
	}

	err = OpenMovieFromMemoryDataRef( &theMovie, memRef, contentType );
	if( err != noErr ){
		Log( qtLogPtr, "OpenMovieFromMemoryDataRef((virtual)%s) returned err=%d, dataRefType='%s'\n",
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

/*!
	toggle a QT Movie controller's visibility. The window size is adapted
	to accomodate for the increased/decreased dimensions.
 */
ErrCode QTMovieWindowToggleMCController( QTMovieWindowH wih )
{ ErrCode err;
	if( QTMovieWindowH_isOpen(wih) && (*wih)->theMC ){
	 ComponentResult vis = MCGetVisible( (*wih)->theMC );
		err = MCSetVisible( (*wih)->theMC, !vis );
		QTMovieWindowSetGeometry( wih, NULL, NULL, 1.0, 0 );
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
{ Rect bounds, mcbounds;
  short MCextraWidth = 0, MCextraHeight = 0;
  QTMovieWindowPtr wi;
  WindowRef w;
  ErrCode err = noErr;
  Cartesian lSize, *Size;
	if( QTMovieWindowH_isOpen(wih) ){
		wi = *wih;
		w = (WindowRef) GetNativeWindowPort(wi->theView);
		if( setEnvelope ){
		  RECT wRect;
			if( !GetWindowRect( wi->theView, &wRect ) ){
				return GetLastError();
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
		// update both bounds rects!
		if( size ){
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
		}
		else{
			Size = NULL;
		}
		if( wi->theMC ){
			// note that we do NOT want to scale the controller...
			mcbounds.right = (short) (bounds.right * sizeScale + 0.5) + MCextraWidth;
			mcbounds.bottom = (short) (bounds.bottom * sizeScale + 0.5) + MCextraHeight;
			wi->info->controllerHeight = (MCGetVisible(wi->theMC))? MCextraHeight : 0;
		}
		else{
			wi->info->controllerHeight = 0;
		}
		bounds.right = (short) (bounds.right * sizeScale + 0.5);
		bounds.bottom = (short) (bounds.bottom * sizeScale + 0.5);
		if( pos ){
			if( !setEnvelope ){
				MacMoveWindow( w, pos->horizontal, pos->vertical, FALSE );
			}
			else{
				mcbounds.left = pos->horizontal;
				mcbounds.top = pos->vertical;
				bounds.left = pos->horizontal;
				bounds.top = pos->vertical;
			}
		}
		if( !setEnvelope ){
			if( wi->theMC ){
				MCSetControllerBoundsRect(wi->theMC, &mcbounds);
			}
			if( !wi->theMC || !MCGetVisible(wi->theMC) ){
				// if the MC isn't visible, we'll want to resize the window such that it doesn't
				// make space for the controller. QuickTime really could have taken care of that for us...
				SizeWindow( w, bounds.right, bounds.bottom, TRUE );
			}
		}
		else{
			MoveWindow( wi->theView, bounds.left, bounds.top, bounds.right, bounds.bottom, TRUE );
		}
		PortChanged( (GrafPtr) w );
		err = GetMoviesError();
		if( wi->theMC /* && MCGetVisible(wi->theMC) */ ){
			MCMovieChanged( wi->theMC, wi->theMovie );
			if( !MCIdle(wi->theMC) ){
				MCDraw(wi->theMC, (WindowRef /*GrafPtr*/) GetNativeWindowPort(wi->theView) );
			}
		}
		QTWMflush();
		PumpMessages(FALSE);
		GetMovieBox( wi->theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		Log( qtLogPtr, "QTMovieWindowSetGeometry(%s%s%gx): %hdx%hd+%hd+%hd movieRect=%hdx%hd+%hd+%hd\n",
			(pos)? "position," : "",
			(Size)? "Size," : "",
			sizeScale,
			bounds.right, bounds.bottom, bounds.left, bounds.top,
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
//  WindowRef w;
  RECT bounds;
	if( QTMovieWindowH_isOpen(wih) ){
		wi = *wih;
		// update theMovieRect:
		GetMovieBox( wi->theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
//		w = (WindowRef) GetNativeWindowPort(wi->theView);
		if( !GetWindowRect( wi->theView, &bounds ) ){
			err = GetLastError();
		}
		Log( qtLogPtr, "GetWindowRect(%p)=%d: envelope=%ldx%ld+%ld+%ld movieRect=%hdx%hd+%hd+%hd\n",
		    wi->theView, err,
		    abs(bounds.right - bounds.left), abs(bounds.bottom - bounds.top),
		    bounds.left, bounds.top,
		    abs(wi->theMovieRect.right - wi->theMovieRect.left), abs(wi->theMovieRect.bottom - wi->theMovieRect.top),
		    wi->theMovieRect.left, wi->theMovieRect.top
		);
		if( getEnvelope ){
			if( pos ){
				pos->horizontal = (short) bounds.left;
				pos->vertical = (short) bounds.top;
			}
			if( size ){
				size->horizontal = (short) abs(bounds.right - bounds.left);
				size->vertical = (short) abs(bounds.bottom - bounds.top);
			}
		}
		else{
			if( pos ){
				pos->horizontal = wi->theMovieRect.left;
				pos->vertical = wi->theMovieRect.top;
			}
			if( size ){
				if( wi->theMC && MCGetVisible(wi->theMC) ){
				  Rect mcbounds;
					// 20110908: content size is supposed to contain the MC dimensions, not
					// only the movie dimensions. This makes more sense.
					MCGetControllerBoundsRect(wi->theMC, &mcbounds);
					MacOffsetRect( &mcbounds, -mcbounds.left, -mcbounds.top );
					wi->info->controllerHeight = mcbounds.bottom - wi->theMovieRect.bottom;
				}
				else{
					wi->info->controllerHeight = 0;
				}
				size->horizontal = abs(wi->theMovieRect.right - wi->theMovieRect.left);
				size->vertical = abs(wi->theMovieRect.bottom - wi->theMovieRect.top) + wi->info->controllerHeight;
			}
		}
	}
	else{
		err = paramErr;
	}
	return err;
}

ErrCode QTMovieWindowPlay( QTMovieWindowH wih )
{ ErrCode err;
 	if( !QTMovieWindowH_Check(wih) ){
		err = paramErr;
	}
	else{
		StartMovie( (*wih)->theMovie );
		err = GetMoviesError();
#if TARGET_OS_WIN32
		if( (*wih)->theMC ){
			MCMovieChanged( (*wih)->theMC, (*wih)->theMovie );
		}
#endif
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
		StopMovie( (*wih)->theMovie );
		err = GetMoviesError();
#if TARGET_OS_WIN32
		if( (*wih)->theMC ){
			MCMovieChanged( (*wih)->theMC, (*wih)->theMovie );
		}
#endif
		PumpMessages(FALSE);
	}
	return err;
}

#pragma mark ---- housekeeping

/*!
	QuickTime must be initialised before it can be used. On MSWin, one should call
	InitializeQTML() before calling EnterMovies(). When called with kInitializeQTMLEnableDoubleBufferedSurface,
	this function activates a double-buffering screen update procedure that should give a better
	user experience.
 */
static ErrCode lOpenQT()
{ ErrCode err;
#ifdef AllowQTMLDoubleBuffering
  int envLen = 0;
  char *envVal = NULL;
	getenv_s( &envLen, NULL, 0, "QTMW_DoubleBuffering" );
	if( envLen > 0 && (envVal = QTils_calloc( envLen, sizeof(char) )) ){
		getenv_s( &envLen, envVal, envLen, "QTMW_DoubleBuffering" );
		UseQTMLDoubleBuffering = 0;
		if( *envVal ){
			if( atoi(envVal) != 0 ){
				UseQTMLDoubleBuffering = 1;
			}
			else if( strcasecmp(envVal, "true") || strcasecmp(envVal, "vrai") || strcasecmp(envVal,"yes") ){
				UseQTMLDoubleBuffering = 1;
			}
		}
		QTils_free(&envVal);
	}
	// If calling InitializeQML() gives an unhandled exception while debugging mentioning
	// "NtClose a été appelé sur un handle qui était protégé contre la fermeture via NtSetInformationObject."
	// you are probably running Avira AntiVir; deactivate the AntiVir Guard for the debugging session
	// and things should be OK. This is NOT a QuickTime error, but a bug in AntiVir!
	// https://forum.avira.com/wbb/index.php?page=Thread&postID=1031803
	if( UseQTMLDoubleBuffering ){
		err = InitializeQTML(kInitializeQTMLEnableDoubleBufferedSurface);
		Log( qtLogPtr, "InitializeQTML() in double buffering mode returned %d", err );
	}
	else{
		err = InitializeQTML(0);
		Log( qtLogPtr, "InitializeQTML() in default mode returned %d", err );
	}
#else
	UseQTMLDoubleBuffering = 0;
	err = InitializeQTML(0);
	Log( qtLogPtr, "InitializeQTML() in default mode returned %d", err );
#endif //AllowQTMLDoubleBuffering
	if( err == noErr ){
		err = EnterMovies();
	}
	return err;
}

static BOOL QTOpened = FALSE;

ErrCode OpenQT()
{ ErrCode err;
	if( !QTOpened ){
		err = lOpenQT();
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
	if( QTOpened ){
		ExitMovies();
		TerminateQTML();
		QTOpened = FALSE;
	}
}

/*!
	return the screen dimensions as a rectangle
 */
void GetMaxBounds(Rect *maxRect)
{ RECT deskRect;

	GetWindowRect(GetDesktopWindow(), &deskRect);

	OffsetRect(&deskRect, -deskRect.left, -deskRect.top);

	maxRect->top = (short)deskRect.top;
	maxRect->bottom = (short)deskRect.bottom;
	maxRect->left = (short)deskRect.left;
	maxRect->right = (short)deskRect.right;
}

#include "QTilsIconXOR48x48.h"

#ifdef _SS_LOG_ACTIVE
char M2LogEntity[MAX_PATH];
#endif

short InitQTMovieWindows( void *hInst )
{ WNDCLASS wc;
  BOOL ret;
  struct BGRA{
	  BYTE blue, green, red, alpha;
  } *iconXBits;
  BYTE *maskBits = NULL;

#ifdef _SS_LOG_ACTIVE
	if( !qtLogPtr ){
	  int i;
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
		qtLogPtr = Initialise_Log( "QTils Log", M2LogEntity, 0 );
//		for( i = 1 ; i < __argc && __argv ; i++ ){
//			Log( qtLogPtr, "argv[%d] = '%s'\n", i, (__argv[i])? __argv[i] : "<NULL>" );
//		}
		// reuse M2LogEntity:
		strcpy( M2LogEntity, "Modula-2" );
		qtLog_Initialised = 1;
	}
#endif // _SS_LOG_ACTIVE
	QTils_LogInit();

	init_QTMWlists();
	init_HRTime();

	if( ghInst ){
		return TRUE;
	}

	if( !hInst ){
		if( !(ghInst = GetModuleHandle(0)) ){
			FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
			);
			if( qtLogPtr ){
				Log( qtLogPtr, "InitQTMovieWindows() cannot GetModuleHandle(): %d:\"%s\"",
					GetLastError(), errbuf
				);
			}
			return FALSE;
		}
	}
	else{
		ghInst = (HINSTANCE) hInst;
	}

	// Fill in window class structure with parameters that describe
	// the window.
	memset( &wc, 0, sizeof(wc) );
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)QTMWProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	iconXBits = QTils_calloc( gimp_image.width * gimp_image.height, sizeof(struct BGRA) );
//	maskBits = QTils_calloc( ICONMASK_H * ICONMASK_W / 8, sizeof(BYTE) );
	if( !iconXBits /* || !maskBits */ ){
		wc.hIcon = LoadIcon( NULL, IDI_WINLOGO ); // Load default icon
	}
	else{
	// convert a Gimp RGB(a) image into something CreateIcon can eat:
	  int i, j, b, m, intens;
	  size_t len = gimp_image.width * gimp_image.height;
		for( m = b = j = i = 0 ; i < gimp_image.width * gimp_image.height ; i++ ){
//			maskBits[m] |= ((iconMask[m+b]>0)? 1 : 0) << (7-b);
//			if( b == 7 ){
//				m += 1;
//				b = 0;
//			}
//			else{
//				b += 1;
//			}
			intens = (iconXBits[i].red = gimp_image.pixel_data[j++]);
			intens += (iconXBits[i].green = gimp_image.pixel_data[j++]);
			intens += (iconXBits[i].blue = gimp_image.pixel_data[j++]);
			// manage icon transparency. Bits with the alpha channel 0 will
			// show the background; alpha=0xff is 100% opaque.
			if( gimp_image.bytes_per_pixel < 4 ){
				// we take pure white as the 'transparency colour'
				iconXBits[i].alpha = (intens < 3*0xff)? 0xff : 0;
			}
			else{
				iconXBits[i].alpha = gimp_image.pixel_data[j++];
			}
		}
		wc.hIcon = CreateIcon( NULL, gimp_image.width, gimp_image.height, 1, 32,
						  maskBits, iconXBits
		);
	}
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = (LPCSTR) QTMWClass;

	// Register the window class and return success/failure code.
	ret = RegisterClass(&wc);
	if( !ret ){
		FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
		);
		if( qtLogPtr ){
			Log( qtLogPtr, "RegisterClass error %d:\"%s\" in InitQTMovieWindows()\n", GetLastError(), errbuf );
		}
	}
	QTWMcounter = -1;
	if( iconXBits ){
		QTils_free( (char**)&iconXBits);
	}
	if( maskBits ){
		QTils_free(&maskBits);
	}
	return ret;
}

/*!
	activate the given QTMovieWindow, bringing it to the front
 */
ErrCode ActivateQTMovieWindow( QTMovieWindowH wih )
{ QTMovieWindowPtr wi;
  ErrCode err;
	if( QTMovieWindowH_Check(wih) ){
		wi = *wih;
		ShowWindow( wi->theView, SW_RESTORE );
		BringWindowToTop( wi->theView );
		if( wi->theMC ){
			err = (ErrCode) MCActivate( wi->theMC, (WindowRef)GetNativeWindowPort(wi->theView), TRUE );
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

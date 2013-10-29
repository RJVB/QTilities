/*!
 *  @file QTMovieWinWM.cpp
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20101130.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the MSWin32-specific routines and those that depend
 * on them directly.
 *
 */

#include "stdafx.h"

#define _QTILS_C

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("QTMovieWinWM: QuickTime utilities: MSWin32 part of the toolkit");

#define HAS_LOG_INIT
#include "../Logging.h"
#include "../../timing.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <winsock2.h>
#include <Windows.h>
#include <commctrl.h>
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "mswin/resource.h"
#include "mswin/SystemTraySDK.h"

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

#include "QTMovieSinkQTStuff.h"
#include "QTMovieSink.h"

#include "../QTilities.h"
#include "../Lists.h"
#include "../QTMovieWin.h"

#define strdup(s)			_strdup((s))
#define strncasecmp(a,b,n)	_strnicmp((a),(b),(n))
#define strcasecmp(a,b)		_stricmp((a),(b))

#ifndef MIN
#	define	MIN(a,b) (((a)<(b))?(a):(b))
#endif /* MIN */
#ifndef MAX
#	define	MAX(a,b) (((a)>(b))?(a):(b))
#endif /* MIN */

static char errbuf[512];

static HINSTANCE ghInst = NULL;
static BOOL tlsQTOSet = FALSE;
static DWORD tlsQTOpenedKey = 0;

char ProgrammeName[MAX_PATH];
#ifdef _SS_LOG_ACTIVE
	char M2LogEntity[64];
#endif

static WSAReadHandler SocketReadHandler = NULL;
static HANDLE *inputEventObjects = NULL;
static DWORD numInputEventObjects = 0;
static SysTrayEventHandler stOpenHandler = NULL, stAboutHandler = NULL, stFrontHandler = NULL;

CSystemTray *TrayIcon = NULL;
NativeWindow TrayIconTargetWnd = NULL;

QTils_WinMSGs QTils_WinMSG;
#define WM_ICON_NOTIFY	WM_APP+10

static inline CSystemTray *CreateTrayIcon()
{
	if( !TrayIcon ){
		TrayIcon = new CSystemTray( ghInst, (HWND) NULL, (UINT) WM_ICON_NOTIFY, ProgrammeName,
							  ::LoadIcon( ghInst, (LPCTSTR)IDI_SYSTRICON ), IDR_SYSTRAY_MENU, FALSE );
		TrayIconTargetWnd = NULL;
		if( TrayIcon ){
			TrayIcon->HideIcon();
		}
	}
	return TrayIcon;
}

WSAReadHandler SetSocketAsyncReadHandler( WSAReadHandler handler )
{ WSAReadHandler prev = SocketReadHandler;
	SocketReadHandler = handler;
	return prev;
}

ErrCode SetSocketAsyncReadForWindow( SOCKET s, HWND win )
{
	if( WSAAsyncSelect( s, win, 'READ', FD_READ ) == SOCKET_ERROR ){
		return WSAGetLastError();
	}
	else{
		return noErr;
	}
}

ErrCode CreateSocketEventObject( SOCKET s, HANDLE *obj, DWORD mask )
{
	if( s == -1 ){
		return paramErr;
	}
	if( !*obj ){
		*obj = WSACreateEvent();
	}
	if( *obj ){
		WSAEventSelect( s, *obj, mask );
		return noErr;
	}
	else{
		return GetLastError();
	}
}

SysTrayEventHandler SetSysTrayOpenHandler( SysTrayEventHandler handler )
{ SysTrayEventHandler prev = stOpenHandler;
	stOpenHandler = handler;
	return prev;
}

SysTrayEventHandler SetSysTrayAboutHandler( SysTrayEventHandler handler )
{ SysTrayEventHandler prev = stAboutHandler;
	stAboutHandler = handler;
	return prev;
}

SysTrayEventHandler SetSysTrayFrontHandler( SysTrayEventHandler handler )
{ SysTrayEventHandler prev = stFrontHandler;
	stFrontHandler = handler;
	return prev;
}

// a minimal message pump which can be called regularly to make sure MSWindows event messages
// get handled.
int PumpMessages(int force)
{ MSG msg;
  BOOL ok;
  int n = 0;
	if( *errbuf && qtLogPtr ){
		Log( qtLogPtr, errbuf );
		errbuf[0] = '\0';
	}
	FlushLog( qtLogPtr );
	if( force ){
		if( numInputEventObjects ){
		  DWORD idx;
			idx = MsgWaitForMultipleObjects( numInputEventObjects, inputEventObjects,
							  FALSE, INFINITE, QS_POSTMESSAGE);
			if( idx == WAIT_OBJECT_0 + numInputEventObjects ){
				ok = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
			}
			else if( idx == WAIT_FAILED || idx >= WAIT_ABANDONED_0 ){
				FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
				);
				Log( qtLogPtr, errbuf );
			}
			else{
				// inputEventObjects[idx - WAIT_OBJECT_0]
			}
		}
		else{
			ok = GetMessage( &msg, NULL, 0, 0 );
		}
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
	if( msg.message == WM_QUIT ){
		Log( qtLogPtr, "WM_QUIT message received - returning -1" );
		n = -1;
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

static inline void SetMaxPOINT( POINT *p, LONG x, LONG y )
{
	if( x > p->x ){
		p->x = x;
	}
	if( y > p->y ){
		p->y = y;
	}
}

static HICON SmallIconHandle = NULL;

/*!
	the event handler that is specific to our window class:
 */
static LRESULT CALLBACK QTMWProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{ PAINTSTRUCT ps;
  HDC hdc;
  QTMovieWindowH wi = NULL;
  LRESULT ret = 0;
  BOOL returnRet = FALSE;

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
		case WM_CREATE:
			if( SmallIconHandle ){
				SendMessage( hWnd, WM_SETICON, ICON_SMALL, (LPARAM) SmallIconHandle );
			}
#ifdef DEBUG
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( wi ){
				Log( qtLogPtr, "WM_CREATE QT window %p=#%u\n", hWnd, (*wi)->idx );
			}
			else{
				Log( qtLogPtr, "WM_CREATE QT window %p=???\n", hWnd );
			}
#endif
			break;
		case WM_GETMINMAXINFO:{
		  MINMAXINFO *specs = (MINMAXINFO*) lParam;
		  RECT dims;
			// this message is received when a window is being created, but also when a
			// resize/replace is being executed.
			if( !wi && ( (wi = QTMovieWindowHFromNativeWindow(hWnd))
				|| (wi = QTMovieWindowH_from_NativeWindow(hWnd)) )
			){
				(*wi)->handlingEvent += 1;
				GetClientRect( hWnd, &dims );
				// override the sizes in ptMaxTrackSize so that the window can be resized at least to
				// dimensions it once had (size limits are not imposed when creating a window, for instance).
				SetMaxPOINT( &specs->ptMaxTrackSize, dims.right - dims.left, dims.bottom - dims.top );
				SetMaxPOINT( &specs->ptMaxTrackSize, (*wi)->restoreSize.horizontal, (*wi)->restoreSize.vertical );
				Log( qtLogPtr, "WM_GETMINMAXINFO QT window %p=#%u, size=(%d,%d) maxSize=(%ld,%ld)\n",
					hWnd, (*wi)->idx,
					dims.right - dims.left, dims.bottom - dims.top,
					specs->ptMaxTrackSize.x, specs->ptMaxTrackSize.y );
			}
			break;
		}
		case WM_PAINT:{
		  WindowRef wr = GetNativeWindowPort(hWnd);
#ifdef AllowQTMLDoubleBuffering
		  HRGN winUpdateRgn;
#endif //AllowQTMLDoubleBuffering
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( wi ){
#ifdef DEBUG
				Log( qtLogPtr, "Paint QT window %p=#%u\n", hWnd, (*wi)->idx );
#endif
			}
			if( wr ){
#ifdef AllowQTMLDoubleBuffering
				if( UseQTMLDoubleBuffering ){
					winUpdateRgn = CreateRectRgn(0,0,0,0);
					// Get the Windows update region
					GetUpdateRgn( hWnd, winUpdateRgn, FALSE );
				}
#endif // AllowQTMLDoubleBuffering
				// Call BeginPaint/EndPaint to clear the update region
				hdc = BeginPaint( hWnd, &ps );
				EndPaint( hWnd, &ps );

#ifdef AllowQTMLDoubleBuffering
				if( wi && (*wi)->theMovie && UseQTMLDoubleBuffering ){
					// Add to the dirty region of the port any region
					// that Windows says needs updating. This allows the
					// union of the two to be copied from the back buffer
					// to the screen on the next flush.
					QTMLAddNativeRgnToDirtyRgn( wr, winUpdateRgn );
					DeleteObject( winUpdateRgn );

					// Flush the entire dirty region to the screen
					QTMLFlushPortDirtyRgn( wr );
				}
#endif // AllowQTMLDoubleBuffering
			}
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

		case WM_SYSCOMMAND:
			if( !wi && (wi = QTMovieWindowHFromNativeWindow(hWnd)) ){
				(*wi)->handlingEvent += 1;
			}
			if( wi && (*wi)->info && !(*wi)->isZooming ){
				if( wParam == SC_MINIMIZE ){
					(*wi)->sizeState |= QTWM_REDUCED;
				}
				else if( wParam == SC_MAXIMIZE ){
				// we received a maximisation request - the SYSCOMMAND version is thrown *before* the target window is
				// resized by the window manager so we have all options available to us, and also the original dimensions.
				  RECT screenRect;
				  long screenWidth, screenHeight, mWidth, mHeight, scaleX, scaleY, scale;
				  enum { WGROW, WSHRINK, WSHRGROW, WGRWSHRINK } scaleTypeX, scaleTypeY, scaleType;
				  bool toggleMC;
					// since we meddle with the default mechanism, we'll in principle never get the corresponding
					// restore request. Instead, we have to maintain our proper state, and decide whether an unzoom
					// is appropriate:
					if( (*wi)->sizeState & QTWM_ZOOMED ){
						// ahem...
						goto unzoom;
					}
					(*wi)->isZooming = 1;
					// heavy kludge to cope with invisible movie controllers: make it visible for
					// the duration of the procedure
					if( !MCGetVisible( (*wi)->theMC ) ){
						toggleMC = true;
						MCSetVisible( (*wi)->theMC, true );
						// it's quite likely that this call already cancels the actual maximisation request...:
						QTMovieWindowSetGeometry( wi, NULL, NULL, 1.0, 0 );
					}
					else{
						toggleMC = false;
					}
					// get the current (= original, to-restore-to) dimensions:
					QTMovieWindowGetGeometry( wi, &(*wi)->restorePos, &(*wi)->restoreSize, 1 );
					QTMovieWindowGetGeometry( wi, &(*wi)->frameShift, &(*wi)->frameMargin, 0 );
					(*wi)->frameShift.horizontal = (*wi)->restorePos.horizontal - (*wi)->frameShift.horizontal;
					(*wi)->frameShift.vertical = (*wi)->restorePos.vertical- (*wi)->frameShift.vertical;
					// calculate the difference between the movie size and the window size. Note that we
					// do not account for the movie controller here, which is probably the reason we're
					// making it visible above in the first place ...
					(*wi)->frameMargin.horizontal = (*wi)->restoreSize.horizontal - (*wi)->frameMargin.horizontal;
					(*wi)->frameMargin.vertical = (*wi)->restoreSize.vertical- (*wi)->frameMargin.vertical;
					if( !((*wi)->sizeState & QTWM_ZOOMED) ){
						// get the work area, i.e. the current (well, primary?!) screen dimensions
						SystemParametersInfo( SPI_GETWORKAREA, 0, &screenRect, 0 );
						screenWidth = screenRect.right - screenRect.left;
						screenHeight = screenRect.bottom - screenRect.top;
						// determine maximum movie size that's an integer times the natural size
						// fitting on the screen:
						mWidth = (*wi)->info->naturalBounds.right - (*wi)->info->naturalBounds.left;
						mHeight = (*wi)->info->naturalBounds.bottom - (*wi)->info->naturalBounds.top;
						if( screenWidth >= mWidth ){
							scaleX = screenWidth / mWidth;
							scaleTypeX = WGROW;
						}
						else{
							scaleX = (int)( mWidth / screenWidth + 0.5 );
							if( scaleX == 1 ){
								scaleX = 2;
							}
							scaleTypeX = WSHRINK;
						}
						if( screenHeight >= mHeight ){
							scaleY = screenHeight / mHeight;
							scaleTypeY = WGROW;
						}
						else{
							scaleY = (int)( mHeight / screenHeight + 0.5 );
							if( scaleY == 1 ){
								scaleY = 2;
							}
							scaleTypeY = WSHRINK;
						}
						if( scaleTypeX == WGROW && scaleTypeY == WGROW ){
						// the window can grow in both dimensions
							scale = MIN( scaleX, scaleY );
							scaleType = WGROW;
							// check for overflow:
							if( scale > 1
							   && (mWidth * scale + (*wi)->frameMargin.horizontal > screenWidth
								  || mHeight * scale + (*wi)->frameMargin.vertical > screenHeight)
							){
								scale -= 1;
							}
							mWidth *= scale;
							mHeight *= scale;
						}
						else{
						// the window must shrink in both dimensions, even if it could grow in one of the two...
							if( scaleTypeX == WGROW ){
								scale = scaleY;
							}
							else if( scaleTypeY == WGROW ){
								scale = scaleX;
							}
							else{
								scale = MAX( scaleX, scaleY );
							}
							scaleType = WSHRINK;
							mWidth /= scale;
							mHeight /= scale;
						}
						// add the space taken up by window borders, titlebar and movie controller:
						mWidth += (*wi)->frameMargin.horizontal;
						// if the movie controller's visibility was changed since frameMargin was determined
						// the height adjustment will be wrong!
						mHeight += (*wi)->frameMargin.vertical;
						Log( qtLogPtr,
							"%s QT window #%u[%ld,%ld], wParam=%u, size=(%ld,%ld); scale=%ld:%d => window size=(%ld,%ld)\n",
							(message==WM_SIZE)? "WM_SIZE" : "WM_SYSCOMMAND",
							(*wi)->idx,
							(*wi)->info->naturalBounds.right - (*wi)->info->naturalBounds.left,
							(*wi)->info->naturalBounds.bottom - (*wi)->info->naturalBounds.top,
							wParam, screenWidth, screenHeight, scale, scaleType,
							mWidth, mHeight );
						{ Cartesian pos, size;
							size.horizontal = (short) mWidth;
							size.vertical = (short) mHeight;
							pos.horizontal = (screenWidth - mWidth)/2;
							pos.vertical = (screenHeight - mHeight)/2;
							QTMovieWindowSetGeometry( wi, &(*wi)->restorePos, &(*wi)->restoreSize, 1, 1 );
							QTMovieWindowSetGeometry( wi, &pos, &size, 1, 1 );
							if( toggleMC ){
								MCSetVisible( (*wi)->theMC, false );
								QTMovieWindowSetGeometry( wi, NULL, NULL, 1.0, 0 );
							}
						}
						(*wi)->sizeState |= QTWM_ZOOMED;
						// documentation says we should return 0 if we handle a WM_SYSCOMMAND message.
						// As a result, follow-up messages are not received (WM_SIZE or WM_EXITSIZEMOVE)
						returnRet = TRUE;
						ret = 0;
					}
					(*wi)->isZooming = 0;
				}
				else if( wParam == SC_RESTORE ){
					if( (*wi)->sizeState & QTWM_REDUCED ){
						(*wi)->sizeState &= ~QTWM_REDUCED;
					}
					// presumable a window cannot be "unzoomed" when it is in reduced state...
					else if( (*wi)->sizeState & QTWM_ZOOMED ){
unzoom:
					  bool toggleMC;
						(*wi)->isZooming = 1;
						// heavy kludge to cope with invisible movie controllers
						// this is required to restore the window's content to the proper dimensions
						if( !MCGetVisible( (*wi)->theMC ) ){
							toggleMC = true;
							MCSetVisible( (*wi)->theMC, true );
							QTMovieWindowSetGeometry( wi, NULL, NULL, 1.0, 0 );
						}
						else{
							toggleMC = false;
						}
						QTMovieWindowSetGeometry( wi, &(*wi)->restorePos, &(*wi)->restoreSize, 1, 1 );
						if( toggleMC ){
							MCSetVisible( (*wi)->theMC, false );
							QTMovieWindowSetGeometry( wi, NULL, NULL, 1.0, 0 );
						}
						(*wi)->sizeState &= ~QTWM_ZOOMED;
						(*wi)->isZooming = 0;
						returnRet = TRUE;
						ret = 0;
					}
				}
			}
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
			// check for a valid theMC even if we ought never get here without one:
			if ( wi && (*wi)->theMovie && (*wi)->theMC ){
			  RECT clientRect;
			  long widthAdjust = 0, heightAdjust = 0;
				GetClientRect( hWnd, &clientRect );
				widthAdjust = (clientRect.right - clientRect.left) - (*wi)->gOldWindowPos.cx;
				heightAdjust = (clientRect.bottom - clientRect.top) - (*wi)->gOldWindowPos.cy;
				Log( qtLogPtr, "ExitSizeMove QT window #%u, size adjust = (%ld,%ld)\n",
						(*wi)->idx, widthAdjust, heightAdjust );
				if( widthAdjust || heightAdjust ){
				  Rect controllerBox;
					MCGetControllerBoundsRect( (*wi)->theMC, &controllerBox);
					controllerBox.right += (short)widthAdjust;
					controllerBox.bottom += (short)heightAdjust;
					MCSetControllerBoundsRect( (*wi)->theMC, &controllerBox);
				}
			}
			break;
		case 'READ':
			if( SocketReadHandler ){
				(*SocketReadHandler)( (unsigned int*) wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam) );
			}
			break;
		case WM_ICON_NOTIFY:
			if( TrayIcon ){
				ret = TrayIcon->OnTrayNotification(wParam, lParam);
				returnRet = TRUE;
			}
			break;
		case WM_COMMAND:{
		  int wmId, wmEvent;
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			switch( wmId ){
				case IDM_OPEN:
					if( stOpenHandler ){
						ret = (*stOpenHandler)( hWnd, wi );
					}
					else{
						ret = (LRESULT) PostMessage( NULL, QTils_WinMSG.IDM_OPEN_MSG, (WPARAM) wi, 0 );
//						Log( qtLogPtr, "PostMessage(NULL,IDM_OPEN_MSG=0x%x)=%d", QTils_WinMSG.IDM_OPEN_MSG, ret );
					}
					returnRet = TRUE;
					break;
				case IDM_ABOUT:
					if( stAboutHandler ){
						ret = (*stAboutHandler)( hWnd, wi );
					}
					else{
						ret = (LRESULT) PostMessage( NULL, QTils_WinMSG.IDM_ABOUT_MSG, (WPARAM) wi, 0 );
//						Log( qtLogPtr, "PostMessage(NULL,IDM_ABOUT_MSG=0x%x)=%d", QTils_WinMSG.IDM_ABOUT_MSG, ret );
					}
					returnRet = TRUE;
					break;
				case IDM_FRONT:
					if( stFrontHandler ){
						ret = (*stFrontHandler)( hWnd, wi );
					}
					else{
						ret = (LRESULT) PostMessage( NULL, QTils_WinMSG.IDM_FRONT_MSG, (WPARAM) wi, 0 );
//						Log( qtLogPtr, "PostMessage(NULL,IDM_FRONT_MSG=0x%x)=%d", QTils_WinMSG.IDM_FRONT_MSG, ret );
					}
					returnRet = TRUE;
					break;
				case IDM_EXIT:
					PostQuitMessage(0);
					break;
			}
			if( returnRet && !ret ){
				FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
				);
				Log( qtLogPtr, "PostMessage raised error '%s'", errbuf );
			}
			break;
		}
	}
	// we might have closed the wi at this point, so do an additional check
	// to see if we can really decrease the event handling counter!
	if( Handle_Check(wi) ){
		(*wi)->handlingEvent -= 1;
	}

	if( returnRet ){
		return ret;
	}
	else{
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
}

void QTWMflush()
{
#ifdef AllowQTMLDoubleBuffering
	if( UseQTMLDoubleBuffering ){
		QTMLFlushDirtyPorts();
	}
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
	if( Handle_Check(WI) && (*WI)->self == *WI && QTMovieWindowH_from_Movie((*WI)->theMovie) ){
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
			QTils_free(wi->theURL);
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
			if( TrayIconTargetWnd == theView ){
				TrayIcon->SetTargetWnd(NULL);
				TrayIconTargetWnd = NULL;
				TrayIcon->HideIcon();
			}
			DestroyWindow( theView );
		}
		wi->idx = -1;
		err = noErr;
		PumpMessages(FALSE);
	}
	else{
		// 20130602
		unregister_QTMovieWindowH(WI);
	}
	return (ErrCode) err;
}

QTMovieWindowH lastQTWMH = NULL;

static Boolean nothing( DialogPtr dialog, EventRecord *event, DialogItemIndex *itemhit )
{
	return 0;
}

typedef struct BGContextInitFromMovieData {
	QTMovieWindowH wih;
	const char *theURL;
	Movie theMovie;
	QTMovieWindowH wihReturn;
	ErrCode errReturn;
	Handle dataRef;
	OSType dataRefType;
	short resId;
	HANDLE thread;
} BGContextInitFromMovieData;

typedef struct BGContext {
	int workers;
	int (*initQTMW)(QTMovieWindowH,void*);
	void *initQTMWData;
	int initQTMWReturn;
	BGContextInitFromMovieData InitQTMovieWindowHFromMovie;
} BGContext;

void *BGInitQTMovieWindowHFromMovie( void *arg )
{ BGContext *cx = (BGContext*) arg;
	if( cx ){
	  BGContextInitFromMovieData *dat = &cx->InitQTMovieWindowHFromMovie;
		dat->wihReturn = InitQTMovieWindowHFromMovie( dat->wih,
											dat->theURL, dat->theMovie,
											dat->dataRef, dat->dataRefType,
											NULL, dat->resId, &dat->errReturn );
		if( cx->initQTMW ){
			cx->initQTMWReturn = (*cx->initQTMW)( dat->wih, cx->initQTMWData );
		}
		cx->workers -= 1;
		return cx->InitQTMovieWindowHFromMovie.wihReturn;
	}
	else{
		return NULL;
	}
}

QTMovieWindowH InitQTMovieWindowHFromMovieInBG( BGContext *cx,
								  QTMovieWindowH wih, const char *theURL, Movie theMovie,
								  Handle dataRef, OSType dataRefType, DataHandler dh, short resId, ErrCode *err )
{
	cx->InitQTMovieWindowHFromMovie.wih = wih;
	cx->InitQTMovieWindowHFromMovie.theURL = theURL;
	cx->InitQTMovieWindowHFromMovie.theMovie = theMovie;
	cx->InitQTMovieWindowHFromMovie.dataRef = dataRef;
	cx->InitQTMovieWindowHFromMovie.dataRefType = dataRefType;
	cx->InitQTMovieWindowHFromMovie.resId = resId;
	cx->InitQTMovieWindowHFromMovie.wihReturn = NULL;
	if( !(cx->InitQTMovieWindowHFromMovie.thread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)BGInitQTMovieWindowHFromMovie,
													&cx, 0, NULL ))
	){
		return NULL;
	}
	else{
		cx->workers += 1;
		return wih;
	}
}

static NativeWindow CreateNativeWindow( const char *name, short width, short height )
{ NativeWindow theView;
	theView = CreateWindowEx(
			WS_EX_APPWINDOW|WS_EX_WINDOWEDGE, (LPCSTR) QTMWClass,
			(LPCSTR) name, WS_OVERLAPPED|WS_CAPTION|WS_VISIBLE
				|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SYSMENU,
			CW_USEDEFAULT, CW_USEDEFAULT, width, height,
			0, 0, ghInst, NULL
	);
	if( theView ){
		if( CreateTrayIcon() && !TrayIconTargetWnd ){
			TrayIcon->ShowIcon();
			TrayIcon->SetTargetWnd(theView);
			TrayIconTargetWnd = theView;
			TrayIcon->ShowIcon();
		}
	}
	return theView;
}

QTMovieWindowH InitQTMovieWindowH( short width, short height )
{ QTMovieWindows *wi = NULL;
  QTMovieWindowH wih = NULL;
  ErrCode err;

	if( wih = AllocQTMovieWindowH() ){
		wi = *wih;
		if( width > 0 && height > 0 ){
			// 20130612: SetRect( &wi->theMovieRect, 0, 0, width, height ) appears to write beyond
			// the structure bounds??!
			wi->theMovieRect.top = wi->theMovieRect.left = 0;
			wi->theMovieRect.right = width;
			wi->theMovieRect.bottom = height;
		}
		else{
			width = height = CW_USEDEFAULT;
		}
		wi->theView = CreateNativeWindow( ProgrammeName, width, height );
		if( wi->theView ){
			register_QTMovieWindowH( wih, wi->theView );
			PumpMessages(FALSE);
			SetLastError(0);
			if( !SetWindowLongPtr( wi->theView, GWL_USERDATA, (LONG_PTR) wih ) ){
				err = GetLastError();
			}
			PumpMessages(FALSE);
		}
	}
	return wih;
}

static QTMovieWindowH _DisplayMovieInQTMovieWindowH_( Movie theMovie, QTMovieWindowH wih, const char *theURL, short resId,
									    Handle dataRef, OSType dataRefType, int visibleController,
									    ErrCode *err, const char *caller )
{ QTMovieWindows *wi = NULL;
  QTMovieWindowH nwih = NULL, owih = QTMovieWindowH_from_Movie(theMovie);

	if( Handle_Check(wih) && (*wih)->self == *wih ){
		wi = *wih;
		if( theURL && (!wi->theURL || strcmp(theURL, wi->theURL)) ){
			wi->theURL = (const char*) QTils_strdup(theURL);
		}
		else if( wi->theURL ){
			theURL = wi->theURL;
		}
		else if( !theURL && !wi->theURL ){
			theURL = wi->theURL = (const char*) QTils_strdup("*in memory*");
		}
		wi->theMovie = theMovie;
		GetMovieBox( theMovie, &wi->theMovieRect );
		MacOffsetRect( &wi->theMovieRect, -wi->theMovieRect.left, -wi->theMovieRect.top );
		if( !wi->theView || (nwih = QTMovieWindowHFromNativeWindow(wi->theView)) != wih ){
			wi->theView = CreateNativeWindow( theURL,
					wi->theMovieRect.right, wi->theMovieRect.bottom );
		}
		else{
			// no need to correct the geometry; QTMovieWindowSetGeometry() will called further down!
		}
		if( wi->theView ){
		 CGrafPtr currentPort;
		 GDHandle currentGDH;
			// associate theView & wi here so that the window's MINMAXINFO can be stored in wi by
			// the window procedure.
			// internal registry based on a C++ hash_map (see list.cpp)
			register_QTMovieWindowH( wih, wi->theView );
			PumpMessages(FALSE);
			// do what it takes to attach the movie to our new window:
			GetGWorld( &currentPort, &currentGDH );
			// Create GrafPort <-> HWND association
			CreatePortAssociation( wi->theView, NULL, 0);
			// Set the port
			SetGWorld((CGrafPtr)GetNativeWindowPort(wi->theView), nil);
			*err = noErr;
			if( *err != noErr ){
				Log( qtLogPtr, "%s: error %d opening \"%s\"\n", caller, *err, theURL );
				CloseQTMovieWindow(wih);
				DisposeQTMovieWindow(wih);
				wi = NULL, wih = NULL;
			}
			else{
			  BGContext cx;
			  DataHandler dh = NULL;
				// final association between the new movie and its new window:
				SetMovieGWorld( wi->theMovie, (CGrafPtr)GetNativeWindowPort(wi->theView), nil );
				memset( &cx, 0, sizeof(cx) );
				if( owih && owih != wih ){
				  QTMovieWindows *owi = (*owih);
					if( !wi->dataRef ){
						dataRef = owi->dataRef;
						dataRefType = owi->dataRefType;
						dh = owi->dataHandler;
						owi->dataRef = NULL;
						owi->dataHandler = NULL;
					}
					if( !wi->MCActionList ){
						wi->MCActionList = owi->MCActionList;
						owi->MCActionList = NULL;
					}
					if( !wi->theMC ){
						wi->theMC = owi->theMC;
						owi->theMC = NULL;
					}
					unregister_QTMovieWindowH_for_Movie(theMovie);
					owi->theMovie = NULL;
				}
				if( dataRef && wi->dataRef != dataRef ){
					InitQTMovieWindowHFromMovie( wih, theURL, theMovie, dataRef, dataRefType, dh, resId, err );
				}

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
						Log( qtLogPtr, "%s('%s') - prerolling a streaming movie may take some time!\n",
						    caller, theURL
						);
					}
					pErr = PrePrerollMovie( theMovie, timeNow, playRate, NULL, NULL );
					if( pErr != noErr ){
						Log( qtLogPtr, "%s('%s') - PrePrerollMovie returned %d\n",
						    caller, theURL, pErr
						);
					}
					pErr = PrerollMovie( theMovie, timeNow, playRate );
					if( pErr != noErr ){
						Log( qtLogPtr, "%s('%s') - PrerollMovie returned %d\n",
						    caller, theURL, pErr
						);
					}
				}

				wi->theViewPtr = &wi->theView;

				// register the QTMovieWindowPtr with the window we just opened. We use SetProp/Getprop for that
				// internally, as there doesn't seem to be a safe way to check the type of information returned
				// by GetWindowLongPtr() (i.e. we could get someone else's GLW_USERDATA??!)
				SetWindowLongPtr( wi->theView, GWL_USERDATA, (LONG_PTR) wih );
				SetProp( wi->theView, (LPCSTR) "QTMovieWindowH", wih );

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
					Log( qtLogPtr, "%s(\"%s\"): %gs @ %gHz/%gHz starts %02d:%02d:%02d;%d loadState=%d\n",
					    caller, wi->theURL, wi->theInfo.duration, wi->theInfo.frameRate, wi->theInfo.TCframeRate,
					    ft.hours, ft.minutes, ft.seconds, ft.frames,
					    wi->loadState
					);
				}
				Log( qtLogPtr, "%s(): movie=%p MC=%p wih,wi=%p,%p registered with HWND=%p in process %u:%u\n",
				    caller, wi->theMovie, wi->theMC,
				    wih, wi, wi->theView,
				    GetCurrentProcessId(), GetCurrentThreadId()
				);
//				ShowMovieInformation( wi->theMovie, nothing, 0 );
				lastQTWMH = wih;
				if( cx.InitQTMovieWindowHFromMovie.thread ){
					WaitForSingleObject( cx.InitQTMovieWindowHFromMovie.thread, 1000 );
					CloseHandle(cx.InitQTMovieWindowHFromMovie.thread);
				}
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
	else{
		CloseQTMovieWindow(wih);
		DisposeQTMovieWindow(wih);
		wi = NULL, wih = NULL;
		*err = paramErr;
	}
	return wih;
}

ErrCode DisplayMovieInQTMovieWindowH( Movie theMovie, QTMovieWindowH *wih, char *theURL, int visibleController )
{ ErrCode err = paramErr;
	if( theMovie && wih && *wih ){
		if( theURL && !*theURL ){
			theURL = NULL;
		}
		*wih = _DisplayMovieInQTMovieWindowH_( theMovie, *wih, theURL, 1, NULL, 0, visibleController,
									   &err, "DisplayMovieInQTMovieWindowH" );
	}
	return err;
}

ErrCode DisplayMovieInQTMovieWindowH_Mod2( Movie theMovie, QTMovieWindowH *wih, char *theURL, int ulen, int visibleController )
{ ErrCode err = paramErr;
	if( theMovie && wih ){
		if( theURL && !*theURL ){
			theURL = NULL;
		}
		if( !*wih && !(*wih = QTMovieWindowH_from_Movie(theMovie)) ){
			*wih = AllocQTMovieWindowH();
			Log( qtLogPtr, "DisplayMovieInQTMovieWindowH_Mod2: AllocQTMovieWindow() returned *wih=%p", *wih );
		}
		*wih = _DisplayMovieInQTMovieWindowH_( theMovie, *wih, theURL, 1, NULL, 0, visibleController,
									   &err, "DisplayMovieInQTMovieWindowH" );
	}
	return err;
}

QTMovieWindowH OpenQTMovieWindowWithMovie( Movie theMovie, const char *theURL, short resId,
									    Handle dataRef, OSType dataRefType, int visibleController )
{ QTMovieWindowH wih = NULL;
  ErrCode err;

	if( !(wih = QTMovieWindowH_from_Movie(theMovie)) ){
		wih = AllocQTMovieWindowH();
	}
	return _DisplayMovieInQTMovieWindowH_( theMovie, wih, theURL, resId, dataRef, dataRefType, visibleController,
								 &err, "OpenQTMovieWindowWithMovie" );
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

QTMovieWindowH OpenQTMovieWindowWithQTMovieSink( struct QTMovieSinks *qms, int addTCTrack, int controllerVisible )
{ QTMovieWindowH wih = NULL;
	if( qms && qms->privQT ){
		if( close_QTMovieSink( &qms, addTCTrack, NULL, FALSE, FALSE ) == noErr ){
			wih = OpenQTMovieWindowWithMovie( qms->privQT->theMovie, qms->theURL, 1,
									  qms->privQT->dataRef, qms->privQT->dataRefType, controllerVisible );
			if( wih ){
				(*wih)->sourceQMS = qms;
			}
		}
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
				QTils_free( (*wih)->theURL );
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
  RECT wRect;
	if( QTMovieWindowH_isOpen(wih) ){
		wi = *wih;
		w = (WindowRef) GetNativeWindowPort(wi->theView);
		if( setEnvelope ){
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
		  long widthAdjust = 0, heightAdjust = 0;
			// code taken from the WM_ENTER/EXITSIZEMOVE events to have the content follow the window:
			wi->gOldWindowPos.cx = wRect.right - wRect.left;
			wi->gOldWindowPos.cy = wRect.bottom - wRect.top;
			// resize/move the window
			SetWindowPos( wi->theView, NULL, bounds.left, bounds.top, bounds.right, bounds.bottom,
				SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER );
			//wRect.left = bounds.left, wRect.top = bounds.top,
			//	wRect.right = bounds.right, wRect.bottom = bounds.bottom;
			if( GetWindowRect( wi->theView, &wRect ) ){
				// compare the new envelope size with what it was just before. Our handling
				// of the MINMAXINFO message in the window procedure means that we should have received
				// a resized window of the correct dimensions.
				// Alternatively, we could copy 'bounds' into wRect and skip the call to GetWindowRect;
				// in that case MCSetControllerBoundsRect will override the window size limits.
				widthAdjust = (wRect.right - wRect.left) - wi->gOldWindowPos.cx;
				heightAdjust = (wRect.bottom - wRect.top) - wi->gOldWindowPos.cy;
				if( widthAdjust || heightAdjust ){
				  Rect controllerBox, mrect, mcrect;
					// adjust the contents
					MCGetControllerBoundsRect( wi->theMC, &controllerBox);
					// FIXME: it's quite possible we ought to be doing something here if the movie controller
					// is invisible, but it's not immediately clear what
// 					if( !MCGetVisible(wi->theMC) ){
// 						GetMovieBox( wi->theMovie, &mrect );
// 						mcrect = controllerBox;
// 						MacOffsetRect( &mcrect, -mcrect.left, -mcrect.top );
// 						MCextraHeight = mcrect.bottom - mrect.bottom;
// 					}
// 					else{
// 						MCextraHeight = 0;
// 					}
					controllerBox.right += (short)widthAdjust;
					controllerBox.bottom += (short)heightAdjust /* + MCextraHeight*/;
					MCSetControllerBoundsRect( wi->theMC, &controllerBox);
				}
			}
		}
		PortChanged( (GrafPtr) w );
		err = GetMoviesError();
		if( wi->theMC ){
			MCMovieChanged( wi->theMC, wi->theMovie );
			if( !MCIdle(wi->theMC) ){
				MCDraw(wi->theMC, (WindowRef) GetNativeWindowPort(wi->theView) );
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
		if( (*wih)->theMC ){
			MCMovieChanged( (*wih)->theMC, (*wih)->theMovie );
		}
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
		if( (*wih)->theMC ){
			MCMovieChanged( (*wih)->theMC, (*wih)->theMovie );
		}
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
  size_t envLen = 0;
  char *envVal = NULL;
	getenv_s( &envLen, NULL, 0, "QTMW_DoubleBuffering" );
	if( envLen > 0 && (envVal = (char*) QTils_calloc( envLen, sizeof(char) )) ){
		getenv_s( &envLen, envVal, envLen, "QTMW_DoubleBuffering" );
		UseQTMLDoubleBuffering = 0;
		if( *envVal ){
			Log( qtLogPtr, "%%QTMW_DoubleBuffering%%=\"%s\"", envVal );
			if( atoi(envVal) != 0 ){
				UseQTMLDoubleBuffering = 1;
			}
			else if( strcasecmp(envVal, "true") || strcasecmp(envVal, "vrai") || strcasecmp(envVal,"yes") ){
				UseQTMLDoubleBuffering = 1;
			}
		}
		QTils_free(envVal);
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
	if( !UseQTMLDoubleBuffering || err != noErr ){
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

#define QTOpened	TlsGetValue(tlsQTOpenedKey)
static BOOL QTOpenedTrue = TRUE;

ErrCode OpenQT()
{ ErrCode err;
	// fixme: use a thread-specific value instead of a global variable
	if( !QTOpened ){
		err = lOpenQT();
		if( err == noErr ){
			TlsSetValue( tlsQTOpenedKey, &QTOpenedTrue );
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
		TlsSetValue( tlsQTOpenedKey, NULL );
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

short InitQTMovieWindows( void *hInst )
{ WNDCLASSEX wc;
  INITCOMMONCONTROLSEX icc;
  BOOL ret;
  struct BGRA{
	  BYTE blue, green, red, alpha;
  } *iconXBits;
  BYTE *maskBits = NULL;

	// obtain programme name: cf. http://support.microsoft.com/kb/126571
	if( __argv ){
	  char *pName = strrchr(__argv[0], '\\'), *ext = strrchr(__argv[0], '.'), *c, *end;
		c = ProgrammeName;
		if( pName ){
			pName++;
		}
		else{
			pName = __argv[0];
		}
		end = &ProgrammeName[sizeof(ProgrammeName)-1];
		while( pName < ext && c < end ){
			*c++ = *pName++;
		}
		*c++ = '\0';
	}
#ifdef _SS_LOG_ACTIVE
	if( !qtLogPtr ){
		qtLogPtr = Initialise_Log( "QTils Log", ProgrammeName, 0 );
//		for( i = 1 ; i < __argc && __argv ; i++ ){
//			Log( qtLogPtr, "argv[%d] = '%s'\n", i, (__argv[i])? __argv[i] : "<NULL>" );
//		}
		strcpy( M2LogEntity, "Modula-2" );
		qtLog_Initialised = 1;
	}
#endif // _SS_LOG_ACTIVE
	QTils_LogInit();

	init_QTMWlists();
	init_HRTime();
	QTils_WinMSG.IDM_ABOUT_MSG = RegisterWindowMessage("IDM_ABOUT"); 
	QTils_WinMSG.IDM_OPEN_MSG = RegisterWindowMessage("IDM_OPEN"); 
	QTils_WinMSG.IDM_FRONT_MSG = RegisterWindowMessage("IDM_FRONT"); 

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

	// Initialise common controls.
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES|ICC_PROGRESS_CLASS|ICC_BAR_CLASSES|ICC_STANDARD_CLASSES;
	if( !InitCommonControlsEx(&icc) ){
		FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
		);
		if( qtLogPtr ){
			Log( qtLogPtr, "InitCommonControlsEx error %d:\"%s\" in InitQTMovieWindows()\n", GetLastError(), errbuf );
		}
	}

	// Fill in window class structure with parameters that describe
	// the window.
	memset( &wc, 0, sizeof(wc) );
	wc.cbSize = sizeof(wc);
	wc.style = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)QTMWProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = ghInst;
	iconXBits = (struct BGRA*) QTils_calloc( gimp_image.width * gimp_image.height, sizeof(struct BGRA) );
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
						  maskBits, (const BYTE*) iconXBits
		);
	}
	if( ghInst ){
		wc.hIconSm = SmallIconHandle = (HICON) LoadImage( ghInst, MAKEINTRESOURCE(IDI_WINICON), IMAGE_ICON, 16, 16, 0 );
	}
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = (LPCSTR) QTMWClass;
// testing
	wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MYMENU);

	// Register the window class and return success/failure code.
	ret = RegisterClassEx(&wc);
	if( !ret ){
		FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR) errbuf, sizeof(errbuf), NULL
		);
		if( qtLogPtr ){
			Log( qtLogPtr, "RegisterClass error %d:\"%s\" in InitQTMovieWindows()\n", GetLastError(), errbuf );
		}
	}
//	else{
//		if( !TrayIcon ){
//			TrayIcon = new CSystemTray( ghInst, (HWND) NULL, (UINT) WM_ICON_NOTIFY, ProgrammeName,
//							::LoadIcon( ghInst, (LPCTSTR)IDI_SYSTRICON ), IDR_SYSTRAY_MENU, FALSE );
//			TrayIconTargetWnd = NULL;
//			if( TrayIcon ){
//				TrayIcon->HideIcon();
//			}
//		}
//	}

	QTWMcounter = -1;
	if( iconXBits ){
		QTils_free( iconXBits);
	}
	if( maskBits ){
		QTils_free(maskBits);
	}
	if( !tlsQTOSet ){
		tlsQTOpenedKey = TlsAlloc();
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

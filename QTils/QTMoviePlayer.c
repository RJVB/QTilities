#include "copyright.h"
IDENTIFY("QuickTime player based on QTils");

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#	include "winixdefs.h"
#	include <Windows.h>
#else
#	include <unistd.h>
#endif

#include "QTilities.h"

#ifndef TRUE
#	define TRUE		1
#endif
#ifndef FALSE
#	define FALSE	0
#endif


QTMovieWindowH *winlist = NULL;
int numQTMW = 0, MaxnumQTMW = 0;

#define xfree(x)	freep((void**)&x)

int movieStep(QTMovieWindowH wi, void *params )
{
//	if( QTMovieWindowH_Check(wi) ){
//	  double t, t2;
//	  short steps = (short) params;
//		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
//			if( !steps ){
//				QTils_LogMsgEx( "movie #%d STEPPED to t=%gs (abs)\n", (*wi)->idx, t );
//			}
//			else{
//#ifdef DEBUG
//			  MovieFrameTime ft, ft2;
//#endif
//				t2 = t + ((double) steps/(*wi)->info->TCframeRate);
//#ifdef DEBUG
//				secondsToFrameTime( t, (*wi)->info->TCframeRate, &ft );
//				secondsToFrameTime( t2, (*wi)->info->TCframeRate, &ft2 );
//				QTils_LogMsgEx( "Current #%d abs movieTime=%gs=%02d:%02d:%02d;%d; stepping to %gs=%02d:%02d:%02d;%d\n",
//					(*wi)->idx,
//					t, ft.hours, ft.minutes, ft.seconds, ft.frames,
//					t2, ft2.hours, ft2.minutes, ft2.seconds, ft2.frames
//				);
//#else
//				QTils_LogMsgEx( "Current #%d abs movieTime=%gs; stepping to %gs\n", (*wi)->idx, t, t2 );
//#endif
//			}
//		}
//	}
	// if we return TRUE the action is discarded!
	return FALSE;
}

int movieScan(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t, *t2 = (double*) params;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			if( t2 ){
				QTils_LogMsgEx( "Scanning movie #%d from %gs to %gs\n", (*wi)->idx, t,
					+ (*wi)->info->startTime + *t2
				);
			}
			else{
				QTils_LogMsgEx( "Scanning movie #%d from %gs to ??s\n", (*wi)->idx, t );
			}
		}
	}
	// if we return TRUE the action is discarded!
	return FALSE;
}

int moviePlay(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			if( (*wi)->wasScanned > 0 ){
				QTils_LogMsgEx( "movie #%d scanned to t=%gs\n", (*wi)->idx, t );
			}
		}
	}
	return FALSE;
}

int movieStart(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			QTils_LogMsgEx( "movie #%d starting to play at t=%gs\n", (*wi)->idx, t );
		}
	}
	return FALSE;
}

int movieStop(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
	  double t;
		if( QTMovieWindowGetTime(wi,&t,1) == noErr ){
			QTils_LogMsgEx( "movie #%d stops playing at t=%gs\n", (*wi)->idx, t );
		}
	}
	return FALSE;
}

int movieFinished(QTMovieWindowH wi, void *params )
{
	//if( QTMovieWindowH_Check(wi) ){
	//  double tr,ta,tt;
	//  MovieFrameTime ft, ft2, ft3;
	//	QTMovieWindowStop( wi );

	//	QTMovieWindowGetTime(wi,&tr, 0);
	//	QTMovieWindowGetTime(wi,&ta, 1);
	//	QTMovieWindowGetFrameTime(wi, &ft, 1);
	//	secondsToFrameTime( ta, (*wi)->info->TCframeRate, &ft2 );
	//	QTils_LogMsgEx( "Movie #%d finished at t %gs(rel) %gs=%02d:%02d:%02d;%d(abs;%gHz) - effective duration=%gs(abs) theoretical %gs\n",
	//		(*wi)->idx, tr,
	//		ta, ft.hours, ft.minutes, ft.seconds, ft.frames, (*wi)->info->TCframeRate,
	//		ta - (*wi)->info->startTime, (*wi)->info->duration
	//	);
	//	secondsToFrameTime( ta, (*wi)->info->frameRate, &ft3 );
	//	QTils_LogMsgEx( "t (abs) %gs=%02d:%02d:%02d;%d (freq=%gHz)\n",
	//		ta, ft3.hours, ft3.minutes, ft3.seconds, ft3.frames, (*wi)->info->frameRate
	//	);

	//	secondsToFrameTime( (*wi)->info->startTime + (*wi)->info->duration, (*wi)->info->TCframeRate, &ft );
	//	QTMovieWindowSetFrameTime( wi, &ft, 1 );

	//	QTMovieWindowSetTime( wi, (tt=(*wi)->info->startTime + (*wi)->info->duration/2), 1 );
	//	QTMovieWindowGetTime(wi,&tr, 0);
	//	QTMovieWindowGetTime(wi,&ta, 1);

	//	secondsToFrameTime( tt, (*wi)->info->TCframeRate, &ft );
	//	secondsToFrameTime( ta, (*wi)->info->TCframeRate, &ft2 );
	//	QTils_LogMsgEx( "Sent movie to %gs from start: %gs=%02d:%02d:%02d;%d, current time now t=%gs (rel) t=%gs=%02d:%02d:%02d;%d (abs)\n",
	//		(*wi)->info->duration/2, tt,
	//		ft.hours, ft.minutes, ft.seconds, ft.frames,
	//		tr, ta,
	//		ft2.hours, ft2.minutes, ft2.seconds, ft2.frames
	//	);
	//	secondsToFrameTime( ta, (*wi)->info->frameRate, &ft3 );
	//	QTils_LogMsgEx( "t (abs) %gs=%02d:%02d:%02d;%d (freq=%gHz)\n",
	//		ta, ft3.hours, ft3.minutes, ft3.seconds, ft3.frames, (*wi)->info->frameRate
	//	);
	//}
	return FALSE;
}

int movieClose(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) && (*wi)->idx >= 0 && (*wi)->idx < MaxnumQTMW ){
		numQTMW -= 1;
		QTils_LogMsgEx( "Closing movie #%d; %d remaining\n", (*wi)->idx, numQTMW );
		// we can dispose of the window here, or leave it to the library.
		// If we do it here the disposing will probably be incomplete to avoid 'dangling events',
		// (esp. on Mac OS X)
		// so we cannot remove the wi from our local list via
		// winlist[ (*wi)->idx ] = NULL;
		DisposeQTMovieWindow(wi);
		return TRUE;
	}
	return FALSE;
}

void CloseMovies(int final)
{ int i;
	for( i = 0 ; i < MaxnumQTMW ; i++ ){
		if( final ){
			if( winlist[i] ){
				DisposeQTMovieWindow( winlist[i] );
				winlist[i] = NULL;
				numQTMW -= 1;
			}
		}
		else{
			if( QTMovieWindowH_Check(winlist[i]) ){
				CloseQTMovieWindow( winlist[i] );
				numQTMW -= 1;
			}
		}
	}
}

void StopMovies()
{ int i;
	for( i = 0 ; i < MaxnumQTMW ; i++ ){
		if( winlist[i] ){
			QTMovieWindowStop( winlist[i] );
		}
	}
}

int quitRequest = FALSE;
int movieKeyUp(QTMovieWindowH wi, void *params )
{ EventRecord *evt = (EventRecord*) params;
	if( evt ){
		if( evt->message == 'q' || evt->message == 'Q' ){
			StopMovies();
			quitRequest = TRUE;
			return TRUE;
		}
		else if( evt->message == 'C' ){
			QTils_LogMsgEx( "Toggling MC visibility\n" );
			QTMovieWindowToggleMCController(wi);
		}
		else if( evt->message == '1' ){
		  Cartesian size;
			QTMovieWindowGetGeometry( wi, NULL, &size, 0 );
			size.horizontal = (*wi)->info->naturalBounds.right;
			size.vertical = (*wi)->info->naturalBounds.bottom + (*wi)->info->controllerHeight;
			QTMovieWindowSetGeometry( wi, NULL, &size, 1.0, 0 );
			QTMovieWindowGetGeometry( wi, NULL, &size, 0 );
		}
	}
	return FALSE;
}

void doSigExit(int sig)
{
	QTils_LogMsgEx( "Exit on signal #%d\n", sig );
	CloseMovies(TRUE);
	CloseQT();
	exit(sig);
}

void register_wi( QTMovieWindowH wi )
{
	if( wi ){
		if( (*wi)->idx != numQTMW ){
		 // the idx field is purely informational, we can change it at leisure:
			(*wi)->idx = numQTMW;
		}
		winlist[numQTMW] = wi;
		//register_MCAction( wi, MCAction()->Step, movieStep );
		register_MCAction( wi, MCAction()->GoToTime, movieScan );
		register_MCAction( wi, MCAction()->Play, moviePlay );
		register_MCAction( wi, MCAction()->Start, movieStart );
		register_MCAction( wi, MCAction()->Stop, movieStop );
		//register_MCAction( wi, MCAction()->Finished, movieFinished );
		register_MCAction( wi, MCAction()->Close, movieClose );
		register_MCAction( wi, MCAction()->KeyUp, movieKeyUp );
		numQTMW += 1;
		if( numQTMW > MaxnumQTMW ){
			MaxnumQTMW = numQTMW;
		}
	}
}

int vsprintfM2( char *dest, int dlen, const char *format, int flen, ... )
{ va_list ap;
  int n;
  extern int vsnprintf_Mod2( char *dest, int dlen, const char *format, int flen, va_list ap );
	va_start( ap, flen );
	if( !flen ){
		flen = strlen(format);
	}
	n = vsnprintf_Mod2( dest, dlen, format, flen, ap );
	va_end(ap);
	return n;
}

int vsscanfM2( char *src, int slen, const char *format, int flen, ... )
{ va_list ap;
  int n;
  extern int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap );
	va_start( ap, flen );
	if( !flen ){
		flen = strlen(format);
	}
	n = vsscanf_Mod2( src, slen, format, flen, ap );
	va_end(ap);
	return n;
}

int vasprintf( char **str, const char *fmt, va_list ap )
{ va_list original_ap = ap;
  char tbuf[8];
  int n = -1;

	// [v]snprintf into any kind of buffer will return the number of characters
	// that would be printed for an unlimited size buffer:
	n = vsnprintf( tbuf, sizeof(tbuf), fmt, ap );
	va_end(ap);
		
	if( n > -1 ){
		// we now know the required string length; (re)allocate
		*str = realloc( *str, n + 1 );
		ap = original_ap;
		n = vsnprintf( *str, n + 1, fmt, ap );
		va_end(ap);
	}
	return n;
}

int asprintf( char **str, const char *fmt, ... )
{ va_list ap;
  int ret;
	
	va_start(ap, fmt);
	ret = vasprintf( str, fmt, ap );
	va_end(ap);
	return ret;
}

#define ClipInt(x,min,max)	if((x)<(min)){ (x)=(min); } else if((x)>(max)){ (x)=(max); }

void freep( void **p )
{
	if( p && *p ){
		free(*p);
		*p = NULL;
	}
}

int main( int argc, char* argv[] )
{ int i, n;
  QTMovieWindowH wi, ewi;
  unsigned long nMsg = 0, nPumps = 0;
  LibQTilsBase QTils;

#ifdef __APPLE_CC__
	NSApplicationLoad();
#endif

	if( argc > 0 ){
		// we need at least 2 windows
		n = (argc == 1)? 2 : argc;
		winlist = (QTMovieWindowH*) calloc( n, sizeof(QTMovieWindowH*) );
		if( !winlist ){
			perror( "Error allocating windowlist" );
			return -1;
		}
		OpenQT();
		initDMBaseQTils( &QTils );

		// make sure the QTils library uses the same allocator/free routines as we do
		init_QTils_Allocator( malloc, calloc, realloc, freep );

		//ewi = InitQTMovieWindowH( 100, 100 );
		if( argc == 1 ){
			// OpeQTMovieInWindow() will present a dialog if a NULL URL is passed in
			wi = OpenQTMovieInWindow( NULL, 1 );
			// register the window in our local list:
			register_wi(wi);
		}
		else{
			wi = NULL;
			for( i = 1 ; i < argc ; i++ ){
				wi = OpenQTMovieInWindow( argv[i], 1 );
				// register the window in our local list:
				register_wi(wi);
			}
		}
		signal( SIGABRT, doSigExit );
		signal( SIGINT, doSigExit );
		signal( SIGTERM, doSigExit );

		while( numQTMW && !quitRequest ){
		  int n = PumpMessages(1);
			if( n < 0 ){
				quitRequest = 1;
			}
			else{
				nMsg += n;
			}
			nPumps += 1;
		}

		for( i = 0 ; i < MaxnumQTMW ; i++ ){
			DisposeQTMovieWindow( winlist[i] );
			winlist[i] = NULL;
		}
		//DisposeQTMovieWindow(ewi);
		if( winlist ){
			free(winlist);
		}
		QTils_LogMsgEx( "Handled %lu messages in %lu pumpcycles\n", nMsg, nPumps );

		CloseQT();
	}
	return 0;
}

#if defined(_MSC_VER) || defined(__MINGW32__)
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	return main( __argc, __argv );
}
#endif
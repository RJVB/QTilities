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
#	include <libgen.h>
#	include <unistd.h>
#endif

#include "QTilities.h"
#include "StreamEx.h"
#include <vector>

#ifndef TRUE
#	define TRUE		1
#endif
#ifndef FALSE
#	define FALSE	0
#endif

std::vector<QTMovieWindowH> winlist;
int numQTMW = 0, MaxnumQTMW = 0;
LibQTilsBase QTils;

QTMovieWindowH activeWiH = NULL;

#define xfree(x)	freep((void**)&x)

extern "C" {
	extern ErrCode GetMetaDataStringFromMovie( Movie theMovie, AnnotationKeys key, char **value, char **lang );
	extern QTMovieWindowH QTMovieWindowH_from_NativeWindow( NativeWindow hwnd );
}

static std::string metaDataNSStr( Movie theMovie, int trackNr, AnnotationKeys key, const char *keyDescr )
{ static std::string ret("");
  char *value = NULL, lang[16];
	// I prefer to keep QTMoviePlayer a pure QTils product, and avoid calling into the QuickTimes API directly
	// hence the use of the LibQTilsBase instance here.
	if( trackNr > 0 && trackNr <= QTils.GetMovieTrackCount(theMovie) ){
	  size_t len;
		// this could of course be done somewhat more simply by including the QuickTime headers which
		// would allow to call GetMovieIndTrack to obtain a track, rather than calling into
		// QTils.GetMetaDataSringFromTrack which actually points to the Modula-2 version expecting a track NUMBER.
		// Or I could add a GetMetaDataStringFromTrackNr to the QTils API ...
		if( QTils.GetMetaDataStringLengthFromTrack( theMovie, trackNr, key, &len ) == noErr
		   && (value = (char*) calloc( len+1, sizeof(char) ))
		){
			QTils.GetMetaDataStringFromTrack( theMovie, trackNr, key, value, len+1, lang, 15 );
			if( value ){
				if( *value ){
				  std::stringstream nr;
					nr << "Track #" << trackNr << ": ";
#if TARGET_OS_WIN32
					// value will be in UTF8 and should be converted to ANSI
					char *uvalue = strdup(value);
					Utf8ToAnsi( uvalue, value, strlen(uvalue)+1, '*' );
					free(uvalue);
#endif
					ret = nr.str() +
						std::string(keyDescr) + std::string(value) + std::string("\n");
				}
				else{
					ret = "";
				}
			}
		}
	}
	else{
		GetMetaDataStringFromMovie( theMovie, key, &value, NULL );
		if( value ){
			if( *value ){
#if TARGET_OS_WIN32
				// value will be in UTF8 and should be converted to ANSI
				char *uvalue = strdup(value);
				Utf8ToAnsi( uvalue, value, strlen(uvalue)+1, '*' );
				free(uvalue);
#endif
				ret = std::string(keyDescr) + std::string(value) + std::string("\n");
			}
			else{
				ret = "";
			}
		}
	}
	if( value ){
		free(value);
	}
	else{
		ret = "";
	}
	return ret;
}

// Mac OS X basename() can modify the input string when not in 'legacy' mode on 10.6
// and indeed it does. So we use our own which doesn't, and also doesn't require internal
// storage. Handy on win32 as well, where the function simply doesn't exist ...
static const char *lbasename( const char *url )
{ const char *c = NULL;
#if TARGET_OS_MAC
  static const int sep = '/';
#elif TARGET_OS_WIN32
  static const int sep = '\\';
#endif
	if( url ){
		if( (c =  strrchr( url, sep )) ){
			c++;
		}
		else{
			c = url;
		}
	}
	return c;
}

void ShowMetaData(QTMovieWindowH wih)
{ StreamEx<std::stringstream> fullText(""), MetaDataDisplayStr("");
  MovieFrameTime ft;
  Movie theMovie = (*wih)->theMovie;
	secondsToFrameTime( (*wih)->info->startTime, (*wih)->info->frameRate, &ft );
	fullText.asnprintf( 1024, "Movie %s duration %gs; starts at %02d:%02d:%02d;%03d\n",
			    (*wih)->theURL, (*wih)->info->duration,
			    (int) ft.hours, (int) ft.minutes, (int) ft.seconds, (int) ft.frames );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akDisplayName, "Name: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akSource, "Original file: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akCreationDate, "Creation date: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akDescr, "Description: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akInfo, "Information: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, 1, akComment, "Comments: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, -1, akDescr, "Description: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, -1, akInfo, "Information: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, -1, akComment, "Comments: " );
	MetaDataDisplayStr << metaDataNSStr( theMovie, -1, akCreationDate, "Creation date cache/.mov file: " );
	{ long trackNr = 1;
	  OSType trackType, trackSubType, creator;
	  char *cName;
		while( GetMovieTrackNrTypes( theMovie, trackNr, &trackType, &trackSubType ) == noErr ){
			cName = NULL;
			if( trackType == 'vide'
			   && GetMovieTrackNrDecompressorInfo( theMovie, trackNr, &trackSubType, &cName, &creator ) == noErr
			){ 
				MetaDataDisplayStr.asprintf( "Track #%ld, type '%s' codec '%s'",
					trackNr, OSTStr(trackSubType), cName );
				MetaDataDisplayStr.asprintf( " by '%s'\n", OSTStr(creator) );
				QTils_free(cName);
			}
			trackNr += 1;
		}
	}
	fullText << MetaDataDisplayStr.str();
	PostMessageBox( lbasename((*wih)->theURL), fullText.str().c_str() );
}


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

int movieActivate(QTMovieWindowH wi, void *params )
{
	if( QTMovieWindowH_Check(wi) ){
		activeWiH = wi;
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
		else if( evt->message == 'i' || evt->message == 'I' ){
			ShowMetaData(wi);
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
	if( QTMovieWindowH_Check(wi) ){
		if( (*wi)->idx != numQTMW ){
		 // the idx field is purely informational, we can change it at leisure:
			(*wi)->idx = numQTMW;
		}
		winlist.push_back(wi);
		//register_MCAction( wi, MCAction()->Step, movieStep );
		register_MCAction( wi, MCAction()->GoToTime, movieScan );
		register_MCAction( wi, MCAction()->Play, moviePlay );
		register_MCAction( wi, MCAction()->Start, movieStart );
		register_MCAction( wi, MCAction()->Stop, movieStop );
		//register_MCAction( wi, MCAction()->Finished, movieFinished );
		register_MCAction( wi, MCAction()->Close, movieClose );
		register_MCAction( wi, MCAction()->KeyUp, movieKeyUp );
		register_MCAction( wi, MCAction()->Activate, movieActivate );
		numQTMW += 1;
		if( numQTMW > MaxnumQTMW ){
			MaxnumQTMW = numQTMW;
		}
	}
}

// int vsprintfM2( char *dest, int dlen, const char *format, int flen, ... )
// { va_list ap;
//   int n;
//   extern int vsnprintf_Mod2( char *dest, int dlen, const char *format, int flen, va_list ap );
// 	va_start( ap, flen );
// 	if( !flen ){
// 		flen = strlen(format);
// 	}
// 	n = vsnprintf_Mod2( dest, dlen, format, flen, ap );
// 	va_end(ap);
// 	return n;
// }
// 
// int vsscanfM2( char *src, int slen, const char *format, int flen, ... )
// { va_list ap;
//   int n;
//   extern int vsscanf_Mod2( const char *source, int slen, const char *format, int flen, va_list ap );
// 	va_start( ap, flen );
// 	if( !flen ){
// 		flen = strlen(format);
// 	}
// 	n = vsscanf_Mod2( src, slen, format, flen, ap );
// 	va_end(ap);
// 	return n;
// }
// 
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
		*str = (char*) realloc( *str, n + 1 );
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

#if TARGET_OS_MAC
#	ifdef __cplusplus
	extern "C" {
#	endif
#	ifdef __cplusplus
	}
#	endif
#endif

#if TARGET_OS_WIN32
static int FrontHandler( NativeWindow hWnd, QTMovieWindowH wih )
{ int i;
	for( i = 0 ; i < MaxnumQTMW ; i++ ){
		if( QTMovieWindowH_isOpen(winlist[i]) ){
			SetWindowPos( (*winlist[i])->theView, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
		}
	}
	return 1;
}

static int OpenHandler( NativeWindow hWnd, QTMovieWindowH wih )
{
	if( (wih = OpenQTMovieInWindow( NULL, 1 )) ){
		register_wi(wih);
	}
	return 1;
}

static int AboutHandler( NativeWindow hWnd, QTMovieWindowH wih )
{
	if( activeWiH ){
		wih = activeWiH;
	}
	if( QTMovieWindowH_Check(wih) ){
		ShowMetaData(wih);
	}
	return 1;
}
#endif

int main( int argc, char* argv[] )
{ int i, n;
  QTMovieWindowH wi;
  unsigned long nMsg = 0, nPumps = 0;

#if TARGET_OS_MAC
	QTils_ApplicationMain( &argc, &argv );
#endif
	OpenQT();
	initDMBaseQTils( &QTils );
	QTils_LogInit();

// 	StreamEx<std::stringstream> *ss = new StreamEx<std::stringstream>( "e=%g",2.78 );
// 	delete ss;
// 	std::stringstream *sss = new std::stringstream;
// 	*sss << "blabla";
// 	delete sss;
	QTils_LogMsgEx( "%s called with %d argument(s)", argv[0], argc - 1 );
	for( i = 1 ; i < argc ; ++i ){
		QTils_LogMsg( argv[i] );
	}
	PumpMessages(0);
	if( argc > 0 ){
		// we need at least 2 windows
		n = (argc == 1)? 2 : argc;
		winlist.clear();

		// make sure the QTils library uses the same allocator/free routines as we do
		init_QTils_Allocator( malloc, calloc, realloc, freep );

#if defined(_WINDOWS_) || defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
		SetSysTrayOpenHandler(OpenHandler);
		SetSysTrayAboutHandler(AboutHandler);
		SetSysTrayFrontHandler(FrontHandler);
#endif
		if( argc == 1 ){
			QTils_LogMsgEx( "OpenQTMovieInWindow() will present a file selection dialog" );
			// OpeQTMovieInWindow() will present a dialog if a NULL URL is passed in
			wi = OpenQTMovieInWindow( NULL, 1 );
			if( !wi ){
				QTils_LogMsgEx( "Failed to open a movie" );
			}
			// register the window in our local list:
			register_wi(wi);
		}
		else{
			wi = NULL;
			for( i = 1 ; i < argc ; i++ ){
				wi = OpenQTMovieInWindow( argv[i], 1 );
				if( !wi ){
					QTils_LogMsgEx( "Error opening \"%s\"", argv[i] );
				}
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
/*!
 *  @file Lists.cpp
 *  QTils
 *
 *  Created by René J.V. Bertin on 20101122.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 *  This file provides a number of list functions callable from C and based on hash_maps
 * (Google implementation: http://code.google.com/p/google-sparsehash/)
 *
 */

#ifdef __APPLE_CC__
#	include <QuickTime/QuickTime.h>
#else
#	include "windows.h"
#	include <QTML.h>
#	include <Movies.h>
#endif //__APPLE_CC__

#define _QTILS_C
#define _LISTS_CXX

#ifdef _MSC_VER
// somebody wants setjmp.h at least when compiling under VSE-2010, so make
// sure this happens before including QTilities.h !
#	include <setjmp.h>
#endif

#include "QTilities.h"
#include "QTMovieWin.h"

#include "google/dense_hash_map"
//#include "google/sparse_hash_map"

#include "Lists.h"

static int q2nw_initialised = 0, nw2q_initialised = 0, m2qtmwh_initialised = 0;

google::dense_hash_map<QTMovieWindowH, NativeWindow> qtwmh2hwnd;
google::dense_hash_map<NativeWindow, QTMovieWindowH> hwnd2qtwmh;
google::dense_hash_map<Movie, QTMovieWindowH> m2qtmwh;

#if defined(WIN32) || defined(_WINDOWS) || defined(_MSC_VER)
#	include "MacErrorTable.h"
	static int met_initialised = 0;
	//google::sparse_hash_map<ErrCode, MacErrorTables*> metMap;
	google::dense_hash_map<ErrCode, MacErrorTables*> metMap;
#endif

// 20130518 - why would we want to shadow a global variable here??
//static QTMovieWindowH lastQTWMH = NULL;

/*!
	initialise the various google::dense_hash_map "lists" that are used throughout QTils
 */
void init_QTMWlists()
{
	if( !q2nw_initialised ){
		// these two keys (variable names) should never occur:
		qtwmh2hwnd.set_empty_key( (QTMovieWindowH) 0 );
		qtwmh2hwnd.set_deleted_key( (QTMovieWindowH) -1 );
		q2nw_initialised = 1;
	}
	if( !nw2q_initialised ){
		// these two keys (variable names) should never occur:
		hwnd2qtwmh.set_empty_key( (NativeWindow) 0 );
		hwnd2qtwmh.set_deleted_key( (NativeWindow) -1 );
		nw2q_initialised = 1;
	}
	if( !m2qtmwh_initialised ){
		// these two keys (variable names) should never occur:
		m2qtmwh.set_empty_key( (Movie) 0 );
		m2qtmwh.set_deleted_key( (Movie) -1 );
		m2qtmwh_initialised = 1;
	}
#if defined(WIN32) || defined(_WINDOWS) || defined(_MSC_VER)
	if( !met_initialised ){
	  int i;
		// these two keys (variable names) should never occur:
		metMap.set_empty_key( (ErrCode) 65535 );
		metMap.set_deleted_key( (ErrCode) -65535 );
		for( i = 0 ; i < sizeof(macErrorTable) / sizeof(MacErrorTables) ; i++ ){
			metMap[macErrorTable[i].errCode] = &macErrorTable[i];
		}
		met_initialised = 1;
	}
#endif
	lastQTWMH = NULL;
}

const char *GetMacOSStatusErrStrings(ErrCode err, const char **comment)
{
#if defined(WIN32) || defined(_WINDOWS) || defined(_MSC_VER)
  MacErrorTables *et;
	if( met_initialised && metMap.count(err) ){
		if( (et = metMap[err]) ){
			if( comment ){
				*comment = et->errComment;
			}
			return et->errString;
		}
	}
	return NULL;
#else
	if( comment ){
		*comment = GetMacOSStatusCommentString(err);
	}
	return GetMacOSStatusErrorString(err);
#endif
}

/*!
	registers a new QTMovieWindowH object, creating associations between it and the
	(native) window in which it is displayed
 */
void register_QTMovieWindowH( QTMovieWindowH qtwmH, NativeWindow hwnd )
{
	if( qtwmH ){
		if( q2nw_initialised ){
			qtwmh2hwnd[qtwmH] = hwnd;
		}
		if( nw2q_initialised ){
			hwnd2qtwmh[hwnd] = qtwmH;
		}
	}
}

/*!
	find the native window in which a QTMovieWindowH object is displayed.
	This exists mostly for double-check reasons, as the window is also accessible as
	(*qtwmH)->theView
 */
NativeWindow NativeWindow_from_QTMovieWindowH( QTMovieWindowH qtwmH )
{ NativeWindow hwnd;
	if( qtwmH && q2nw_initialised ){
		// some caching to speed things up ...
		if( qtwmH == lastQTWMH && (*lastQTWMH)->theView ){
			return (*lastQTWMH)->theView;
		}
		else if( qtwmh2hwnd.count(qtwmH) ){
			hwnd = qtwmh2hwnd[qtwmH];
			if( (*qtwmH)->theView == hwnd ){
				lastQTWMH = qtwmH;
				return hwnd;
			}
			else if( !(*qtwmH)->theView ){
				// 20110228: a window that has been closed through the GUI but not yet
				// completely flushed from our management. We will need to suppose that
				// it was once a valid window, and thus cannot return NULL.
				return hwnd;
			}
		}
	}
	return NULL;
}

/*!
	look up the QTMovieWindowH that uses the given window for displaying. Used in event processing.
 */
QTMovieWindowH QTMovieWindowH_from_NativeWindow( NativeWindow hwnd )
{ QTMovieWindowH wi;
	if( hwnd2qtwmh.count(hwnd) ){
		wi = hwnd2qtwmh[hwnd];
		if( QTMovieWindowH_Check(wi) && (*wi)->theView == hwnd ){
			return wi;
		}
	}
	return NULL;
}

/*!
	remove a NativeWindow -> QTMovieWindowH association
 */
void unregister_QTMovieWindowH_from_NativeWindow( NativeWindow hwnd )
{
	if( nw2q_initialised && hwnd && hwnd2qtwmh.count(hwnd) ){
		hwnd2qtwmh.erase(hwnd);
		hwnd2qtwmh.resize(0);
	}
}

/*!
	unregisters a QTMovieWindowH object
 */
void unregister_QTMovieWindowH( QTMovieWindowH qtwmH )
{ NativeWindow hwnd;
	if( qtwmH && q2nw_initialised && qtwmh2hwnd.count(qtwmH) ){
		hwnd = qtwmh2hwnd[qtwmH];
		if( (*qtwmH)->theView == hwnd ){
			if( nw2q_initialised && hwnd2qtwmh.count(hwnd) ){
				hwnd2qtwmh.erase(hwnd);
				hwnd2qtwmh.resize(0);
			}
		}
		qtwmh2hwnd.erase(qtwmH);
		qtwmh2hwnd.resize(0);
		if( lastQTWMH == qtwmH ){
			lastQTWMH = NULL;
		}
	}
}

/*!
	associates a Movie with the QTMovieWindowH object that displays it, or that
	is simply used to store additional information about the movie.
 */
QTMovieWindowH register_QTMovieWindowH_for_Movie( Movie movie, QTMovieWindowH qtwmH )
{
	if( !m2qtmwh_initialised ){
		init_QTMWlists();
	}
	if( movie && qtwmH && m2qtmwh_initialised ){
		m2qtmwh[movie] = qtwmH;
		return qtwmH;
	}
	return NULL;
}

/*!
	given a movie, find the associated QTMovieWindowH object
 */
QTMovieWindowH QTMovieWindowH_from_Movie( Movie movie )
{ QTMovieWindowH wi;
	if( movie && m2qtmwh_initialised && m2qtmwh.count(movie) ){
		wi = m2qtmwh[movie];
		if( Handle_Check(wi) && (*wi)->self == *wi && (*wi)->theMovie == movie ){
			return wi;
		}
	}
	return NULL;
}

/*!
	unregisters a QTMovieWindowH from Movie association
 */
void unregister_QTMovieWindowH_for_Movie( Movie movie )
{
	if( movie && m2qtmwh_initialised && m2qtmwh.count(movie) ){
		m2qtmwh.erase(movie);
		m2qtmwh.resize(0);
	}
}


typedef google::dense_hash_map<short, MCActionCallback> MCActionLists;

#if TARGET_OS_MAC
	typedef struct NSMCActionInfo {
		void *target;
		void *selector;
		NSMCActionCallback callback;
	} NSMCActionInfo;
	typedef google::dense_hash_map<short, NSMCActionInfo> NSMCActionLists;
#endif

/*!
	creates and initialises a google::dense_hash_map that will be used to associate MCActions with
	MCActionCallbacks for a given QTMovieWindowH object
 */
void *init_MCActionList()
{ MCActionLists *list = new MCActionLists;
	if( list ){
		// these two keys (variable names) should never occur:
		list->set_empty_key( 0 );
		list->set_deleted_key( -9999 );
	}
	return (void*) list;
}

#if TARGET_OS_MAC
	/*!
		creates and initialises a google::dense_hash_map that will be used to associate MCActions with
		NSMCActionCallbacks for a given QTMovieWindowH object
	 */
	void *init_NSMCActionList()
	{ NSMCActionLists *list = new NSMCActionLists;
		if( list ){
			// these two keys (variable names) should never occur:
			list->set_empty_key( 0 );
			list->set_deleted_key( -9999 );
		}
		return (void*) list;
	}
#endif

/*!
	destroys an MCActionList
 */
void unregister_MCActionList(void *list)
{ MCActionLists *mlist = (MCActionLists*) list;
	if( mlist ){
		delete mlist;
	}
}

#if TARGET_OS_MAC
	/*!
		destroys an NSMCActionList
	 */
	void unregister_NSMCActionList(void *list)
	{ NSMCActionLists *mlist = (NSMCActionLists*) list;
		if( mlist ){
			delete mlist;
		}
	}
#endif

extern "C" {
	extern const MCActions _MCAction_;
}

/*!
	register an MCActionCallback <- MCAction association
 */
void register_MCAction( QTMovieWindowH wi, short action, MCActionCallback callback )
{
	if( action && QTMovieWindowH_Check(wi) && (*wi)->MCActionList ){
	  MCActionLists *mlist = (MCActionLists*) (*wi)->MCActionList;
		(*mlist)[action] = callback;
		if( action == _MCAction_.AnyAction ){
			(*wi)->hasAnyMCAction = 1;
		}
	}
}

#if TARGET_OS_MAC
	/*!
		register an target,selector,NSMCActionCallback <- MCAction association
	 */
	void register_NSMCAction( void *target, QTMovieWindowH wi, short action, void *selector, NSMCActionCallback callback )
	{ NSMCActionInfo info;
		if( action && QTMovieWindowH_Check(wi) && (*wi)->NSMCActionList ){
		  NSMCActionLists *mlist = (NSMCActionLists*) (*wi)->NSMCActionList;
			info.target = target;
			info.selector = selector;
			info.callback = callback;
			(*mlist)[action] = info;
			if( action == _MCAction_.AnyAction ){
				(*wi)->hasAnyMCAction = 1;
			}
		}
	}
#endif

/*!
	returns the MCActionCallback for a given action in a given QTMovieWindowH object,
	or NULL if no callback has been registered
 */
MCActionCallback get_MCAction( QTMovieWindowH wi, short action )
{
	if( action && QTMovieWindowH_Check(wi) && (*wi)->MCActionList ){
	  MCActionLists *mlist = (MCActionLists*) (*wi)->MCActionList;
		if( mlist->count(action) ){
			return (*mlist)[action];
		}
		else{
			return NULL;
		}
	}
	else{
		return NULL;
	}
}

#if TARGET_OS_MAC
	/*!
		returns the NSMCActionCallback for a given action in a given QTMovieWindowH object,
		or NULL if no callback has been registered. The target and selector to be passed on
		to the callback are returned in target_return and selector_return; these are the callback's
		first 2 arguments.
	 */
	NSMCActionCallback get_NSMCAction( QTMovieWindowH wi, short action, void **target_return, void **selector_return )
	{
		if( action && QTMovieWindowH_Check(wi) && (*wi)->NSMCActionList && target_return && selector_return ){
		  NSMCActionLists *mlist = (NSMCActionLists*) (*wi)->NSMCActionList;
		  NSMCActionInfo info;
			if( mlist->count(action) ){
				info = (*mlist)[action];
				*target_return = info.target;
				*selector_return = info.selector;
				return info.callback;
			}
			else{
				return NULL;
			}
		}
		else{
			return NULL;
		}
	}
#endif

/*!
	unregisters an MCActionCallback <- MCAction association
 */
void unregister_MCAction( QTMovieWindowH wi, short action )
{
	if( action && QTMovieWindowH_Check(wi) && (*wi)->MCActionList ){
	  MCActionLists *mlist = (MCActionLists*) (*wi)->MCActionList;
		if( mlist->count(action) ){
			mlist->erase(action);
			mlist->resize(0);
			if( action == _MCAction_.AnyAction ){
				(*wi)->hasAnyMCAction = 1;
			}
		}
	}
}

#if TARGET_OS_MAC
	/*!
		unregisters an target,selector,NSMCActionCallback <- MCAction association
	 */
	void unregister_NSMCAction( QTMovieWindowH wi, short action )
	{
		if( action && QTMovieWindowH_Check(wi) && (*wi)->NSMCActionList ){
		  NSMCActionLists *mlist = (NSMCActionLists*) (*wi)->NSMCActionList;
			if( mlist->count(action) ){
				mlist->erase(action);
				mlist->resize(0);
				if( action == _MCAction_.AnyAction ){
					(*wi)->hasAnyMCAction = 1;
				}
			}
		}
	}
#endif

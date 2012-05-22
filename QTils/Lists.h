/*!
 *  @file Lists.h
 *  QTils
 *
 *  Created by René J.V. Bertin on 20101122.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 */

#ifndef _LISTS_H

#undef QTLSext
#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _LISTS_CXX
#		define QTLSext __declspec(dllexport)
#	else
#		define QTLSext __declspec(dllimport)
#	endif
#else
#	define QTLSext /**/
#endif


#ifdef __cplusplus
extern "C"
{
#endif

extern void init_QTMWlists();
extern void register_QTMovieWindowH( QTMovieWindowH qtwmH, NativeWindow hwnd );
extern NativeWindow NativeWindow_from_QTMovieWindowH( QTMovieWindowH qtwmH );
extern QTMovieWindowH QTMovieWindowH_from_NativeWindow( NativeWindow hwnd );
extern void unregister_QTMovieWindowH( QTMovieWindowH qtwmH );
extern void unregister_QTMovieWindowH_from_NativeWindow( NativeWindow hwnd );

extern QTMovieWindowH register_QTMovieWindowH_for_Movie( Movie movie, QTMovieWindowH qtwmH );
extern QTMovieWindowH QTMovieWindowH_from_Movie( Movie movie );
extern void unregister_QTMovieWindowH_for_Movie( Movie movie );

QTLSext const char *GetMacOSStatusErrStrings(ErrCode err, const char **comment);

extern void *init_MCActionList();
extern void unregister_MCActionList(void *list);
#if TARGET_OS_MAC
	extern void *init_NSMCActionList();
	extern void unregister_NSMCActionList(void *list);
#endif

#ifdef __cplusplus
}
#endif

#define _LISTS_H
#endif

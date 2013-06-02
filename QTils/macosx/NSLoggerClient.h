/*
 *  NSLoggerClient.h
 *  FFusion-RJVB
 *
 *  Created by René J.V. Bertin on 20130210.
 *  Copyright 2013 RJVB. All rights reserved.
 *
 */

#ifndef _NSLOGGERCLIENT_H

#	ifdef __cplusplus
	extern "C" {
#	endif

extern int NSLogvprintf( const char *fileName, int lineNumber, const char *functionName, int doLog,
						  const char *item_name, void *avc, int level, const char *format, va_list ap );
#ifdef __OBJC__
extern int NSLogvprintf2( const char *fileName, int lineNumber, const char *functionName, int doLog,
						  const char *item_name, void *avc, int level, NSString *format, va_list ap );
#endif
extern int NSLogprintf( const char *fileName, int lineNumber, const char *functionName, int doLog, const char *format, ...);

extern void NSLogFlushLog();

extern int SwitchCocoaToMultiThreadedMode();

#	ifdef __cplusplus
	}
#	endif
#define _NSLOGGERCLIENT_H
#endif
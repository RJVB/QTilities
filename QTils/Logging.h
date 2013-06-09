/*
 *  Logging.h
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20100713.
 *  Copyright 2010 INRETS / RJVB. All rights reserved.
 *
 */

#ifndef _LOGGING_H

#include <stdio.h>

#if defined(_WINDOWS) || defined(WIN32) || defined(_MSC_VER) || TARGET_OS_WIN32
#	include "Logger.h"
#	ifdef __cplusplus
extern "C" {
#	endif
#	ifdef _SS_LOG_ACTIVE
#		ifdef HAS_LOG_INIT
			SS_Log *qtLogPtr = NULL;
			int qtLog_Initialised = 0;
#		else
			extern SS_Log *qtLogPtr;
			extern int qtLog_Initialised;
#		endif // HAS_LOG_INIT
#	else
#		define qtLogPtr	NULL
#		define qtLog_Initialised	0
#	endif // _SS_LOG_ACTIVE
#	ifdef __cplusplus
}
#	endif
#else
#	include <libgen.h>
#	include <stdarg.h>
#	include <string.h>
//#	define	Log		fprintf
//#	define	qtLogPtr	stderr
#	ifdef _PC_LOG_ACTIVE
#		ifndef _PCLOGCONTROLLER_H
			extern int vPCLogStatus( void*, void*, ... );
#		endif
		extern char lastSSLogMsg[2048];
		extern void *qtLogName();
		extern void *nsString(char *s);
		static inline int __QTPCLog(void* sender, const char *fileName, int lineNr, char* format, ...)
		{ va_list ap;
		  int ret;
		  extern char lastPCLogMsg[];

		  va_start(ap, format);
		  ret = vPCLogStatus(sender, basename((char*)fileName), lineNr, nsString(format), ap);
		  va_end(ap);
		  strncpy( lastSSLogMsg, (const char*) lastPCLogMsg, sizeof(lastSSLogMsg) );
		  return ret;
		}

#		define	Log(d,f,...)	__QTPCLog((d),__FILE__,__LINE__,(char*)(f), ##__VA_ARGS__)
#		define	qtLogPtr		qtLogName()
#	else // !_PC_LOG_ACTIVE
#		ifdef __OBJC__
#			include "macosx/LoggerClient.h"
#		endif
#		include "macosx/NSLoggerClient.h"
#		ifndef NSLOGGER_WAS_HERE
#			ifdef __cplusplus
				extern "C" {
#			endif
#			ifndef __OBJC__
#				define	BOOL		_Bool
#			endif
			extern BOOL LoggerActive(void*);
			extern void* LoggerStart(void*);
			extern void* LoggerStop(void*);
#			ifdef __cplusplus
				}
#			endif
#		endif
#		define	Log(d,f,...)	NSLogprintf(basename((char*)__FILE__),__LINE__,__FUNCTION__,LoggerActive(NULL),\
								(f), ##__VA_ARGS__)
#		define	qtLogPtr		LoggerActive(NULL)
#	endif // _PC_LOG_ACTIVE
#endif


#define _LOGGING_H
#endif

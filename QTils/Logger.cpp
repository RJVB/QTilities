// (C) 20100630 RJVB, INRETS
// SS_Log facility to C interface

#define _LOGGER_INTERNAL_
#include "Logger.h"

#include <stdio.h>
#include <stdlib.h>
#include "winixdefs.h"
#include <stdarg.h>

#include <list>
using namespace std;

static SS_Log qtLog;

static DWORD filter = LOGTYPE_LOGTOWINDOW|LOGTYPE_TRACE;

SS_Log *Initialise_Log(char *title, char *progName, int erase )
{
	qtLog.Filter( filter );
	qtLog.WindowName( (LPCTSTR)title );
	if( progName && *progName ){
		qtLog.ProgName( (LPCTSTR)progName );
	}
	if( erase ){
		qtLog.EraseLog();
	}
	else{
		Log( &qtLog,
		    (TCHAR*) "*######################== %s ==######################*",
		    qtLog.ProgName()
		);
	}
	Log( &qtLog, (TCHAR *)"%s initalised", title );
	return &qtLog;
}

// It is not a good idea to allow "recursive" logging, something that may happen when logging messages
// from a QuickTime MovieController action handler or callback. Thus, we queue messages arriving while we're
// busy handling a logging request, and flush the queue before we start to handle a new, acceptable request.
static BOOLEAN active = FALSE;
char lastSSLogMsg[2048] = "";

typedef struct logEntries {
	SS_Log *pLog;
	char *msg;
} logEntries;
static list<logEntries> queuedEntries;

static void _FlushLog_( SS_Log *pLog )
{
	if( queuedEntries.size() ){
	  int i, n = queuedEntries.size();
	  logEntries head;
#ifdef _SS_LOG_ACTIVE
		LogStoreFileLine( (TCHAR*) "queued", -1 );
#endif // __SS_LOG_ACTIVE
		for( i = 0 ; i < n ; i++ ){
			head = queuedEntries.front();
			queuedEntries.pop_front();
			// fprintf( stderr, "%d : %s\n", queuedEntries.size(), head.msg );
#ifdef _SS_LOG_ACTIVE
			WriteLog( head.pLog, filter, (TCHAR*) head.msg );
#endif // _SS_LOG_ACTIVE
			free(head.msg);
		}
	}
}

void FlushLog( SS_Log *pLog )
{
	if( !active ){
		_FlushLog_(pLog);
	}
}

void cWriteLog(SS_Log *pLog, char* pMsg, ...)
{ va_list ap;
	va_start( ap, pMsg );
	vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), pMsg, ap );
#ifdef _SS_LOG_ACTIVE
	if( !active ){
		active = TRUE;
		WriteLog( pLog, filter, (TCHAR*) pMsg, &ap );
		active = FALSE;
	}
	else{
	  logEntries entry;
		entry.pLog = pLog;
		entry.msg = strdup(lastSSLogMsg);
		queuedEntries.push_back(entry);
	}
#endif
	va_end(ap);
}

void cWriteLogEx(SS_Log *pLog, char* pMsg, va_list ap)
{
	vsnprintf( lastSSLogMsg, sizeof(lastSSLogMsg), pMsg, ap );
#ifdef _SS_LOG_ACTIVE
	if( !active ){
		active = TRUE;
		WriteLog( pLog, filter, (TCHAR*) pMsg, &ap );
		active = FALSE;
	}
	else{
	  logEntries entry;
		entry.pLog = pLog;
		entry.msg = strdup(lastSSLogMsg);
		queuedEntries.push_back(entry);
	}
#endif
	va_end(ap);
}

void cLogStoreFileLine(SS_Log *pLog, char* szFile, int nLine)
{
	if( !active ){
		active = TRUE;
#ifdef _SS_LOG_ACTIVE
		_FlushLog_(&qtLog);
		pLog->StoreFileLine( (TCHAR*) szFile, nLine );
#endif
		active = FALSE;
	}
}
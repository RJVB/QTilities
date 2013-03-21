// (C) 20100630 RJVB, INRETS

#ifndef _LOGGER_H_

#ifdef _LOGGER_INTERNAL_
#	include "SS_Log_Include.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef _LOGGER_INTERNAL_
	typedef void	SS_Log;
//	typedef struct SS_Log	SS_Log;
#	ifdef _SS_LOG_ACTIVE

#		define Log(d,f,...)		cLogStoreFileLine((d),__FILE__, __LINE__);cWriteLog((d),(f),##__VA_ARGS__)


#	else // _SS_LOG_ACTIVE

#		define Log

#	endif // _SS_LOG_ACTIVE

#endif //_LOGGER_INTERNAL_

extern char lastSSLogMsg[2048];

extern SS_Log *Initialise_Log(char *title, char *progName, int erase);
extern void cWriteLog(SS_Log *pLog, char* pMsg, ...);
extern void cWriteLogEx(SS_Log *pLog, char* pMsg, va_list ap);
extern void cLogStoreFileLine(SS_Log *pLog, char* szFile, int nLine);
extern void FlushLog(SS_Log *pLog);

#ifdef __cplusplus
}
#endif

#define _LOGGER_H_
#endif
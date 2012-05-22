// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <QTML.h>
#include <Movies.h>

#define _QTILS_C

#include "QTilities.h"

#include <stdio.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{ BOOL ret = TRUE;
	switch (ul_reason_for_call)
	{
		// fprintf( stderr, "DllMain(%p,%ld,%p)\n", hModule, ul_reason_for_call, lpReserved );
		case DLL_PROCESS_ATTACH:
			ret = (BOOL) InitQTMovieWindows( (HINSTANCE) hModule );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			CloseQT();
			QTils_LogFinish();
			break;
	}
	return ret;
}


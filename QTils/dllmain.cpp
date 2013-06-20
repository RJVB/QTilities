// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <QTML.h>
#include <Movies.h>

#define _QTILS_C

#include "QTilities.h"

#include <stdio.h>

#include <windows.h>
#include "mswin/resource.h"
#include "mswin/SystemTraySDK.h"
extern CSystemTray *TrayIcon;

extern char ProgrammeName[MAX_PATH];

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
			delete TrayIcon;
			CloseQT();
			QTils_LogFinish();
			break;
	}
	return ret;
}


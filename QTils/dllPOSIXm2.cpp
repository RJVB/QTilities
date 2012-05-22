// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

#define _POSIXm2_C

#include "POSIXm2.h"

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
			ret = (BOOL) InitPOSIXm2( (HINSTANCE) hModule );
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return ret;
}


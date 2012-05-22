// winutils.cpp : Some MSWin features made available to console applications
//

#define _WINUTILS_C

#include "stdafx.h"
#include "winutils.h"

void sleep(short n)
{
	Sleep(n * 1000);
}

int set_realtime()
{
	return (int) SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
}

int exit_realtime()
{
	return (int) SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
}

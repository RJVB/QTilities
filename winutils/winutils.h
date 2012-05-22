#ifndef _WINUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _WINUTILS_C
#		define WINUText _declspec(dllexport)
#	else
#		define WINUText _declspec(dllimport)
#	endif
#else
#	define WINUText /**/
#endif

WINUText void sleep(short n);

WINUText int set_realtime();
WINUText int exit_realtime();


#ifdef __cplusplus
}
#endif

#define _WINUTILS_H
#endif
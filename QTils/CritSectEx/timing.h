/*!
 *  @file timing.h
 *
 *  (C) René J.V. Bertin on 20080926.
 *
 */

#ifndef TIMINGext

#if defined(_MSC_VER)
#	ifdef _TIMING_C
#		define TIMINGext __declspec(dllexport)
#	else
#		define TIMINGext __declspec(dllimport)
#	endif
#else
#	define TIMINGext /**/
#endif

#endif

#ifdef __cplusplus
extern "C"
{
#endif

TIMINGext extern void init_HRTime();
TIMINGext extern double HRTime_Time();
TIMINGext extern double HRTime_tic();
TIMINGext extern double HRTime_toc();

#ifdef __cplusplus
}
#endif

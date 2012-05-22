/*!
 *  @file timing.h
 *
 *  (C) RenE J.V. Bertin on 20080926.
 *
 */

#ifndef _TIMING_H

#if defined(_WIN32) || defined(__WIN32__)
#	ifdef _TIMING_C
#		define TIMINGext __declspec(dllexport)
#	else
#		define TIMINGext __declspec(dllimport)
#	endif
#else
#	define TIMINGext extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif

TIMINGext void init_HRTime();
TIMINGext double HRTime_Time(), HRTime_tic(), HRTime_toc();

#ifdef __cplusplus
}
#endif

#define _TIMING_H
#endif // !_TIMING_H
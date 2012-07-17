/*!
 *  @file msemul4win.h
 *	MSWin glue allowing the emulation of multithreading related functions
 *	from MS Windows on OSX & Linux
 *
 *  Created by René J.V. Bertin on 20120630.
 *  Copyright 2012 RJVB. All rights reserved.
 *
 */

#ifndef _MSEMUL_H

#	include <Windows.h>
#	include <tchar.h>
#	if defined(_MSC_VER)
#		include <intrin.h>
#		define inline			__forceinline
#	elif defined(__MINGW32__) || defined(__MINGW64__)
#		include <intrin.h>
#		define __forceinline	inline
#	else
#		define __forceinline	inline
#	endif

	typedef DWORD	THREAD_RETURN;

#	ifdef __cplusplus
	extern int MSEmul_UseSharedMemory();
extern "C" {
#	endif

	extern int MSEmul_UseSharedMemory(int useShared);
	extern int MSEmul_UsesSharedMemory();
#	ifdef __cplusplus
}
#	endif

/*!
	set the referenced state variable to True in an atomic operation
	(which avoids changing the state while another thread is reading it)
 */
static inline void _InterlockedSetTrue( volatile long *atomic )
{
	if /*while*/( !*atomic ){
		if( !_InterlockedIncrement(atomic) ){
			YieldProcessor();
		}
	}
}

/*!
	set the referenced state variable to False in an atomic operation
	(which avoids changing the state while another thread is reading it)
 */
static inline void _InterlockedSetFalse( volatile long *atomic )
{
	if /*while*/( *atomic ){
		if( _InterlockedDecrement(atomic) ){
			YieldProcessor();
		}
	}
}

#define _MSEMUL_H
#endif

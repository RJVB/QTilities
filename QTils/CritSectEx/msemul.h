/*!
 *  @file msemul.h
 *	emulation of multithreading related functions from MS Windows
 *
 *  Created by René J.V. Bertin on 20111204.
 *  Copyright 2011 RJVB. All rights reserved.
 *
 */

#ifdef SWIG

%module MSEmul
%{
#	if !(defined(WIN32) || defined(_MSC_VER) || defined(__MINGW32__) || defined(SWIGWIN))
#		include "msemul.h"
#	endif
#include "CritSectEx.h"
%}
%include <windows.i>
%include "exception.i"
%exception {
	try {
		$action
	}
	catch( const std::exception& e ){
		SWIG_exception(SWIG_RuntimeError, e.what());
	}
}
#include "CritSectEx.h"

%init %{
	init_HRTime();
%}

%feature("autodoc","3");

#endif //SWIG

#ifndef _MSEMUL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#ifdef linux
#	include <fcntl.h>
#	include <sys/syscall.h>
#endif
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#ifdef __SSE2__
#	include <xmmintrin.h>
#endif

#include "timing.h"

#ifndef __forceinline
#	define __forceinline	inline
#endif
#ifndef __OBJC__
#	define	BOOL		bool
#endif

/*!
	macro defining an infinite timeout wait.
 */
#define	INFINITE		-1

#ifndef __cplusplus
#	ifndef bool
#		define bool		unsigned char
#	endif
#	ifndef false
#		define false		((bool)0)
#	endif
#	ifndef true
#		define true		((bool)1)
#	endif
#endif // !__cplusplus

//#	ifndef DWORD
//		typedef unsigned int	DWORD;
//#	endif

static inline long GetCurrentThreadId()
{
	return (long) pthread_self();
}

typedef void*		PVOID;
#if !defined(__MINGW32__)
#	ifndef DWORD
		typedef uint32_t	DWORD;
#	endif

	/*!
	 the types of MS Windows HANDLEs that we can emulate:
	 */
	typedef enum { MSH_EMPTY, MSH_SEMAPHORE, MSH_MUTEX, MSH_EVENT, MSH_THREAD } MSHANDLETYPE;
#	define CREATE_SUSPENDED	0x00000004

	enum { THREAD_PRIORITY_ABOVE_NORMAL=1, THREAD_PRIORITY_BELOW_NORMAL=-1, THREAD_PRIORITY_HIGHEST=2,
		THREAD_PRIORITY_IDLE=-15, THREAD_PRIORITY_LOWEST=-2, THREAD_PRIORITY_NORMAL=0,
		THREAD_PRIORITY_TIME_CRITICAL=15 };

#ifdef SWIG
#	define HANDLE	MSHANDLE*
#else
	typedef struct MSHANDLE*	HANDLE;
#endif
	/*!
	 an emulated semaphore HANDLE
	 */
	typedef struct MSHSEMAPHORE {
		char *name;
		sem_t *sem;
		pthread_t owner;
	} MSHSEMAPHORE;

	/*!
	 an emulated mutex HANDLE
	 */
	typedef struct MSHMUTEX {
		pthread_mutex_t mutbuf, *mutex;
		pthread_t owner;
	} MSHMUTEX;

	typedef struct MSHEVENT {
		pthread_cond_t condbuf, *cond;
		pthread_mutex_t mutbuf, *mutex;
		pthread_t waiter;
		bool isManual;
		long isSignalled;
	} MSHEVENT;

	/*!
	 an emulated thread HANDLE
	 */
	typedef struct MSHTHREAD {
		pthread_t theThread, *pThread;
#if defined(__APPLE__) || defined(__MACH__)
		mach_port_t machThread;
#else
		HANDLE threadLock;
		HANDLE lockOwner;
#endif
		void *returnValue;
		DWORD threadId, suspendCount;
	} MSHTHREAD;

	typedef union MSHANDLEDATA {
		MSHSEMAPHORE s;
		MSHMUTEX m;
		MSHEVENT e;
		MSHTHREAD t;
	} MSHANDLEDATA;

	/*!
	 an emulated HANDLE of one of the supported types (see MSHANDLETYPE)
	 @n
	 MS Windows uses the same type for a wealth of things, here we are only concerned
	 with its use in the context of multithreaded operations.
	 */
	typedef struct MSHANDLE {
		MSHANDLETYPE type;
		// for SWIG: no support for union-in-struct, so we sacrifice some space and use a struct
		MSHANDLEDATA d;
#	ifdef __cplusplus
	 private:
		DWORD NextThreadID()
		{ static DWORD Id = 1;
			return Id++;
		}
	 public:
		/*!
			initialise a semaphore HANDLE
		 */
		MSHANDLE( void* ign_lpSemaphoreAttributes, long lInitialCount, long ign_lMaximumCount, char *lpName )
		{
			if( lpName && (d.s.sem = sem_open( lpName, O_CREAT, S_IRWXU, lInitialCount)) != SEM_FAILED ){
				d.s.name = lpName;
				d.s.owner = 0;
				type = MSH_SEMAPHORE;
			}
			else{
				type = MSH_EMPTY;
			}
		}
		/*!
			initialise a mutex HANDLE
		 */
		MSHANDLE( void *ign_lpMutexAttributes, BOOL bInitialOwner, char *ign_lpName )
		{
			if( !pthread_mutex_init( &d.m.mutbuf, NULL ) ){
				d.m.mutex = &d.m.mutbuf;
				type = MSH_MUTEX;
				if( bInitialOwner ){
					pthread_mutex_trylock(d.m.mutex);
					d.m.owner = pthread_self();
				}
				else{
					d.m.owner = 0;
				}
			}
			else{
				type = MSH_EMPTY;
			}
		}
		/*!
			initialise an event HANDLE
		 */
		MSHANDLE( void *ign_lpEventAttributes, BOOL bManualReset, BOOL bInitialState, char *ign_lpName )
		{
			if( !pthread_cond_init( &d.e.condbuf, NULL ) ){
				d.e.cond = &d.e.condbuf;
			}
			else{
				d.e.cond = NULL;
			}
			if( d.e.cond && !pthread_mutex_init( &d.e.mutbuf, NULL ) ){
				d.e.mutex = &d.e.mutbuf;
				d.e.waiter = 0;
			}
			else{
				d.e.mutex = NULL;
				if( d.e.cond ){
					pthread_cond_destroy(d.e.cond);
					d.e.cond = NULL;
				}
			}
			if( d.e.cond && d.e.mutex ){
				type = MSH_EVENT;
				d.e.isManual = bManualReset;
				d.e.isSignalled = bInitialState;
			}
			else{
				type = MSH_EMPTY;
			}
		}
		/*!
			initialise a thread HANDLE
		 */
		MSHANDLE( void *ign_lpThreadAttributes, size_t ign_dwStackSize, void *(*lpStartAddress)(void *),
			    void *lpParameter, DWORD dwCreationFlags, DWORD *lpThreadId )
		{
#if defined(__APPLE__) || defined(__MACH__)
			if( (dwCreationFlags & CREATE_SUSPENDED) ){
				if( !pthread_create_suspended_np( &d.t.theThread, NULL, lpStartAddress, lpParameter ) ){
					d.t.pThread = &d.t.theThread;
					d.t.machThread = pthread_mach_thread_np(d.t.theThread);
					d.t.suspendCount = 1;
				}
			}
			else{
				if( !pthread_create( &d.t.theThread, NULL, lpStartAddress, lpParameter ) ){
					d.t.pThread = &d.t.theThread;
					d.t.machThread = pthread_mach_thread_np(d.t.theThread);
					d.t.suspendCount = 0;
				}
			}
#else
		  extern int pthread_create_suspendable( HANDLE mshThread, const pthread_attr_t *attr,
						  void *(*start_routine)(void *), void *arg, bool suspended );
			d.t.threadLock = new MSHANDLE(NULL, false, NULL);
			d.t.lockOwner = NULL;
			if( d.t.threadLock
			   && !pthread_create_suspendable( this, NULL, lpStartAddress, lpParameter, (dwCreationFlags & CREATE_SUSPENDED) )
			){
				d.t.pThread = &d.t.theThread;
			}
			else{
				if( d.t.threadLock ){
					delete d.t.threadLock;
					d.t.threadLock = NULL;
				}
			}
#endif // !__APPLE__ && !__MACH__
			if( d.t.pThread ){
				type = MSH_THREAD;
				d.t.threadId = NextThreadID();
				if( lpThreadId ){
					*lpThreadId = d.t.threadId;
				}
			}
			else{
				type = MSH_EMPTY;
			}
		}
		/*!
			Initialise a HANDLE from an existing pthread identifier
		 */
		MSHANDLE(pthread_t fromThread)
		{
			type = MSH_THREAD;
			d.t.pThread = &d.t.theThread;
			d.t.theThread = fromThread;
#if defined(__APPLE__) || defined(__MACH__)
			if( fromThread == pthread_self() && pthread_main_np() ){
				d.t.threadId = 0;
			}
			else
#elif defined(linux)
			if( fromThread == pthread_self() && syscall(SYS_gettid) == getpid() ){
				d.t.threadId = 0;
			}
			else
#endif
			{
				d.t.threadId = NextThreadID();
			}

		}
		/*!
			HANDLE destructor
		 */
		~MSHANDLE()
		{ bool ret = false;
			switch( type ){
				case MSH_SEMAPHORE:
					if( d.s.sem ){
						sem_post(d.s.sem);
						ret = (sem_close(d.s.sem) == 0);
						if( d.s.name ){
							sem_unlink(d.s.name);
							free(d.s.name);
						}
						ret = true;
					}
					break;
				case MSH_MUTEX:
					if( d.m.mutex ){
						ret = (pthread_mutex_destroy(d.m.mutex) == 0);
					}
					break;
				case MSH_EVENT:
					if( d.e.cond ){
						ret = (pthread_cond_destroy(d.e.cond) == 0);
					}
					if( ret && d.e.mutex ){
						ret = (pthread_mutex_destroy(d.e.mutex) == 0 );
					}
					break;
				case MSH_THREAD:
					if( d.t.pThread ){
						ret = (pthread_join(d.t.theThread, &d.t.returnValue) == 0);
					}
#if !defined(__APPLE__) && !defined(__MACH__)
					if( d.t.threadLock ){
						delete d.t.threadLock;
						d.t.threadLock = NULL;
					}
#endif
					break;
			}
			if( ret ){
				memset( this, 0, sizeof(MSHANDLE) );
			}
		}
#	endif
	} MSHANDLE;
#endif // !__MINGW32__

/*!
 Emulates the Microsoft-specific intrinsic of the same name.
 @n
 compare Destination with Comparand, and replace with Exchange if equal;
 returns the entry value of *Destination.
 @n
 @param Destination	address of the value to be changed potentially
 @param Exchange	new value to store in *Destination if the condition is met
 @param Comparand	the value *Destination must have in order to be replaced with Exchange
 */
static inline long _InterlockedCompareExchange( volatile long *Destination,
							long Exchange,
							long Comparand)
{ long result, old = *Destination;

	__asm__ __volatile__ (
#ifdef __x86_64__
					  "lock; cmpxchgq %q2, %1"
#else
					  "lock; cmpxchgl %2, %1"
#endif
					  : "=a" (result), "=m" (*Destination)
					  : "r" (Exchange), "m" (*Destination), "0" (Comparand));

	return old;
}

#if !defined(__MINGW32__)
/*!
 Performs an atomic compare-and-exchange operation on the specified values.
 The function compares two specified pointer values and exchanges with another pointer
 value based on the outcome of the comparison.
 @n
 To operate on non-pointer values, use the _InterlockedCompareExchange function.
 */
inline PVOID InterlockedCompareExchangePointer( volatile PVOID *Destination,
							PVOID Exchange,
							PVOID Comparand)
{ PVOID result, old = *Destination;

	__asm__ __volatile__ (
#ifdef __x86_64__
					  "lock; cmpxchgq %q2, %1"
#else
					  "lock; cmpxchgl %2, %1"
#endif
					  : "=a" (result), "=m" (*Destination)
					  : "r" (Exchange), "m" (*Destination), "0" (Comparand));

	return old;
}
#endif // !__MINGW32__

/*
 Emulates the Microsoft-specific intrinsic of the same name.
 @n
 Increments the value at the address pointed to by atomic with 1 in locked fashion,
 i.e. preempting any other access to the same memory location
 */
static inline long _InterlockedIncrement( volatile long *atomic )
{
	__asm__ __volatile__ ("lock; addl %1,%0"
			: "=m" (*atomic)
			: "ir" (1), "m" (*atomic));
	return *atomic;
}

/*
 Emulates the Microsoft-specific intrinsic of the same name.
 @n
 Decrements the value at the address pointed to by atomic with 1 in locked fashion,
 i.e. preempting any other access to the same memory location
 */
static inline long _InterlockedDecrement( volatile long *atomic )
{
	__asm__ __volatile__ ("lock; addl %1,%0"
			: "=m" (*atomic)
			: "ir" (-1), "m" (*atomic));
	return *atomic;
}

/*
 Emulates the Microsoft-specific intrinsic of the same name.
 @n
 Signals to the processor to give resources to threads that are waiting for them.
 This macro is only effective on processors that support technology allowing multiple threads
 running on a single processor, such as Intel's Hyperthreading technology.
 */
static inline void YieldProcessor()
{
#ifdef __SSE2__
	_mm_pause();
#else
	__asm__ __volatile__("rep; nop");
//	__asm__ __volatile__("pause");
#endif
}

/*!
 millisecond timer
 */
static inline DWORD GetTickCount()
{
	return (DWORD) (HRTime_Time() * 1000);
}

static inline DWORD GetLastError()
{
	return (DWORD) errno;
}

#if !defined(__MINGW32__)
#	define ZeroMemory(p,s)	memset((p), 0, (s))

#	define WAIT_ABANDONED	0
#	define WAIT_OBJECT_0	1
#	define WAIT_TIMEOUT	2

/*!
 Release the given semaphore.
 @n
 @param ign_lReleaseCount	ignored
 @param ign_lpPreviousCount	ignored
 */
static inline bool ReleaseSemaphore( HANDLE hSemaphore, long ign_lReleaseCount, long *ign_lpPreviousCount )
{ bool ok = false;
	if( hSemaphore && hSemaphore->type == MSH_SEMAPHORE ){
		ok = (sem_post(hSemaphore->d.s.sem) == 0);
		if( ok ){
			hSemaphore->d.s.owner = 0;
		}
	}
	return ok;
}

static inline bool ReleaseMutex(HANDLE hMutex)
{
	if( hMutex && hMutex->type == MSH_MUTEX ){
		if( !pthread_mutex_unlock(hMutex->d.m.mutex) ){
			hMutex->d.m.owner = 0;
			return false;
		}
	}
	return true;
}

static inline void _InterlockedSetTrue( volatile long *atomic );
static inline void _InterlockedSetFalse( volatile long *atomic );

static inline bool SetEvent( HANDLE hEvent )
{ bool ret = false;
	if( hEvent && hEvent->type == MSH_EVENT ){
		_InterlockedSetTrue(&hEvent->d.e.isSignalled);
		if( hEvent->d.e.isManual ){
			ret = (pthread_cond_broadcast(hEvent->d.e.cond) == 0);
		}
		else{
			ret = (pthread_cond_signal(hEvent->d.e.cond) == 0);
		}
	}
	return ret;
}

static inline bool ResetEvent( HANDLE hEvent )
{ bool ret;
	if( hEvent && hEvent->type == MSH_EVENT ){
		_InterlockedSetFalse(&hEvent->d.e.isSignalled);
		ret = true;
	}
	else{
		ret = false;
	}
	return ret;
}

#if defined(TARGET_OS_MAC) && defined(__THREADS__)
#	define GetCurrentThread()	GetCurrentThreadHANDLE()
#endif

#	ifdef __cplusplus
extern "C" {
#	endif
	extern DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
	extern HANDLE CreateSemaphore( void* ign_lpSemaphoreAttributes, long lInitialCount, long ign_lMaximumCount, char *lpName );
	extern HANDLE CreateMutex( void *ign_lpMutexAttributes, BOOL bInitialOwner, char *ign_lpName );
	extern HANDLE msCreateEvent( void *ign_lpEventAttributes, BOOL bManualReset, BOOL bInitialState, char *ign_lpName );
#	define CreateEvent(A,R,I,N)	msCreateEvent((A),(R),(I),(N))
	extern HANDLE CreateThread( void *ign_lpThreadAttributes, size_t ign_dwStackSize, void *(*lpStartAddress)(void *),
				void *lpParameter, DWORD dwCreationFlags, DWORD *lpThreadId );
	extern DWORD ResumeThread( HANDLE hThread );
	extern DWORD SuspendThread( HANDLE hThread );
	extern HANDLE GetCurrentThread();
	extern int GetThreadPriority(HANDLE hThread);
	extern bool SetThreadPriority( HANDLE hThread, int nPriority );
	extern bool CloseHandle( HANDLE hObject );
#	ifdef __cplusplus
}
#	endif
#endif // !__MINGW32__

#define _MSEMUL_H
#endif

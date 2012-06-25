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
#else
#	include <string>
#	include <sstream>
#endif // !__cplusplus

//#	ifndef DWORD
//		typedef unsigned int	DWORD;
//#	endif

typedef void*		PVOID;
typedef void*		LPVOID;
#if !defined(__MINGW32__)
#	ifndef DWORD
		typedef unsigned long	DWORD;
#	endif

	/*!
	 the types of MS Windows HANDLEs that we can emulate:
	 */
	typedef enum { MSH_EMPTY, MSH_SEMAPHORE, MSH_MUTEX, MSH_EVENT, MSH_THREAD, MSH_CLOSED } MSHANDLETYPE;
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
		Microsoft's semaphore SECURITY_ATTRIBUTES
	 */
	typedef struct _SECURITY_ATTRIBUTES {
		DWORD  nLength;			//!< set to sizeof(SECURITY_ATTRIBUTES)
		LPVOID lpSecurityDescriptor;
		BOOL   bInheritHandle;
	} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
	/*!
		OpenSemaphore() must be called with dwDesiredAccess==DELETE|SYNCHRONIZE|SEMAPHORE_MODIFY_STATE
		in order to return an existing named semaphore object on MS Win. This mask is compatible
		with semaphores created with CreateSemaphore's lpSemaphoreAttributes==NULL or a SECURITY_ATTRIBUTES
		structure with lpSecurityDescriptor==NULL .
	 */
	enum SEMAPHORE_SECURITY_ATTRMASKS { DELETE=0x00010000L, SYNCHRONIZE=0x00100000L,
		SEMAPHORE_MODIFY_STATE=0x0002 };
	/*!
		A structure to keep track of the semaphore's count and Microsoft's maximumCount feature
		curCount is required because Mac OS X doesn't have a functional sem_getvalue().
	 */
	typedef struct MSHSEMAPHORECOUNTER {
		long curCount, maxCount;
		unsigned int *refHANDLEp;
#ifdef __cplusplus
		/*!
		 new() operator that allocates from anonymous shared memory - necessary to be able
		 to share semaphore handles among processes
		 */
		void *operator new(size_t size)
		{ extern void *MSEreallocShared( void* ptr, size_t N, size_t oldN );
			return MSEreallocShared( NULL, size, 0 );
		}
		/*!
		 delete operator that frees anonymous shared memory
		 */
		void operator delete(void *p)
		{ extern void MSEfreeShared(void *ptr);
			MSEfreeShared(p);
		}
		MSHSEMAPHORECOUNTER(long curCnt, long maxCnt, unsigned int *refs )
		{
			curCount = curCnt;
			maxCount = maxCnt;
			refHANDLEp = refs;
		}
#endif
	} MSHSEMAPHORECOUNTER;
	/*!
	 an emulated semaphore HANDLE
	 */
	typedef struct MSHSEMAPHORE {
		char *name;
		sem_t *sem;
		MSHSEMAPHORECOUNTER *counter;
		unsigned int refHANDLEs;
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
		void mmfree(void *p)
		{ extern void MSEfreeShared(void *ptr);
			MSEfreeShared(p);
		}
		void Register()
		{ extern void AddSemaListEntry(HANDLE h);
		  void RegisterHANDLE(HANDLE h);
			switch( type ){
				case MSH_SEMAPHORE:
					AddSemaListEntry(this);
					break;
				default:
					break;
			}
			RegisterHANDLE(this);
		}
		void Unregister()
		{ extern void RemoveSemaListEntry(sem_t*);
		  void UnregisterHANDLE(HANDLE h);
			switch( type ){
				case MSH_SEMAPHORE:
					RemoveSemaListEntry(d.s.sem);
					break;
				default:
					break;
			}
			UnregisterHANDLE(this);
		}
	 public:
#pragma mark new MSHANDLE
		/*!
			new() operator that allocates from anonymous shared memory - necessary to be able
			to share semaphore handles among processes
		 */
		void *operator new(size_t size)
		{ extern void *MSEreallocShared( void* ptr, size_t N, size_t oldN );
			return MSEreallocShared( NULL, size, 0 );
		}
		/*!
			delete operator that frees anonymous shared memory
		 */
		void operator delete(void *p)
		{ extern void MSEfreeShared(void *ptr);
			MSEfreeShared(p);
		}
#pragma mark initialise a new semaphore HANDLE
		/*!
			initialise a new semaphore HANDLE
		 */
		MSHANDLE( void* ign_lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, char *lpName )
		{
			type = MSH_EMPTY;
			if( lpName ){
				if( lInitialCount >= 0 && lMaximumCount > 0
				   && (d.s.sem = sem_open( lpName, O_CREAT, S_IRWXU, lInitialCount)) != SEM_FAILED
				){
					d.s.name = lpName;
					d.s.owner = 0;
					d.s.refHANDLEs = 0;
					d.s.counter = new MSHSEMAPHORECOUNTER(lInitialCount, lMaximumCount, &d.s.refHANDLEs);
					type = MSH_SEMAPHORE;
					Register();
				}
			}
		}
#pragma mark initialise a duplicate semaphore HANDLE
		/*!
			initialise a new semaphore HANDLE that references an existing semaphore HANDLE
		 */
		MSHANDLE( sem_t *sema, MSHSEMAPHORECOUNTER *counter, char *lpName )
		{
			d.s.name = lpName;
			d.s.sem = sema;
			d.s.owner = 0;
			d.s.counter = counter;
			*(counter->refHANDLEp) += 1;
			type = MSH_SEMAPHORE;
			Register();
		}
#pragma mark initialise a mutex HANDLE
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
				Register();
			}
			else{
				type = MSH_EMPTY;
			}
		}
#pragma mark initialise an event HANDLE
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
				Register();
			}
			else{
				type = MSH_EMPTY;
			}
		}
#pragma mark initialise a thread HANDLE
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
				Register();
			}
			else{
				type = MSH_EMPTY;
			}
		}
#pragma mark initialise a HANDLE from an existing pthread identifier
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
			Register();
		}
#pragma mark HANDLE destructor
		/*!
			HANDLE destructor
		 */
		~MSHANDLE()
		{ bool ret = false;
			switch( type ){
				case MSH_SEMAPHORE:{
					if( d.s.sem ){
						if( d.s.counter->refHANDLEp != &d.s.refHANDLEs ){
							// just decrement the counter of the semaphore HANDLE we're referring to:
							*(d.s.counter->refHANDLEp) -= 1;
							if( d.s.counter->curCount == 0 ){
								sem_post(d.s.sem);
								d.s.counter->curCount += 1;
							}
							ret == (sem_close(d.s.sem) == 0);
						}
						else if( d.s.refHANDLEs == 0 ){
							if( d.s.counter->curCount == 0 ){
								sem_post(d.s.sem);
								d.s.counter->curCount += 1;
							}
							ret = (sem_close(d.s.sem) == 0);
							if( d.s.name ){
								sem_unlink(d.s.name);
							}
							delete d.s.counter;
							d.s.counter = NULL;
						}
						if( d.s.name ){
							mmfree(d.s.name);
							d.s.name = NULL;
						}
						ret = true;
					}
					break;
				}
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
					else{
						// 20120625: thread already exited, we can set ret=TRUE!
						ret = true;
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
				Unregister();
				memset( this, 0, sizeof(MSHANDLE) );
				type = MSH_CLOSED;
			}
		}
		std::string asString()
		{ std::ostringstream ret;
			switch( type ){
				case MSH_SEMAPHORE:{
				  char *name = (d.s.name)? d.s.name : (char*) "<NULL>";
					if( d.s.counter ){
						ret << "<MSH_SEMAPHORE \"" << name << "\" curCnt=" << d.s.counter->curCount << " " << d.s.refHANDLEs << " references owner=" << d.s.owner << ">";
					}
					else{
						ret << "<MSH_SEMAPHORE \"" << name << "\" curCnt=??" << " " << d.s.refHANDLEs << " references owner=" << d.s.owner << ">";
					}
					break;
				}
				case MSH_MUTEX:
					ret << "<MSH_MUTEX owner=" << d.m.owner << ">";
					break;
				case MSH_EVENT:
					ret << "<MSH_EVENT manual=" << d.e.isManual << " signalled=" << d.e.isSignalled << " waiter=" << d.e.waiter << ">";
					break;
				case MSH_THREAD:
					ret << "<MSH_THREAD thread=" << d.t.theThread << " Id=" << d.t.threadId << ">";
					break;
				default:
					ret << "<Unknown HANDLE>";
					break;
			}
			return ret.str();
		}
#	endif //cplusplus
	} MSHANDLE;
#endif // !__MINGW32__

static inline DWORD GetCurrentProcessId()
{
	return (DWORD) getpid();
}

static inline DWORD GetCurrentThreadId()
{
	return (DWORD) pthread_self();
}

static inline DWORD GetThreadId(HANDLE Thread)
{
	if( Thread && Thread->type == MSH_THREAD ){
		return (DWORD) Thread->d.t.pThread;
	}
	else{
		errno = ESRCH;
		return 0;
	}
}

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

static inline void Sleep(DWORD dwMilliseconds)
{
	usleep( dwMilliseconds * 1000 );
}

#if !defined(__MINGW32__)
#	define ZeroMemory(p,s)	memset((p), 0, (s))

#	define WAIT_ABANDONED	0x00000080L
#	define WAIT_OBJECT_0	0x00000000L
#	define WAIT_TIMEOUT		0x00000102L
#	define WAIT_FAILED		(DWORD) 0xffffffff

/*!
 Release the given semaphore.
 @n
 @param lReleaseCount	increase the semaphore count by this number (must be > 0)
 @param lpPreviousCount	optional: return the count on entry
 */
static inline bool ReleaseSemaphore( HANDLE hSemaphore, long lReleaseCount, long *lpPreviousCount )
{ bool ok = false;
	if( hSemaphore && hSemaphore->type == MSH_SEMAPHORE && lReleaseCount > 0
	   && hSemaphore->d.s.counter->curCount + lReleaseCount <= hSemaphore->d.s.counter->maxCount
	){
		if( lpPreviousCount ){
			*lpPreviousCount = hSemaphore->d.s.counter->curCount;
		}
		lReleaseCount += hSemaphore->d.s.counter->curCount;
		do{
			ok = (sem_post(hSemaphore->d.s.sem) == 0);
				hSemaphore->d.s.counter->curCount += 1;
		} while( ok && hSemaphore->d.s.counter->curCount < lReleaseCount );
#ifdef linux
		{ int cval;
			if( sem_getvalue( hSemaphore->d.s.sem, &cval ) != -1 ){
				if( cval != hSemaphore->d.s.counter->curCount ){
					fprintf( stderr, "ReleaseSemaphore(\"%s\"): value mismatch, %ld != %d\n",
						   hSemaphore->d.s.name, hSemaphore->d.s.counter->curCount, cval
					);
				}
			}
		}
#endif
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
	extern HANDLE CreateSemaphore( void* ign_lpSemaphoreAttributes, long lInitialCount, long lMaximumCount, char *lpName );
	extern HANDLE OpenSemaphore( DWORD ign_dwDesiredAccess, BOOL ign_bInheritHandle, char *lpName );
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

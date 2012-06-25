/*
 *  msemul.cpp
 *  emulation of certain concurrent-programming MS Windows functions for gcc.
	Developed/tested on Mac OS X and Debian 6
 *
 *  Created by René J.V. Bertin on 20111204.
 *  Copyright 2011 IFSTTAR — LEPSIS. All rights reserved.
 *
 */

#include "msemul.h"
#if defined(__APPLE__) || defined(__MACH__)
#	include <mach/thread_act.h>
#endif

#ifdef linux
#	include <fcntl.h>
#	include <sys/time.h>
#endif

#include "CritSectEx.h"

static void cseUnsleep( int sig )
{
//	fprintf( stderr, "SIGALRM\n" );
}

#include <google/dense_hash_map>
#include <sys/mman.h>

int mseShFD = -1;
#define MSESHAREDMEMNAME	"/dev/zero"; //"MSEShMem-XXXXXX"
char MSESharedMemName[64] = "";
static size_t mmapCount = 0;
static BOOL theMSEShMemListReady = FALSE;

typedef google::dense_hash_map<void*,size_t> MSEShMemLists;
static MSEShMemLists theMSEShMemList;

typedef google::dense_hash_map<HANDLE,MSHANDLETYPE> OpenHANDLELists;
static OpenHANDLELists theOpenHandleList;
static google::dense_hash_map<int,const char*> HANDLETypeName;
static BOOL theOpenHandleListReady = FALSE;

static pthread_key_t currentThreadKey = 0;
bool ForceCloseHandle(HANDLE);

void MSEfreeShared(void *ptr)
{ static short calls = 0;
	if( ptr && theMSEShMemList.count(ptr) ){
		size_t N = theMSEShMemList[ptr];
		if( munmap( ptr, N ) == 0 ){
			theMSEShMemList[ptr] = 0;
			theMSEShMemList.erase(ptr);
			if( ++calls >= 32 ){
				theMSEShMemList.resize(0);
				calls = 0;
			}
			mmapCount -= 1;
		}
		ptr = NULL;
	}
	if( mmapCount == 0 ){
		if( mseShFD >= 0 ){
			close(mseShFD);
			if( strcmp( MSESharedMemName, "/dev/zero" ) ){
				shm_unlink(MSESharedMemName);
			}
		}
	}
}

void MSEfreeAllShared()
{ 
	if( currentThreadKey ){
		// this means that at least one invocation to GetCurrentThread was made, and
		// thus that there is a corresponding entry in theOpenHandleList that will not have
		// been removed. Do that now - and of course BEFORE we release dangling memory ...
		ForceCloseHandle( GetCurrentThread() );
	}
	while( theMSEShMemList.size() > 0 ){
	  MSEShMemLists::iterator i = theMSEShMemList.begin();
	  std::pair<void*,size_t> elem = *i;
//		fprintf( stderr, "MSEfreeShared(0x%p) of %lu remaining elements\n", elem.first, theMSEShMemList.size() );
		MSEfreeShared(elem.first);
	}
	if( theOpenHandleListReady && theOpenHandleList.size() ){
		fprintf( stderr, "@@@ Exit with %lu HANDLEs still open\n", theOpenHandleList.size() );
	}
}

void *MSEreallocShared( void* ptr, size_t N, size_t oldN )
{ void *mem;
	int flags = MAP_SHARED;
#ifndef MAP_ANON
	if( mseShFD < 0 ){
		if( !MSESharedMemName[0] ){
			strcpy( MSESharedMemName, MSESHAREDMEMNAME );
// 			mktemp(MSESharedMemName);
		}
 		if( (mseShFD = open( MSESharedMemName, O_RDWR )) < 0 ){
//			fprintf( stderr, "MSEreallocShared(): can't open/create descriptor for allocating %s=0x%lx, size %s=%lu -> 0x%lx (%s)\n",
//				   (name)? name : "<unknown>", ptr, size, (unsigned long) N, mem, serror()
//			);
			return NULL;
		}
	}
#else
	flags |= MAP_ANON;
#endif
	mem = mmap( NULL, N, (PROT_READ|PROT_WRITE), flags, mseShFD, 0 );
	if( mem ){
		memset( mem, 0, N );
		mmapCount += 1;
		if( !theMSEShMemListReady ){
			theMSEShMemList.set_empty_key(NULL);
			theMSEShMemList.set_deleted_key( (void*)-1 );
			atexit(MSEfreeAllShared);
			theMSEShMemListReady = TRUE;
		}
		theMSEShMemList[mem] = N;
		if( ptr ){
			memmove( mem, ptr, N );
			if( munmap( ptr, oldN ) == 0 ){
				mmapCount -= 1;
			}
		}
	}
	return( mem );
}

static char *mmstrdup( char *str )
{ char *ret = NULL;
	if( (ret = (char*) MSEreallocShared(NULL, strlen(str), 0 )) ){
		strcpy( ret, str );
	}
	return ret;
}

void RegisterHANDLE(HANDLE h)
{
	if( !theOpenHandleListReady ){
		theOpenHandleList.set_empty_key(NULL);
		theOpenHandleList.set_deleted_key( (HANDLE)-1 );
		HANDLETypeName.set_empty_key(-1);
		HANDLETypeName.set_deleted_key(-2);
		HANDLETypeName[MSH_EMPTY] = "MSH_EMPTY";
		HANDLETypeName[MSH_SEMAPHORE] = "MSH_SEMAPHORE";
		HANDLETypeName[MSH_MUTEX] = "MSH_MUTEX";
		HANDLETypeName[MSH_EVENT] = "MSH_EVENT";
		HANDLETypeName[MSH_THREAD] = "MSH_THREAD";
		HANDLETypeName[MSH_CLOSED] = "MSH_CLOSED";
		theOpenHandleListReady = TRUE;
	}
	theOpenHandleList[h] = h->type;
//	fprintf( stderr, "@@ Registering HANDLE 0x%p (type %d %s)\n", h, h->type, h->asString().c_str() );
}

void UnregisterHANDLE(HANDLE h)
{
	if( theOpenHandleListReady && theOpenHandleList.count(h) ){
		theOpenHandleList.erase(h);
//		fprintf( stderr, "@@ Unregistering HANDLE 0x%p (type %d %s)\n", h, h->type, h->asString().c_str() );
	}
}

#ifndef __MINGW32__
/*!
 Emulates the Microsoft function of the same name:
 @n
 Wait for an event to occur on hHandle, for a maximum of dwMilliseconds
 @n
 semaphore: wait to lock the semaphore
 @n
 mutex: wait to lock the mutex
 @n
 Returns WAIT_OBJECT_0 on success, WAIT_TIMEOUT on a timeout and WAIT_ABANDONED on error.
 */
DWORD WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds )
{
	if( !hHandle ){
		return WAIT_ABANDONED;
	}

	if( dwMilliseconds != (DWORD) -1 ){
	  struct timespec timeout;
#ifdef __APPLE__
	  struct sigaction h, oh;
	  struct itimerval rtt, ortt;
		if( hHandle->type != MSH_EVENT ){
			h.__sigaction_u.__sa_handler = cseUnsleep;
			rtt.it_value.tv_sec= (unsigned long) (dwMilliseconds/1000);
			rtt.it_value.tv_usec= (unsigned long) ( (dwMilliseconds- rtt.it_value.tv_sec*1000)* 1000 );
			rtt.it_interval.tv_sec= 0;
			rtt.it_interval.tv_usec= 0;
			if( sigaction( SIGALRM, &h, &oh ) ){
//				fprintf( stderr, "Error calling sigaction: %s\n", strerror(errno) );
				return WAIT_ABANDONED;
			}
			setitimer( ITIMER_REAL, &rtt, &ortt );
		}
		else{
		  struct timeval tv;
		  time_t sec = (time_t) (dwMilliseconds/1000);
			gettimeofday(&tv, NULL);
			timeout.tv_sec = tv.tv_sec + sec;
			timeout.tv_nsec = tv.tv_usec * 1000 + ( (dwMilliseconds- sec*1000)* 1000000 );
			while( timeout.tv_nsec > 999999999 ){
				timeout.tv_sec += 1;
				timeout.tv_nsec -= 1000000000;
			}
		}
#else
	  	clock_gettime( CLOCK_REALTIME, &timeout );
	  	{ time_t sec = (time_t) (dwMilliseconds/1000);
			timeout.tv_sec += sec;
			timeout.tv_nsec += (long) ( (dwMilliseconds- sec*1000)* 1000000 );
			while( timeout.tv_nsec > 999999999 ){
				timeout.tv_sec += 1;
				timeout.tv_nsec -= 1000000000;
			}
	  	}
#endif
		switch( hHandle->type ){
			case MSH_SEMAPHORE:
#ifdef __APPLE__
				if( sem_wait(hHandle->d.s.sem) ){
//					fprintf( stderr, "sem_wait error %s\n", strerror(errno) );
					setitimer( ITIMER_REAL, &ortt, &rtt );
					return (errno == EINTR)? WAIT_TIMEOUT : WAIT_ABANDONED;
				}
				else{
					setitimer( ITIMER_REAL, &ortt, &rtt );
					hHandle->d.s.owner = pthread_self();
					if( hHandle->d.s.counter->curCount > 0 ){
						hHandle->d.s.counter->curCount -= 1;
#ifdef linux
						{ int cval;
							if( sem_getvalue( hHandle->d.s.sem, &cval ) != -1 ){
								if( cval != hHandle->d.s.counter->curCount ){
									fprintf( stderr, "WaitForSingleObject(\"%s\"): value mismatch, %ld != %d\n",
										   hHandle->d.s.name, hHandle->d.s.counter->curCount, cval
									);
								}
							}
						}
#endif
					}
					return WAIT_OBJECT_0;
				}
#else
				if( sem_timedwait(hHandle->d.s.sem, &timeout) ){
//					fprintf( stderr, "sem_timedwait error %s\n", strerror(errno) );
					return (errno == ETIMEDOUT)? WAIT_TIMEOUT : WAIT_ABANDONED;
				}
				else{
					hHandle->d.s.owner = pthread_self();
					if( hHandle->d.s.counter->curCount > 0 ){
						hHandle->d.s.counter->curCount -= 1;
#ifdef linux
						{ int cval;
							if( sem_getvalue( hHandle->d.s.sem, &cval ) != -1 ){
								if( cval != hHandle->d.s.counter->curCount ){
									fprintf( stderr, "WaitForSingleObject(\"%s\"): value mismatch, %ld != %d\n",
										   hHandle->d.s.name, hHandle->d.s.counter->curCount, cval
									);
								}
							}
						}
#endif
					}
					return WAIT_OBJECT_0;
				}
#endif
				break;
			case MSH_MUTEX:
#ifdef __APPLE__
				if( pthread_mutex_lock( hHandle->d.m.mutex ) ){
//					fprintf( stderr, "pthread_mutex_lock error %s\n", strerror(errno) );
					setitimer( ITIMER_REAL, &ortt, &rtt );
					return (errno == EINTR)? WAIT_TIMEOUT : WAIT_ABANDONED;
				}
				else{
					setitimer( ITIMER_REAL, &ortt, &rtt );
					hHandle->d.m.owner = pthread_self();
					return WAIT_OBJECT_0;
				}
#else
				if( pthread_mutex_timedlock( hHandle->d.m.mutex, &timeout ) ){
//					fprintf( stderr, "pthread_mutex_timedlock error %s\n", strerror(errno) );
					return (errno == ETIMEDOUT)? WAIT_TIMEOUT : WAIT_ABANDONED;
				}
				else{
					hHandle->d.m.owner = pthread_self();
					return WAIT_OBJECT_0;
				}
#endif
				break;
			case MSH_EVENT:{
			  int err;
				if( hHandle->d.e.isSignalled ){
					if( !hHandle->d.e.isManual ){
						_InterlockedSetFalse(hHandle->d.e.isSignalled);
					}
					return WAIT_OBJECT_0;
				}
				if( pthread_mutex_lock( hHandle->d.e.mutex ) == 0 ){
					hHandle->d.e.waiter = pthread_self();
					errno = 0;
					if( (err = pthread_cond_timedwait( hHandle->d.e.cond, hHandle->d.e.mutex, &timeout )) ){
						pthread_mutex_unlock( hHandle->d.e.mutex );
						hHandle->d.e.waiter = 0;
						return (err == ETIMEDOUT || errno == ETIMEDOUT)? WAIT_TIMEOUT : WAIT_ABANDONED;
					}
					else{
						pthread_mutex_unlock( hHandle->d.e.mutex );
						hHandle->d.e.waiter = 0;
						if( !hHandle->d.e.isManual ){
							_InterlockedSetFalse(hHandle->d.e.isSignalled);
						}
						return WAIT_OBJECT_0;
					}
				}
				else{
					return WAIT_ABANDONED;
				}
				break;
			}
			case MSH_THREAD:
				if( pthread_join( hHandle->d.t.theThread, &hHandle->d.t.returnValue ) ){
//					fprintf( stderr, "pthread_join error %s\n", strerror(errno) );
#ifdef __APPLE__
					setitimer( ITIMER_REAL, &ortt, &rtt );
#endif
					return (errno == EINTR)? WAIT_TIMEOUT : WAIT_ABANDONED;
				}
				else{
					hHandle->d.t.pThread = NULL;
#ifdef __APPLE__
					setitimer( ITIMER_REAL, &ortt, &rtt );
#endif
					return WAIT_OBJECT_0;
				}
				break;
		}
	}
	else switch( hHandle->type ){
		case MSH_SEMAPHORE:
			if( sem_wait(hHandle->d.s.sem) ){
//				fprintf( stderr, "sem_wait error %s\n", strerror(errno) );
				return (errno == EINTR)? WAIT_TIMEOUT : WAIT_ABANDONED;
			}
			else{
				hHandle->d.s.owner = pthread_self();
				return WAIT_OBJECT_0;
			}
			break;
		case MSH_MUTEX:
			if( pthread_mutex_lock( hHandle->d.m.mutex ) ){
//				fprintf( stderr, "pthread_mutex_lock error %s\n", strerror(errno) );
				return WAIT_ABANDONED;
			}
			else{
				hHandle->d.m.owner = pthread_self();
				return WAIT_OBJECT_0;
			}
			break;
		case MSH_EVENT:
			if( hHandle->d.e.isSignalled ){
				if( !hHandle->d.e.isManual ){
					_InterlockedSetFalse(hHandle->d.e.isSignalled);
				}
				return WAIT_OBJECT_0;
			}
			// get the mutex and then wait for the condition to be signalled:
			if( pthread_mutex_lock( hHandle->d.e.mutex ) == 0 ){
				hHandle->d.e.waiter = pthread_self();
				if( pthread_cond_wait( hHandle->d.e.cond, hHandle->d.e.mutex ) ){
					pthread_mutex_unlock( hHandle->d.e.mutex );
					hHandle->d.e.waiter = 0;
					return WAIT_ABANDONED;
				}
				else{
					pthread_mutex_unlock( hHandle->d.e.mutex );
					hHandle->d.e.waiter = 0;
					if( !hHandle->d.e.isManual ){
						_InterlockedSetFalse(hHandle->d.e.isSignalled);
					}
					return WAIT_OBJECT_0;
				}
			}
			else{
				return WAIT_ABANDONED;
			}
			break;
		case MSH_THREAD:
			if( pthread_join( hHandle->d.t.theThread, &hHandle->d.t.returnValue ) ){
				return WAIT_ABANDONED;
			}
			else{
				hHandle->d.t.pThread = NULL;
				return WAIT_OBJECT_0;
			}
			break;
	}
	return WAIT_ABANDONED;
}

#ifdef linux
#	include <dlfcn.h>
#endif

#include <vector>
typedef std::vector<HANDLE> SemaLists;
static SemaLists theSemaList;
static BOOL theSemaListReady = FALSE;

void FreeAllSemaHandles()
{ long i;
  HANDLE h;
	while( !theSemaList.empty() ){
		i = 0;
		do{
			if( (h = theSemaList.at(i)) ){
				if( h->d.s.counter->refHANDLEp == &h->d.s.refHANDLEs && h->d.s.refHANDLEs > 0 ){
					// a source HANDLE that is still referenced: skip for now
//					fprintf( stderr, "Skipping referred-to semaphore %d\n", i );
					i += 1;
				}
				else{
//					fprintf( stderr, "Closing semaphore %d\n", i );
					CloseHandle(h);
					i = -1;
				}
			}
			else{
//				fprintf( stderr, "Removing stale semaphore %d\n", i );
				theSemaList.erase( theSemaList.begin() + i );
			}
		} while( i >= 0 && i < theSemaList.size() );
	}
}

void AddSemaListEntry(HANDLE h)
{
	if( !theSemaListReady ){
//		theSemaList.set_empty_key(NULL);
//		theSemaList.set_deleted_key( SEM_FAILED );
		atexit(FreeAllSemaHandles);
		theSemaListReady = TRUE;
	}
	if( h->type == MSH_SEMAPHORE ){
		theSemaList.push_back(h);
	}
}

void RemoveSemaListEntry(sem_t *sem)
{ unsigned int i, N = theSemaList.size();
	for( i = 0 ; i < N ; i++ ){
		if( theSemaList.at(i)->d.s.sem == sem ){
			theSemaList.erase( theSemaList.begin() + i );
			return;
		}
	}
}

HANDLE FindSemaphoreHANDLE(sem_t *sem, char *name)
{ unsigned int i, N = theSemaList.size();
  HANDLE ret = NULL;
	if( sem != SEM_FAILED || name ){
		for( i = 0 ; i < N && !ret; i++ ){
			if( sem != SEM_FAILED ){
				if( theSemaList.at(i)->d.s.sem == sem ){
					ret = theSemaList.at(i);
				}
			}
			else if( name ){
				ret = theSemaList.at(i);
				if( strcmp( ret->d.s.name, name ) || ret->d.s.counter->refHANDLEp != &ret->d.s.refHANDLEs ){
					// wrong name or not the source semaphore 
					ret = NULL;
				}
			}
		}
	}
	return ret;
}

/*!
 Opens the named semaphore that must already exist. The ign_ arguments are ignored in this emulation
 of the MS function of the same name.
 */
HANDLE OpenSemaphore( DWORD ign_dwDesiredAccess, BOOL ign_bInheritHandle, char *lpName )
{ HANDLE org, ret = NULL;
	if( lpName ){
	  sem_t *sema = sem_open( lpName, 0 );
		if( sema != SEM_FAILED ){
			if( (org = FindSemaphoreHANDLE(SEM_FAILED, lpName))
			   && strcmp( org->d.s.name, lpName ) == 0
			){
				ret = new MSHANDLE( sema, org->d.s.counter, mmstrdup(lpName) );
			}
		}
	}
	else{
		fprintf( stderr, "OpenSemaphore(%lu,%d,NULL): call is meaningless without a semaphore name\n",
			   ign_dwDesiredAccess, ign_bInheritHandle );
		return NULL;
	}
	return ret;
}

/*!
 Creates the named semaphore with the given initial count (value). The ign_ arguments are ignored in this emulation
 of the MS function of the same name.
 */
HANDLE CreateSemaphore( void* ign_lpSemaphoreAttributes, long lInitialCount, long ign_lMaximumCount, char *lpName )
{ HANDLE ret = NULL;
	if( lpName ){
		lpName = mmstrdup(lpName);
	}
	else{
		if( (lpName = mmstrdup( (char*) "/CSEsemXXXXXX" )) ){
#ifdef linux
			{ char *(*fun)(char *) = (char* (*)(char*))dlsym(RTLD_DEFAULT, (char*) "mktemp");
				lpName = (**fun)(lpName);
			}
//			{ int fd = mkstemp(lpName);
//				if( fd >= 0 ){
//					close(fd);
//					unlink(lpName);
//				}
//			}
#else
			lpName = mktemp(lpName);
#endif
		}
	}
	if( !(ret = OpenSemaphore( 0, FALSE, lpName )) ){
		if( !(ret = (HANDLE) new MSHANDLE( ign_lpSemaphoreAttributes, lInitialCount, ign_lMaximumCount, lpName ))
		   || ret->type != MSH_SEMAPHORE
		){
			fprintf( stderr, "CreateSemaphore(%p,%ld,%ld,%s) failed (%s)\n",
				ign_lpSemaphoreAttributes, lInitialCount, ign_lMaximumCount, lpName, strerror(errno) );
			free(lpName);
			delete ret;
			ret = NULL;
		}
	}
	return ret;
}

HANDLE CreateMutex( void *ign_lpMutexAttributes, BOOL bInitialOwner, char *ign_lpName )
{ HANDLE ret = NULL;
	if( !(ret = (HANDLE) new MSHANDLE( ign_lpMutexAttributes, bInitialOwner, ign_lpName ))
	   || ret->type != MSH_MUTEX
	){
		fprintf( stderr, "CreateMutex(%p,%s,%s) failed with errno=%d (%s)\n",
			   ign_lpMutexAttributes, (bInitialOwner)? "TRUE" : "FALSE", ign_lpName,
			   errno, strerror(errno)
		);
		delete ret;
		ret = NULL;
	}
	return ret;
}

HANDLE msCreateEvent( void *ign_lpEventAttributes, BOOL bManualReset, BOOL ign_bInitialState, char *ign_lpName )
{ HANDLE ret = NULL;
	if( !(ret = (HANDLE) new MSHANDLE( ign_lpEventAttributes, bManualReset, ign_bInitialState, ign_lpName ))
	   || ret->type != MSH_EVENT
	){
		fprintf( stderr, "CreateEvent(%p,%s,%s,%s) failed with errno=%d (%s)\n",
			   ign_lpEventAttributes, (bManualReset)? "TRUE" : "FALSE",
			   (ign_bInitialState)? "TRUE" : "FALSE", ign_lpName,
			   errno, strerror(errno)
		);
		delete ret;
		ret = NULL;
	}
	return ret;
}

HANDLE CreateThread( void *ign_lpThreadAttributes, size_t ign_dwStackSize, void *(*lpStartAddress)(void *),
				void *lpParameter, DWORD dwCreationFlags, DWORD *lpThreadId )
{ HANDLE ret = NULL;
	if( !(ret = (HANDLE) new MSHANDLE( ign_lpThreadAttributes, ign_dwStackSize, lpStartAddress,
							    lpParameter, dwCreationFlags, lpThreadId ))
		|| ret->type != MSH_THREAD
	){
		fprintf( stderr, "CreateThread(%p,%lu,0x%p,0x%p,%lu,0x%p) failed with errno=%d (%s)\n",
			   ign_lpThreadAttributes, ign_dwStackSize, lpStartAddress,
			   lpParameter, dwCreationFlags, lpThreadId,
			   errno, strerror(errno)
		);
		delete ret;
		ret = NULL;
	}
	return ret;
}

#if !defined(__APPLE__) && !defined(__MACH__)

struct TFunParams {
	HANDLE mshThread;
	void *(*start_routine)(void *);
	void *arg;
	TFunParams(HANDLE h, void *(*threadFun)(void*), void *args)
	{
		mshThread = h;
		start_routine = threadFun;
		arg = args;
	}
};

static pthread_key_t suspendKey = 0;
static BOOL suspendKeyCreated = false;

/*!
	specific USR2 signal handler that will attempt to suspend the current thread by
	locking an already locked mutex. It does this only for suspendable threads, i.e. threads
	which have a thread HANDLE stored in the suspendKey.
 */
static void pthread_u2_handler(int sig)
{
	if( suspendKey ){
		switch( sig ){
			case SIGUSR2:{
			  HANDLE mshThread;
				// get the mutex from a specific key
				if( (mshThread = (HANDLE) pthread_getspecific(suspendKey)) && mshThread->type == MSH_THREAD
				   && mshThread->d.t.threadLock && mshThread->d.t.threadLock->d.m.mutex
				){
					// when we receive this signal, the mutex ought to be lock by the thread
					// trying to suspend us. If so, trying to lock the mutex will suspend us.
					pthread_mutex_lock(mshThread->d.t.threadLock->d.m.mutex);
					// got the lock ... meaning we were just resumed.
					// now unlock it ASAP so that someone else can try to suspend us again.
					pthread_mutex_unlock(mshThread->d.t.threadLock->d.m.mutex);
				}
				break;
			}
		}
	}
}

static void *ThreadFunStart(void *params)
{ struct TFunParams *tp = (struct TFunParams*) params;
  void *(*threadFun)(void*) = tp->start_routine;
  void *args = tp->arg;
	pthread_setspecific( suspendKey, tp->mshThread );
	delete tp;
	return (*threadFun)(args);
}

/*!
	create a suspendable thread on platforms that don't support this by default. The 'trick' is to store
	a reference to the thread HANDLE in a specific thread key, and the thread HANDLE contains
	a dedicated mutex. To suspend the thread, we lock that mutex, and then send a signal (SIGUSR2) that
	will trigger an exception handler that will attempt to unlock that same mutex. To resume the thread,
	all we need to do is to unlock the mutex (the signal handler will also unlock immediately after
	obtaining the lock, so that the mutex remains free).
	The thread function is launched through a proxy that stores the thread HANDLE in the suspendKey.
 */
int pthread_create_suspendable( HANDLE mshThread, const pthread_attr_t *attr,
						  void *(*start_routine)(void *), void *arg, bool suspended )
{ int ret;
	if( !suspendKeyCreated ){
		cseAssertEx( pthread_key_create( &suspendKey, NULL ), __FILE__, __LINE__,
				  "failure to create the thread suspend key in pthread_create_suspendable()" );
		suspendKeyCreated = true;
	}

	struct TFunParams *params = new TFunParams( mshThread, start_routine, arg );

	mshThread->d.t.pThread = &mshThread->d.t.theThread;
	mshThread->type = MSH_THREAD;
	// it doesn't seem to work to install the signal handler from the background thread. Since
	// it's process-wide anyway we can just as well do it here.
	signal( SIGUSR2, pthread_u2_handler );
	ret = pthread_create( &mshThread->d.t.theThread, attr, ThreadFunStart, params );
	if( ret == 0 && suspended ){
		SuspendThread(mshThread);
	}
	return ret;
}

#endif // !__APPLE__ && !__MACH__

DWORD ResumeThread( HANDLE hThread )
{ DWORD prevCount = -1;
	if( hThread && hThread->type == MSH_THREAD && hThread->d.t.theThread != pthread_self() ){
		prevCount = hThread->d.t.suspendCount;
		if( hThread->d.t.suspendCount ){
			if( (--hThread->d.t.suspendCount) == 0 ){
#if defined(__APPLE__) || defined(__MACH__)
				if( thread_resume( hThread->d.t.machThread ) != KERN_SUCCESS ){
					// failure ... we're still suspended
					hThread->d.t.suspendCount += 1;
				}
#else
				if( pthread_mutex_unlock( hThread->d.t.threadLock->d.m.mutex ) ){
					// failure ... we're still suspended
					hThread->d.t.suspendCount += 1;
				}
				else{
					hThread->d.t.lockOwner = NULL;
				}
#endif
			}
		}
	}
	return prevCount;
}

/*!
	Suspend the given thread. On Apple/MACH, the suspend/resume feature of the underlying
	Mach threads is used. On other systems, the thread is sent an interrupt that should
	suspend it while it tries to obtain the lock on an already locked mutex.
	NB: with that approach it is thus not possible for a thread to suspend itself!
 */
DWORD SuspendThread( HANDLE hThread )
{ DWORD prevCount = -1;
  HANDLE current = GetCurrentThread();
	if( hThread && hThread->type == MSH_THREAD && hThread != current ){
		prevCount = hThread->d.t.suspendCount;
		if( hThread->d.t.suspendCount == 0 ){
#if defined(__APPLE__) || defined(__MACH__)
			if( thread_suspend( hThread->d.t.machThread ) == KERN_SUCCESS ){
				hThread->d.t.suspendCount = 1;
			}
#else
			if( !pthread_mutex_lock( hThread->d.t.threadLock->d.m.mutex ) ){
				hThread->d.t.lockOwner = current;
				if( !pthread_kill( hThread->d.t.theThread, SIGUSR2 ) ){
					hThread->d.t.suspendCount = 1;
				}
			}
#endif
		}
		else{
			hThread->d.t.suspendCount += 1;
		}
	}
	return prevCount;
}

HANDLE GetCurrentThread()
{ HANDLE currentThread = NULL;
	if( !currentThreadKey ){
		cseAssertEx( pthread_key_create( &currentThreadKey, (void (*)(void*))ForceCloseHandle ), __FILE__, __LINE__,
				  "failure to create the currentThreadKey in GetCurrentThread()" );
	}
	if( !(currentThread = (HANDLE) pthread_getspecific(currentThreadKey)) || currentThread->type != MSH_THREAD ){
		if( currentThread ){
			delete currentThread;
		}
		if( (currentThread = new MSHANDLE(pthread_self())) ){
			pthread_setspecific( currentThreadKey, currentThread );
		}
	}
	return currentThread;
}

static inline int SchedPriorityFromThreadPriority(int policy, int nPriority)
{ int sched_priority, pmin, pmax, pnormal;
	pmin = sched_get_priority_min(policy);
	pmax = sched_get_priority_max(policy);
	pnormal = (pmin + pmax) / 2;
	switch( nPriority ){
		case THREAD_PRIORITY_ABOVE_NORMAL:
			sched_priority = (pnormal + pmax) / 2;
			break;
		case THREAD_PRIORITY_BELOW_NORMAL:
			sched_priority = (pmin + pnormal) / 2;
			break;
		case THREAD_PRIORITY_HIGHEST:
		case THREAD_PRIORITY_TIME_CRITICAL:
			sched_priority = pmax;
			break;
		case THREAD_PRIORITY_IDLE:
		case THREAD_PRIORITY_LOWEST:
			sched_priority = pmin;
			break;
		case THREAD_PRIORITY_NORMAL:
			sched_priority = pnormal;
			break;
		default:
			sched_priority = nPriority;
			break;
	}
	return sched_priority;
}

/*!
	set the priority of the thread identified by the hThread HANDLE to the given
	priority level, THREAD_PRIORITY_{LOWEST,BELOW_NORMAL,NORMAL,ABOVE_NORMAL,HIGHEST}
 */
bool SetThreadPriority( HANDLE hThread, int nPriority )
{ struct sched_param param;
  int policy;
	if( hThread && hThread->type == MSH_THREAD
	   && !pthread_getschedparam( hThread->d.t.theThread, &policy, &param )
	){
		param.sched_priority = SchedPriorityFromThreadPriority( policy, nPriority );
		return (bool) pthread_setschedparam( hThread->d.t.theThread, policy, &param );
	}
	return true;
}

static inline int ThreadPriorityFromSchedPriority(int policy, int sched_priority)
{ int ret, pmin, pmax, pnormal, pbelow, pabove;
	pmin = sched_get_priority_min(policy);
	pmax = sched_get_priority_max(policy);
	pnormal = (pmin + pmax) / 2;
	pbelow = (pmin + pnormal) / 2;
	pabove =(pnormal + pmax) / 2;
	if( sched_priority < (pmin + pbelow)/2 ){
		ret = THREAD_PRIORITY_LOWEST;
	}
	else if( sched_priority < (pbelow + pnormal)/2 ){
		ret = THREAD_PRIORITY_BELOW_NORMAL;
	}
	else if( sched_priority <= (pnormal + pabove)/2 ){
		ret = THREAD_PRIORITY_NORMAL;
	}
	else if( sched_priority <= (pabove + pmax)/2 ){
		ret = THREAD_PRIORITY_ABOVE_NORMAL;
	}
	else{
		ret = THREAD_PRIORITY_HIGHEST;
	}
	return ret;
}

/*!
	get a thread's priority value. The current POSIX sched_priority is mapped
	onto the range of Microsoft values THREAD_PRIORITY_{LOWEST,BELOW_NORMAL,NORMAL,ABOVE_NORMAL,HIGHEST} .
	A check is then made to see if that value maps back onto the current POSIX sched_priority; if not,
	the POSIX priority is adjusted so that an expression like
	@n
	SetThreadPriority( B, GetThreadPriority(A) );
	@n
	always gives 2 threads with identical priorities.
 */
int GetThreadPriority(HANDLE hThread)
{ int ret = sched_get_priority_min(SCHED_FIFO) - sched_get_priority_min(SCHED_RR);
	if( hThread && hThread->type == MSH_THREAD ){
	  int policy, Prior;
	  struct sched_param param;
		if( !pthread_getschedparam( hThread->d.t.theThread, &policy, &param ) ){
//				fprintf( stderr, "pmin=%d pmax=%d\n", sched_get_priority_min(policy), sched_get_priority_max(policy) );
			ret = ThreadPriorityFromSchedPriority( policy, param.sched_priority );
			// check if the thread priority is different from the current sched_priority
			// and correct
			if( (Prior = SchedPriorityFromThreadPriority(policy, ret)) != param.sched_priority ){
				param.sched_priority = Prior;
				pthread_setschedparam( hThread->d.t.theThread, policy, &param );
			}
		}
	}
	return ret;
}

/*!
 Emulates the MS function of the same name:
 Closes the specified HANDLE, after closing the semaphore or mutex.
 */
bool CloseHandle( HANDLE hObject, bool joinThread=true )
{
	if( hObject && theOpenHandleListReady && theOpenHandleList.count(hObject) ){
		if( hObject->type == MSH_SEMAPHORE ){
			if( hObject->d.s.counter->refHANDLEp != &hObject->d.s.refHANDLEs || hObject->d.s.refHANDLEs == 0 ){
				delete hObject;
			}
		}
		else{
			if( hObject->type == MSH_THREAD && !joinThread ){
				// set d.t.pThread=NULL so ~MSHANDLE won't try to join the thread before deleting the HANDLE
				hObject->d.t.pThread = NULL;
			}
			delete hObject;
		}
		return true;
	}
	fprintf( stderr, "CloseHandle(0x%p) invalid HANDLE\n", hObject );
	return false;
}

bool CloseHandle( HANDLE hObject )
{
	return CloseHandle( hObject, true );
}

bool ForceCloseHandle( HANDLE hObject )
{
	return CloseHandle( hObject, false );
}
#endif // MINGW32

#ifndef __THREADLIB_H__
#define __THREADLIB_H__

#include "types.h"
#ifdef WIN32
#include <windows.h>
#include <process.h>
#define HTHREAD		HANDLE
#define HEVENT		HANDLE
#elif __UNIX__
#include <pthread.h>
#define HTHREAD		pthread_t
#define HEVENT		pthread_mutex_t
#elif __BEOS__
#include <be/kernel/OS.h>
#define HTHREAD		thread_id
#define HEVENT		sem_id
#endif

typedef struct THREAD_HANDLEst
{
	HTHREAD	thread_handle;
	HEVENT	_event;
} THREAD_HANDLE;


/*********************************************************************************
 * Public functions
 *********************************************************************************/
extern error_code	threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *));
extern BOOL			threadlib_isrunning(THREAD_HANDLE *thread);
extern void			threadlib_waitforclose(THREAD_HANDLE *thread);
extern void			threadlib_endthread(THREAD_HANDLE *thread);
extern BOOL			threadlib_event_signaled(HEVENT *e);

extern HEVENT		threadlib_create_event();
extern error_code	threadlib_waitfor_event(HEVENT *e);
extern error_code	threadlib_signel_event(HEVENT *e);
extern void			threadlib_destroy_event(HEVENT *e);


#endif //__THREADLIB__

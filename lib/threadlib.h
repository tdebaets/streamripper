#ifndef __THREADLIB_H__
#define __THREADLIB_H__

#include "types.h"
#ifdef WIN32
#include <windows.h>
#define HTHREAD		HANDLE
#define HMUTEX		CRITICAL_SECTION
#elif __UNIX__
#include <pthread.h>
#define HTHREAD		pthread_t
#define HMUTEX		pthread_mutex_t
#elif __BEOS__
#include <be/kernel/OS.h>
#define HTHREAD		thread_id
#define HMUTEX		sem_id
#endif

typedef struct THREAD_HANDLEst
{
	HTHREAD	thread_handle;
	thread_state_e state;
	HMUTEX crit;
} THREAD_HANDLE;


/*********************************************************************************
 * Public functions
 *********************************************************************************/
extern error_code	threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *));
extern void			threadlib_stop(THREAD_HANDLE *thread);
extern void			threadlib_close(THREAD_HANDLE *thread);
extern void			threadlib_sleep(THREAD_HANDLE *thread, int sec);
extern BOOL			threadlib_isrunning(THREAD_HANDLE *thread);
extern void			threadlib_waitforclose(THREAD_HANDLE *thread);


#endif //__THREADLIB__

/* threadlib.c - jonclegg@yahoo.com
 * really bad threading library from hell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if WIN32
#include <process.h>
#include <windows.h>
#elif __UNIX__
#include <pthread.h>
#elif __BEOS__
#include <be/kernel/OS.h>
#endif
#include <stdio.h>
#include <time.h>
#include "types.h"
#include "util.h"
#include "threadlib.h"

#if WIN32
	#define beginthread(thread, callback) \
		_beginthread((void *)callback, 0, (void *)NULL)
#elif __UNIX__
    #include <unistd.h>
	#define Sleep(x) usleep(x)
	#define InitializeCriticalSection(mutex) \
		pthread_mutex_init(mutex, NULL)
	#define EnterCriticalSection(mutex) \
		pthread_mutex_lock(mutex)
	#define DeleteCriticalSection(mutex) \
		pthread_mutex_destroy(mutex)
	#define LeaveCriticalSection(mutex) \
		pthread_mutex_unlock(mutex)
	#define beginthread(thread, callback) \
		pthread_create(thread, NULL, \
                          (void *)callback, (void *)NULL)
#elif __BEOS__
	#define Sleep(x) snooze(x*100)
	#define InitializeCriticalSection(mutex) \
		((*mutex) = create_sem(1, "mymutex"))
	#define EnterCriticalSection(mutex) \
		acquire_sem((*mutex))
	#define LeaveCriticalSection(mutex) \
		release_sem((*mutex))
	#define DeleteCriticalSection(mutex) \
		delete_sem((*mutex))
	#define beginthread(thread, callback) \
		resume_thread( \
		((*thread) = spawn_thread((int32 (*)(void*))callback, "sr_thread", 60, NULL)))
#endif


/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *));
BOOL		threadlib_isrunning(THREAD_HANDLE *thread);
void		threadlib_waitforclose(THREAD_HANDLE *thread);
void		threadlib_endthread(THREAD_HANDLE *thread);

HEVENT		threadlib_create_event();
error_code	threadlib_waitfor_event(HEVENT *e);
error_code	threadlib_signel_event(HEVENT *e);
void		threadlib_destroy_event(HEVENT *e);
BOOL		threadlib_event_signaled(HEVENT *e);


error_code threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *))
{
	debug_printf("starting thread\n");

	thread->_event = threadlib_create_event();
	thread->thread_handle = (HANDLE) _beginthread(callback, 0, NULL);
	if (thread->thread_handle == NULL)
		return SR_ERROR_CANT_CREATE_THREAD;

	return SR_SUCCESS;
}

BOOL threadlib_isrunning(THREAD_HANDLE *thread)
{
	return thread->thread_handle != NULL;
}

void threadlib_waitforclose(THREAD_HANDLE *thread)
{
	WaitForSingleObject(thread->_event, INFINITE);
	thread->thread_handle = NULL;
}

void threadlib_endthread(THREAD_HANDLE *thread)
{
	SetEvent(thread->_event);
	_endthread();
}


HEVENT threadlib_create_event()
{
	return CreateEvent(NULL, FALSE, FALSE, NULL);
}

BOOL threadlib_event_signaled(HEVENT *e)
{
	DWORD ret;
	if (!e)
		return FALSE;

	ret = WaitForSingleObject(*e, 0);
	return ret == WAIT_OBJECT_0;
}


error_code threadlib_waitfor_event(HEVENT *e)
{
	if (!e)
		return SR_ERROR_INVALID_PARAM;
	if (WaitForSingleObject(*e, INFINITE) != WAIT_OBJECT_0)
		return SR_ERROR_CANT_WAIT_ON_THREAD;

	return SR_SUCCESS;
}
error_code threadlib_signel_event(HEVENT *e)
{
	if (!e)
		return SR_ERROR_INVALID_PARAM;

	SetEvent(*e);

	return SR_SUCCESS;
}
void threadlib_destroy_event(HEVENT *e)
{
	CloseHandle(*e);
}

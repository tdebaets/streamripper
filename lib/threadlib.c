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
void		threadlib_stop(THREAD_HANDLE *thread);
void		threadlib_close(THREAD_HANDLE *thread);
void		threadlib_sleep(THREAD_HANDLE *thread, int sec);
BOOL		threadlib_isrunning(THREAD_HANDLE *thread);
void		threadlib_waitforclose(THREAD_HANDLE *thread);

error_code threadlib_beginthread(THREAD_HANDLE *thread, void (*callback)(void *))
{
	debug_printf("starting thread\n");

	InitializeCriticalSection(&thread->crit);
	thread->state = THREAD_STATE_RUNNING;
	// the < 0 is for BeOS, all other OS's expect -1
	if (beginthread(&thread->thread_handle, callback) < 0)	
	{
		thread->state = THREAD_STATE_CLOSED;
		return SR_ERROR_CANT_CREATE_THREAD;
	}

	return SR_SUCCESS;
}

void threadlib_stop(THREAD_HANDLE *thread)
{
	EnterCriticalSection(&thread->crit);
	if (thread->state == THREAD_STATE_RUNNING)
		thread->state = THREAD_STATE_CLOSE_PENDING;
	LeaveCriticalSection(&thread->crit);
}

void threadlib_close(THREAD_HANDLE *thread)
{
	EnterCriticalSection(&thread->crit);
	thread->state = THREAD_STATE_CLOSED;
	LeaveCriticalSection(&thread->crit);
	
	debug_printf("Thread closed!!!!\n");
}

void threadlib_sleep(THREAD_HANDLE *thread, int sec)
{
	time_t start, now;
	
	time(&start);
	while(TRUE)
	{
		EnterCriticalSection(&thread->crit);
		if (thread->state != THREAD_STATE_RUNNING)
		{
			LeaveCriticalSection(&thread->crit);
			break;
		}
		LeaveCriticalSection(&thread->crit);

		time(&now);
		if (now-start >= sec)
			break;
		Sleep(100);
	}
}
BOOL threadlib_isrunning(THREAD_HANDLE *thread)
{
	BOOL state;

	EnterCriticalSection(&thread->crit);
	state = thread->state == THREAD_STATE_RUNNING;
	LeaveCriticalSection(&thread->crit);
	return state;
	
}

void threadlib_waitforclose(THREAD_HANDLE *thread)
{
	BOOL needloop;

	EnterCriticalSection(&thread->crit);
	needloop  = thread->state == THREAD_STATE_CLOSE_PENDING;
	LeaveCriticalSection(&thread->crit);

	if (!needloop)
		return;

	while(needloop)
	{
		EnterCriticalSection(&thread->crit);
		needloop = thread->state != THREAD_STATE_CLOSED;
		LeaveCriticalSection(&thread->crit);
		Sleep(100);
	}

	DeleteCriticalSection(&thread->crit);
}



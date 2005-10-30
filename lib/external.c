/* external.c
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
#include <stdio.h>
#include <stdlib.h>
#if defined (WIN32)
#include <process.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "external.h"
#include "compat.h"

/* REFERENCES:
http://www.cs.uleth.ca/~holzmann/C/system/pipeforkexec.html
http://www.ecst.csuchico.edu/~beej/guide/ipc/fork.html
for nonblocking, on windows don't use fcntl(), use ioctlsocket() */
/* Win32
Non-blocking pipe using PeekNamedPipe():
http://list-archive.xemacs.org/xemacs-beta/199910/msg00263.html
*/

/* ----------------------------- WIN32 VERSION --------------------------- */
#if defined (WIN32)

HANDLE hChildStdinRd, hChildStdinWr,  
   hChildStdoutRd, hChildStdoutWr, 
   hStdout;
 
BOOL CreateChildProcess(char* cmd); 
VOID WriteToPipe(VOID); 
VOID ReadFromPipe(VOID); 
VOID ErrorExit(LPTSTR); 
VOID ErrMsg(LPTSTR, BOOL); 

External_Process*
spawn_external (char* cmd)
{
    External_Process* ep;
    SECURITY_ATTRIBUTES saAttr; 
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL rc;

    ep = (External_Process*) malloc (sizeof (External_Process));
    if (!ep) return 0;
    ep->line_buf[0] = 0;
    ep->line_buf_idx = 0;
    ep->album_buf[0] = 0;
    ep->artist_buf[0] = 0;
    ep->title_buf[0] = 0;

    /* Set the bInheritHandle flag so pipe handles are inherited. */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    /* Get the handle to the current STDOUT.  */
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
 
    /* Create a pipe for the child process's STDOUT. */
    if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
        debug_printf ("Stdout pipe creation failed\n");
	free (ep);
	return 0;
    }

    /* Ensure the read handle to the pipe for STDOUT is not inherited.*/
    SetHandleInformation( hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

    /* Create a pipe for the child process's STDIN. */
    if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
        debug_printf ("Stdin pipe creation failed\n");
	free (ep);
	return 0;
    }

    /* Ensure the write handle to the pipe for STDIN is not inherited. */
    SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

    /* create the child process */
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = hChildStdoutWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    rc = CreateProcess (
		NULL,          // executable name
		cmd,	       // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
    if (rc == 0) {
	free (ep);
	return 0;
    }
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return ep;
}
 
int
read_external (External_Process* ep, TRACK_INFO* ti)
{
    char c;
    int rc;
    int got_metadata = 0;
    DWORD num_read;

    ti->have_track_info = 0;

    while (1) {
	DWORD bytes_avail = 0;
	rc = PeekNamedPipe(hChildStdoutRd, NULL, 0, NULL, &bytes_avail, NULL);
	if (!rc) {
	    DWORD error_code;
	    /* Pipe closed? */
	    /* GCS FIX: Restart external program if pipe closed */
	    error_code = GetLastError ();
	    debug_printf ("PeekNamedPipe failed, error_code = %d\n",
			    error_code);
	    return 0;
	}
	if (bytes_avail <= 0) {
	    /* Pipe blocked */
	    return got_metadata;
	}
	rc = ReadFile (hChildStdoutRd, &c, 1, &num_read, NULL);
	if (rc > 0 && num_read > 0) {
	    if (c != '\r' && c != '\n') {
		if (ep->line_buf_idx < MAX_EXT_LINE_LEN-1) {
		    ep->line_buf[ep->line_buf_idx++] = c;
		    ep->line_buf[ep->line_buf_idx] = 0;
		}
	    } else {
		if (!strcmp (".",ep->line_buf)) {
		    /* End of record */
		    strcpy (ti->artist,ep->artist_buf);
		    strcpy (ti->album,ep->album_buf);
		    strcpy (ti->title,ep->title_buf);
		    snprintf (ti->raw_metadata, MAX_EXT_LINE_LEN, "%s - %s",
			      ti->artist, ti->title);
		    ti->have_track_info = 1;
		    ti->save_track = TRUE;

		    ep->artist_buf[0] = 0;
		    ep->album_buf[0] = 0;
		    ep->title_buf[0] = 0;
		    got_metadata = 1;
		} else if (!strncmp("ARTIST=",ep->line_buf,strlen("ARTIST="))) {
		    strcpy (ep->artist_buf, &ep->line_buf[strlen("ARTIST=")]);
		} else if (!strncmp("ALBUM=",ep->line_buf,strlen("ALBUM="))) {
		    strcpy (ep->album_buf, &ep->line_buf[strlen("ALBUM=")]);
		} else if (!strncmp("TITLE=",ep->line_buf,strlen("TITLE="))) {
		    strcpy (ep->title_buf, &ep->line_buf[strlen("TITLE=")]);
		}
		ep->line_buf[0] = 0;
		ep->line_buf_idx = 0;
	    }
	}
    }
}

/* ----------------------------- UNIX VERSION --------------------------- */
#else
External_Process*
spawn_external (char* cmd)
{
    External_Process* ep;
    int rc;

    ep = (External_Process*) malloc (sizeof (External_Process));
    if (!ep) return 0;
    ep->line_buf[0] = 0;
    ep->line_buf_idx = 0;
    ep->album_buf[0] = 0;
    ep->artist_buf[0] = 0;
    ep->title_buf[0] = 0;

    /* Create the pipes */
    rc = pipe (ep->mypipe);
    if (rc) {
	fprintf (stderr, "Can't open pipes\n");
	free (ep);
	return 0;
    }
    /* Create the child process. */
    ep->pid = fork ();
    if (ep->pid == (pid_t) 0) {
	/* This is the child process. */
	close (ep->mypipe[0]);
	dup2 (ep->mypipe[1],1);
	close (ep->mypipe[1]);
	execlp (cmd,cmd,NULL);
	/* Doesn't return */
	fprintf (stderr, "Error, returned from execlp\n");
	exit (-1);
    } else if (ep->pid < (pid_t) 0) {
	/* The fork failed. */
	close (ep->mypipe[0]);
	close (ep->mypipe[1]);
	fprintf (stderr, "Fork failed.\n");
	free (ep);
	return 0;
    } else {
	/* This is the parent process. */
	close (ep->mypipe[1]);
	rc = fcntl (ep->mypipe[0], F_SETFL, O_NONBLOCK);
	return ep;
    }
}

int
read_external (External_Process* ep, TRACK_INFO* ti)
{
    char c;
    int rc;
    int got_metadata = 0;

    ti->have_track_info = 0;

    while (1) {
	rc = read (ep->mypipe[0],&c,1);
	if (rc > 0) {
	    if (c != '\r' && c != '\n') {
		if (ep->line_buf_idx < MAX_EXT_LINE_LEN-1) {
		    ep->line_buf[ep->line_buf_idx++] = c;
		    ep->line_buf[ep->line_buf_idx] = 0;
		}
		continue;
	    }
	    if (!strcmp (".",ep->line_buf)) {
		/* End of record */
		strcpy (ti->artist,ep->artist_buf);
		strcpy (ti->album,ep->album_buf);
		strcpy (ti->title,ep->title_buf);
		snprintf (ti->raw_metadata, MAX_EXT_LINE_LEN, "%s - %s",
			  ti->artist, ti->title);
		ti->have_track_info = 1;
		ti->save_track = TRUE;

		ep->artist_buf[0] = 0;
		ep->album_buf[0] = 0;
		ep->title_buf[0] = 0;
		got_metadata = 1;
	    } else if (!strncmp("ARTIST=",ep->line_buf,strlen("ARTIST="))) {
		strcpy (ep->artist_buf, &ep->line_buf[strlen("ARTIST=")]);
	    } else if (!strncmp("ALBUM=",ep->line_buf,strlen("ALBUM="))) {
		strcpy (ep->album_buf, &ep->line_buf[strlen("ALBUM=")]);
	    } else if (!strncmp("TITLE=",ep->line_buf,strlen("TITLE="))) {
		strcpy (ep->title_buf, &ep->line_buf[strlen("TITLE=")]);
	    }
	    ep->line_buf[0] = 0;
	    ep->line_buf_idx = 0;
	    continue;
	} else if (rc == 0) {
	    /* Pipe closed */
	    /* GCS FIX: Restart external program if pipe closed */
	    return 0;
	} else {
	    if (errno == EAGAIN) {
		/* Would block */
		return got_metadata;
	    }
	    /* GCS FIX: Figure out the error here. */
	    return 0;
	}
    }
}

/* GCS FIX: This is never actually called. */
void
close_external (External_Process* ep)
{
    int rv;
    printf ("I should be exiting soon...\n");
    kill (ep->pid,SIGTERM);
    usleep (0);
    if (waitpid (ep->pid,&rv,WNOHANG) == 0) {
	printf ("Waiting for cleanup\n");
	usleep (2000);
	kill (ep->pid,SIGKILL);
    }
    wait(&rv);
}
#endif

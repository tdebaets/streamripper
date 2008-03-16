/* rip_manager.c
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
/******************************************************************************
 * Rip Manager API
 *
 *   Callback Function
 *     void (*status_callback)(RIP_MANAGER_INFO* rmi, 
 *                              int message, void *data));
 *   Functions
 *     void rip_manager_init (void);
 *     error_code rip_manager_start (RIP_MANAGER_INFO **rmi, 
 *	  STREAM_PREFS *prefs, RIP_MANAGER_CALLBACK status_callback);
 *     void rip_manager_stop (RIP_MANAGER_INFO *rmi);
 *     void rip_manager_cleanup (void);
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <sys/types.h>
#if !defined (WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "errors.h"
#include "filelib.h"
#include "socklib.h"
#include "mchar.h"
#include "findsep.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"
#include "parse.h"
#include "http.h"

/******************************************************************************
 * Private functions
 *****************************************************************************/
static void ripthread (void *thread_arg);
static void post_status (RIP_MANAGER_INFO* rmi, int status);
static error_code start_ripping (RIP_MANAGER_INFO* rmi);
void destroy_subsystems (RIP_MANAGER_INFO* rmi);

/******************************************************************************
 * Private Vars
 *****************************************************************************/
static const char* overwrite_opt_strings[] = {
    "",		// UNKNOWN
    "always",
    "never",
    "larger"
};

/******************************************************************************
 * Public functions
 *****************************************************************************/
void
rip_manager_init (void)
{
    errors_init ();
    socklib_init();
}

/* Create a RMI structure and start the ripping thread */
error_code
rip_manager_start (RIP_MANAGER_INFO **rmip,
		   STREAM_PREFS *prefs,
		   RIP_MANAGER_CALLBACK status_callback)
{
    RIP_MANAGER_INFO* rmi;
    int rc;

    if (!prefs || !rmip) {
	return SR_ERROR_INVALID_PARAM;
    }

    rmi = (*rmip) = (RIP_MANAGER_INFO*) malloc (sizeof(RIP_MANAGER_INFO));
    memset (rmi, 0, sizeof(RIP_MANAGER_INFO));
    rmi->prefs = prefs;

    rmi->started_sem = threadlib_create_sem();

    register_codesets (rmi, &prefs->cs_opt);

    /* From select() man page:
       On systems that lack pselect() reliable (and more
       portable)  signal  trapping  can  be achieved using the self-pipe trick
       (where a signal handler writes a byte to a pipe whose other end is mon-
       itored by select() in the main program.
    */
#if __UNIX__
    rc = pipe (rmi->abort_pipe);
    if (rc != 0) {
	return SR_ERROR_CREATE_PIPE_FAILED;
    }
#endif

    rmi->status_callback = status_callback;
    rmi->bytes_ripped = 0;
    rmi->write_data = 1;

    /* Initialize the parsing rules */
    init_metadata_parser (rmi, prefs->rules_file);

    /* Start the ripping thread */
    debug_printf ("Pre ripthread: %s\n", rmi->prefs->url);
    rmi->started = 1;
    return threadlib_beginthread (&rmi->hthread_ripper, 
				  ripthread, (void*) rmi);
}

void
rip_manager_stop (RIP_MANAGER_INFO *rmi)
{
    // Make sure this function isn't getting called twice
    if (!rmi->started) {
	debug_printf ("rip_manager_stop() called while not started\n");
	return;
    }
    
    /* Write to pipe so ripping thread will exit select() */
#if __UNIX__
    debug_printf ("Writing to abort_pipe.\n");
    write (rmi->abort_pipe[1], "0", 1);
#endif

    // Make sure the ripping started before we try to stop
    debug_printf ("Waiting for m_started_sem...\n");
    threadlib_waitfor_sem(&rmi->started_sem);
    rmi->started = 0;

    // Causes the code running in the thread to bail
    debug_printf ("Closing stream_sock...\n");
    socklib_close(&rmi->stream_sock);

    // Kill external process
    if (rmi->ep) {
	debug_printf ("Close external\n");
	close_external (&rmi->ep);
    }

    // blocks until everything is ok and closed
    debug_printf ("Waiting for hthread_ripper to close...\n");
    threadlib_waitforclose(&rmi->hthread_ripper);
    debug_printf ("Destroying subsystems...\n");
    destroy_subsystems (rmi);
    debug_printf ("Destroying m_started_sem\n");
    threadlib_destroy_sem(&rmi->started_sem);
    debug_printf ("Done with rip_manager_stop\n");

    /* Close pipes */
#if __UNIX__
    debug_printf ("Closing abort_pipe.\n");
    close (rmi->abort_pipe[0]);
    close (rmi->abort_pipe[1]);
#endif
}


void
rip_manager_cleanup (void)
{
    socklib_cleanup();
}


/******************************************************************************
 * Private functions
 *****************************************************************************/
static void
post_error (RIP_MANAGER_INFO* rmi, error_code err)
{
    ERROR_INFO err_info;
    err_info.error_code = err;
    strcpy(err_info.error_str, errors_get_string (err));
    debug_printf ("post_error: %d %s\n", err_info.error_code, 
		  err_info.error_str);
    rmi->status_callback (rmi, RM_ERROR, &err_info);
}

static void
post_status (RIP_MANAGER_INFO* rmi, int status)
{
    if (status != 0)
        rmi->status = status;
    rmi->status_callback (rmi, RM_UPDATE, 0);
}

static void
compose_console_string (RIP_MANAGER_INFO *rmi, TRACK_INFO* ti)
{
    mchar console_string[SR_MAX_PATH];
    msnprintf (console_string, SR_MAX_PATH, m_S m_(" - ") m_S, 
	       ti->artist, ti->title);
    string_from_mstring (rmi, rmi->filename, SR_MAX_PATH, console_string,
			 CODESET_LOCALE);
}

/* 
 * This is called by ripstream when we get a new track. 
 * most logic is handled by filelib_start() so we just
 * make sure there are no bad characters in the name and 
 * update via the callback 
 */
error_code
rip_manager_start_track (RIP_MANAGER_INFO *rmi, TRACK_INFO* ti)
{
    int ret;

#if defined (commmentout)
    /* GCS FIX -- here is where i would compose the incomplete filename */
    char* trackname = ti->raw_metadata;
    debug_printf("rip_manager_start_track: %s\n", trackname);
    if (rmi->write_data && (ret = filelib_start(trackname)) != SR_SUCCESS) {
        return ret;
    }
#endif

    rmi->write_data = ti->save_track;

    if (rmi->write_data && (ret = filelib_start (rmi, ti)) != SR_SUCCESS) {
        return ret;
    }

    rmi->filesize = 0;

    /* Compose the string for the console output */
    compose_console_string (rmi, ti);
    rmi->filename[SR_MAX_PATH-1] = '\0';
    rmi->status_callback (rmi, RM_NEW_TRACK, (void*) rmi->filename);
    post_status(rmi, 0);

    return SR_SUCCESS;
}

/* Ok, the end_track()'s function is actually to move 
 * tracks out from the incomplete directory. It does 
 * get called, but only after the 2nd track is played. 
 * the first track is *never* complete.
 */
error_code
rip_manager_end_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    mchar mfullpath[SR_MAX_PATH];
    char fullpath[SR_MAX_PATH];

    if (rmi->write_data) {
        filelib_end (rmi, ti, rmi->prefs->overwrite,
		     GET_TRUNCATE_DUPS(rmi->prefs->flags),
		     mfullpath);
    }
    post_status(rmi, 0);

    string_from_mstring (rmi, fullpath, SR_MAX_PATH, mfullpath, CODESET_FILESYS);
    rmi->status_callback (rmi, RM_TRACK_DONE, (void*)fullpath);

    return SR_SUCCESS;
}

error_code
rip_manager_put_data (RIP_MANAGER_INFO *rmi, char *buf, int size)
{
    int ret;

    if (GET_INDIVIDUAL_TRACKS(rmi->prefs->flags)) {
	if (rmi->write_data) {
	    ret = filelib_write_track (rmi, buf, size);
	    if (ret != SR_SUCCESS) {
		debug_printf ("filelib_write_track returned: %d\n",ret);
		return ret;
	    }
	}
    }

    rmi->filesize += size;	/* This is used by the GUI */
    rmi->bytes_ripped += size;	/* This is used to determine when to quit */

    return SR_SUCCESS;
}

char *
client_relay_header_generate (RIP_MANAGER_INFO* rmi, int icy_meta_support)
{
    int ret;
    char *headbuf;

    headbuf = (char *) malloc (MAX_HEADER_LEN);
    ret = http_construct_sc_response (&rmi->http_info, headbuf, MAX_HEADER_LEN,
				      icy_meta_support);
    if (ret != SR_SUCCESS) {
	headbuf[0] = 0;
    }
    
    return headbuf;
}

void
client_relay_header_release (char *ch)
{
    free (ch);
}

static void
debug_ripthread (RIP_MANAGER_INFO* rmi)
{
    debug_printf ("------ RIP_MANAGER_INFO -------\n");
    debug_printf ("streamname = %s\n", rmi->streamname);
    debug_printf ("server_name = %s\n", rmi->server_name);
    debug_printf ("bitrate = %d\n", rmi->bitrate);
    debug_printf ("meta_interval = %d\n", rmi->meta_interval);
    debug_printf ("filename = %s\n", rmi->filename);
    debug_printf ("filesize = %d\n", rmi->filesize);
    debug_printf ("status = %d\n", rmi->status);
    debug_printf ("track_count = %d\n", rmi->track_count);
    debug_printf ("external_process = %p\n", rmi->ep);
}

static void
ripthread (void *thread_arg)
{
    error_code ret;
    RIP_MANAGER_INFO* rmi = (RIP_MANAGER_INFO*) thread_arg;
    debug_ripthread (rmi);
    debug_stream_prefs (rmi->prefs);

    /* Connect to remote server */
    ret = start_ripping(rmi);
    if (ret != SR_SUCCESS) {
	debug_printf ("Ripthread did start_ripping()\n");
	threadlib_signal_sem (&rmi->started_sem);
	post_error (rmi, ret);
	goto DONE;
    }

    rmi->status_callback (rmi, RM_STARTED, (void *)NULL);
    post_status (rmi, RM_STATUS_BUFFERING);
    debug_printf ("Ripthread did initialization\n");
    threadlib_signal_sem(&rmi->started_sem);

    while (TRUE) {
	debug_printf ("Gonna ripstream_rip\n");
        ret = ripstream_rip(rmi);
	debug_printf ("Did ripstream_rip\n");

	/* If the user told us to stop, well, then we bail */
	if (!rmi->started)
	    break;

	/* 
	 * Added 8/4/2001 jc --
	 * for max number of MB ripped stuff
	 * At the time of writting this, i honestly 
	 * am not sure i understand what happens 
	 * when once trys to stop the ripping from the lib and
	 * the interface it's a weird combonation of threads
	 * wnd locks, etc.. all i know is that this appeared to work
	 */
        /* GCS Aug 23, 2003: bytes_ripped can still overflow */
	if (rmi->bytes_ripped/1000000 >= (rmi->prefs->maxMB_rip_size) &&
		GET_CHECK_MAX_BYTES (rmi->prefs->flags)) {
	    socklib_close (&rmi->stream_sock);
	    destroy_subsystems (rmi);
	    post_error (rmi, SR_ERROR_MAX_BYTES_RIPPED);
	    break;
	}

	/*
	 * If the error was anything but a connection failure
	 * then we need to report it and bail
	 */
	if (ret == SR_SUCCESS_BUFFERING) {
	    post_status (rmi, RM_STATUS_BUFFERING);
	}
	else if (ret == SR_ERROR_CANT_DECODE_MP3) {
	    post_error (rmi, ret);
	    continue;
	}
	else if ((ret == SR_ERROR_RECV_FAILED || 
		  ret == SR_ERROR_TIMEOUT || 
		  ret == SR_ERROR_NO_TRACK_INFO || 
		  ret == SR_ERROR_SELECT_FAILED) && 
		 GET_AUTO_RECONNECT (rmi->prefs->flags)) {
	    /* Try to reconnect, if thats what the user wants */
	    post_status (rmi, RM_STATUS_RECONNECTING);
	    while (rmi->started) {
		// Hopefully this solves a lingering bug 
		// with auto-reconnects failing to bind to the relay port
		// (a long with other unknown problems)
		// it seems that maybe we need two levels of shutdown and startup
		// functions. init_system, init_rip, shutdown_system and shutdown_rip
		// this would be a shutdown_rip, which only lacks the filelib and socklib
		// shutdowns
		// because filelib needs to keep track of it's file counters, and socklib.. umm..
		// not sure about. no reasdon to i imagine.
		socklib_close(&rmi->stream_sock);
		if (rmi->ep) {
		    debug_printf ("Close external\n");
		    close_external (&rmi->ep);
		}
		relaylib_stop (rmi);
		filelib_shutdown (rmi);
		ripstream_clear (rmi);
		ret = start_ripping (rmi);
		if (ret == SR_SUCCESS)
		    break;

		/*
		 * Should send a message that we are trying 
		 * .. or something
		 */
		Sleep(1000);
	    }
	    if (!rmi->started) break;
	}
	else if (ret != SR_SUCCESS) {
	    destroy_subsystems (rmi);
	    post_error (rmi, ret);
	    break;
	}

	if (rmi->filesize > 0)
	    post_status (rmi, RM_STATUS_RIPPING);
    }

    // We get here when there was either a fatal error
    // or we we're not auto-reconnecting and the stream just stopped
    // or when we have been told to stop, via the rmi->started flag
 DONE:
    rmi->status_callback (rmi, RM_DONE, 0);
    rmi->started = 0;
    debug_printf ("ripthread() exiting!\n");
}

void
destroy_subsystems (RIP_MANAGER_INFO* rmi)
{
    ripstream_clear (rmi);
    relaylib_stop (rmi);
    /* GCS Feb 17,2008.  The socklib_cleanup() is done at program 
       shutdown, not rip_manager shutdown. */
    // socklib_cleanup();
    filelib_shutdown (rmi);
}

static int
create_pls_file (RIP_MANAGER_INFO* rmi)
{
    FILE *fid;

    if  ('\0' == rmi->prefs->relay_ip[0]) {
	fprintf(stderr, "can not determine relaying ip, pass ip to -r \n");
	return -1;
    }

    fid = fopen (rmi->prefs->pls_file, "w");

    if (NULL == fid) {
	fprintf(stderr, "could not create playlist file '%s': %d '%s'\n",
		rmi->prefs->pls_file, errno, strerror(errno));
    } else {
	fprintf(fid, "[playlist]\n");
	fprintf(fid, "NumberOfEntries=1\n");
	fprintf(fid, "File1=http://%s:%d\n", rmi->prefs->relay_ip, 
		rmi->prefs->relay_port);
	fclose(fid);
    }

    return 0;
}

static error_code
start_ripping (RIP_MANAGER_INFO* rmi)
{
    STREAM_PREFS* prefs = rmi->prefs;
    error_code ret;

    char *pproxy = prefs->proxyurl[0] ? prefs->proxyurl : NULL;
    debug_printf ("start_ripping: checkpoint 1\n");

    /* If proxy URL not spec'd on command line (or plugin field), 
       check the environment variable */
    if (!pproxy) {
	char const *env_http_proxy = getenv ("http_proxy");
	if (env_http_proxy) {
	    strncpy (prefs->proxyurl, env_http_proxy, MAX_URL_LEN);
	}
    }

    debug_printf ("start_ripping: checkpoint 2\n");

    /* Connect to the stream */
    ret = http_sc_connect (rmi, &rmi->stream_sock, prefs->url, 
			   pproxy, &rmi->http_info, 
			   prefs->useragent, prefs->if_name);
    if (ret != SR_SUCCESS) {
	goto RETURN_ERR;
    }

    /* If the icy_name exists, but is empty, set to a bogus name so 
       that we can create the directory correctly, etc. */
    if (strlen(rmi->http_info.icy_name) == 0) {
	strcpy (rmi->http_info.icy_name, "Streamripper_rips");
    }

    /* Set the ripinfo struct from the data we now know about the 
     * stream, this info are things like server name, type, 
     * bitrate etc.. */
    rmi->meta_interval = rmi->http_info.meta_interval;
    rmi->http_bitrate = rmi->http_info.icy_bitrate;
    rmi->detected_bitrate = -1;
    rmi->bitrate = -1;
    strcpy (rmi->streamname, rmi->http_info.icy_name);
    strcpy (rmi->server_name, rmi->http_info.server);

    /* Initialize file writing code. */
    ret = filelib_init
	    (rmi, 
	     GET_INDIVIDUAL_TRACKS(rmi->prefs->flags),
	     GET_COUNT_FILES(rmi->prefs->flags),
	     rmi->prefs->count_start,
	     GET_KEEP_INCOMPLETE(rmi->prefs->flags),
	     GET_SINGLE_FILE_OUTPUT(rmi->prefs->flags),
	     rmi->http_info.content_type, 
	     rmi->prefs->output_directory,
	     rmi->prefs->output_pattern,
	     rmi->prefs->showfile_pattern,
	     GET_SEPERATE_DIRS(rmi->prefs->flags),
	     GET_DATE_STAMP(rmi->prefs->flags),
	     rmi->http_info.icy_name);
    if (ret != SR_SUCCESS)
	goto RETURN_ERR;

    /* Start up external program to get metadata. */
    rmi->ep = 0;
    if (GET_EXTERNAL_CMD(rmi->prefs->flags)) {
	debug_printf ("Spawn external: %s\n", rmi->prefs->ext_cmd);
	rmi->ep = spawn_external (rmi->prefs->ext_cmd);
	if (rmi->ep) {
	    debug_printf ("Spawn external succeeded\n");
	} else {
	    debug_printf ("Spawn external failed\n");
	}
    }

    /* Allocate buffers for ripstream */
    strcpy(rmi->no_meta_name, rmi->http_info.icy_name);
    rmi->getbuffer = 0;
    ripstream_clear (rmi);
    ret = ripstream_init(rmi);
    if (ret != SR_SUCCESS) {
	ripstream_clear (rmi);
	goto RETURN_ERR;
    }

    /* Launch relay server threads */
    if (GET_MAKE_RELAY (rmi->prefs->flags)) {
	u_short new_port = 0;
	ret = relaylib_start (rmi, 
			      GET_SEARCH_PORTS(rmi->prefs->flags), 
			      rmi->prefs->relay_port,
			      rmi->prefs->max_port, 
			      &new_port,
			      rmi->prefs->if_name, 
			      rmi->prefs->max_connections,
			      rmi->prefs->relay_ip,
			      rmi->http_info.meta_interval != NO_META_INTERVAL);
	if (ret != SR_SUCCESS) {
	    goto RETURN_ERR;
	}

	rmi->prefs->relay_port = new_port;

	/* Create pls file with address of relay stream */
	if (0 != rmi->prefs->pls_file[0]) {
	    create_pls_file (rmi);
	}
    }

    /* Done. */
    post_status (rmi, RM_STATUS_BUFFERING);
    return SR_SUCCESS;

 RETURN_ERR:
    socklib_close(&rmi->stream_sock);
    return ret;	
}

/* Winamp plugin needs to get content type */
int
rip_manager_get_content_type (RIP_MANAGER_INFO* rmi)
{
    return rmi->http_info.content_type;
}

enum OverwriteOpt
string_to_overwrite_opt (char* str)
{
    int i;
    for (i = 0; i < 4; i++) {
	if (strcmp(str, overwrite_opt_strings[i]) == 0) {
	    return i;
	}
    }
    return OVERWRITE_UNKNOWN;
}

const char*
overwrite_opt_to_string (enum OverwriteOpt oo)
{
    return overwrite_opt_strings[(int) oo];
}

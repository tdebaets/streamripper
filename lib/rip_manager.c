/* rip_manager.c - jonclegg@yahoo.com
 * ties together lots of shit, main entry point for clients to call
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
  
/*
 * This is the top level for the streamripper library. The it handles the 
 * connecting the rippers (ripstream, ripshout, riplive) to the sockets 
 * (socklib), the output stuff (filelib) and the relay server thing (relaylib)
 * and also all the "features" like auto connecting, directory names.
 *
 * Lots of stuff, and in retrospect it should probably be more splite up. My 
 * excuss is that this grew from a bunch of unit tests for the various 
 * "sub-systems". In the end i just called it "rip_mananger".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "filelib.h"
#include "socklib.h"
#include "http.h"
#include "util.h"
#include "riplive365.h"
#include "ripshout.h"
#include "findsep.h"
//#include "live365info.h"
#include "inet.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"
#include "debug.h"
#include "compat.h"

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code		rip_manager_start(void (*status_callback)(int message, void *data), RIP_MANAGER_OPTIONS *options);
void			rip_manager_stop();
char *			rip_manager_get_error_str(error_code code);
u_short			rip_mananger_get_relay_port();	

/******************************************************************************
 * Private functions
 ******************************************************************************/
static void			ripthread(void *bla);
static int			myrecv(char* buffer, int size);
static error_code		start_relay();
static void			post_status(int status);
static int			set_output_directory();
//static error_code		start_track(char *track);
//static error_code		rip_manager_end_track(char *track);
//static error_code		rip_manager_put_data(char *buf, int size);
static error_code		start_ripping();
static void			destroy_subsystems();


/******************************************************************************
 * Private Vars
 ******************************************************************************/
static SR_HTTP_HEADER		m_info;
static HSOCKET			m_sock;
static void			(*m_destroy_func)();
static RIP_MANAGER_INFO		m_ripinfo;		// used for UPDATE callback messages
static RIP_MANAGER_OPTIONS	m_options;		// local copy of the options passed to rip_manager_start()
static THREAD_HANDLE		m_hthread;		// rip thread handle
static IO_DATA_INPUT		m_in;			// Gets raw stream data
static IO_GET_STREAM		m_ripin;		// Raw stream data + song information
//static IO_PUT_STREAM		m_ripout;		// Generic output interface
static void			(*m_status_callback)(int message, void *data);
static BOOL			m_ripping = FALSE;
static u_long			m_bytes_ripped;
static HSEM			m_started_sem;	// to prevent deadlocks when ripping is stopped before its
													// started.

/*
 * Needs english type messages, just copy pasted for now
 */
static char m_error_str[NUM_ERROR_CODES][MAX_ERROR_STR];
#define SET_ERR_STR(str, code)	strncpy(m_error_str[code], str, MAX_ERROR_STR);

void init_error_strings()
{
    SET_ERR_STR("SR_SUCCESS",				0x00);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_SEPERATION",	0x01);
    SET_ERR_STR("SR_ERROR_DECODE_FAILURE",			0x02);
    SET_ERR_STR("SR_ERROR_INVALID_URL",			0x03);
    SET_ERR_STR("SR_ERROR_WIN32_INIT_FAILURE",		0x04);
    SET_ERR_STR("Could not connect to the stream. Try checking that the stream is up\n"
		"and that your proxy settings are correct.",0x05);
    SET_ERR_STR("SR_ERROR_CANT_RESOLVE_HOSTNAME",		0x06);
    SET_ERR_STR("SR_ERROR_RECV_FAILED",				0x07);
    SET_ERR_STR("SR_ERROR_SEND_FAILED",				0x08);
    SET_ERR_STR("SR_ERROR_PARSE_FAILURE",			0x09);
    SET_ERR_STR("SR_ERROR_NO_RESPOSE_HEADER",			0x0a);
    SET_ERR_STR("Server returned an unknown error code",0x0b);
    SET_ERR_STR("SR_ERROR_NO_META_INTERVAL",			0x0c);
    SET_ERR_STR("SR_ERROR_INVALID_PARAM",			0x0d);
    SET_ERR_STR("SR_ERROR_NO_HTTP_HEADER",			0x0e);
    SET_ERR_STR("SR_ERROR_CANT_GET_LIVE365_ID",			0x0f);
    SET_ERR_STR("SR_ERROR_CANT_ALLOC_MEMORY",			0x10);
    SET_ERR_STR("SR_ERROR_CANT_FIND_IP_PORT",			0x11);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MEMBERNAME",		0x12);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_NAME",		0x13);
    SET_ERR_STR("SR_ERROR_NULL_MEMBER_NAME",			0x14);
    SET_ERR_STR("SR_ERROR_CANT_FIND_TIME_TAG",			0x15);
    SET_ERR_STR("SR_ERROR_BUFFER_EMPTY",			0x16);
    SET_ERR_STR("SR_ERROR_BUFFER_FULL",				0x17);
    SET_ERR_STR("SR_ERROR_CANT_INIT_XAUDIO",			0x18);
    SET_ERR_STR("SR_ERROR_BUFFER_TOO_SMALL",			0x19);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_THREAD",			0x1A);
    SET_ERR_STR("SR_ERROR_CANT_FIND_MPEG_HEADER",		0x1B);
    SET_ERR_STR("SR_ERROR_INVALID_METADATA",			0x1C);
    SET_ERR_STR("SR_ERROR_NO_TRACK_INFO",			0x1D);
    SET_ERR_STR("SR_EEROR_CANT_FIND_SUBSTR",			0x1E);
    SET_ERR_STR("SR_ERROR_CANT_BIND_ON_PORT",			0x1F);
    SET_ERR_STR("SR_ERROR_HOST_NOT_CONNECTED",			0x20);
    SET_ERR_STR("HTTP:404 Not Found",				0x21);
    SET_ERR_STR("HTTP:401 Unauthorized",			0x22);
    SET_ERR_STR("HTTP:502 Bad Gateway",				0x23);	// Connection Refused
    SET_ERR_STR("SR_ERROR_CANT_CREATE_FILE",			0x24);
    SET_ERR_STR("SR_ERROR_CANT_WRITE_TO_FILE",			0x25);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_DIR",			0x26);
    SET_ERR_STR("HTTP:400 Bad Request ",			0x27);	// Server Full
    SET_ERR_STR("SR_ERROR_CANT_SET_SOCKET_OPTIONS",		0x28);
    SET_ERR_STR("SR_ERROR_SOCK_BASE",				0x29);
    SET_ERR_STR("SR_ERROR_INVALID_DIRECTORY",			0x2a);
    SET_ERR_STR("SR_ERROR_FAILED_TO_MOVE_FILE",			0x2b);
    SET_ERR_STR("SR_ERROR_CANT_LOAD_MPGLIB",			0x2c);
    SET_ERR_STR("SR_ERROR_CANT_INIT_MPGLIB",			0x2d);
    SET_ERR_STR("SR_ERROR_CANT_UNLOAD_MPGLIB",			0x2e);
    SET_ERR_STR("SR_ERROR_PCM_BUFFER_TO_SMALL",			0x2f);
    SET_ERR_STR("SR_ERROR_CANT_DECODE_MP3",			0x30);
    SET_ERR_STR("SR_ERROR_SOCKET_CLOSED",			0x31);
    SET_ERR_STR("Due to legal reasons Streamripper can no longer work with Live365(tm).\r\n"
		"See streamripper.sourceforge.net for more on this matter.", 0x32);
    SET_ERR_STR("The maximum number of bytes ripped has been reached", 0x33);
    SET_ERR_STR("SR_ERROR_CANT_WAIT_ON_THREAD",			0x34);
    SET_ERR_STR("SR_ERROR_CANT_CREATE_EVENT",			0x35);
    SET_ERR_STR("SR_ERROR_NOT_SHOUTCAST_STREAM",		0x36);
    SET_ERR_STR("HTTP:407 - Proxy Authentication Required",	0x37);
    SET_ERR_STR("HTTP:403 - Access Forbidden (try changing the UserAgent)", 0x38);
    SET_ERR_STR("The output directory length is too long", 0x39);
    SET_ERR_STR("SR_ERROR_PROGRAM_ERROR", 0x3a);
    SET_ERR_STR("SR_ERROR_TIMEOUT", 0x3b);
    SET_ERR_STR("SR_ERROR_SELECT_FAILED", 0x3c);
}

char*
rip_manager_get_error_str(error_code code)
{
    if (code > 0 || code < -NUM_ERROR_CODES)
        return NULL;
    return m_error_str[-code];
}

u_short
rip_mananger_get_relay_port()
{
    return m_options.relay_port;
}

void
post_error(int err)
{
    ERROR_INFO err_info;
    err_info.error_code = err;
    strcpy(err_info.error_str, m_error_str[abs(err)]);
    m_status_callback(RM_ERROR, &err_info);
}


void
post_status(int status)
{
    if (status != 0)
        m_ripinfo.status = status;
    m_status_callback(RM_UPDATE, &m_ripinfo);
}


int
myrecv(char* buffer, int size)
{
    int ret;
    /* GCS: Jun 5, 2004.  Here is where I think we are getting aussie's 
       problem with the SR_ERROR_INVALID_METADATA or SR_ERROR_NO_TRACK_INFO
       messages */
    ret = socklib_recvall(&m_sock, buffer, size, m_options.timeout);
    if (ret >= 0 && ret != size) {
	debug_printf ("rip_manager_recv: expected %d, got %d\n",size,ret);
	ret = SR_ERROR_RECV_FAILED;
    }
    return ret;
}

/* 
 * This is called by ripstream when we get a new track. 
 * most logic is handled by filelib_start() so we just
 * make sure there are no bad characters in the name and 
 * update via the callback 
 */
error_code
rip_manager_start_track(char *trackname, int track_count)
{
    int ret;

    debug_printf("start_track: %s\n", trackname);

    if ((ret = filelib_start(trackname)) != SR_SUCCESS) {
        return ret;
    }

    strcpy(m_ripinfo.filename, trackname);
    m_ripinfo.filesize = 0;
    m_ripinfo.track_count = track_count;
    m_status_callback(RM_NEW_TRACK, (void *)trackname);
    post_status(0);

    return SR_SUCCESS;
}

/* Ok, the end_track()'s function is actually to move 
 * tracks out from the incomplete directory. It does 
 * get called, but only after the 2nd track is played. 
 * the first track is *never* complete.
 */
error_code
rip_manager_end_track(char *trackname)
{
    char fullpath[SR_MAX_PATH];

    filelib_end(trackname, GET_OVER_WRITE_TRACKS(m_options.flags), fullpath);
    post_status(0);
    m_status_callback(RM_TRACK_DONE, (void*)fullpath);

    return SR_SUCCESS;
}

error_code
rip_manager_put_data(char *buf, int size)
{
    filelib_write_track(buf, size);

    m_ripinfo.filesize += size;
    m_bytes_ripped += size;

    return SR_SUCCESS;
}

error_code
rip_manager_put_raw_data(char *buf, int size)
{
    relaylib_send(buf, size);
    filelib_write_show (buf, size);
    return SR_SUCCESS;
}


/* GCS - Honestly, the filename code needs to be ripped out and rewritten.
   Problems include: (1) no wchar support, (2) truncation and stripping
   should not be applied to ID3 data, (3) MAX_PATH and NAME_MAX should 
   be properly used, or even better pathconf (see glibc manual for example),
   (4) why is it smeared across rip_manager.c and filelib.c ?

   But "for now", I've hacked it to do better truncation at least in this 
   limited circumstance: ASCII streams with MAX_PATH == NAME_MAX.
*/
int
set_output_directory()
{
    int rc;
    rc = filelib_set_output_directory (m_options.output_directory, 
	GET_SEPERATE_DIRS(m_options.flags),
	GET_DATE_STAMP(m_options.flags),
	m_info.icy_name);
    if (rc != SR_SUCCESS)
	return rc;
//    m_status_callback(RM_OUTPUT_DIR, (void*)newpath);
    m_status_callback(RM_OUTPUT_DIR, (void*)filelib_get_output_directory);
    return rc;
}

/* 
 * Fires up the relaylib stuff. Most of it is so relaylib 
 * knows what to call the stream which is what the 'construct_sc_repsonse()'
 * call is about.
 */
error_code
start_relay()
{	
    int ret;
    SR_HTTP_HEADER info = m_info;
    char temp_icyname[MAX_SERVER_LEN];

    char headbuf[MAX_HEADER_LEN];
//    info.meta_interval = NO_META_INTERVAL;
    sprintf(temp_icyname, "[%s] %s", "relay stream", info.icy_name);
    strcpy(info.icy_name, temp_icyname);
    if ((ret = httplib_construct_sc_response(&info, headbuf, MAX_HEADER_LEN)) != SR_SUCCESS)
	return ret;

    if (!relaylib_isrunning())
	if ((ret = relaylib_start()) != SR_SUCCESS)
	    return ret;

    if ((ret = relaylib_set_response_header(headbuf)) != SR_SUCCESS)
	return ret;

    return SR_SUCCESS;
}


void
ripthread(void *notused)
{
    error_code ret;

    if ((ret = start_ripping()) != SR_SUCCESS) {
	threadlib_signel_sem(&m_started_sem);
	post_error(ret);
	goto DONE;
    }
    m_status_callback(RM_STARTED, (void *)NULL);
    post_status(RM_STATUS_BUFFERING);
    threadlib_signel_sem(&m_started_sem);

    while(TRUE) {
        ret = ripstream_rip();

	/* If the user told use to stop, well, then we bail */
	if (!m_ripping)
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
	if (m_bytes_ripped >= (m_options.maxMB_rip_size*1000000) &&
		GET_CHECK_MAX_BYTES(m_options.flags)) {
	    socklib_close(&m_sock);
	    destroy_subsystems();
	    post_error(SR_ERROR_MAX_BYTES_RIPPED);
	    break;
	}

	/*
	 * If the error was anything but a connection failure
	 * then we need to report it and bail
	 */
	if (ret == SR_SUCCESS_BUFFERING) {
	    post_status(RM_STATUS_BUFFERING);
	}
	else if (ret == SR_ERROR_CANT_DECODE_MP3) {
	    post_error(ret);
	    continue;
	}
	else if ((ret == SR_ERROR_RECV_FAILED || ret == SR_ERROR_TIMEOUT ||
		  ret == SR_ERROR_SELECT_FAILED) && 
		     GET_AUTO_RECONNECT(m_options.flags)) {
	    /*
	     * Try to reconnect, if thats what the user wants
	     */
	    post_status(RM_STATUS_RECONNECTING);
	    while(m_ripping) {

		// Hopefully this solves a lingering bug 
		// with auto-reconnects failing to bind to the relay port
		// (a long with other unknown problems)
		// it seems that maybe we need two levels of shutdown and startup
		// functions. init_system, init_rip, shutdown_system and shutdown_rip
		// this would be a shutdown_rip, which only lacks the filelib and socklib
		// shutdowns
		// because filelib needs to keep track of it's file counters, and socklib.. umm..
		// not sure about. no reasdon to i imagine.
		socklib_close(&m_sock);
		relaylib_shutdown();
		ripstream_destroy();
		if (m_destroy_func)
			m_destroy_func();

		ret = start_ripping();
		if (ret == SR_SUCCESS)
		    break;

		/*
		 * Should send a message that we are trying 
		 * .. or something
		 */
		Sleep(1000);
	    }
	}
	else if (ret != SR_SUCCESS) {
	    destroy_subsystems();
	    post_error(ret);
	    break;
	}

	if (m_ripinfo.filesize > 0)
	    post_status(RM_STATUS_RIPPING);
    }

    // We get here when there was either a fatal error
    // or we we're not auto-reconnecting and the stream just stopped
    // or when we have been told to stop, via the m_ripping flag
DONE:
    m_status_callback(RM_DONE, &m_ripinfo);
    m_ripping = FALSE;
}


void
rip_manager_stop()
{
    // Make sure this function isn't getting called twice
    if (!m_ripping)
	    return;
    
    // Make sure the ripping started before we try to stop
    threadlib_waitfor_sem(&m_started_sem);
    m_ripping = FALSE;

    // Causes the code running in the thread to bail
    socklib_close(&m_sock);

    // blocks until everything is ok and closed
    threadlib_waitforclose(&m_hthread);
    destroy_subsystems();
    threadlib_destroy_sem(&m_started_sem);
}

void
destroy_subsystems()
{
    ripstream_destroy();
    if (m_destroy_func) {
	m_destroy_func();
    }
    relaylib_shutdown();
    socklib_cleanup();
    filelib_shutdown();
}

error_code
start_ripping()
{
    error_code ret;
    char *pproxy = m_options.proxyurl[0] ? m_options.proxyurl : NULL;

    /*
     * Connect to the stream
     */
    ret = inet_sc_connect(&m_sock, m_options.url, pproxy, &m_info, 
			  m_options.useragent, m_options.if_name);
    if (ret != SR_SUCCESS) {
	goto RETURN_ERR;
    }

#if defined (COMMENTOUT_FOR_OGG)
    /* GCS 09/10/04 - I wonder if this is worth doing */
    if (!m_info.have_icy_name) {
	ret = SR_ERROR_NOT_SHOUTCAST_STREAM;
	goto RETURN_ERR;
    }
#endif

    /* If the icy_name exists, but is empty, set to a bogus name so 
       that we can create the directory correctly, etc. */
    if (strlen(m_info.icy_name) == 0) {
	strcpy (m_info.icy_name, "Streamripper_rips");
    }

    /*
     * Set the ripinfo struct from the data we now know about the 
     * stream, this info are things like server name, type, 
     * bitrate etc.. 
     */
    memset(&m_ripinfo, 0, sizeof(RIP_MANAGER_INFO));
    m_ripinfo.meta_interval = m_info.meta_interval;
    m_ripinfo.bitrate = m_info.icy_bitrate;
    strcpy(m_ripinfo.streamname, m_info.icy_name);
    strcpy(m_ripinfo.server_name, m_info.server);

    ret = set_output_directory();
    if (ret != SR_SUCCESS) {
	goto RETURN_ERR;
    }

    /*
     * currently it works like this: 
     * ripshout and riplive are responsable for providing a track name
     * and pulling data. ripshout, is responsable for seeing when a track
     * changed, then decoding a buffer to PCM (raw audio) and looking for
     * a silent point, it then call tells the "output" interface to change 
     * track.
     *
     * All of this is handled through structures with function pointers, 
     * or a poor mans virtual classes.
     */
//  if (strcmp(m_info.server, "Nanocaster/2.0") == 0)	// This should be user setable, the name changed once in the last month
//  {
//	ret = SR_ERROR_LIVE365;
//	goto RETURN_ERR;
//
//	/*
//	 * NOTE: live365 needs a url, proxy stuff so it can 
//	 * page the webpage with the track information. this is
//	 * a good example of whats wrong with the current idea of 
//	 * plugins
//	 */
//	if ((ret = riplive365_init(&m_in, &m_ripin, m_info.icy_name, m_options.url, pproxy)) != SR_SUCCESS)
//		goto RETURN_ERR;
//
//	/*
//	 * this happened with rogers classical, just default to 56k
//	 */
//	if (m_info.icy_bitrate == 0)
//	    m_info.icy_bitrate = 56;
//	mult = m_info.icy_bitrate * 3;
//	m_destroy_func = riplive365_destroy;
//  }
//  else
    {
	/* prepares the ripshout lib for ripping */
	ret = ripshout_init(&m_in, &m_ripin, m_info.meta_interval, 
		m_info.icy_name);
	if (ret != SR_SUCCESS)
	    goto RETURN_ERR;
	m_destroy_func = ripshout_destroy;
    }

    /*
     * ripstream is good to go, it knows how to get data, and where
     * it's sending it to
     */
    ripstream_destroy();
#if defined (commentout)
    ret = ripstream_init(&m_ripin, &m_ripout, 
			 m_info.icy_name, m_options.dropstring,
			 &m_options.sp_opt, m_ripinfo.bitrate, 
			 GET_ADD_ID3(m_options.flags));
#endif
    ret = ripstream_init(&m_ripin, 
			 m_info.icy_name, m_options.dropstring, m_options.dropcount,
			 &m_options.sp_opt, m_ripinfo.bitrate, 
			 GET_ADD_ID3(m_options.flags));
    if (ret != SR_SUCCESS) {
	ripstream_destroy();
	goto RETURN_ERR;
    }

    /*
     * tells the socket relay to start, this local wrapper is for 
     * setting the respond header, it needs to know server info to 
     * give winamp any usfull information about
     * the stream we are relaying.. this just sets the header to 
     * something very simulare to what we got from the stream.
     */
    if (GET_MAKE_RELAY(m_options.flags)) {
	int new_port = 0;
	ret = relaylib_init(GET_SEARCH_PORTS(m_options.flags), 
			    m_options.relay_port, m_options.max_port, 
			    &new_port, m_options.if_name, 
			    m_options.max_connections);
	if (ret != SR_SUCCESS) {
		goto RETURN_ERR;
	}
	m_options.relay_port = new_port;
	start_relay();
    }
    post_status(RM_STATUS_BUFFERING);
    return SR_SUCCESS;

RETURN_ERR:
    socklib_close(&m_sock);
    if (m_destroy_func)
	m_destroy_func();
    return ret;	
}

/* 
 * From an email to oddsock he asked how one would be able to add ID3 data
 * to the tracks with the current "system". this was my reply:
 * ----
 * Personally I think this system sucks. feel free to just remove all of it 
 * and do it the most strait forward way possible, i have some much better 
 * ideas for handling these systems now, so i don't really care about what 
 * happens to it. However if you don't want to fuck with it to much the 
 * easyest way to provide id3 info to filelib would probably be in 
 * ripstream.c, before you call 'end_track()' or (IO_PUT_STREAM::end_track() 
 * as it is) append the ID3 data to the "strema". of course the first track 
 * wouldn't get it, so you might have to do something clever around the 
 * start_track call or something, i'm not sure.
 * 
 * -Jon
 */
error_code
rip_manager_start(void (*status_callback)(int message, void *data), 
			     RIP_MANAGER_OPTIONS *options)
{

	int ret = 0;
	m_started_sem = threadlib_create_sem();
	if (m_ripping)
		return SR_SUCCESS;		// to prevent reentrenty

	m_ripping = TRUE;

	initialize_default_locale();

	if (!options)
		return SR_ERROR_INVALID_PARAM;

	filelib_init(GET_COUNT_FILES(options->flags),
			GET_KEEP_INCOMPLETE(options->flags),
			GET_SINGLE_FILE_OUTPUT(options->flags),
			options->output_file);
	socklib_init();

	init_error_strings();
	m_in.get_input_data = myrecv;
	m_status_callback = status_callback;
	m_destroy_func = NULL;
	m_bytes_ripped = 0;

	/*
  	 * Get a local copy of the options passed
  	 */
	memcpy(&m_options, options, sizeof(RIP_MANAGER_OPTIONS));

	/*
	 * Start the ripping thread
	 */
	m_ripping = TRUE;
	if ((ret = threadlib_beginthread(&m_hthread, ripthread)) != SR_SUCCESS)
		return ret;
	return SR_SUCCESS;
}

void
set_rip_manager_options_defaults (RIP_MANAGER_OPTIONS *m_opt)
{
    m_opt->relay_port = 8000;
    m_opt->max_port = 18000;
    m_opt->flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPERATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    OPT_ADD_ID3;
    m_opt->timeout = 0;
    m_opt->max_connections = 1;

    strcpy(m_opt->output_directory, "./");
    m_opt->proxyurl[0] = (char)NULL;
    m_opt->url[0] = '\0';
    m_opt->output_file[0] = 0;
    strcpy(m_opt->useragent, "sr-POSIX/" SRVERSION);

    // Defaults for splitpoint
    // Times are in ms
    m_opt->sp_opt.xs = 1;
    m_opt->sp_opt.xs_min_volume = 1;
    m_opt->sp_opt.xs_silence_length = 1000;
    m_opt->sp_opt.xs_search_window_1 = 6000;
    m_opt->sp_opt.xs_search_window_2 = 6000;
    m_opt->sp_opt.xs_offset = 0;
    m_opt->sp_opt.xs_padding_1 = 300;
    m_opt->sp_opt.xs_padding_2 = 300;
}

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
#include "live365info.h"
#include "inet.h"
#include "relaylib.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "threadlib.h"

#if __UNIX__
	#include <unistd.h>
	#define Sleep(x) usleep(x)
#elif __BEOS__
	#define Sleep(x) snooze(x)
#endif

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code		rip_manager_start(void (*status_callback)(int message, void *data), RIP_MANAGER_OPTIONS *options);
void			rip_manager_stop();

/******************************************************************************
 * Private functions
 ******************************************************************************/
static void				ripthread(void *bla);
static int				myrecv(char* buffer, int size);
static error_code		start_relay();
static void				set_ripinfo();
static void				set_status(int status);
static void				set_output_directory();
static error_code		start_track(char *track);
static error_code		end_track(char *track);
static error_code		put_data(char *buf, int size);
static void				one_time_init();


/******************************************************************************
 * Private Vars
 ******************************************************************************/
static SR_HTTP_HEADER		m_info;
static HSOCKET				m_sock;
static void					(*m_destroy_func)();
static RIP_MANAGER_INFO		m_ripinfo;
static RIP_MANAGER_OPTIONS	m_options;
static THREAD_HANDLE		m_hthread;
static IO_DATA_INPUT		m_in;	 // Gets raw stream data
static IO_GET_STREAM		m_ripin; // Raw stream data + song information
static IO_PUT_STREAM		m_ripout;// Generic output interface
static void					(*m_status_callback)(int message, void *data);
static BOOL					m_told_to_stop = FALSE;
static u_long				m_bytes_ripped;

/*
 * Needs english type messages, just copy pasted for now
 */
static char m_error_str[MAX_ERROR_STR][NUM_ERROR_CODES];
#define SET_ERR_STR(str, code)	strncpy(m_error_str[code], str, MAX_ERROR_STR);

void init_error_strings()
{
	SET_ERR_STR("SR_SUCCESS", 0x00);
	SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_SEPERATION",	0x01)
	SET_ERR_STR("SR_ERROR_DECODE_FAILURE",				0x02)
	SET_ERR_STR("SR_ERROR_INVALID_URL",					0x03)
	SET_ERR_STR("SR_ERROR_WIN32_INIT_FAILURE",			0x04)
	SET_ERR_STR("SR_ERROR_CONNECT_FAILED",				0x05)
	SET_ERR_STR("SR_ERROR_CANT_RESOLVE_HOSTNAME",		0x06)
	SET_ERR_STR("SR_ERROR_RECV_FAILED",					0x07)
	SET_ERR_STR("SR_ERROR_SEND_FAILED",					0x08)
	SET_ERR_STR("SR_ERROR_PARSE_FAILURE",				0x09)
	SET_ERR_STR("SR_ERROR_NO_RESPOSE_HEADER",			0x0a)
	SET_ERR_STR("SR_ERROR_NO_ICY_CODE",					0x0b)
	SET_ERR_STR("SR_ERROR_NO_META_INTERVAL",			0x0c)
	SET_ERR_STR("SR_ERROR_INVALID_PARAM",				0x0d)
	SET_ERR_STR("SR_ERROR_NO_HTTP_HEADER",				0x0e)
	SET_ERR_STR("SR_ERROR_CANT_GET_LIVE365_ID",			0x0f)
	SET_ERR_STR("SR_ERROR_CANT_ALLOC_MEMORY",			0x10)
	SET_ERR_STR("SR_ERROR_CANT_FIND_IP_PORT",			0x11)
	SET_ERR_STR("SR_ERROR_CANT_FIND_MEMBERNAME",		0x12)
	SET_ERR_STR("SR_ERROR_CANT_FIND_TRACK_NAME",		0x13)
	SET_ERR_STR("SR_ERROR_NULL_MEMBER_NAME",			0x14)
	SET_ERR_STR("SR_ERROR_CANT_FIND_TIME_TAG",			0x15)
	SET_ERR_STR("SR_ERROR_BUFFER_EMPTY",				0x16)
	SET_ERR_STR("SR_ERROR_BUFFER_FULL",					0x17)
	SET_ERR_STR("SR_ERROR_CANT_INIT_XAUDIO",			0x18)
	SET_ERR_STR("SR_ERROR_BUFFER_TOO_SMALL",			0x19)
	SET_ERR_STR("SR_ERROR_CANT_CREATE_THREAD",			0x1A)
	SET_ERR_STR("SR_ERROR_CANT_FIND_MPEG_HEADER",		0x1B)
	SET_ERR_STR("SR_ERROR_INVALID_METADATA",			0x1C)
	SET_ERR_STR("SR_ERROR_NO_TRACK_INFO",				0x1D)
	SET_ERR_STR("SR_EEROR_CANT_FIND_SUBSTR",			0x1E)
	SET_ERR_STR("SR_ERROR_CANT_BIND_ON_PORT",			0x1F)
	SET_ERR_STR("SR_ERROR_HOST_NOT_CONNECTED",			0x20)
	SET_ERR_STR("SR_ERROR_HTTP_404_ERROR",				0x21)
	SET_ERR_STR("SR_ERROR_HTTP_401_ERROR",				0x22)	
	SET_ERR_STR("SR_ERROR_HTTP_502_ERROR",				0x23)	// Connection Refused
	SET_ERR_STR("SR_ERROR_CANT_CREATE_FILE",			0x24)
	SET_ERR_STR("SR_ERROR_CANT_WRITE_TO_FILE",			0x25)
	SET_ERR_STR("SR_ERROR_CANT_CREATE_DIR",				0x26)
	SET_ERR_STR("SR_ERROR_HTTP_400_ERROR",				0x27)	// Server Full
	SET_ERR_STR("SR_ERROR_CANT_SET_SOCKET_OPTIONS",		0x28)
	SET_ERR_STR("SR_ERROR_SOCK_BASE",					0x29)
	SET_ERR_STR("SR_ERROR_INVALID_DIRECTORY",			0x2a)
	SET_ERR_STR("SR_ERROR_FAILED_TO_MOVE_FILE",			0x2b)
	SET_ERR_STR("SR_ERROR_CANT_LOAD_MPGLIB",			0x2c)
	SET_ERR_STR("SR_ERROR_CANT_INIT_MPGLIB",			0x2d)
	SET_ERR_STR("SR_ERROR_CANT_UNLOAD_MPGLIB",			0x2e)
	SET_ERR_STR("SR_ERROR_PCM_BUFFER_TO_SMALL",			0x2f)
	SET_ERR_STR("SR_ERROR_CANT_DECODE_MP3",				0x30)
	SET_ERR_STR("SR_ERROR_SOCKET_CLOSED",				0x31)
	SET_ERR_STR("Due to legal reasons Streamripper can no longer work with Live365(tm).\r\n"
			"See streamripper.sourceforge.net for more on this matter.", 0x32)
	SET_ERR_STR("The maximum number of bytes ripped has been reached", 0x33)
	
}

char *rip_manager_get_error_str(error_code code)
{
	if (code > 0 || code < -NUM_ERROR_CODES)
		return NULL;

	return m_error_str[-code];
}

void post_error(int err)
{
	ERROR_INFO err_info;
	err_info.error_code = err;
	strcpy(err_info.error_str, m_error_str[abs(err)]);
	m_status_callback(RM_ERROR, &err_info);
}

/*
 * Entry point for the ripping thread. for the most part 
 * it just calls ripstream_rip() which in turn will 'pull' stream
 * data. It gets this stream data from it's rip*'r. ripper's are 
 * part of a half baked plugin concept... i'll talk more about it latter
 * anyway. All you need to know for know is that ripstream_rip()
 * will start_track(), end_track() and put_data(). 
 *
 * The rest of the function does the auto-reconnect stuff. it's pretty
 * hacky, but works.
 */
void ripthread(void *bla)
{
	int ret = SR_SUCCESS;

	debug_printf("just entered ripthread()\n");
	m_status_callback(RM_STARTED, (void *)NULL);
	set_status(RM_STATUS_BUFFERING);
	m_status_callback(RM_UPDATE, &m_ripinfo);

	while(threadlib_isrunning(&m_hthread))
	{
		ret = ripstream_rip();
		if (ret == SR_SUCCESS_BUFFERING)
		{
			set_status(RM_STATUS_BUFFERING);
			m_status_callback(RM_UPDATE, &m_ripinfo);
		}

		if (ret == SR_ERROR_CANT_DECODE_MP3)
		{
			post_error(ret);
		}
		else if (ret < SR_SUCCESS)
		{
			break;
		}

		if (m_ripinfo.filesize > 0)
			set_status(RM_STATUS_RIPPING);

		/* 
		 * Added 8/4/2001 jc --
		 * for max number of MB ripped stuff
		 * At the time of writting this, i honestly 
		 * am not sure i understand what happens 
		 * when once trys to stop the ripping from the lib and
		 * the interface it's a weird combonation of threads
		 * wnd locks, etc.. all i know is that this appeared to work
		 */
		if (m_bytes_ripped >= (m_options.maxMB_rip_size*1048576) &&
			OPT_FLAG_ISSET(m_options.flags, OPT_CHECK_MAX_BYTES))
		{
			m_told_to_stop = TRUE;
			post_error(SR_ERROR_MAX_BYTES_RIPPED);
			break;
		}

	}
	
	if (m_told_to_stop)
		threadlib_close(&m_hthread);
	else
	{
		/*
		 * This happens if the connection got messed up.. or some other error
		 */
		threadlib_close(&m_hthread);
		if (ret != SR_ERROR_RECV_FAILED)
		{
			/*
			 * some other error, post it to the host
			 */
			post_error(ret);
		}
		else
		{
			/*
			 * Try to reconnect 
			 */
			if (OPT_FLAG_ISSET(m_options.flags, OPT_AUTO_RECONNECT))
			{
				rip_manager_stop();
				
				set_status(RM_STATUS_RECONNECTING);
				while(rip_manager_start(m_status_callback, &m_options) != SR_SUCCESS && !m_told_to_stop)
				{
					/*
					 * Should send a message that we are trying 
					 * .. or something
					 */
					m_status_callback(RM_UPDATE, &m_ripinfo);
					Sleep(1000);
				}
			}
		}
	}

	m_status_callback(RM_DONE, (void *)NULL);
	debug_printf("done with ripthread()\n");
}

void rip_manager_stop()
{
debug_printf("just entered rip_manager_stop()\n");
	m_told_to_stop = TRUE;

	// Tells the thread lib it's ok to stop
	threadlib_stop(&m_hthread);
	// Causes the code running in the thread to bail
	socklib_close(&m_sock);
	// blocks until everything is ok and closed
	threadlib_waitforclose(&m_hthread);

	ripstream_destroy();
	if (m_destroy_func)
		m_destroy_func();
	m_told_to_stop = FALSE;

	relaylib_shutdown();
	socklib_cleanup();
	filelib_shutdown();
debug_printf("leaving rip_manager_stop()\n");
}


void set_status(int status)
{
	m_ripinfo.status = status;
}


int myrecv(char* buffer, int size)
{
	return socklib_recvall(&m_sock, buffer, size);
}

/* 
 * This is called by ripstream when we get a new track. 
 * most logic is handled by filelib_start() so we just
 * make sure there are no bad characters in the name and 
 * update via the callback 
 */
error_code start_track(char *trackname)
{
	char temptrack[MAX_TRACK_LEN];
	int ret;
	
	strcpy(temptrack, trackname);

	strip_invalid_chars(temptrack);
	if ((ret = filelib_start(temptrack)) != SR_SUCCESS)
	{
		return ret;
	}

	strcpy(m_ripinfo.filename, temptrack);
	m_ripinfo.filesize = 0;
	m_status_callback(RM_NEW_TRACK, (void *)temptrack);
	m_status_callback(RM_UPDATE, (void*)&m_ripinfo);

	return SR_SUCCESS;
}

/* Ok, the end_track()'s function is actually to move 
 * tracks out from the incomplete directory. It does 
 * get called, but only after the 2nd track is played. 
 * the first track is *never* complete.
 */
error_code end_track(char *trackname)
{
	char temptrack[MAX_TRACK_LEN];

	strcpy(temptrack, trackname);

	strip_invalid_chars(temptrack);
	filelib_end(temptrack, m_options.over_write_existing_tracks);
	m_status_callback(RM_UPDATE, &m_ripinfo);
	m_status_callback(RM_TRACK_DONE, (void*)temptrack);

	return SR_SUCCESS;
}

error_code put_data(char *buf, int size)
{
	relaylib_send(buf, size);
	filelib_write(buf, size);
//	printf(".");
	
	m_ripinfo.filesize += size;
	m_bytes_ripped += size;
	m_status_callback(RM_UPDATE, &m_ripinfo);

	return SR_SUCCESS;
}


void set_output_directory()
{
	char newpath[MAX_PATH_LEN] = {'\0'};
	char *striped_icy_name;

	if (*m_options.output_directory)
	{
		strcat(newpath, m_options.output_directory);
#ifdef WIN32
		if (newpath[strlen(newpath)-1] != '\\')
			strcat(newpath, "\\");
#else
		if (newpath[strlen(newpath)-1] != '/')
			strcat(newpath, "/");
#endif

	}
	if (OPT_FLAG_ISSET(m_options.flags, OPT_SEPERATE_DIRS))
	{
		/* felix@blasphemo.net: insert date after icy_name for per-session directories */
		char *timestring  ;
		time_t timestamp;
		struct tm *theTime;
		int time_len;

		timestring = malloc(36);
		time(&timestamp);
		theTime = localtime(&timestamp);
		time_len = strftime(timestring, 35, "%Y-%B-%d", theTime);

		striped_icy_name = malloc(strlen(m_info.icy_name)+ 1);
		strcpy(striped_icy_name, m_info.icy_name);
		strip_invalid_chars(striped_icy_name);
		left_str(striped_icy_name, MAX_DIR_LEN);
		trim(striped_icy_name);
		strcat(newpath, striped_icy_name);

		if (OPT_FLAG_ISSET(m_options.flags, OPT_DATE_STAMP))
		{
			strcat(newpath, "_");
			strcat(newpath, timestring);
		}

		free(timestring);


		free(striped_icy_name);
	}
	filelib_set_output_directory(newpath);
	m_status_callback(RM_OUTPUT_DIR, (void*)newpath);

}

void set_ripinfo()
{
	memset(&m_ripinfo, 0, sizeof(RIP_MANAGER_INFO));
	m_ripinfo.meta_interval = m_info.meta_interval;
	m_ripinfo.bitrate = m_info.icy_bitrate;
	strcpy(m_ripinfo.streamname, m_info.icy_name);
	strcpy(m_ripinfo.server_name, m_info.server);
	//m_status_callback(RM_UPDATE, &m_ripinfo);
}
/* 
 * Fires up the relaylib stuff. Most of it is so relaylib 
 * knows what to call the stream which is what the 'construct_sc_repsonse()'
 * call is about.
 */
error_code start_relay()
{	
	int ret;
	SR_HTTP_HEADER info = m_info;
	char temp_icyname[MAX_SERVER_LEN];

	char headbuf[MAX_HEADER_LEN];
	info.meta_interval = NO_META_INTERVAL;
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

void one_time_init()
{
	static BOOL done = FALSE;

	if (done)
		return;

	init_error_strings();
	m_in.get_data = myrecv;
	m_ripout.start_track = start_track;
	m_ripout.end_track = end_track;
	m_ripout.put_data = put_data;
	done = TRUE;

	return;
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
error_code rip_manager_start(void (*status_callback)(int message, void *data), 
			     RIP_MANAGER_OPTIONS *options)
{
	int ret = 0;
	char *pproxy = options->proxyurl[0] ? m_options.proxyurl : NULL;
	int mult;

	if (!options)
		return SR_ERROR_INVALID_PARAM;

	one_time_init();
	filelib_init(OPT_FLAG_ISSET(options->flags, OPT_COUNT_FILES));
	socklib_init();
	m_status_callback = status_callback;
	m_told_to_stop = FALSE;
	m_destroy_func = NULL;
	m_bytes_ripped = 0;

	/*
  	 * Get a local copy of the options passed
  	 */
	memcpy(&m_options, options, sizeof(RIP_MANAGER_OPTIONS));

	/*
 	 * Connect to the stream
	 */
	if ((ret = inet_sc_connect(&m_sock, m_options.url, 
				   pproxy, &m_info)) != SR_SUCCESS)
	{
		goto RETURN_ERR;
	}


	/*
	 * Set the ripinfo struct from the data we now know about the 
	 * stream, this info are things like server name, type, 
	 * bitrate etc.. 
   	 * I just made it a function to cut down on the code in this function
  	 */
	set_ripinfo();

	set_output_directory();

 	/*
 	 * Ahh.. live365. They call there server's Nanocaster (i belive
 	 * it's actually icecast+mods). Anyway, as of 5/10/2001 (aprox)
 	 * I'm not allowed to let people do this, so you'll notice two
	 * lines below which "disable" this feature. don't uncomment those 
	 * lines :), or do, just don't tell me about it, heh.
 	 *
 	 * Hopefully soon i'll have a real plugin system. As I said earlyer
	 * this file just grew out of a test harness and the plugin thing
	 * never got finished. 
	 * 
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
	if (strcmp(m_info.server, "Nanocaster/2.0") == 0)	// This should be user setable, the name changed once in the last month
	{

		ret = SR_ERROR_LIVE365;
		goto RETURN_ERR;

		/*
		 * NOTE: live365 needs a url, proxy stuff so it can 
		 * page the webpage with the track information. this is
		 * a good example of whats wrong with the current idea of 
		 * plugins
	 	 */
		if ((ret = riplive365_init(&m_in, &m_ripin, m_info.icy_name, options->url, pproxy)) != SR_SUCCESS)
			goto RETURN_ERR;

		/*
		 * this happened with rogers classical, just default to 56k
	 	 */
		if (m_info.icy_bitrate == 0)
			m_info.icy_bitrate = 56;
		mult = m_info.icy_bitrate * 3;
		m_destroy_func = riplive365_destroy;

	}
	else
	{
		/*
		 * prepares the ripshout lib for ripping
		 */
		if ((ret = ripshout_init(&m_in, &m_ripin, m_info.meta_interval, m_info.icy_name)) != SR_SUCCESS)
			goto RETURN_ERR;

		/* 
		 * mult = metaint*mult. size of the buffer. i've tried 6, but it seems to still
		 * cut short on many streams, so i'm trying 10
		 */
		mult = 10;
		m_destroy_func = ripshout_destroy;
	}

	/*
	 * ripstream is good to go, it knows how to get data, and where
	 * it's sending it to
	 */
	if ((ret = ripstream_init(mult, &m_ripin, &m_ripout, m_info.icy_name, OPT_FLAG_ISSET(options->flags, OPT_ADD_ID3))) != SR_SUCCESS)
	{
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
	if (!OPT_FLAG_ISSET(options->flags, OPT_NO_RELAY))
	{
		int new_port = 0;
		if ((ret = relaylib_init(OPT_FLAG_ISSET(options->flags, OPT_SEARCH_PORTS), 
				options->relay_port, options->max_port, &new_port)) != SR_SUCCESS)
		{
			return ret;
		}
		options->relay_port = new_port;
		start_relay();
	}

	m_status_callback(RM_UPDATE, &m_ripinfo);

	/*
	 * Start the ripping thread
	 */
	if ((ret = threadlib_beginthread(&m_hthread, ripthread)) != SR_SUCCESS)
		goto RETURN_ERR;

	return SR_SUCCESS;


RETURN_ERR:
	socklib_close(&m_sock);
	if (m_destroy_func)
		m_destroy_func();

	return ret;
		
}


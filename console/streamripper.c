/* streamripper.c -
 * This little app should be seen as a demo for how to use the stremripper lib. 
 * The only file you need from the /lib dir is rip_mananger.h, and perhapes 
 * util.h (for stuff like formating the number of bytes).
 * 
 * the two functions of interest are main() for how to call rip_mananger_start
 * and rip_callback, which is a how you find out whats going on with the rip.
 * the rest of this file is really just boring command line parsing code.
 * and a signal handler for when the user hits CTRL+C
 */

#include <signal.h>
#if WIN32
#define sleep	Sleep
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "types.h"
#include "rip_manager.h"
#include "util.h"
#include "filelib.h"

/*******************************************************************************
 * Private functions
 ******************************************************************************/
static void print_usage();
static void print_status();
static void catch_sig(int code);
static void parse_arguments(int argc, char **argv);
static void rip_callback(int message, void *data);
static void parse_splitpoint_rules (char* rule);
static void verify_splitpoint_rules (void);

/*******************************************************************************
 * Private Vars 
 ******************************************************************************/
static char m_buffer_chars[] = {'\\', '|', '/', '-', '*'}; /* for formating */
static RIP_MANAGER_INFO 	m_curinfo; /* from the rip_manager callback */
static BOOL			m_started = FALSE;
static BOOL			m_alldone = FALSE;
static BOOL			m_got_sig = FALSE;
static BOOL 			m_dont_print = FALSE;
RIP_MANAGER_OPTIONS 		m_opt;
time_t				m_stop_time = 0;

/* main()
 * parse the aguments, tell the rip_mananger to start, we get in rip
 * status from our rip_callback function. m_opt was set from parse args
 * and contains all the various things one can do with the rip_mananger
 * like a relay server, auto-reconnects, output dir's stuff like that.
 *
 * Notice the crappy while loop, this is because the streamripper lib 
 * is asyncrouns (spelling?) only. It probably should have a blocking 
 * call as well, but i needed this for window'd apps.
 */

int main(int argc, char* argv[])
{
	int ret;
	time_t temp_time;
	signal(SIGINT, catch_sig);

	parse_arguments(argc, argv);
	fprintf(stderr, "Connecting...\n");
	if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
	{
		fprintf(stderr, "Couldn't connect to %s\n", m_opt.url);
		exit(1);
	}

	/* 
 	 * The m_got_sig thing is because you can't call into a thread 
  	 * (i.e. rip_manager_stop) from a signal handler.. or at least not
  	 * in FreeBSD 3.4, i don't know about linux or NT.
	 */
	while(!m_got_sig)
	{
		sleep(1);
		time(&temp_time);
		if (m_stop_time && (temp_time >= m_stop_time))
		{
			fprintf(stderr, "\nTime to stop is here, bailing\n");
			break; 
		}	
	}

	m_dont_print = TRUE;
	fprintf(stderr, "shutting down\n");
	rip_manager_stop();
	return 0;
}

void catch_sig(int code)
{
	fprintf(stderr, "\n");
	if (!m_started)
		exit(2);
	m_got_sig = TRUE;
}

/* 
 * This is to handle RM_UPDATE messages from rip_callback(), and more
 * importantly the RIP_MANAGER_INFO struct. Most of the code here
 * is for handling the pretty formating stuff otherwise it could be
 * much smaller.
 */
void print_status()
{
	char status_str[128];
	char filesize_str[64];
	static int buffering_tick = 0;
	BOOL static printed_fullinfo = FALSE;

	if (m_dont_print)
		return;

	if (printed_fullinfo && m_curinfo.filename[0])
	{

		switch(m_curinfo.status)
		{
			case RM_STATUS_BUFFERING:
				buffering_tick++;
				if (buffering_tick == 5)
					buffering_tick = 0;

				sprintf(status_str,"buffering - %c ",
					m_buffer_chars[buffering_tick]);

				fprintf(stderr, "[%14s] %.50s\r",
			   			status_str,
			   			m_curinfo.filename);
				break;

			case RM_STATUS_RIPPING:
				strcpy(status_str, "ripping...    ");
				format_byte_size(filesize_str, m_curinfo.filesize);
                                fprintf(stderr, "[%14s] %.50s [%7s]\r",
                                                status_str,
                                                m_curinfo.filename,
                                		filesize_str);
				break;
			case RM_STATUS_RECONNECTING:
				strcpy(status_str, "re-connecting..");
                                fprintf(stderr, "[%14s]\r", status_str);
				break;
		}
			
	}
	if (!printed_fullinfo)
	{
		fprintf(stderr, 
			   "stream: %s\n"
			   "server name: %s\n"
			   "bitrate: %d\n"
			   "meta interval: %d\n",
			   m_curinfo.streamname,
			   m_curinfo.server_name,
			   m_curinfo.bitrate,
			   m_curinfo.meta_interval);
		if(GET_MAKE_RELAY(m_opt.flags))
		{
			fprintf(stderr, "relay port: %d\n"
					"[%14s]\r",
					m_opt.relay_port,
					"getting track name... ");
		}

		printed_fullinfo = TRUE;
	}
}

/*
 * This will get called whenever anything interesting happens with the 
 * stream. Interesting are progress updates, error's, when the rip
 * thread stops (RM_DONE) starts (RM_START) and when we get a new track.
 *
 * for the most part this function just checks what kind of message we got
 * and prints out stuff to the screen.
 */
void rip_callback(int message, void *data)
{
	RIP_MANAGER_INFO *info;
	ERROR_INFO *err;
	switch(message)
	{
		case RM_UPDATE:
			info = (RIP_MANAGER_INFO*)data;
			memcpy(&m_curinfo, info, sizeof(RIP_MANAGER_INFO));
			print_status();
			break;
		case RM_ERROR:
			err = (ERROR_INFO*)data;
			fprintf(stderr, "\nerror %d [%s]\n", err->error_code, err->error_str);
			m_alldone = TRUE;
			break;
		case RM_DONE:
			fprintf(stderr, "bye..\n");
			m_alldone = TRUE;
			break;
		case RM_NEW_TRACK:
			fprintf(stderr, "\n");
			break;
		case RM_STARTED:
			m_started = TRUE;
			break;
	}
}

void print_usage()
{
        fprintf(stderr, "Usage: streamripper URL [OPTIONS]\n");
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "        -d <dir>       - The destination directory\n");
        fprintf(stderr, "        -s             - Don't create a directory for each stream\n");
        fprintf(stderr, "        -r <base port> - Create a relay server on base port, defaults to port 8000\n");
        fprintf(stderr, "        -z             - Don't scan for free ports if base port is not avail\n");
        fprintf(stderr, "        -p <url>       - Use HTTP proxy server at <url>\n");
        fprintf(stderr, "        -o             - Overwrite tracks in complete\n");
        fprintf(stderr, "        -t             - Don't overwrite tracks in incomplete\n");
        fprintf(stderr, "        -c             - Don't auto-reconnect\n");
        fprintf(stderr, "        -v             - Print version info and quite\n");
        fprintf(stderr, "        -l <seconds>   - number of seconds to run, otherwise runs forever\n");
        fprintf(stderr, "        -q             - add sequence number to output file\n");
        fprintf(stderr, "        -i             - dont add ID3V1 Tags to output file\n");
        fprintf(stderr, "        -u <useragent> - Use a different UserAgent then \"Streamripper\"\n");
        fprintf(stderr, "        -f <dstring>   - Don't create new track if metainfo contains <dstring>\n");
        fprintf(stderr, "        --x            - Invoke splitpoint detection rules\n");
}

/* 
 * Bla, boreing agument parsing crap, only reason i didn't use getopt
 * (which i did for an earlyer version) is because i couldn't find a good
 * port of it under Win32.. there probably is one, maybe i didn't look 
 * hard enough. 
 */
void parse_arguments(int argc, char **argv)
{
    int i;
    char *c;

    if (argc < 2)
    {
	print_usage();
	exit(2);
    }

    // Defaults
    set_rip_manager_options_defaults (&m_opt);

#if defined (commentout)
    m_opt.relay_port = 8000;
    m_opt.max_port = 18000;
    m_opt.flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPERATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    OPT_ADD_ID3;

    strcpy(m_opt.output_directory, "./");
    m_opt.proxyurl[0] = (char)NULL;
    strncpy(m_opt.url, argv[1], MAX_URL_LEN);
    strcpy(m_opt.useragent, "sr-POSIX/" SRVERSION);

    // Defaults for splitpoint
    // Times are in ms
    m_opt.sp_opt.xs = 1;
    m_opt.sp_opt.xs_min_volume = 1;
    m_opt.sp_opt.xs_silence_length = 1000;
    m_opt.sp_opt.xs_search_window_1 = 6000;
    m_opt.sp_opt.xs_search_window_2 = 6000;
    m_opt.sp_opt.xs_offset = 0;
    m_opt.sp_opt.xs_padding_1 = 300;
    m_opt.sp_opt.xs_padding_2 = 300;
#endif

    // get URL
    strncpy(m_opt->url, argv[1], MAX_URL_LEN);

    //get arguments
    for(i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    continue;

	c = strchr("dplu", argv[i][1]);
        if (c != NULL) {
            if ((i == (argc-1)) || (argv[i+1][0] == '-')) {
		fprintf(stderr, "option %s requires an argument\n", argv[i]);
		exit(1);
	    }
	}
	switch (argv[i][1])
	{
	case 'd':
	    i++;
	    //dynamically allocat enough space for the dest dir
	    strncpy(m_opt.output_directory, argv[i], MAX_DIR_LEN);
	    break;
	case 'i':
	    m_opt.flags ^= OPT_ADD_ID3;
	    break;
	case 'q':
	    m_opt.flags ^= OPT_COUNT_FILES;
	    break;
	case 's':
	    m_opt.flags ^= OPT_SEPERATE_DIRS;
	    break;
	case 'r':
	    m_opt.flags ^= OPT_MAKE_RELAY;
	    // Default
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    m_opt.relay_port = atoi(argv[i]);
	    break;
	case 'z':
	    m_opt.flags ^= OPT_SEARCH_PORTS;
	    m_opt.max_port = m_opt.relay_port+1000;
	    break;
#if defined (commentout)
	case 'x':
	    m_opt.flags ^= OPT_PAD_SONGS;
	    break;
#endif
	case '-':
	    parse_splitpoint_rules(&argv[i][2]);
	    break;
	case 'p':
	    i++;
	    strncpy(m_opt.proxyurl, argv[i], MAX_URL_LEN);
	    break;
	case 'o':
	    m_opt.flags |= OPT_OVER_WRITE_TRACKS;
	    break;
	case 't':
	    m_opt.flags |= OPT_KEEP_INCOMPLETE;
	    break;
	case 'c':
	    m_opt.flags ^= OPT_AUTO_RECONNECT;
	    break;
	case 'v':
	    printf("Streamripper %s by Jon Clegg <jonclegg@yahoo.com>\n", SRVERSION);
	    exit(0);
	case 'l':
	    i++;
	    time(&m_stop_time);
	    m_stop_time += atoi(argv[i]);
	    break;
	case 'u':
	    i++;
	    strncpy(m_opt.useragent, argv[i], MAX_USERAGENT_STR);
	    break;
	case 'f':
	    i++;
	    strncpy(m_opt.dropstring, argv[i], MAX_DROPSTRING_LEN);
	    break;
	}
    }

    /* Need to verify that splitpoint rules were sane */
    verify_splitpoint_rules ();
}

static void
parse_splitpoint_rules (char* rule)
{
    int x,y;

    if (!strcmp(rule,"xs_none")) {
	m_opt.sp_opt.xs = 0;
	printf ("Disable silence detection");
	return;
    }
    if (1==sscanf(rule,"xs_min_volume=%d",&x)) {
	m_opt.sp_opt.xs_min_volume = x;
	printf ("Setting minimum volume to %d\n",x);
	return;
    }
    if (1==sscanf(rule,"xs_silence_length=%d",&x)) {
	m_opt.sp_opt.xs_silence_length = x;
	printf ("Setting silence length to %d\n",x);
	return;
    }
    if (2==sscanf(rule,"xs_search_window=%d:%d",&x,&y)) {
	m_opt.sp_opt.xs_search_window_1 = x;
	m_opt.sp_opt.xs_search_window_2 = y;
	printf ("Setting search window to (%d:%d)\n",x,y);
	return;
    }
    if (1==sscanf(rule,"xs_offset=%d",&x)) {
	m_opt.sp_opt.xs_offset = x;
	printf ("Setting silence offset to %d\n",x);
	return;
    }
    if (2==sscanf(rule,"xs_padding=%d:%d",&x,&y)) {
	m_opt.sp_opt.xs_padding_1 = x;
	m_opt.sp_opt.xs_padding_2 = y;
	printf ("Setting file output padding to (%d:%d)\n",x,y);
	return;
    }

    /* All rules failed */
    fprintf (stderr, "Can't parse command option: --%s\n", rule);
    exit (-1);
}

static void
verify_splitpoint_rules (void)
{
    fprintf (stderr, "Warning: splitpoint sanity check not yet implemented.\n");
    /* OK, maybe just a few */
    
    /* xs_silence_length must be non-negative and divisible by two */
    if (m_opt.sp_opt.xs_silence_length < 0) {
	m_opt.sp_opt.xs_silence_length = 0;
    }
    if (m_opt.sp_opt.xs_silence_length % 2) {
        m_opt.sp_opt.xs_silence_length ++;
    }

    /* search_window values must be non-negative */
    if (m_opt.sp_opt.xs_search_window_1 < 0) {
	m_opt.sp_opt.xs_search_window_1 = 0;
    }
    if (m_opt.sp_opt.xs_search_window_2 < 0) {
	m_opt.sp_opt.xs_search_window_2 = 0;
    }

    /* if silence_length is 0, then search window should be zero */
    if (m_opt.sp_opt.xs_silence_length == 0) {
	m_opt.sp_opt.xs_search_window_1 = 0;
	m_opt.sp_opt.xs_search_window_2 = 0;
    }

    /* search_window values must be longer than silence_length */
    if (m_opt.sp_opt.xs_search_window_1 + m_opt.sp_opt.xs_search_window_2
	    < m_opt.sp_opt.xs_silence_length) {
	/* if this happens, disable search */
	m_opt.sp_opt.xs_search_window_1 = 0;
	m_opt.sp_opt.xs_search_window_2 = 0;
	m_opt.sp_opt.xs_silence_length = 0;
    }

    /* search window lengths must be at least 1/2 of silence_length */
    if (m_opt.sp_opt.xs_search_window_1 < m_opt.sp_opt.xs_silence_length) {
	m_opt.sp_opt.xs_search_window_1 = m_opt.sp_opt.xs_silence_length;
    }
    if (m_opt.sp_opt.xs_search_window_2 < m_opt.sp_opt.xs_silence_length) {
	m_opt.sp_opt.xs_search_window_2 = m_opt.sp_opt.xs_silence_length;
    }
}

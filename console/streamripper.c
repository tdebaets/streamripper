/* streamripper.c
 * This little app should be seen as a demo for how to use the stremripper lib. 
 * The only file you need from the /lib dir is rip_mananger.h, and perhapes 
 * util.h (for stuff like formating the number of bytes).
 * 
 * the two functions of interest are main() for how to call rip_mananger_start
 * and rip_callback, which is a how you find out whats going on with the rip.
 * the rest of this file is really just boring command line parsing code.
 * and a signal handler for when the user hits CTRL+C
 */

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
#include "srtypes.h"
#include "rip_manager.h"
#include "util.h"
#include "filelib.h"
#include "debug.h"

/*******************************************************************************
 * Private functions
 ******************************************************************************/
static void print_usage();
static void print_status();
static void catch_sig(int code);
static void parse_arguments(int argc, char **argv);
static void rip_callback(int message, void *data);
static void parse_extended_options (char* rule);
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
    signal(SIGTERM, catch_sig);

    parse_arguments(argc, argv);
    if (!m_dont_print)
	fprintf(stderr, "Connecting...\n");
    if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS) {
	fprintf(stderr, "Couldn't connect to %s\n", m_opt.url);
	exit(1);
    }

    /* 
     * The m_got_sig thing is because you can't call into a thread 
     * (i.e. rip_manager_stop) from a signal handler.. or at least not
     * in FreeBSD 3.4, i don't know about linux or NT.
     */
    while(!m_got_sig && !m_alldone) {
	sleep(1);
	time(&temp_time);
	if (m_stop_time && (temp_time >= m_stop_time)) {
	    if (!m_dont_print) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Time to stop is here, bailing\n");
	    }
	    break; 
	}	
    }

    if (!m_dont_print) {
	fprintf(stderr, "shutting down\n");
    }
    /* GCS: Why? */
#if defined (commentout)
    m_dont_print = TRUE;
#endif
    rip_manager_stop();
    return 0;
}

void catch_sig(int code)
{
    if (!m_dont_print)
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

    if (printed_fullinfo && m_curinfo.filename[0]) {

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
	    if (m_curinfo.track_count < m_opt.dropcount) {
		strcpy(status_str, "skipping...   ");
	    } else {
		strcpy(status_str, "ripping...    ");
	    }
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
	if (!m_dont_print)
	    fprintf(stderr, "bye..\n");
	m_alldone = TRUE;
	break;
    case RM_NEW_TRACK:
	if (!m_dont_print)
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
    fprintf(stderr, "Opts: -h             - Print this listing\n");
    fprintf(stderr, "      -v             - Print version info and quit\n");
    fprintf(stderr, "      -a [file]      - Rip to single file, default name is timestamped\n");
    fprintf(stderr, "      -A             - Don't write individual tracks\n");
    fprintf(stderr, "      -d dir         - The destination directory\n");
    fprintf(stderr, "      -D pattern     - Write files using specified pattern\n");
    fprintf(stderr, "      -s             - Don't create a directory for each stream\n");
    fprintf(stderr, "      -r [[ip:]port] - Create relay server on base ip:port, default port 8000\n");
    fprintf(stderr, "      -R #connect    - Max connections to relay, default 1, -R 0 is no limit\n");
    fprintf(stderr, "      -L file        - Create a relay playlist file\n");
    fprintf(stderr, "      -z             - Don't scan for free ports if base port is not avail\n");
    fprintf(stderr, "      -p url         - Use HTTP proxy server at <url>\n");
    fprintf(stderr, "      -o always      - Always overwrite tracks in complete\n");
    fprintf(stderr, "      -o never       - Never overwrite tracks in complete\n");
    fprintf(stderr, "      -t             - Don't overwrite tracks in incomplete\n");
    fprintf(stderr, "      -c             - Don't auto-reconnect\n");
    fprintf(stderr, "      -l seconds     - Number of seconds to run, otherwise runs forever\n");
    fprintf(stderr, "      -M megabytes   - Stop ripping after this many megabytes\n");
    fprintf(stderr, "      -q [start]     - Add sequence number to output file\n");
    fprintf(stderr, "      -i             - Don't add ID3V1 Tags to output file\n");
    fprintf(stderr, "      -u useragent   - Use a different UserAgent than \"Streamripper\"\n");
    fprintf(stderr, "      -w rulefile    - Parse metadata using rules in file.\n");
    fprintf(stderr, "      -m timeout     - Number of seconds before force-closing stalled conn\n");
    fprintf(stderr, "      -k count       - Skip over first <count> tracks before starting to rip\n");
#if !defined (WIN32)
    fprintf(stderr, "      -I interface   - Rip from specified interface (e.g. eth0)\n");
#endif
    fprintf(stderr, "      -T             - Truncate duplicated tracks in incomplete\n");
    fprintf(stderr, "      --quiet        - Don't print ripping status to console\n");
    fprintf(stderr, "      --debug        - Save debugging trace\n");
    fprintf(stderr, "      --xs_???       - Invoke splitpoint detection rules (see README/man page)\n");
}

/* 
 * Bla, boring agument parsing crap, only reason i didn't use getopt
 * (which i did for an earlyer version) is because i couldn't find a good
 * port of it under Win32.. there probably is one, maybe i didn't look 
 * hard enough. 
 */
void parse_arguments(int argc, char **argv)
{
    int i;
    char *c;

    if (argc < 2) {
	print_usage();
	exit(2);
    }

    // Set default options
    set_rip_manager_options_defaults (&m_opt);

    // Get URL
    strncpy(m_opt.url, argv[1], MAX_URL_LEN);

    // Parse arguments
    for(i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    continue;

	c = strchr("dDfIklLmMopRuw", argv[i][1]);
        if (c != NULL) {
            if ((i == (argc-1)) || (argv[i+1][0] == '-')) {
		fprintf(stderr, "option %s requires an argument\n", argv[i]);
		exit(1);
	    }
	}
	switch (argv[i][1])
	{
	case 'a':
	    /* Create single file output + cue sheet */
	    m_opt.flags |= OPT_SINGLE_FILE_OUTPUT;
	    m_opt.showfile_pattern[0] = 0;
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    strncpy (m_opt.showfile_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'c':
	    m_opt.flags ^= OPT_AUTO_RECONNECT;
	    break;
	case 'd':
	    i++;
	    strncpy(m_opt.output_directory, argv[i], SR_MAX_PATH);
	    break;
	case 'D':
	    i++;
	    strncpy(m_opt.output_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'f':
	    i++;
	    printf ("Error: -f dropstring option is obsolete. "
		    "Please use -w parse_rules instead.\n");
	    exit (1);
	case 'h':
	    print_usage();
            exit(0);
	    break;
	case 'i':
	    m_opt.flags ^= OPT_ADD_ID3;
	    break;
	case 'k':
	    i++;
	    m_opt.dropcount = atoi(argv[i]);
	    break;
	case 'l':
	    i++;
	    time(&m_stop_time);
	    m_stop_time += atoi(argv[i]);
	    break;
	case 'm':
	    i++;
	    m_opt.timeout = atoi(argv[i]);
	    break;
	case 'o':
	    i++;
	    m_opt.overwrite = string_to_overwrite_opt (argv[i]);
	    if (m_opt.overwrite == OVERWRITE_UNKNOWN) {
		printf ("Error: -o option requires an argument\n"
			"Please use \"-o always\" or \"-o never\"\n");
		exit (1);
	    }
	    break;
	case 'p':
	    i++;
	    strncpy(m_opt.proxyurl, argv[i], MAX_URL_LEN);
	    break;
	case 'P':
	    i++;
	    printf ("Error: -P prefix option is obsolete. "
		    "Please use -D pattern instead.\n");
	    exit (1);
	case 'q':
	    m_opt.flags ^= OPT_COUNT_FILES;
	    m_opt.count_start = -1;     /* -1 means auto-detect */
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    m_opt.count_start = atoi(argv[i]);
	    break;
	case 'r':
	    m_opt.flags ^= OPT_MAKE_RELAY;
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;

	    c = strstr(argv[i], ":");

	    if (NULL == c) {
	    	m_opt.relay_port = atoi(argv[i]);
	    } else {
	    	*c = '\0';
		strncpy(m_opt.relay_ip, argv[i], SR_MAX_PATH);
		m_opt.relay_port = atoi(++c);
 	    }

	    break;
	case 's':
	    m_opt.flags ^= OPT_SEPERATE_DIRS;
	    break;
	case 't':
	    m_opt.flags |= OPT_KEEP_INCOMPLETE;
	    break;
	case 'T':
	    m_opt.flags |= OPT_TRUNCATE_DUPS;
	    break;
	case 'u':
	    i++;
	    strncpy(m_opt.useragent, argv[i], MAX_USERAGENT_STR);
	    break;
	case 'v':
	    printf("Streamripper %s\n", SRVERSION);
	    exit(0);
	case 'w':
	    i++;
	    strncpy(m_opt.rules_file, argv[i], SR_MAX_PATH);
	    break;
	case 'z':
	    m_opt.flags ^= OPT_SEARCH_PORTS;
	    m_opt.max_port = m_opt.relay_port+1000;
	    break;
	case 'A':
	    m_opt.flags ^= OPT_INDIVIDUAL_TRACKS;
	    break;
	case 'I':
	    i++;
	    strncpy(m_opt.if_name, argv[i], SR_MAX_PATH);
	    break;
	case 'L':
	    i++;
	    strncpy(m_opt.pls_file, argv[i], SR_MAX_PATH);
	    break;
 	case 'M':
 	    i++;
 	    m_opt.maxMB_rip_size = atoi(argv[i]);
 	    m_opt.flags |= OPT_CHECK_MAX_BYTES;
 	    break;
	case 'R':
	    i++;
	    m_opt.max_connections = atoi(argv[i]);
	    break;
	case '-':
	    parse_extended_options(&argv[i][2]);
	    break;
	}
    }

    /* Need to verify that splitpoint rules were sane */
    verify_splitpoint_rules ();

    /* Verify that first parameter is URL */
    if (argv[1][0] == '-') {
	fprintf(stderr, "*** The first parameter MUST be the URL\n\n");
	exit(2);
    }
}

static void
parse_extended_options (char* rule)
{
    int x,y;

    /* Logging options */
    if (!strcmp(rule,"debug")) {
	debug_enable();
	return;
    }
    if (!strcmp(rule,"quiet")) {
	m_dont_print = TRUE;
	return;
    }

    /* Splitpoint options */
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

    /* codeset options */
    x = strlen("codeset=");
    if (!strncmp(rule,"codeset=",x)) {
	m_opt.cs_opt.codeset = &rule[x];
	printf ("Setting codeset to %s\n",m_opt.cs_opt.codeset);
	return;
    }

    /* All rules failed */
    fprintf (stderr, "Can't parse command option: --%s\n", rule);
    exit (-1);
}

static void
verify_splitpoint_rules (void)
{
#if defined (commentout)
    /* This is still not complete, but the warning causes people to 
       wonder what is going on. */
    fprintf (stderr, "Warning: splitpoint sanity check not yet complete.\n");
#endif
    
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

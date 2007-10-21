/* streamripper.c
 * This little app should be seen as a demo for how to use the stremripper lib. 
 * The only file you need from the /lib dir is rip_mananger.h, and perhapes 
 * util.h (for stuff like formating the number of bytes).
 * 
 * the two functions of interest are main() for how to call rip_mananger_start
 * and rip_callback, which is a how you find out whats going on with the rip.
 * the rest of this file is really just boring command line parsing code.
 * and a signal handler for when the user hits CTRL+C
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
#include "prefs.h"
#include "mchar.h"
#include "filelib.h"
#include "debug.h"

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void print_usage();
static void print_status (RIP_MANAGER_INFO *rmi);
static void catch_sig (int code);
static void parse_arguments (PREFS *prefs, int argc, char **argv);
static void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data);
static void parse_extended_options (PREFS *prefs, char *rule);
static void verify_splitpoint_rules (PREFS *prefs);

/*****************************************************************************
 * Private variables
 *****************************************************************************/
static char m_buffer_chars[] = {'\\', '|', '/', '-', '*'}; /* for formating */
//static RIP_MANAGER_INFO 	m_curinfo; /* from the rip_manager callback */
static BOOL			m_started = FALSE;
static BOOL			m_alldone = FALSE;
static BOOL			m_got_sig = FALSE;
static BOOL			m_dont_print = FALSE;
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

int
main (int argc, char *argv[])
{
    int ret;
    time_t temp_time;
    PREFS prefs;
    RIP_MANAGER_INFO *rmi = 0;

    signal (SIGINT, catch_sig);
    signal (SIGTERM, catch_sig);

    parse_arguments (&prefs, argc, argv);

    if (!m_dont_print)
	fprintf (stderr, "Connecting...\n");

    
    rip_manager_init ();
    if ((ret = rip_manager_start (&rmi, &prefs, rip_callback)) != SR_SUCCESS) {
	fprintf(stderr, "Couldn't connect to %s\n", prefs.url);
	exit(1);
    }

    /* 
     * The m_got_sig thing is because you can't call into a thread 
     * (i.e. rip_manager_stop) from a signal handler.. or at least not
     * in FreeBSD 3.4, i don't know about linux or NT.
     */
    while (!m_got_sig && !m_alldone) {
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

    rip_manager_stop (rmi);
    return 0;
}

void
catch_sig(int code)
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
void
print_status (RIP_MANAGER_INFO *rmi)
{
    PREFS *prefs = rmi->prefs;
    char status_str[128];
    char filesize_str[64];
    static int buffering_tick = 0;
    BOOL static printed_fullinfo = FALSE;

    if (m_dont_print)
	return;

    if (printed_fullinfo && rmi->filename[0]) {

	switch(rmi->status)
	{
	case RM_STATUS_BUFFERING:
	    buffering_tick++;
	    if (buffering_tick == 5)
		buffering_tick = 0;

	    sprintf(status_str,"buffering - %c ",
		    m_buffer_chars[buffering_tick]);

	    fprintf(stderr, "[%14s] %.50s\r",
		    status_str,
		    rmi->filename);
	    break;

	case RM_STATUS_RIPPING:
	    if (rmi->track_count < prefs->dropcount) {
		strcpy(status_str, "skipping...   ");
	    } else {
		strcpy(status_str, "ripping...    ");
	    }
	    format_byte_size(filesize_str, rmi->filesize);
	    fprintf(stderr, "[%14s] %.50s [%7s]\r",
		    status_str,
		    rmi->filename,
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
		rmi->streamname,
		rmi->server_name,
		rmi->bitrate,
		rmi->meta_interval);
	if(GET_MAKE_RELAY(prefs->flags))
	{
	    fprintf(stderr, "relay port: %d\n"
		    "[%14s]\r",
		    prefs->relay_port,
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
void
rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data)
{
    ERROR_INFO *err;
    switch(message)
    {
    case RM_UPDATE:
	print_status (rmi);
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

static void
print_usage()
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
    fprintf(stderr, "      -o (always|never|larger)    - When to tracks in complete\n");
    fprintf(stderr, "      -t             - Don't overwrite tracks in incomplete\n");
    fprintf(stderr, "      -c             - Don't auto-reconnect\n");
    fprintf(stderr, "      -l seconds     - Number of seconds to run, otherwise runs forever\n");
    fprintf(stderr, "      -M megabytes   - Stop ripping after this many megabytes\n");
    fprintf(stderr, "      -q [start]     - Add sequence number to output file\n");
    fprintf(stderr, "      -u useragent   - Use a different UserAgent than \"Streamripper\"\n");
    fprintf(stderr, "      -w rulefile    - Parse metadata using rules in file.\n");
    fprintf(stderr, "      -m timeout     - Number of seconds before force-closing stalled conn\n");
    fprintf(stderr, "      -k count       - Skip over first <count> tracks before starting to rip\n");
#if !defined (WIN32)
    fprintf(stderr, "      -I interface   - Rip from specified interface (e.g. eth0)\n");
#endif
    fprintf(stderr, "      -T             - Truncate duplicated tracks in incomplete\n");
    fprintf(stderr, "      -E command     - Run external command to fetch metadata\n");
    fprintf(stderr, "      --quiet        - Don't print ripping status to console\n");
    fprintf(stderr, "      --debug        - Save debugging trace\n");
    fprintf(stderr, "ID3 opts (mp3/aac/nsv):  [The default behavior is adding ID3V2.3 only]\n");
    fprintf(stderr, "      -i                           - Don't add any ID3 tags to output file\n");
    fprintf(stderr, "      --with-id3v1                 - Add ID3V1 tags to output file\n");
    fprintf(stderr, "      --without-id3v2              - Don't add ID3V2 tags to output file\n");
    fprintf(stderr, "Splitpoint opts (mp3 only):\n");
    fprintf(stderr, "      --xs-offset=num              - Shift relative to metadata (msec)\n");
    fprintf(stderr, "      --xs-padding=num:num         - Add extra to prev:next track (msec)\n");
    fprintf(stderr, "      --xs-search-window=num:num   - Search window relative to metadata (msec)\n");
    fprintf(stderr, "      --xs-silence-length=num      - Expected length of silence (msec)\n");
    fprintf(stderr, "Codeset opts:\n");
    fprintf(stderr, "      --codeset-filesys=codeset    - Specify codeset for the file system\n");
    fprintf(stderr, "      --codeset-id3=codeset        - Specify codeset for id3 tags\n");
    fprintf(stderr, "      --codeset-metadata=codeset   - Specify codeset for metadata\n");
    fprintf(stderr, "      --codeset-relay=codeset      - Specify codeset for the relay stream\n");
}

/* 
 * Bla, boring agument parsing crap, only reason i didn't use getopt
 * (which i did for an earlyer version) is because i couldn't find a good
 * port of it under Win32.. there probably is one, maybe i didn't look 
 * hard enough. 
 */
static void
parse_arguments (PREFS* prefs, int argc, char **argv)
{
    int i;
    char *c;

    if (argc < 2) {
	print_usage();
	exit(2);
    }

    // Set default options
    // set_rip_manager_options_defaults (&m_opt);

    // Get URL
    strncpy (prefs->url, argv[1], MAX_URL_LEN);

    // Load prefs (including URL-specific)
    prefs_load ();
    prefs_get_stream_prefs (prefs, prefs->url);

    // Parse arguments
    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    continue;

	c = strchr ("dDEfIklLmMopRuw", argv[i][1]);
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
	    prefs->flags |= OPT_SINGLE_FILE_OUTPUT;
	    prefs->showfile_pattern[0] = 0;
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    strncpy (prefs->showfile_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'A':
	    prefs->flags ^= OPT_INDIVIDUAL_TRACKS;
	    break;
	case 'c':
	    prefs->flags ^= OPT_AUTO_RECONNECT;
	    break;
	case 'd':
	    i++;
	    strncpy(prefs->output_directory, argv[i], SR_MAX_PATH);
	    break;
	case 'D':
	    i++;
	    strncpy(prefs->output_pattern, argv[i], SR_MAX_PATH);
	    break;
	case 'E':
	    prefs->flags |= OPT_EXTERNAL_CMD;
	    i++;
	    strncpy(prefs->ext_cmd, argv[i], SR_MAX_PATH);
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
	    //	    prefs->flags ^= OPT_ADD_ID3;
	    OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V1,0);
	    OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V2,0);
	    break;
	case 'I':
	    i++;
	    strncpy(prefs->if_name, argv[i], SR_MAX_PATH);
	    break;
	case 'k':
	    i++;
	    prefs->dropcount = atoi(argv[i]);
	    break;
	case 'l':
	    i++;
	    time(&m_stop_time);
	    m_stop_time += atoi(argv[i]);
	    break;
	case 'L':
	    i++;
	    strncpy(prefs->pls_file, argv[i], SR_MAX_PATH);
	    break;
	case 'm':
	    i++;
	    prefs->timeout = atoi(argv[i]);
	    break;
 	case 'M':
 	    i++;
 	    prefs->maxMB_rip_size = atoi(argv[i]);
 	    prefs->flags |= OPT_CHECK_MAX_BYTES;
 	    break;
	case 'o':
	    i++;
	    prefs->overwrite = string_to_overwrite_opt (argv[i]);
	    if (prefs->overwrite == OVERWRITE_UNKNOWN) {
		printf ("Error: -o option requires an argument\n"
			"Please use \"-o always\" or \"-o never\"\n");
		exit (1);
	    }
	    break;
	case 'p':
	    i++;
	    strncpy(prefs->proxyurl, argv[i], MAX_URL_LEN);
	    break;
	case 'P':
	    i++;
	    printf ("Error: -P prefix option is obsolete. "
		    "Please use -D pattern instead.\n");
	    exit (1);
	case 'q':
	    prefs->flags ^= OPT_COUNT_FILES;
	    prefs->count_start = -1;     /* -1 means auto-detect */
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    prefs->count_start = atoi(argv[i]);
	    break;
	case 'r':
	    prefs->flags ^= OPT_MAKE_RELAY;
	    if (i == (argc-1) || argv[i+1][0] == '-')
		break;
	    i++;
	    c = strstr(argv[i], ":");
	    if (NULL == c) {
	    	prefs->relay_port = atoi(argv[i]);
	    } else {
	    	*c = '\0';
		strncpy(prefs->relay_ip, argv[i], SR_MAX_PATH);
		prefs->relay_port = atoi(++c);
 	    }
	    break;
	case 'R':
	    i++;
	    prefs->max_connections = atoi(argv[i]);
	    break;
	case 's':
	    prefs->flags ^= OPT_SEPERATE_DIRS;
	    break;
	case 't':
	    prefs->flags |= OPT_KEEP_INCOMPLETE;
	    break;
	case 'T':
	    prefs->flags |= OPT_TRUNCATE_DUPS;
	    break;
	case 'u':
	    i++;
	    strncpy(prefs->useragent, argv[i], MAX_USERAGENT_STR);
	    break;
	case 'v':
	    printf("Streamripper %s\n", SRVERSION);
	    exit(0);
	case 'w':
	    i++;
	    strncpy(prefs->rules_file, argv[i], SR_MAX_PATH);
	    break;
	case 'z':
	    prefs->flags ^= OPT_SEARCH_PORTS;
	    prefs->max_port = prefs->relay_port+1000;
	    break;
	case '-':
	    parse_extended_options (prefs, &argv[i][2]);
	    break;
	}
    }

    /* Need to verify that splitpoint rules were sane */
    verify_splitpoint_rules (prefs);

    /* Verify that first parameter is URL */
    if (argv[1][0] == '-') {
	fprintf(stderr, "*** The first parameter MUST be the URL\n\n");
	exit(2);
    }

    /* Save prefs (as URL-specific) */
    prefs_save (prefs);
}

static void
parse_extended_options (PREFS* prefs, char* rule)
{
    int x,y;

    /* Version */
    if (!strcmp(rule,"version")) {
	printf("Streamripper %s\n", SRVERSION);
	exit(0);
    }

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
    if ((!strcmp(rule,"xs-none"))
	|| (!strcmp(rule,"xs_none"))) {
	prefs->sp_opt.xs = 0;
	debug_printf ("Disable silence detection");
	return;
    }
    if ((1==sscanf(rule,"xs-min-volume=%d",&x)) 
	|| (1==sscanf(rule,"xs_min_volume=%d",&x))) {
	prefs->sp_opt.xs_min_volume = x;
	debug_printf ("Setting minimum volume to %d\n",x);
	return;
    }
    if ((1==sscanf(rule,"xs-silence-length=%d",&x))
	|| (1==sscanf(rule,"xs_silence_length=%d",&x))) {
	prefs->sp_opt.xs_silence_length = x;
	debug_printf ("Setting silence length to %d\n",x);
	return;
    }
    if ((2==sscanf(rule,"xs-search-window=%d:%d",&x,&y))
	|| (2==sscanf(rule,"xs_search_window=%d:%d",&x,&y))) {
	prefs->sp_opt.xs_search_window_1 = x;
	prefs->sp_opt.xs_search_window_2 = y;
	debug_printf ("Setting search window to (%d:%d)\n",x,y);
	return;
    }
    if ((1==sscanf(rule,"xs-offset=%d",&x))
	|| (1==sscanf(rule,"xs_offset=%d",&x))) {
	prefs->sp_opt.xs_offset = x;
	debug_printf ("Setting silence offset to %d\n",x);
	return;
    }
    if ((2==sscanf(rule,"xs-padding=%d:%d",&x,&y))
	|| (2==sscanf(rule,"xs_padding=%d:%d",&x,&y))) {
	prefs->sp_opt.xs_padding_1 = x;
	prefs->sp_opt.xs_padding_2 = y;
	debug_printf ("Setting file output padding to (%d:%d)\n",x,y);
	return;
    }

    /* id3 options */
    if (!strcmp(rule,"with-id3v2")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V2,1);
	return;
    }
    if (!strcmp(rule,"without-id3v2")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V2,0);
	return;
    }
    if (!strcmp(rule,"with-id3v1")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V1,1);
	return;
    }
    if (!strcmp(rule,"without-id3v1")) {
	OPT_FLAG_SET(prefs->flags,OPT_ADD_ID3V1,0);
	return;
    }

    /* codeset options */
    x = strlen("codeset-filesys=");
    if (!strncmp(rule,"codeset-filesys=",x)) {
	strncpy (prefs->cs_opt.codeset_filesys, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting filesys codeset to %s\n",
		      prefs->cs_opt.codeset_filesys);
	return;
    }
    x = strlen("codeset-id3=");
    if (!strncmp(rule,"codeset-id3=",x)) {
	strncpy (prefs->cs_opt.codeset_id3, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting id3 codeset to %s\n",
		      prefs->cs_opt.codeset_id3);
	return;
    }
    x = strlen("codeset-metadata=");
    if (!strncmp(rule,"codeset-metadata=",x)) {
	strncpy (prefs->cs_opt.codeset_metadata, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting metadata codeset to %s\n",
		      prefs->cs_opt.codeset_metadata);
	return;
    }
    x = strlen("codeset-relay=");
    if (!strncmp(rule,"codeset-relay=",x)) {
	strncpy (prefs->cs_opt.codeset_relay, &rule[x], MAX_CODESET_STRING);
	debug_printf ("Setting relay codeset to %s\n",
		      prefs->cs_opt.codeset_relay);
	return;
    }

    /* All rules failed */
    fprintf (stderr, "Can't parse command option: --%s\n", rule);
    exit (-1);
}

static void
verify_splitpoint_rules (PREFS *prefs)
{
#if defined (commentout)
    /* This is still not complete, but the warning causes people to 
       wonder what is going on. */
    fprintf (stderr, "Warning: splitpoint sanity check not yet complete.\n");
#endif
    
    /* xs_silence_length must be non-negative and divisible by two */
    if (prefs->sp_opt.xs_silence_length < 0) {
	prefs->sp_opt.xs_silence_length = 0;
    }
    if (prefs->sp_opt.xs_silence_length % 2) {
        prefs->sp_opt.xs_silence_length ++;
    }

    /* search_window values must be non-negative */
    if (prefs->sp_opt.xs_search_window_1 < 0) {
	prefs->sp_opt.xs_search_window_1 = 0;
    }
    if (prefs->sp_opt.xs_search_window_2 < 0) {
	prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* if silence_length is 0, then search window should be zero */
    if (prefs->sp_opt.xs_silence_length == 0) {
	prefs->sp_opt.xs_search_window_1 = 0;
	prefs->sp_opt.xs_search_window_2 = 0;
    }

    /* search_window values must be longer than silence_length */
    if (prefs->sp_opt.xs_search_window_1 + prefs->sp_opt.xs_search_window_2
	    < prefs->sp_opt.xs_silence_length) {
	/* if this happens, disable search */
	prefs->sp_opt.xs_search_window_1 = 0;
	prefs->sp_opt.xs_search_window_2 = 0;
	prefs->sp_opt.xs_silence_length = 0;
    }

    /* search window lengths must be at least 1/2 of silence_length */
    if (prefs->sp_opt.xs_search_window_1 < prefs->sp_opt.xs_silence_length) {
	prefs->sp_opt.xs_search_window_1 = prefs->sp_opt.xs_silence_length;
    }
    if (prefs->sp_opt.xs_search_window_2 < prefs->sp_opt.xs_silence_length) {
	prefs->sp_opt.xs_search_window_2 = prefs->sp_opt.xs_silence_length;
    }
}

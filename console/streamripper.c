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

/*******************************************************************************
 * Private functions
 ******************************************************************************/
static void print_usage();
static void print_status();
static void catch_sig(int code);
static void parse_arguments(int argc, char **argv);
static void rip_callback(int message, void *data);
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
	fprintf(stderr, "shuting down\n");
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
		if(!OPT_FLAG_ISSET(m_opt.flags, OPT_NO_RELAY))
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
        fprintf(stderr, "        -o             - Write over tracks from incomplete\n");
        fprintf(stderr, "        -c             - Don't auto-reconnect\n");
        fprintf(stderr, "        -v             - Print version info and quite\n");
        fprintf(stderr, "        -l <seconds>   - number of seconds to run, otherwise runs forever\n");
        fprintf(stderr, "        -q             - add sequence number to output file\n");
        fprintf(stderr, "        -i             - dont add ID3V1 Tags to output file\n");

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
	m_opt.relay_port = 8000;
	m_opt.max_port = 18000;
	m_opt.flags = OPT_AUTO_RECONNECT | 
		      OPT_SEPERATE_DIRS | 
		      OPT_SEARCH_PORTS |
		      OPT_NO_RELAY;

	m_opt.add_id3tag = TRUE;
	
	strcpy(m_opt.output_directory, "./");
	m_opt.proxyurl[0] = (char)NULL;
	strncpy(m_opt.url, argv[1], MAX_URL_LEN);

        //get arguments
        for(i = 1; i < argc; i++)
        {
                if (argv[i][0] != '-')
                        continue;

                c = strchr("dpl", argv[i][1]);
                if (c != NULL)
                        if ((i == (argc-1)) || (argv[i+1][0] == '-'))
                        {
                                fprintf(stderr, "option %s requires an argument\n", argv[i]);
                                exit(1);
                        }
                switch (argv[i][1])
                {
                        case 'd':
                                i++;
                                //dynamically allocat enough space for the dest dir
				strncpy(m_opt.output_directory, argv[i], MAX_DIR_LEN);
                                break;
                        case 'i':
                                m_opt.add_id3tag = FALSE;
                                break;
                        case 'q':
                                m_opt.add_seq_number = TRUE;
                                m_opt.sequence_number = 0;
                                break;
                        case 's':
                                m_opt.flags ^= OPT_SEPERATE_DIRS;
                                break;
                        case 'r':
                                m_opt.flags ^= OPT_NO_RELAY;
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
                        case 'p':
				i++;
				strncpy(m_opt.proxyurl, argv[i], MAX_URL_LEN);
                                break;
                        case 'o':
                                m_opt.flags |= OPT_OVER_WRITE_TRACKS;
                                break;
                        case 'c':
                                m_opt.flags ^= OPT_AUTO_RECONNECT;
                                break;
                        case 'v':
				printf("streamripper 1.0.4 by Jon Clegg <jonclegg@yahoo.com>\n");
				exit(0);
                        case 'l':
				i++;
				time(&m_stop_time);
				m_stop_time += atoi(argv[i]);
				break;
                }
        }
}

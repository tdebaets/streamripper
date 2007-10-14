/* prefs.h
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
#ifndef __PREFS_H__
#define __PREFS_H__

#include <glib.h>

typedef struct stream_prefs PREFS;
typedef struct stream_prefs STREAM_PREFS;
struct stream_prefs
{
    char label[MAX_URL_LEN];		// logical name for this stream
    char url[MAX_URL_LEN];		// url of the stream to connect to
    char proxyurl[MAX_URL_LEN];		// url of a http proxy server, 
                                        //  '\0' otherwise
    char output_directory[SR_MAX_PATH];	// base directory to output files too
    char output_pattern[SR_MAX_PATH];	// filename pattern when ripping 
                                        //  with splitting
    char showfile_pattern[SR_MAX_PATH];	// filename base when ripping to
                                        //  single file without splitting
    char if_name[SR_MAX_PATH];		// local interface to use
    char rules_file[SR_MAX_PATH];       // file that holds rules for 
                                        //  parsing metadata
    char pls_file[SR_MAX_PATH];		// optional, where to create a 
                                        //  rely .pls file
    char relay_ip[SR_MAX_PATH];		// optional, ip to bind relaying 
                                        //  socket to
    char ext_cmd[SR_MAX_PATH];          // cmd to spawn for external metadata
    char useragent[MAX_USERAGENT_STR];	// optional, use a different useragent
    u_short relay_port;			// port to use for the relay server
					//  GCS 3/30/07 change to u_short
    u_short max_port;			// highest port the relay server 
                                        //  can look if it needs to search
    u_long max_connections;             // max number of connections 
                                        //  to relay stream
					//  GCS 8/18/07 change int to u_long
    u_long maxMB_rip_size;		// max number of megabytes that 
                                        //  can by writen out before we stop
    u_long flags;			// all booleans logically OR'd 
                                        //  together (see above)
    u_long timeout;			// timeout, in seconds, before a 
                                        //  stalled connection is forcefully 
                                        //  closed
					//  GCS 8/18/07 change int to u_long
    u_long dropcount;			// number of tracks at beginning 
                                        //  of connection to always ignore
					//  GCS 8/18/07 change int to u_long
    int count_start;                    // which number to start counting?
    enum OverwriteOpt overwrite;	// overwrite file in complete?
    SPLITPOINT_OPTIONS sp_opt;		// options for splitpoint rules
    CODESET_OPTIONS cs_opt;             // which codeset should i use?
};

typedef struct global_prefs GLOBAL_PREFS;
struct global_prefs
{
    PREFS prefs;                        // default prefs for new streams
    char default_url[MAX_URL_LEN];      // url of the stream to connect to
};


#if defined (commentout)
// Rip manager flags options
#define OPT_AUTO_RECONNECT	0x00000001	// reconnect automatticly if dropped
#define OPT_SEPERATE_DIRS	0x00000002	// create a directory named after the server
#define OPT_SEARCH_PORTS	0x00000008	// relay server should search for a open port
#define OPT_MAKE_RELAY		0x00000010	// don't make a relay server
#define OPT_COUNT_FILES		0x00000020	// add a index counter to the filenames
#define OPT_OBSOLETE		0x00000040	// Used to be OPT_ADD_ID3, now ignored
#define OPT_DATE_STAMP		0x00000100	// add a date stamp to the output directory
#define OPT_CHECK_MAX_BYTES	0x00000200	// use the maxMB_rip_size value to know how much to rip
#define OPT_KEEP_INCOMPLETE	0x00000400	// overwrite files in the incomplete directory, add counter instead
#define OPT_SINGLE_FILE_OUTPUT	0x00000800	// enable ripping to single file
#define OPT_TRUNCATE_DUPS	0x00001000	// truncate file in the incomplete directory already present in complete
#define OPT_INDIVIDUAL_TRACKS	0x00002000	// should we write the individual tracks?
#define OPT_EXTERNAL_CMD	0x00004000	// use external command to get metadata?
#define OPT_ADD_ID3V1		0x00008000	// Add ID3V1
#define OPT_ADD_ID3V2		0x00010000	// Add ID3V2

#define OPT_FLAG_ISSET(flags, opt)	\
    (flags & opt)
#define OPT_FLAG_SET(flags, opt, val)	\
    (val ? (flags |= opt) : (flags &= (~opt)))
#endif

/* Prototypes */
void prefs_load (void);
void prefs_save (PREFS* prefs);
void prefs_get_stream_prefs (STREAM_PREFS* prefs, char* label);

#endif

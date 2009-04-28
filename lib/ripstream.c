/* ripstream.c
 * buffer stream data, when a track changes decodes the audio and 
 * finds a silent point to split the track
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
#include <string.h>
#ifndef _WIN32
#include <sys/types.h>
#include <netinet/in.h>
#endif
#include "srtypes.h"
#include "cbuf3.h"
#include "findsep.h"
#include "mchar.h"
#include "parse.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "ripstream_mp3.h"
#include "ripstream_ogg.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"
#include "socklib.h"
#include "external.h"
#include "ripogg.h"
#include "track_info.h"

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void
compute_cbuf2_size (RIP_MANAGER_INFO* rmi, 
		    SPLITPOINT_OPTIONS *sp_opt, 
		    int bitrate, 
		    int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes, int bitrate);
static int
ripstream_recvall (RIP_MANAGER_INFO* rmi, char* buffer, int size);
static error_code get_track_from_metadata (RIP_MANAGER_INFO* rmi, 
					   int size, char *newtrack);

/*****************************************************************************
 * Private structs
 *****************************************************************************/
typedef struct ID3V1st
{
        char    tag[3];
        char    songtitle[30];
        char    artist[30];
        char    album[30];
        char    year[4];
        char    comment[30];
        char    genre;
} ID3V1Tag;

typedef struct ID3V2framest {
	char	id[4];
	int	size;
	char	pad[3];
} ID3V2frame;


/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
ripstream_rip (RIP_MANAGER_INFO* rmi)
{
    /* Main loop for ripping */
    if (rmi->http_info.content_type == CONTENT_TYPE_OGG) {
	return ripstream_ogg_rip (rmi);
    } else {
	return ripstream_mp3_rip (rmi);
    }
}

error_code
ripstream_init (RIP_MANAGER_INFO* rmi)
{
    debug_printf ("RIPSTREAM_INIT\n");

    rmi->track_count = 0;
    /* GCS RMK: Ripchunk_size is the metaint size, or default size
       if stream doesn't have meta data */
    rmi->cue_sheet_bytes = 0;
    rmi->getbuffer_size = (rmi->meta_interval == NO_META_INTERVAL) 
	    ? DEFAULT_META_INTERVAL : rmi->meta_interval;

    track_info_clear (&rmi->old_track);
    track_info_clear (&rmi->new_track);
    track_info_clear (&rmi->current_track);
    rmi->ripstream_first_time_through = 1;

    if ((rmi->getbuffer = malloc (rmi->getbuffer_size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    rmi->find_silence = -1;
    rmi->no_meta_name[0] = '\0';
    rmi->track_count = 0;

    memset (&rmi->cbuf3, 0, sizeof (Cbuf3));

    return SR_SUCCESS;
}

#if defined (commentout)
void
ripstream_clear(RIP_MANAGER_INFO* rmi)
{
    debug_printf ("RIPSTREAM_CLEAR\n");

    if (rmi->getbuffer) {free(rmi->getbuffer); rmi->getbuffer = NULL;}
    rmi->getbuffer_size = 0;

    rmi->find_silence = -1;
    rmi->cbuf2_size = 0;
    cbuf3_destroy (&rmi->cbuf3);

    track_info_clear (&rmi->old_track);
    track_info_clear (&rmi->new_track);
    track_info_clear (&rmi->current_track);

    rmi->ripstream_first_time_through = 1;
    rmi->no_meta_name[0] = '\0';
    rmi->track_count = 0;
}
#endif

void
ripstream_destroy (RIP_MANAGER_INFO* rmi)
{
    debug_printf ("RIPSTREAM_DESTROY\n");

    if (rmi->getbuffer) {free(rmi->getbuffer); rmi->getbuffer = NULL;}
    rmi->getbuffer_size = 0;

    rmi->find_silence = -1;
    rmi->cbuf2_size = 0;

    cbuf3_destroy (&rmi->cbuf3);

    track_info_clear (&rmi->old_track);
    track_info_clear (&rmi->new_track);
    track_info_clear (&rmi->current_track);

    rmi->ripstream_first_time_through = 1;
    rmi->no_meta_name[0] = '\0';
    rmi->track_count = 0;
}

/* Data followed by meta-data */
error_code
ripstream_get_data (RIP_MANAGER_INFO* rmi, char *data_buf, char *track_buf)
{
    int ret = 0;
    char c;
    char newtrack[MAX_TRACK_LEN];

    *track_buf = 0;
    rmi->current_track.have_track_info = 0;
    ret = ripstream_recvall (rmi, data_buf, rmi->getbuffer_size);
    if (ret <= 0)
	return ret;

    if (rmi->meta_interval == NO_META_INTERVAL) {
	return SR_SUCCESS;
    }

    if ((ret = ripstream_recvall (rmi, &c, 1)) <= 0)
	return ret;

    debug_printf ("METADATA LEN: %d\n",(int)c);
    if (c < 0) {
	debug_printf ("Got invalid metadata: %d\n",c);
	return SR_ERROR_INVALID_METADATA;
    } else if (c == 0) {
	/* We didn't get any metadata this time. */
	return SR_SUCCESS;
    } else {
	/* We got metadata this time. */
	ret = get_track_from_metadata (rmi, c * 16, newtrack);
	if (ret != SR_SUCCESS) {
	    debug_printf("get_trackname had a bad return %d\n", ret);
	    return ret;
	}

	strncpy(track_buf, newtrack, MAX_TRACK_LEN);
	rmi->current_track.have_track_info = 1;
    }
    return SR_SUCCESS;
}

error_code
ripstream_start_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    error_code ret;

    if (ti->save_track && GET_INDIVIDUAL_TRACKS(rmi->prefs->flags)) {
	rmi->write_data = 1;
    } else {
	rmi->write_data = 0;
    }

    /* Update data for callback */
    debug_printf ("calling rip_manager_start_track(#2)\n");
    ret = rip_manager_start_track (rmi, ti);

    /* Do content-specific stuff (no ogg-specific stuff at start track?) */
    if (rmi->http_info.content_type != CONTENT_TYPE_OGG) {
	ripstream_mp3_start_track (rmi, ti);
    }
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
#if defined (commentout)
/* GCS: This converts either positive or negative ms to blocks,
   and must work for rounding up and rounding down */
static int
ms_to_blocks (int ms, int bitrate, int round_up)
{
    int ms_abs = ms > 0 ? ms : -ms;
    int ms_sign = ms > 0 ? 1 : 0;
    int bits = ms_abs * bitrate;
    int bits_per_block = 8 * rmi->getbuffer_size;
    int blocks = bits / bits_per_block;
    if (bits % bits_per_block > 0) {
	if (!(round_up ^ ms_sign)) {
	    blocks++;
	}
    }
    if (!ms_sign) {
	blocks = -blocks;
    }
    return blocks;
}
#endif

/* Simpler routine, rounded toward zero */
static int
ms_to_bytes (int ms, int bitrate)
{
    int bits = ms * bitrate;
    if (bits > 0)
	return bits / 8;
    else
	return -((-bits)/8);
}

/* Assume positive, round toward zero */
static int
bytes_to_secs (unsigned int bytes, int bitrate)
{
    /* divided by 125 because 125 = 1000 / 8 */
    int secs = (bytes / bitrate) / 125;
    return secs;
}

/* --------------------------------------------------------------------------
   Buffering for silence splitting & padding

   We may flush the circular buffer as soon as anything that 
   needs to go into the next song has passed.   For simplicity, 
   we also buffer up to the latest point that can go into the 
   next song.  This is called the "required window."

   The "required window" is part that is decoded, even though 
   we don't need volume data for all of it.  We simply mark the 
   frame boundaries so we don't chop any frames.

   The circular buffer is a bit bigger than the required window. 
   This includes all of the stuff which cannot be flushed out of 
   the cbuf, because cbuf is flushed in blocks.

   Some abbreviations:
     mic      meta inf change
     cb	      cbuf3, aka circular buffer
     rw	      required window
     sw	      search window

   This is the complete picture:

    A---------A---------A----+----B---------B     meta intervals

                                  /mic            meta-inf change (A to B)
                             +--->|
                             /mi                  meta-inf point
                             |
                   |---+-----+------+---|         search window
                       |            |   
                       |            |   
                   |---+---|    |---+---|         silence length
                       |            |
         |-------------|            |-----|
              prepad                postpad

         |--------------------------------|       required window

                        |---------|               minimum rw (1 meta int, 
                                                  includes sw, need not 
                                                  be aligned to metadata)

    |---------------------------------------|     cbuf

                   |<-------------+               mic_to_sw_start (usu neg)
                                  +---->|         mic_to_sw_end   (usu pos)
         |<-----------------------+               mic_to_rw_start
                                  +------>|       mic_to_rw_end
    |<----------------------------+               mic_to_cb_start
                                  +-------->|     mic_to_cb_end

   ------------------------------------------------------------------------*/
static void
compute_cbuf2_size (RIP_MANAGER_INFO* rmi, 
		    SPLITPOINT_OPTIONS *sp_opt, 
		    int bitrate, 
		    int meta_interval)
{
    long sws, sl;
    long mi_to_mic;
    long prepad, postpad;
    long offset;
    long rw_len;
    long mic_to_sw_start, mic_to_sw_end;
    long mic_to_rw_start, mic_to_rw_end;
    long mic_to_cb_start, mic_to_cb_end;
    int xs_silence_length;
    int xs_search_window_1;
    int xs_search_window_2;
    int xs_offset;
    int xs_padding_1;
    int xs_padding_2;

    debug_printf ("---------------------------------------------------\n");
    debug_printf ("xs: %d\n", sp_opt->xs);
    debug_printf ("xs_search_window: %d,%d\n",
		  sp_opt->xs_search_window_1,sp_opt->xs_search_window_2);
    debug_printf ("xs_silence_length: %d\n", sp_opt->xs_silence_length);
    debug_printf ("xs_padding: %d,%d\n", sp_opt->xs_padding_1,
		  sp_opt->xs_padding_2);
    debug_printf ("xs_offset: %d\n", sp_opt->xs_offset);
    debug_printf ("---------------------------------------------------\n");
    debug_printf ("bitrate = %d, meta_inf = %d\n", bitrate, meta_interval);
    debug_printf ("---------------------------------------------------\n");
    
    /* If xs-none, clear out the other xs options */
    if (sp_opt->xs == 0){
	xs_silence_length = 0;
	xs_search_window_1 = 0;
	xs_search_window_2 = 0;
	xs_offset = 0;
	xs_padding_1 = 0;
	xs_padding_2 = 0;
    } else {
	xs_silence_length = sp_opt->xs_silence_length;
	xs_search_window_1 = sp_opt->xs_search_window_1;
	xs_search_window_2 = sp_opt->xs_search_window_2;
	xs_offset = sp_opt->xs_offset;
	xs_padding_1 = sp_opt->xs_padding_1;
	xs_padding_2 = sp_opt->xs_padding_2;
    }


    /* mi_to_mic is the "half of a meta-inf" from the meta inf 
       change to the previous (non-changed) meta inf */
    mi_to_mic = meta_interval / 2;
    debug_printf ("mi_to_mic: %d\n", mi_to_mic);

    /* compute the search window size (sws) */
    sws = ms_to_bytes (xs_search_window_1, bitrate) 
	    + ms_to_bytes (xs_search_window_2, bitrate);
    debug_printf ("sws: %d\n", sws);

    /* compute the silence length (sl) */
    sl = ms_to_bytes (xs_silence_length, bitrate);
    debug_printf ("sl: %d\n", sl);

    /* compute padding */
    prepad = ms_to_bytes (xs_padding_1, bitrate);
    postpad = ms_to_bytes (xs_padding_2, bitrate);
    debug_printf ("padding: %d %d\n", prepad, postpad);

    /* compute offset */
    offset = ms_to_bytes (xs_offset,bitrate);
    debug_printf ("offset: %d\n", offset);

    /* compute interval from mi to search window */
    mic_to_sw_start = - mi_to_mic + offset 
	    - ms_to_bytes(xs_search_window_1,bitrate);
    mic_to_sw_end = - mi_to_mic + offset 
	    + ms_to_bytes(xs_search_window_2,bitrate);
    debug_printf ("mic_to_sw_start: %d\n", mic_to_sw_start);
    debug_printf ("mic_to_sw_end: %d\n", mic_to_sw_end);

    /* compute interval from mi to required window */
    mic_to_rw_start = mic_to_sw_start + sl / 2 - prepad;
    if (mic_to_rw_start > mic_to_sw_start) {
	mic_to_rw_start = mic_to_sw_start;
    }
    mic_to_rw_end = mic_to_sw_end - sl / 2 + postpad;
    if (mic_to_rw_end < mic_to_sw_end) {
	mic_to_rw_end = mic_to_sw_end;
    }
    debug_printf ("mic_to_rw_start: %d\n", mic_to_rw_start);
    debug_printf ("mic_to_rw_end: %d\n", mic_to_rw_end);

    /* if rw is not long enough, make it longer */
    rw_len = mic_to_rw_end - mic_to_rw_start;
    if (rw_len < meta_interval) {
	long start_extra = (meta_interval - rw_len) / 2;
	long end_extra = meta_interval - start_extra;
	mic_to_rw_start -= start_extra;
	mic_to_rw_end += end_extra;
	debug_printf ("mic_to_rw_start (2): %d\n", mic_to_rw_start);
	debug_printf ("mic_to_rw_end (2): %d\n", mic_to_rw_end);
    }

    /* This code replaces the 3 cases (see OBSOLETE in gcs_notes.txt) */
    mic_to_cb_start = mic_to_rw_start;
    mic_to_cb_end = mic_to_rw_end;
    if (mic_to_cb_start > -meta_interval) {
	mic_to_cb_start = -meta_interval;
    }
    if (mic_to_cb_end < 0) {
	mic_to_cb_end = 0;
    }

    /* Convert to chunks & compute cbuf size */
    mic_to_cb_end = (mic_to_cb_end + (meta_interval-1)) / meta_interval;
    mic_to_cb_start = -((-mic_to_cb_start + (meta_interval-1)) 
			/ meta_interval);
    rmi->cbuf2_size = - mic_to_cb_start + mic_to_cb_end;
    if (rmi->cbuf2_size < 3) {
	rmi->cbuf2_size = 3;
    }
    debug_printf ("mic_to_cb_start: %d\n", mic_to_cb_start * meta_interval);
    debug_printf ("mic_to_cb_end: %d\n", mic_to_cb_end * meta_interval);
    debug_printf ("CBUF2 (BLOCKS): %d:%d -> %d\n", mic_to_cb_start,
		  mic_to_cb_end, rmi->cbuf2_size);

    /* Set some global variables to be used by splitting algorithm */
    rmi->mic_to_cb_end = mic_to_cb_end;
    rmi->rw_start_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_start;
    rmi->rw_end_to_cb_end = mic_to_cb_end * meta_interval - mic_to_rw_end;
    rmi->rw_start_to_sw_start = xs_padding_1 - xs_silence_length / 2;
    if (rmi->rw_start_to_sw_start < 0) {
	rmi->rw_start_to_sw_start = 0;
    }
    debug_printf ("m_mic_to_cb_end: %d\n", rmi->mic_to_cb_end);
    debug_printf ("m_rw_start_to_cb_end: %d\n", rmi->rw_start_to_cb_end);
    debug_printf ("m_rw_end_to_cb_end: %d\n", rmi->rw_end_to_cb_end);
    debug_printf ("m_rw_start_to_sw_start: %d\n", rmi->rw_start_to_sw_start);
}

/* GCS: This used to be myrecv in rip_manager.c */
static int
ripstream_recvall (RIP_MANAGER_INFO* rmi, char* buffer, int size)
{
    int ret;
    ret = socklib_recvall (rmi, &rmi->stream_sock, buffer, size, 
			   rmi->prefs->timeout);
    if (ret >= 0 && ret != size) {
	debug_printf ("rip_manager_recv: expected %d, got %d\n",size,ret);
	ret = SR_ERROR_RECV_FAILED;
    }
    return ret;
}

static error_code
get_track_from_metadata (RIP_MANAGER_INFO* rmi, int size, char *newtrack)
{
    int i;
    int ret;
    char *namebuf;
    char *p;
    gchar *gnamebuf;

    /* Default is no track info */
    *newtrack = 0;

    if ((namebuf = malloc(size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    ret = ripstream_recvall (rmi, namebuf, size);
    if (ret <= 0) {
	free(namebuf);
	return ret;
    }

    debug_printf ("METADATA TITLE\n");
    for (i=0; i<size; i++) {
	debug_printf ("%2x ",(unsigned char)namebuf[i]);
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");
    for (i=0; i<size; i++) {
	if (namebuf[i] != 0) {
	    debug_printf ("%2c ",namebuf[i]);
	}
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");

    /* Depending on version, Icecast/Shoutcast use one of the following.
         StreamTitle='Title';StreamURL='URL';
         StreamTitle='Title';
       Limecast has no semicolon, and only example I've seen had no title.
          StreamTitle=' '
    */

    /* GCS NOTE: This assumes ASCII-compatible charset for quote & semicolon.
       Shoutcast protocol has no specification on this... */
    if (!g_str_has_prefix (namebuf, "StreamTitle='")) {
	free (namebuf);
	return SR_SUCCESS;
    }
    gnamebuf = g_strdup (namebuf+strlen("StreamTitle='"));
    free(namebuf);

    if ((p = strstr (gnamebuf, "';"))) {
	*p = 0;
    }
    else if ((p = strrchr (gnamebuf, '\''))) {
	*p = 0;
    }
    g_strstrip (gnamebuf);
    debug_printf ("gnamebuf (stripped) = %s\n", gnamebuf);

    if (strlen (gnamebuf) == 0) {
	g_free (gnamebuf);
	return SR_SUCCESS;
    }

    g_strlcpy (newtrack, gnamebuf, MAX_TRACK_LEN);
    g_free (gnamebuf);

    return SR_SUCCESS;
}

/* ripstream.c - jonclegg@yahoo.com
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
#include "cbuf2.h"
#include "findsep.h"
#include "util.h"
#include "parse.h"
#include "rip_manager.h"
#include "ripstream.h"
#include "debug.h"
#include "filelib.h"
#include "relaylib.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
/* -- See ripstream.h -- */

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code find_sep (u_long *pos1, u_long *pos2);
static error_code end_track(u_long pos1, u_long pos2, TRACK_INFO* ti);
static error_code start_track (TRACK_INFO* ti);

static void compute_cbuf2_size (SPLITPOINT_OPTIONS *sp_opt,
				  int bitrate, int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes);
static void clear_track_info (TRACK_INFO* ti);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static CBUF2			m_cbuffer;
static IO_GET_STREAM	*m_in;
static TRACK_INFO m_old_track;	    /* The track that's being ripped now */
static TRACK_INFO m_new_track;	    /* The track that's gonna start soon */
static TRACK_INFO m_current_track;  /* The metadata as I'm parsing it */
static int m_first_time_through;
static char			m_no_meta_name[MAX_TRACK_LEN] = {'\0'};
static char			*m_getbuffer = NULL;
static int			m_find_silence = -1;
static BOOL			m_addID3tag = TRUE;
static SPLITPOINT_OPTIONS	*m_sp_opt;
static int			m_bitrate;
static int			m_http_bitrate;
static int			m_ripchunk_size;
static int			m_meta_interval;
static unsigned int		m_cue_sheet_bytes = 0;

static int			m_cbuf2_size;
static int			m_mi_to_cbuf2_start;
static int			m_mi_to_cbuf2_end;
static int			m_sw_start_to_cbuf2_end;
static int			m_sw_end_to_cbuf2_end;
static int			m_min_search_win;
static int			m_drop_count;
static int			m_track_count = 0;
static int m_content_type;

/*
 * oddsock's id3 tags
 */
typedef struct ID3V1st
{
        char    tag[3];
        char    songtitle[30];
        char    artist[30];
        char    album[30];
        char    year[4];
        char    comment[30];
        char    genre;
} ID3Tag;

typedef struct ID3V2headst {
	char	tag[3];
	int	version;
	char	flags;
	int	size;
} ID3V2head;

typedef struct ID3V2framest {
	char	id[4];
	int	size;
	char	pad[3];
} ID3V2frame;



error_code
ripstream_init (IO_GET_STREAM *in, char *no_meta_name, 
		int drop_count,
		SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, 
		int meta_interval, 
		int content_type, 
		BOOL addID3tag)
{
    if (!in || !sp_opt || !no_meta_name) {
	printf ("Error: invalid ripstream parameters\n");
	return SR_ERROR_INVALID_PARAM;
    }

    m_in = in;
    m_sp_opt = sp_opt;
    m_track_count = 0;
    m_addID3tag = addID3tag;
    strcpy(m_no_meta_name, no_meta_name);
    m_drop_count = drop_count;
    m_http_bitrate = bitrate;
    m_bitrate = -1;
    m_content_type = content_type;
    /* GCS RMK: Ripchunk_size is the metaint size, or default size
       if stream doesn't have meta data */
    m_ripchunk_size = in->getsize;
    m_meta_interval = meta_interval;
    m_cue_sheet_bytes = 0;

    clear_track_info (&m_old_track);
    clear_track_info (&m_new_track);
    clear_track_info (&m_current_track);
    m_first_time_through = 1;

    if ((m_getbuffer = malloc(in->getsize)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    return SR_SUCCESS;
}

void
ripstream_destroy()
{
    debug_printf ("RIPSTREAM_DESTROY\n");
    if (m_getbuffer) {free(m_getbuffer); m_getbuffer = NULL;}
    m_find_silence = -1;
    m_in = NULL;
    m_cbuf2_size = 0;
    cbuf2_destroy(&m_cbuffer);

    clear_track_info (&m_old_track);
    clear_track_info (&m_new_track);
    clear_track_info (&m_current_track);
    m_first_time_through = 1;

    m_no_meta_name[0] = '\0';
    m_track_count = 0;
    m_addID3tag = TRUE;
}

BOOL is_buffer_full()
{
    return (cbuf2_get_free(&m_cbuffer) - m_in->getsize) < m_in->getsize;
}

BOOL
is_track_changed()
{
    /* If metadata is duplicate of previous, then no change. */
    if (!strcmp(m_old_track.raw_metadata, m_current_track.raw_metadata))
	return 0;

    /* Otherwise, there was a change. */
    return 1;
}

static void
format_track_info (TRACK_INFO* ti, char* tag)
{
    debug_printf ("----- TRACK_INFO %s\n"
	"HAVETI: %d\n"
	"RAW_MD: %s\n"
	"ARTIST: %s\n"
	"TITLE:  %s\n"
	"ALBUM:  %s\n",
	tag,
        ti->have_track_info,
	ti->raw_metadata,
	ti->artist,
	ti->title,
	ti->album);
}

static void
clear_track_info (TRACK_INFO* ti)
{
    ti->have_track_info = 0;
    ti->raw_metadata[0] = 0;
    ti->artist[0] = 0;
    ti->title[0] = 0;
    ti->album[0] = 0;
}

static void
copy_track_info (TRACK_INFO* dest, TRACK_INFO* src)
{
    dest->have_track_info = src->have_track_info;
    strcpy (dest->raw_metadata, src->raw_metadata);
    strcpy (dest->artist, src->artist);
    strcpy (dest->title, src->title);
    strcpy (dest->album, src->album);
}

error_code
ripstream_rip()
{
    int ret;
    int real_ret = SR_SUCCESS;
    u_long extract_size;

    /* get the data & title */
    debug_printf ("RIPSTREAM_RIP: get_stream_data:%p\n",m_in);
    ret = m_in->get_stream_data(m_getbuffer, m_current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("m_in->get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    /* Immediately dump data to relay & show file */
    rip_manager_put_raw_data (m_getbuffer, m_ripchunk_size);

    /* First time through, determine the bitrate. 
       The bitrate is needed to do the track splitting parameters 
       properly in seconds.  See the readme file for details.  */
    if (m_bitrate == -1) {
        unsigned long test_bitrate;
	debug_printf("Querying stream for bitrate - first time.\n");
	if (m_content_type == CONTENT_TYPE_MP3) {
	    find_bitrate(&test_bitrate, m_getbuffer, m_ripchunk_size);
	    m_bitrate = test_bitrate / 1000;
	    debug_printf("Got bitrate: %d\n",m_bitrate);
	} else {
	    m_bitrate = 0;
	}

	if (m_bitrate == 0) {
	    /* Couldn't decode from mp3, so let's go with what the 
	       http header says, or fallback to 24.  */
	    if (m_http_bitrate > 0)
		m_bitrate = m_http_bitrate;
	    else
		m_bitrate = 24;
	}
        compute_cbuf2_size (m_sp_opt, m_bitrate, m_in->getsize);
	ret = cbuf2_init(&m_cbuffer, m_in->getsize * m_cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
    }

    /* Parse metadata */
    if (m_current_track.raw_metadata[0]) {
        parse_metadata (&m_current_track);
    } else {
        clear_track_info (&m_current_track);
    }

    /* First time through, so start a track. */
    if (m_first_time_through) {
	int ret;
	debug_printf ("First time through...\n");
	m_first_time_through = 0;
	if (!m_current_track.have_track_info) {
	    strcpy (m_current_track.raw_metadata, m_no_meta_name);
	}
	ret = rip_manager_start_track (&m_current_track, m_track_count);
	if (ret != SR_SUCCESS) {
	    debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
	    return ret;
	}
	filelib_write_cue (&m_current_track, 0);
	copy_track_info (&m_old_track, &m_current_track);
    }

    /* Copy the data into cbuffer */
    ret = cbuf2_insert_chunk(&m_cbuffer, m_getbuffer, m_in->getsize);
    if (ret != SR_SUCCESS) {
	debug_printf("start_track had bad return code %d\n", ret);
	return ret;
    }

    /* Check for track change. */
    if (m_current_track.have_track_info && is_track_changed()) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	debug_printf ("VERIFIED TRACK CHANGE (m_find_silence = %d)\n",
		      m_find_silence);
	copy_track_info (&m_new_track, &m_current_track);
	relaylib_send_meta_data (m_current_track.raw_metadata);
	if (m_find_silence < 0) {
	    if (m_mi_to_cbuf2_end > 0) {
		m_find_silence = m_mi_to_cbuf2_end;
	    } else {
		m_find_silence = 0;
	    }
	}
    } else {
	if (m_meta_interval != NO_META_INTERVAL)
	    relaylib_send_meta_data (0);
    }

    debug_printf ("Checking for silence\n");
    format_track_info (&m_old_track, "old");
    format_track_info (&m_new_track, "new");
    format_track_info (&m_current_track, "current");

    if (m_find_silence == 0) {
	/* Find separation point */
	u_long pos1, pos2;
	ret = find_sep (&pos1, &pos2);
	if (ret != SR_SUCCESS) {
	    debug_printf("find_sep had bad return code %d\n", ret);
	    return ret;
	}

	/* Write out previous track */
	ret = end_track(pos1, pos2, &m_old_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
        m_cue_sheet_bytes += pos2;

	/* Start next track */
	ret = start_track (&m_new_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	m_find_silence = -1;

	copy_track_info (&m_old_track, &m_new_track);
    }
    if (m_find_silence >= 0) m_find_silence --;

    /* If buffer almost full, dump extra to current song. */
    if (cbuf2_get_free(&m_cbuffer) < m_in->getsize) {
        /* Extract only as much as needed */
	extract_size = m_in->getsize - cbuf2_get_free(&m_cbuffer);
        ret = cbuf2_extract(&m_cbuffer, m_getbuffer, extract_size);
        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_extract had bad return code %d\n", ret);
	    return ret;
	}
	/* Post to caller */
	m_cue_sheet_bytes += extract_size;
	rip_manager_put_data(m_getbuffer, extract_size);
    }

    return real_ret;
}

static u_long
clip_to_cbuffer (int pos)
{
    if (pos < 0) {
	return 0;
    } else if ((u_long)pos > cbuf2_get_used(&m_cbuffer)) {
	return cbuf2_get_used(&m_cbuffer);
    } else {
	return (u_long) pos;
    }
}

/* New version */
error_code
find_sep (u_long *pos1, u_long *pos2)
{
    int pos1i, pos2i;
    int sw_start, sw_end, sw_sil;
    u_long psilence;
    int ret;

    debug_printf ("*** Finding separation point\n");

    /* First, find the search region w/in cbuffer. */
    sw_start = cbuf2_get_used(&m_cbuffer) - m_sw_start_to_cbuf2_end;
    if (sw_start < 0) sw_start = 0;
    sw_end = cbuf2_get_used(&m_cbuffer) - m_sw_end_to_cbuf2_end;
    if (sw_end < 0) sw_end = 0;
    debug_printf ("search window (bytes): %d,%d,%d\n", sw_start, sw_end,
		  cbuf2_get_used(&m_cbuffer));

    if (m_content_type != CONTENT_TYPE_MP3) {
        /* If the search region is too small, take the middle. */
	sw_sil = (sw_end + sw_start) / 2;
	debug_printf ("(not mp3) taking middle: sw_sil=%d\n", sw_sil);
    }
    else if (sw_end - sw_start < m_min_search_win) {
        /* If the search region is too small, take the middle. */
	sw_sil = (sw_end + sw_start) / 2;
	debug_printf ("taking middle: sw_sil=%d\n", sw_sil);
    } else {
	/* Otherwise, copy from search window into private buffer */
	int bufsize = sw_end - sw_start;
	char* buf = (u_char *)malloc(bufsize);
	ret = cbuf2_peek_rgn (&m_cbuffer, buf, sw_start, bufsize);
	if (ret != SR_SUCCESS) {
	    debug_printf ("PEEK FAILED: %d\n", ret);
	    free(buf);
	    return ret;
	}
	debug_printf ("PEEK OK\n");

	/* Find silence point */
	ret = findsep_silence(buf, bufsize, m_sp_opt->xs_silence_length,
			      &psilence);
	free(buf);
	if (ret != SR_SUCCESS && ret != SR_ERROR_CANT_DECODE_MP3) {
	    return ret;
	}
	sw_sil = sw_start + psilence;

	/* Add 1/2 of the silence window to get middle */
	sw_sil = sw_sil + ms_to_bytes(m_sp_opt->xs_silence_length/2,m_bitrate);
    }

    /* Compute padding.  We need pos1 == end of 1st song and 
       pos2 == beginning of 2nd song. */
    pos1i = sw_sil + ms_to_bytes(m_sp_opt->xs_padding_2,m_bitrate);
    pos2i = sw_sil - ms_to_bytes(m_sp_opt->xs_padding_1,m_bitrate);

    /* Clip padding to amount available in cbuffer (there could be 
       less than the full cbuffer in a few cases, such as track 
       changes close to each other). */
    *pos1 = clip_to_cbuffer (pos1i);
    *pos2 = clip_to_cbuffer (pos2i);

    return SR_SUCCESS;
}

error_code
end_track(u_long pos1, u_long pos2, TRACK_INFO* ti)
{
    // pos1 is end of prev track
    // pos2 is beginning of next track
    int ret;

    // I think pos can be zero if the silence is right at the beginning
    // i.e. it is a bug in s.r.
    u_char *buf = (u_char *)malloc(pos1);

    // pos1 is end of prev track
    // pos2 is beginning of next track

    // First, dump the part only in prev track
    if (pos1 <= pos2) {
	if ((ret = cbuf2_extract(&m_cbuffer, buf, pos1)) != SR_SUCCESS)
	    goto BAIL;
	// Next, skip past portion not in either track
	if (pos1 < pos2) {
	    if ((ret = cbuf2_fastforward(&m_cbuffer, pos2-pos1)) != SR_SUCCESS)
		goto BAIL;
	}
    } else {
	if ((ret = cbuf2_extract(&m_cbuffer, buf, pos2)) != SR_SUCCESS)
	    goto BAIL;
	// Next, grab, but don't skip, past portion in both tracks
	if ((ret = cbuf2_peek(&m_cbuffer, buf+pos2, pos1-pos2)) != SR_SUCCESS)
	    goto BAIL;
    }

    // Write that out to the current file
    if ((ret = rip_manager_put_data(buf, pos1)) != SR_SUCCESS)
	goto BAIL;

    if (m_addID3tag) {
	ID3Tag id3;
	memset(&id3, '\000',sizeof(id3));
	strncpy(id3.tag, "TAG", strlen("TAG"));
	strncpy(id3.artist, ti->artist, sizeof(id3.artist));
	strncpy(id3.songtitle, ti->title, sizeof(id3.songtitle));
	strncpy(id3.album, ti->album, sizeof(id3.album));
	id3.genre = (char) 0xFF; // see http://www.id3.org/id3v2.3.0.html#secA
	if ((ret = rip_manager_put_data((char *)&id3, sizeof(id3))) != SR_SUCCESS)
	    goto BAIL;
    }

    // Only save this track if we've skipped over enough cruft 
    // at the beginning of the stream
    debug_printf("Current track number %d (skipping if %d or less)\n", 
		 m_track_count, m_drop_count);
    if (m_track_count > m_drop_count)
	if ((ret = rip_manager_end_track(ti)) != SR_SUCCESS)
	    goto BAIL;

 BAIL:
    free(buf);
    return ret;
}

error_code
start_track (TRACK_INFO* ti)
{
#define HEADER_SIZE 1600
    int ret;
    int i;
    //ID3V2head id3v2header;
    ID3V2frame id3v2frame1;
    ID3V2frame id3v2frame2;
    char comment[1024] = "Ripped with Streamripper";
    char bigbuf[HEADER_SIZE] = "";
    int	sent = 0;
    char header1[6] = "ID3\x03\0\0";
    int header_size = HEADER_SIZE;
    unsigned long int framesize = 0;
    unsigned int secs;

    debug_printf ("calling rip_manager_start_track(#2)\n");
    ret = rip_manager_start_track (ti, m_track_count);
    if (ret != SR_SUCCESS) {
	debug_printf ("rip_manager_start_track failed(#2): %d\n",ret);
        return ret;
    }

    /* Dump to artist/title to cue sheet */
    secs = bytes_to_secs (m_cue_sheet_bytes);
    ret = filelib_write_cue (ti, secs);
    if (ret != SR_SUCCESS)
        return ret;

    /* Oddsock's ID3 stuff, (oddsock@oddsock.org) */
    if (m_addID3tag) {
	memset(bigbuf, '\000', sizeof(bigbuf));
	//memset(&id3v2header, '\000', sizeof(id3v2header));
	memset(&id3v2frame1, '\000', sizeof(id3v2frame1));
	memset(&id3v2frame2, '\000', sizeof(id3v2frame2));

#if defined (commentout)
	strncpy(id3v2header.tag, "ID3", 3);
	id3v2header.size = 1599;
	framesize = htonl(id3v2header.size);
	id3v2header.version = 3;
	buf[0] = 3;
	buf[1] = '\000';
	ret = rip_manager_put_data((char *)&(id3v2header.tag), 3);
	if (ret != SR_SUCCESS) return ret;
	ret = rip_manager_put_data((char *)&buf, 2);
	if (ret != SR_SUCCESS) return ret;
	ret = rip_manager_put_data((char *)&(id3v2header.flags), 1);
	if (ret != SR_SUCCESS) return ret;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(id3v2header);
#endif
	/* Write header */
	ret = rip_manager_put_data(header1, 6);
	if (ret != SR_SUCCESS) return ret;
	for (i = 0; i < 4; i++) {
	    char x = (header_size >> (3-i)*7) & 0x7F;
	    ret = rip_manager_put_data((char *)&x, 1);
	    if (ret != SR_SUCCESS) return ret;
	}

	// Write ID3V2 frame1 with data
	strncpy(id3v2frame1.id, "TPE1", 4);
	framesize = htonl(strlen(ti->artist)+1);
	ret = rip_manager_put_data((char *)&(id3v2frame1.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame1.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data(ti->artist, strlen(ti->artist));
	if (ret != SR_SUCCESS) return ret;
	sent += strlen(ti->artist);

	// Write ID3V2 frame2 with data
	strncpy(id3v2frame2.id, "TIT2", 4);
	framesize = htonl(strlen(ti->title)+1);
	ret = rip_manager_put_data((char *)&(id3v2frame2.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame2.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data(ti->title, strlen(ti->title));
	if (ret != SR_SUCCESS) return ret;
	sent += strlen(ti->title);

	// Write ID3V2 frame2 with data
	strncpy(id3v2frame2.id, "TENC", 4);
	framesize = htonl(strlen(comment)+1);
	ret = rip_manager_put_data((char *)&(id3v2frame2.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame2.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	sent += sizeof(id3v2frame2);
	ret = rip_manager_put_data(comment, strlen(comment));
	if (ret != SR_SUCCESS) return ret;
	sent += strlen(comment);

	// Write ID3V2 frame2 with data
	memset(&id3v2frame2, '\000', sizeof(id3v2frame2));
	strncpy(id3v2frame2.id, "TALB", 4);
	framesize = htonl(strlen(ti->album)+1);
	ret = rip_manager_put_data((char *)&(id3v2frame2.id), 4);
	if (ret != SR_SUCCESS) return ret;
	sent += 4;
	ret = rip_manager_put_data((char *)&(framesize), sizeof(framesize));
	if (ret != SR_SUCCESS) return ret;
	sent += sizeof(framesize);
	ret = rip_manager_put_data((char *)&(id3v2frame2.pad), 3);
	if (ret != SR_SUCCESS) return ret;
	sent += 3;
	ret = rip_manager_put_data(ti->album, strlen(ti->album));
	if (ret != SR_SUCCESS) return ret;
	sent += strlen(ti->album);

	ret = rip_manager_put_data(bigbuf, 1600-sent);
	if (ret != SR_SUCCESS) return ret;
    }
    m_track_count ++;
    debug_printf ("Changed track count to %d\n", m_track_count);

    return SR_SUCCESS;
}

/* GCS: This converts either positive or negative ms to blocks,
   and must work for rounding up and rounding down */
static int
ms_to_blocks (int ms, int bitrate, int round_up)
{
    int ms_abs = ms > 0 ? ms : -ms;
    int ms_sign = ms > 0 ? 1 : 0;
    int bits = ms_abs * bitrate;
    int bits_per_block = 8 * m_ripchunk_size;
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
bytes_to_secs (unsigned int bytes)
{
    /* divided by 125 because 125 = 1000 / 8 */
    int secs = (bytes / m_bitrate) / 125;
    return secs;
}

static void
compute_cbuf2_size (SPLITPOINT_OPTIONS *sp_opt, int bitrate, 
		      int meta_interval)
{
    u_long sws;

    debug_printf ("---------------------------------------------------\n");
    debug_printf ("xs_search_window: %d,%d\n",
		  sp_opt->xs_search_window_1,sp_opt->xs_search_window_2);
    debug_printf ("xs_silence_length: %d\n", sp_opt->xs_silence_length);
    debug_printf ("xs_padding: %d,%d\n", sp_opt->xs_padding_1,
		  sp_opt->xs_padding_2);
    debug_printf ("xs_offset: %d\n", sp_opt->xs_offset);
    debug_printf ("---------------------------------------------------\n");
    
    /* compute the search window size */
    sws = sp_opt->xs_search_window_1 + sp_opt->xs_search_window_2;

    /* compute interval from mi to beginning of buffer */
    m_mi_to_cbuf2_start = sp_opt->xs_offset - sp_opt->xs_search_window_1
	+ sp_opt->xs_silence_length/2 - sp_opt->xs_padding_1;
    debug_printf ("m_mi_to_cbuf2_start (ms): %d\n", m_mi_to_cbuf2_start);

    /* compute interval from mi to end of buffer */
    m_mi_to_cbuf2_end = sp_opt->xs_offset + sp_opt->xs_search_window_2
	- sp_opt->xs_silence_length/2 + sp_opt->xs_padding_2;
    debug_printf ("m_mi_to_cbuf2_end (ms): %d\n", m_mi_to_cbuf2_end);
    debug_printf ("bitrate = %d, meta_inf = %d\n", bitrate, meta_interval);

    debug_printf ("CBUF2 (ms): %d:%d\n",m_mi_to_cbuf2_start,
	m_mi_to_cbuf2_end);

    /* convert from ms to meta-inf blocks */
    m_mi_to_cbuf2_start = ms_to_blocks (m_mi_to_cbuf2_start, bitrate, 0);
    m_mi_to_cbuf2_end = ms_to_blocks (m_mi_to_cbuf2_end, bitrate, 1);

    /* Case 1 -- see readme (GCS) */
    if (m_mi_to_cbuf2_start > 0) {
	debug_printf ("CASE 1\n");
        m_cbuf2_size = m_mi_to_cbuf2_end;
	m_sw_end_to_cbuf2_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_end_to_cbuf2_end < 0) {
	    m_sw_end_to_cbuf2_end = 0;
	}
	m_sw_start_to_cbuf2_end = m_sw_end_to_cbuf2_end + sws;
    }

    /* Case 2 */
    else if (m_mi_to_cbuf2_end >= 0) {
	debug_printf ("CASE 2\n");
        m_cbuf2_size = m_mi_to_cbuf2_end - m_mi_to_cbuf2_start;
	m_sw_end_to_cbuf2_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_end_to_cbuf2_end < 0) {
	    m_sw_end_to_cbuf2_end = 0;
	}
	m_sw_start_to_cbuf2_end = m_sw_end_to_cbuf2_end + sws;
    }

    /* Case 3 */
    else {
	debug_printf ("CASE 3\n");
        m_cbuf2_size = - m_mi_to_cbuf2_start;
        m_sw_start_to_cbuf2_end = m_cbuf2_size 
	    + sp_opt->xs_silence_length/2 
	    - sp_opt->xs_padding_1;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_start_to_cbuf2_end > m_cbuf2_size) {
	    m_sw_start_to_cbuf2_end = m_cbuf2_size;
	}
	m_sw_end_to_cbuf2_end = m_sw_start_to_cbuf2_end + sws;
    }

    /* Convert these from ms to bytes */
    debug_printf ("SEARCH WIN (ms): %d | %d | %d\n",
		  m_sw_start_to_cbuf2_end,
		  sws, m_sw_end_to_cbuf2_end);
    m_sw_start_to_cbuf2_end = ms_to_bytes (m_sw_start_to_cbuf2_end, 
	bitrate);
    m_sw_end_to_cbuf2_end = ms_to_bytes (m_sw_end_to_cbuf2_end, bitrate);

    /* Get minimum search window (in bytes) */
    m_min_search_win = sp_opt->xs_silence_length + (sws - sp_opt->xs_silence_length) / 2 ;
    m_min_search_win = ms_to_bytes (m_min_search_win, bitrate);

    debug_printf ("CBUF2 (BLOCKS): %d:%d -> %d\n",m_mi_to_cbuf2_start,
	m_mi_to_cbuf2_end, m_cbuf2_size);
    debug_printf ("SEARCH WIN (bytes): %d | %d | %d\n",
		  m_sw_start_to_cbuf2_end,
		  ms_to_bytes(sws,bitrate),
		  m_sw_end_to_cbuf2_end);
}

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
#include "types.h"
#include "cbuffer.h"
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

static void compute_cbuffer_size (SPLITPOINT_OPTIONS *sp_opt,
				  int bitrate, int meta_interval);
static int ms_to_bytes (int ms, int bitrate);
static int bytes_to_secs (unsigned int bytes);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static CBUFFER			m_cbuffer;
static IO_GET_STREAM	*m_in;
static TRACK_INFO m_last_track;
static TRACK_INFO m_current_track;
static int m_first_time_through;
static char			m_no_meta_name[MAX_TRACK_LEN] = {'\0'};
static char			*m_getbuffer = NULL;
static int			m_find_silence = -1;
static BOOL			m_addID3tag = TRUE;
static SPLITPOINT_OPTIONS	*m_sp_opt;
static int			m_bitrate;
static int			m_http_bitrate;
static int			m_meta_interval;
static unsigned int		m_cue_sheet_bytes = 0;

static int			m_cbuffer_size;
static int			m_mi_to_cbuffer_start;
static int			m_mi_to_cbuffer_end;
static int			m_sw_start_to_cbuffer_end;
static int			m_sw_end_to_cbuffer_end;
static int			m_min_search_win;
static int			m_drop_count;
static int			m_track_count = 0;

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
		int bitrate, BOOL addID3tag)
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
    /* GCS RMK: If no meta data, then this is the default chunk size. */
    m_meta_interval = in->getsize;
    m_cue_sheet_bytes = 0;

    m_last_track.raw_metadata[0] = '\0';
    m_last_track.have_track_info = 0;
    m_current_track.raw_metadata[0] = '\0';
    m_current_track.have_track_info = 0;
    m_first_time_through = 1;

    if ((m_getbuffer = malloc(in->getsize)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    return SR_SUCCESS;
}

void
ripstream_destroy()
{
    if (m_getbuffer) {free(m_getbuffer); m_getbuffer = NULL;}
    m_find_silence = -1;
    m_in = NULL;
    m_cbuffer_size = 0;
    cbuffer_destroy(&m_cbuffer);

    m_last_track.raw_metadata[0] = '\0';
    m_last_track.have_track_info = 0;
    m_current_track.raw_metadata[0] = '\0';
    m_current_track.have_track_info = 0;
    m_first_time_through = 1;

    m_no_meta_name[0] = '\0';
    m_track_count = 0;
    m_addID3tag = TRUE;
}

BOOL is_buffer_full()
{
    return (cbuffer_get_free(&m_cbuffer) - m_in->getsize) < m_in->getsize;
}

BOOL is_track_changed()
{
#if defined (commentout)
    debug_printf ("is_track_changed\n"
		  "|%s|\n|%s|\n",m_last_track, m_current_track);

    /* GCS: this is obsolete */
    if (strstr(m_current_track,m_drop_string) && *m_drop_string) {
	strcpy(m_current_track,m_last_track);
	return 0;
    }

    return strcmp(m_last_track, m_current_track) != 0 && *m_last_track;
#endif

    /* Not sure about this one.  (GCS FIX) */
    if (!(*m_last_track.raw_metadata))
	return 0;

    /* If metadata is duplicate of previous, then no change. */
    if (!strcmp(m_last_track.raw_metadata, m_current_track.raw_metadata))
	return 0;

    /* Otherwise, there was a change. */
    return 1;
}

#if defined (commentout)
/* GCS April 17, 2004 - This never happens.  It is leftover from 
   the live365 stuff */
BOOL
is_no_meta_track()
{
    return strcmp(m_current_track, NO_TRACK_STR) == 0;
}
#endif

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
    int track_status;
    u_long extract_size;

    // only copy over the last track name if 
    // we are not waiting to buffer for a silent point
    if (m_find_silence < 0) {
#if defined (commentout)
	strcpy(m_last_track, m_current_track);
#endif
	copy_track_info (&m_last_track, &m_current_track);
	strcpy(m_last_track.raw_metadata, m_current_track.raw_metadata);
    }

    // get the data
    ret = m_in->get_stream_data(m_getbuffer, &track_status, m_current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf("m_in->get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    /* Immediately dump to relay & show file */
    rip_manager_put_raw_data (m_getbuffer, m_meta_interval);

    /* Query stream for bitrate, but only first time through. */
    if (m_bitrate == -1) {
        unsigned long test_bitrate;
	debug_printf("Querying stream for bitrate - first time.\n");
        find_bitrate(&test_bitrate, m_getbuffer, m_meta_interval);
	m_bitrate = test_bitrate / 1000;

        /* The bitrate is needed to do the track splitting parameters 
	 * properly in seconds.  See the readme file for details.  */
	debug_printf("Got bitrate: %d\n",m_bitrate);
	if (m_bitrate == 0) {
	    /* I'm not sure what this means, but let's go with 
	     * what the http header says... */
	    if (m_http_bitrate > 0)
		m_bitrate = m_http_bitrate;
	    else
		m_bitrate = 24;
	}
        compute_cbuffer_size (m_sp_opt, m_bitrate, m_in->getsize);
	ret = cbuffer_init(&m_cbuffer, m_in->getsize * m_cbuffer_size);
	if (ret != SR_SUCCESS) return ret;
    }

    /* Parse metadata */
    if (track_status == 1) {
        parse_metadata (&m_current_track);
    }

    /* First time through, so start a track. */
    if (m_first_time_through) {
	int ret;
	debug_printf ("First time through...\n");
	if (!m_current_track.have_track_info) {
	    strcpy (m_current_track.raw_metadata, m_no_meta_name);
	}
	ret = rip_manager_start_track (&m_current_track, m_track_count);
	if (ret != SR_SUCCESS) {
	    debug_printf ("rip_manager_start_track failed(#1): %d\n",ret);
	    return ret;
	}
	/* write the cue sheet */
	filelib_write_cue (&m_current_track, 0);
	/* set last track info */
	strcpy (m_last_track.raw_metadata, m_current_track.raw_metadata);
	
	m_first_time_through = 0;
    }

    /* Copy the data into cbuffer */
    ret = cbuffer_insert(&m_cbuffer, m_getbuffer, m_in->getsize);
    if (ret != SR_SUCCESS) {
	debug_printf("start_track had bad return code %d\n", ret);
	return ret;
    }													

    if (is_track_changed()) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	debug_printf ("is_track_changed: m_find_silence = %d\n",
		      m_find_silence);
        relay_send_meta_data (m_current_track.raw_metadata);
	if (m_find_silence < 0) {
	    if (m_mi_to_cbuffer_end > 0) {
		m_find_silence = m_mi_to_cbuffer_end;
	    } else {
		m_find_silence = 0;
	    }
	}
    } else {
        relay_send_meta_data (0);
    }

    if (m_find_silence == 0) {
	/* Find separation point */
	u_long pos1, pos2;
	ret = find_sep (&pos1, &pos2);
	if (ret != SR_SUCCESS) {
	    debug_printf("find_sep had bad return code %d\n", ret);
	    return ret;
	}

	/* Write out previous track */
	ret = end_track(pos1, pos2, &m_last_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
        m_cue_sheet_bytes += pos2;
	//ret = start_track(m_current_track.raw_metadata);
	ret = start_track (&m_current_track);
	if (ret != SR_SUCCESS)
	    real_ret = ret;
	m_find_silence = -1;
    }
    if (m_find_silence >= 0) m_find_silence --;

    /* If buffer almost full, then post to caller */
    if (cbuffer_get_free(&m_cbuffer) < m_in->getsize) {
        /* Extract only as much as needed */
	extract_size = m_in->getsize - cbuffer_get_free(&m_cbuffer);
        ret = cbuffer_extract(&m_cbuffer, m_getbuffer, extract_size);
        if (ret != SR_SUCCESS) {
	    debug_printf("cbuffer_extract had bad return code %d\n", ret);
	    return ret;
	}
        /* Post to caller */
	if (*m_current_track.raw_metadata) {
	    m_cue_sheet_bytes += extract_size;
	    rip_manager_put_data(m_getbuffer, extract_size);
	}
    }

    return real_ret;
}

static u_long
clip_to_cbuffer (int pos)
{
    if (pos < 0) {
	return 0;
    } else if ((u_long)pos > cbuffer_get_used(&m_cbuffer)) {
	return cbuffer_get_used(&m_cbuffer);
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
    sw_start = cbuffer_get_used(&m_cbuffer) - m_sw_start_to_cbuffer_end;
    if (sw_start < 0) sw_start = 0;
    sw_end = cbuffer_get_used(&m_cbuffer) - m_sw_end_to_cbuffer_end;
    if (sw_end < 0) sw_end = 0;
    debug_printf ("search window (bytes): %d,%d,%d\n", sw_start, sw_end,
		  cbuffer_get_used(&m_cbuffer));

    if (sw_end - sw_start < m_min_search_win) {
        /* If the search region is too small, take the middle. */
	sw_sil = (sw_end + sw_start) / 2;
	debug_printf ("taking middle: sw_sil=%d\n", sw_sil);
    } else {
	/* Otherwise, copy from search window into private buffer */
	int bufsize = sw_end - sw_start;
	char* buf = (u_char *)malloc(bufsize);
	ret = cbuffer_peek_rgn (&m_cbuffer, buf, sw_start, bufsize);
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
	if ((ret = cbuffer_extract(&m_cbuffer, buf, pos1)) != SR_SUCCESS)
	    goto BAIL;
	// Next, skip past portion not in either track
	if (pos1 < pos2) {
	    if ((ret = cbuffer_fastforward(&m_cbuffer, pos2-pos1)) != SR_SUCCESS)
		goto BAIL;
	}
    } else {
	if ((ret = cbuffer_extract(&m_cbuffer, buf, pos2)) != SR_SUCCESS)
	    goto BAIL;
	// Next, grab, but don't skip, past portion in both tracks
	if ((ret = cbuffer_peek(&m_cbuffer, buf+pos2, pos1-pos2)) != SR_SUCCESS)
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

#if defined (commentout)
error_code
end_track(u_long pos1, u_long pos2, TRACK_INFO* ti)
{
    // pos1 is end of prev track
    // pos2 is beginning of next track
    int ret;
    ID3Tag	id3;
    char	*p1;

    // I think pos can be zero if the silence is right at the beginning
    // i.e. it is a bug in s.r.
    u_char *buf = (u_char *)malloc(pos1);

    if (m_addID3tag) {
	char    artist[1024] = "";
	char    title[1024] = "";

	memset(&id3, '\000',sizeof(id3));
	strncpy(id3.tag, "TAG", strlen("TAG"));
	//   strncpy(id3.comment, "Streamripper!", strlen("Streamripper!"));

	memset(&artist, '\000',sizeof(artist));
	memset(&title, '\000',sizeof(title));
	p1 = strchr(trackname, '-');
	if (p1) {
            strncpy(artist, trackname, p1-trackname);
            p1++;
	    if (*p1 == ' ') {
		p1++;
	    }
	    strcpy(title, p1);
	    strncpy(id3.artist, artist, sizeof(id3.artist));
	    strncpy(id3.songtitle, title, sizeof(id3.songtitle));
	    //fprintf(stdout, "Adding ID3 (%s) (%s)\n", artist, title);
        }
    }

    // pos1 is end of prev track
    // pos2 is beginning of next track

    // First, dump the part only in prev track
    if (pos1 <= pos2) {
	if ((ret = cbuffer_extract(&m_cbuffer, buf, pos1)) != SR_SUCCESS)
	    goto BAIL;
	// Next, skip past portion not in either track
	if (pos1 < pos2) {
	    if ((ret = cbuffer_fastforward(&m_cbuffer, pos2-pos1)) != SR_SUCCESS)
		goto BAIL;
	}
    } else {
	if ((ret = cbuffer_extract(&m_cbuffer, buf, pos2)) != SR_SUCCESS)
	    goto BAIL;
	// Next, grab, but don't skip, past portion in both tracks
	if ((ret = cbuffer_peek(&m_cbuffer, buf+pos2, pos1-pos2)) != SR_SUCCESS)
	    goto BAIL;
    }

    // Write that out to the current file
    if ((ret = rip_manager_put_data(buf, pos1)) != SR_SUCCESS)
	goto BAIL;

    if (m_addID3tag) {
	if ((ret = rip_manager_put_data((char *)&id3, sizeof(id3))) != SR_SUCCESS)
	    goto BAIL;
    }

    // Only save this track if we've skipped over enough cruft 
    // at the beginning of the stream
    debug_printf("Current track number %d (skipping if %d or less)\n", 
		 m_track_count, m_drop_count);
    if (m_track_count > m_drop_count)
	if ((ret = rip_manager_end_track(trackname)) != SR_SUCCESS)
	    goto BAIL;

 BAIL:
    free(buf);
    return ret;
}
#endif

error_code
start_track (TRACK_INFO* ti)
{
    int ret;
    ID3V2head id3v2header;
    ID3V2frame id3v2frame1;
    ID3V2frame id3v2frame2;
    char comment[1024] = "Ripped with Streamripper";
    char bigbuf[1600] = "";
    int	sent = 0;
    char buf[3] = "";
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
	memset(&id3v2header, '\000', sizeof(id3v2header));
	memset(&id3v2frame1, '\000', sizeof(id3v2frame1));
	memset(&id3v2frame2, '\000', sizeof(id3v2frame2));

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
    int bits_per_block = 8 * m_meta_interval;
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
#if defined (commentout)
    int secs = ((bytes / m_bitrate) * 8) / 1000;
#endif
    int secs = (bytes / m_bitrate) / 125;
    return secs;
}

static void
compute_cbuffer_size (SPLITPOINT_OPTIONS *sp_opt, int bitrate, 
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
    m_mi_to_cbuffer_start = sp_opt->xs_offset - sp_opt->xs_search_window_1
	+ sp_opt->xs_silence_length/2 - sp_opt->xs_padding_1;
    debug_printf ("m_mi_to_cbuffer_start (ms): %d\n", m_mi_to_cbuffer_start);

    /* compute interval from mi to end of buffer */
    m_mi_to_cbuffer_end = sp_opt->xs_offset + sp_opt->xs_search_window_2
	- sp_opt->xs_silence_length/2 + sp_opt->xs_padding_2;
    debug_printf ("m_mi_to_cbuffer_end (ms): %d\n", m_mi_to_cbuffer_end);
    debug_printf ("bitrate = %d, meta_inf = %d\n", bitrate, meta_interval);

    debug_printf ("CBUFFER (ms): %d:%d\n",m_mi_to_cbuffer_start,
	m_mi_to_cbuffer_end);

    /* convert from ms to meta-inf blocks */
    m_mi_to_cbuffer_start = ms_to_blocks (m_mi_to_cbuffer_start, bitrate, 0);
    m_mi_to_cbuffer_end = ms_to_blocks (m_mi_to_cbuffer_end, bitrate, 1);

    /* Case 1 -- see readme (GCS) */
    if (m_mi_to_cbuffer_start > 0) {
	debug_printf ("CASE 1\n");
        m_cbuffer_size = m_mi_to_cbuffer_end;
	m_sw_end_to_cbuffer_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_end_to_cbuffer_end < 0) {
	    m_sw_end_to_cbuffer_end = 0;
	}
	m_sw_start_to_cbuffer_end = m_sw_end_to_cbuffer_end + sws;
    }

    /* Case 2 */
    else if (m_mi_to_cbuffer_end >= 0) {
	debug_printf ("CASE 2\n");
        m_cbuffer_size = m_mi_to_cbuffer_end - m_mi_to_cbuffer_start;
	m_sw_end_to_cbuffer_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_end_to_cbuffer_end < 0) {
	    m_sw_end_to_cbuffer_end = 0;
	}
	m_sw_start_to_cbuffer_end = m_sw_end_to_cbuffer_end + sws;
    }

    /* Case 3 */
    else {
	debug_printf ("CASE 3\n");
        m_cbuffer_size = - m_mi_to_cbuffer_start;
        m_sw_start_to_cbuffer_end = m_cbuffer_size 
	    + sp_opt->xs_silence_length/2 
	    - sp_opt->xs_padding_1;
	/* GCS: Mar 27, 2004 - it's possible for padding to be less 
	   than search window */
	if (m_sw_start_to_cbuffer_end > m_cbuffer_size) {
	    m_sw_start_to_cbuffer_end = m_cbuffer_size;
	}
	m_sw_end_to_cbuffer_end = m_sw_start_to_cbuffer_end + sws;
    }

    /* Convert these from ms to bytes */
    debug_printf ("SEARCH WIN (ms): %d | %d | %d\n",
		  m_sw_start_to_cbuffer_end,
		  sws, m_sw_end_to_cbuffer_end);
    m_sw_start_to_cbuffer_end = ms_to_bytes (m_sw_start_to_cbuffer_end, 
	bitrate);
    m_sw_end_to_cbuffer_end = ms_to_bytes (m_sw_end_to_cbuffer_end, bitrate);

    /* Get minimum search window (in bytes) */
    m_min_search_win = sp_opt->xs_silence_length + (sws - sp_opt->xs_silence_length) / 2 ;
    m_min_search_win = ms_to_bytes (m_min_search_win, bitrate);

    debug_printf ("CBUFFER (BLOCKS): %d:%d -> %d\n",m_mi_to_cbuffer_start,
	m_mi_to_cbuffer_end, m_cbuffer_size);
    debug_printf ("SEARCH WIN (bytes): %d | %d | %d\n",
		  m_sw_start_to_cbuffer_end,
		  ms_to_bytes(sws,bitrate),
		  m_sw_end_to_cbuffer_end);
}

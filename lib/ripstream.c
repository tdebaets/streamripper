/* ripstream.c - jonclegg@yahoo.com
 * buffer stream data, when a track changes decodes the audio and finds a silent point to
 * splite the track
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
#include "ripstream.h"
#include "debug.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
/* -- See ripstream.h -- */

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code	find_sep (u_long *pos1, u_long *pos2);
static error_code end_track(u_long pos1, u_long pos2, char *trackname);
static error_code start_track(char *trackname);

static void compute_cbuffer_size (SPLITPOINT_OPTIONS *sp_opt,
				  int bitrate, int meta_interval);
static int ms_to_bytes (int ms, int bitrate);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static CBUFFER			m_cbuffer;
static IO_GET_STREAM	*m_in;
static IO_PUT_STREAM	*m_out;
static char			m_last_track[MAX_TRACK_LEN] = {'\0'};
static char			m_current_track[MAX_TRACK_LEN] = {'\0'};
static char			m_no_meta_name[MAX_TRACK_LEN] = {'\0'};
static char			*m_getbuffer = NULL;
static int			m_find_silence = -1;
static BOOL			m_on_first_track = TRUE;
static BOOL			m_addID3tag = TRUE;
static char                     m_drop_string[MAX_DROPSTRING_LEN]={'\0'};
static SPLITPOINT_OPTIONS	*m_sp_opt;
static int			m_bitrate;
static int			m_meta_interval;

static int			m_cbuffer_size;
static int			m_mi_to_cbuffer_start;
static int			m_mi_to_cbuffer_end;
static int			m_sw_start_to_cbuffer_end;
static int			m_sw_end_to_cbuffer_end;
static int			m_min_search_win;

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
ripstream_init (IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, 
		char *drop_string, SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, BOOL addID3tag)
{
    if (!in || !out || !sp_opt || !no_meta_name) {
	printf ("Error: invalid ripstream parameters\n");
	return SR_ERROR_INVALID_PARAM;
    }

    m_in = in;
    m_out = out;
    m_sp_opt = sp_opt;
    m_on_first_track = TRUE;
    m_addID3tag = addID3tag;
    strcpy(m_no_meta_name, no_meta_name);
    strcpy(m_drop_string, drop_string);
    m_bitrate = bitrate;
    m_meta_interval = in->getsize;

    if ((m_getbuffer = malloc(in->getsize)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    // Compute some constants needed to do track splitting
    // See readme files for details
    compute_cbuffer_size (sp_opt, m_bitrate, in->getsize);
	
    return cbuffer_init(&m_cbuffer, in->getsize * m_cbuffer_size);
}

void ripstream_destroy()
{
	if (m_getbuffer) {free(m_getbuffer); m_getbuffer = NULL;}
    m_find_silence = -1;
	m_in = NULL;
	m_out = NULL;
    m_cbuffer_size = 0;
	cbuffer_destroy(&m_cbuffer);
	m_last_track[0] = '\0';
	m_current_track[0] = '\0';
	m_no_meta_name[0] = '\0';
	m_on_first_track = TRUE;
	m_addID3tag = TRUE;
}

BOOL is_buffer_full()
{
	return (cbuffer_get_free(&m_cbuffer) - m_in->getsize) < m_in->getsize;
}

BOOL is_track_changed()
{
#if defined (commentout)
    printf ("   <>%s\n   >>%s\n",m_last_track, m_current_track);
#endif

    if (strstr(m_current_track,m_drop_string) && *m_drop_string) {
	strcpy(m_current_track,m_last_track);
	return 0;
    }

    return strcmp(m_last_track, m_current_track) != 0 && *m_last_track;
}

BOOL is_no_meta_track()
{
	return strcmp(m_current_track, NO_TRACK_STR) == 0;
}

error_code ripstream_rip()
{
	int ret;
	int real_ret = SR_SUCCESS;
	u_long extract_size;

	// only copy over the last track name if 
	// we are not waiting to buffer for a silent point
	if (m_find_silence < 0)
		strcpy(m_last_track, m_current_track);

    // get the data
	if ((ret = m_in->get_data(m_getbuffer, m_current_track)) != SR_SUCCESS)
	{
	DEBUG0(("m_in->get_data bad return code(?) %d", ret));
	// GCS - presumably we still want to return the original error 
		// If it is a single track recording, finish the track
	// if end_track is successful
	if (is_no_meta_track()) {
	    int ret2;
	    ret2 = end_track(cbuffer_get_used(&m_cbuffer),
			     cbuffer_get_used(&m_cbuffer),
			     m_no_meta_name);
	    if (ret2 != SR_SUCCESS) {
		return ret2;
	    }
	}
		return ret;
	}
	
    /* GCS - The next two tests will both happen at most once each, right? */
	// if the current track matchs with the special no track info 
    // name we declair that we are no longer on the first track 
    // (or the last for that matter)
	// this is so end_track will actually call end.
    if (is_no_meta_track() && m_on_first_track)
	{
	if ((ret = start_track(m_no_meta_name)) != SR_SUCCESS) {
	    DEBUG0(("start_track had bad return code %d", ret));
			return ret;
	} 
    }
	// if this is the first time we have received a track name, then we
	// can start the track
	else if	(*m_current_track && *m_last_track == '\0')
	{
		// Done say set first track on, because the first track 
		// should not be ended. It will always be incomplete.
	if ((ret = m_out->start_track(m_current_track)) != SR_SUCCESS) {
	    DEBUG0(("start_track had bad return code %d", ret));
			return ret;
	}
    }

    /* Copy the data into cbuffer */
    ret = cbuffer_insert(&m_cbuffer, m_getbuffer, m_in->getsize);
    if (ret != SR_SUCCESS) {
	DEBUG0(("start_track had bad return code %d", ret));
		return ret;
	}													

    if (is_track_changed()) {
	/* Set m_find_silence equal to the number of additional blocks 
	   needed until we can do silence separation. */
	if (m_find_silence < 0) {
	    if (m_mi_to_cbuffer_end > 0) {
		m_find_silence = m_mi_to_cbuffer_end;
	    } else {
		m_find_silence = 0;
	    }
	}
    }

    if (m_find_silence == 0) {
	/* Find separation point */
	u_long pos1, pos2;
	ret = find_sep (&pos1, &pos2);
	if (ret != SR_SUCCESS) {
	    DEBUG0(("find_sep had bad return code %d", ret));
	    return ret;
	}

	/* Write out previous track */
	if ((ret = end_track(pos1, pos2, m_last_track)) != SR_SUCCESS)
			real_ret = ret;
		if ((ret = start_track(m_current_track)) != SR_SUCCESS)
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
	    DEBUG0(("cbuffer_extract had bad return code %d", ret));
		return ret;
	}
        /* Post to caller */
	if (*m_current_track)
		m_out->put_data(m_getbuffer, extract_size);
    }

	return real_ret;
}

static u_long
clip_to_cbuffer (int pos)
{
    if (pos < 0) {
	return 0;
    } else if (pos > cbuffer_get_used(&m_cbuffer)) {
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
    int sw_start, sw_end, sw_sil, bufsize;
    u_long psilence;
    int ret;

    debug_printf ("*** Finding separation point\n");

    /* First, find the search region w/in cbuffer. */
    sw_start = cbuffer_get_used(&m_cbuffer) - m_sw_start_to_cbuffer_end;
    if (sw_start < 0) sw_start = 0;
    sw_end = cbuffer_get_used(&m_cbuffer) - m_sw_end_to_cbuffer_end;
    if (sw_end < 0) sw_end = 0;

    debug_printf ("search window (bytes): %d,%d\n", sw_start, sw_end);

    if (sw_end - sw_start < m_min_search_win) {
        /* If the search region is too small, take the middle. */
	sw_sil = (sw_end + sw_start) / 2;
    } else {
	/* Otherwise, copy from search window into private buffer */
	int bufsize = sw_end - sw_start;
	char* buf = (u_char *)malloc(bufsize);
	ret = cbuffer_peek_rgn (&m_cbuffer, buf, sw_start, bufsize);
	if (ret != SR_SUCCESS) {
	    free(buf);
	    return ret;
	}

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

error_code end_track(u_long pos1, u_long pos2, char *trackname)
{
    // pos1 is end of prev track
    // pos2 is beginning of next track
	int ret;
	ID3Tag	id3;
	char	*p1;

    // I think pos can be zero if the silence is right at the beginning
    // i.e. it is a bug in s.r.
    u_char *buf = (u_char *)malloc(pos1);

    if (m_addID3tag) 
	{
            char    artist[1024] = "";
            char    title[1024] = "";

            memset(&id3, '\000',sizeof(id3));
            strncpy(id3.tag, "TAG", strlen("TAG"));
	//            strncpy(id3.comment, "Streamripper!", strlen("Streamripper!"));

            memset(&artist, '\000',sizeof(artist));
            memset(&title, '\000',sizeof(title));
			p1 = strchr(trackname, '-');
            if (p1) 
			{
                    strncpy(artist, trackname, p1-trackname);
                    p1++;
				if (*p1 == ' ') 
				{
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
    if ((ret = m_out->put_data(buf, pos1)) != SR_SUCCESS)
		goto BAIL;

        if (m_addID3tag) {
		if ((ret = m_out->put_data((char *)&id3, sizeof(id3))) != SR_SUCCESS)
			goto BAIL;
	}
	if (!m_on_first_track)
		if ((ret = m_out->end_track(trackname)) != SR_SUCCESS)
			goto BAIL;

 BAIL:
	free(buf);
	return ret;
}


error_code start_track(char *trackname)
{
	int ret;
	ID3V2head	id3v2header;
	ID3V2frame	id3v2frame1;
	ID3V2frame	id3v2frame2;
	char    comment[1024] = "Ripped with Streamripper";
	char    artist[1024] = "";
	char    title[1024] = "";
	char    album[1024] = "";
	char    bigbuf[1600] = "";
	char	*p1,*p2;
	int	sent = 0;
	char	buf[3] = "";
	unsigned long int framesize = 0;

	if ((ret = m_out->start_track(trackname)) != SR_SUCCESS)
		return ret;

	/*
	 * Oddsock's ID3 stuff, (oddsock@oddsock.org)
	 */
    if (m_addID3tag) 
	{
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
		if ((ret = m_out->put_data((char *)&(id3v2header.tag), 3)) != SR_SUCCESS)
			return ret;
		if ((ret = m_out->put_data((char *)&buf, 2)) != SR_SUCCESS)
			return ret;
		if ((ret = m_out->put_data((char *)&(id3v2header.flags), 1)) != SR_SUCCESS)
			return ret;
		if ((ret = m_out->put_data((char *)&(framesize), sizeof(framesize))) != SR_SUCCESS)
			return ret;
		sent += sizeof(id3v2header);

		memset(&album, '\000',sizeof(album));
		memset(&artist, '\000',sizeof(artist));
		memset(&title, '\000',sizeof(title));

		/* 
		 * Look for a '-' in the track name
		 * i.e. Moby - sux0rs (artist/track)
		 */
		p1 = strchr(trackname, '-');
		if (p1) 
		{
			strncpy(artist, trackname, p1-trackname);
			p1++;
			p2 = strchr(p1, '-');
			if (p2) 
			{
				if (*p1 == ' ') 
				{
					p1++;
				}
				strncpy(album, p1, p2-p1);
				p2++;
				if (*p2 == ' ') 
				{
					p2++;
				}
				strcpy(title, p2);
			}
			else 
			{
				if (*p1 == ' ') 
				{
					p1++;
				}
				strcpy(title, p1);
			}

			strncpy(id3v2frame1.id, "TPE1", 4);
			framesize = htonl(strlen(artist)+1);

			// Write ID3V2 frame1 with data
			if ((ret = m_out->put_data((char *)&(id3v2frame1.id), 4)) != SR_SUCCESS)
				return ret;
			sent += 4;
			if ((ret = m_out->put_data((char *)&(framesize), sizeof(framesize))) != SR_SUCCESS)
				return ret;
			sent += sizeof(framesize);
			if ((ret = m_out->put_data((char *)&(id3v2frame1.pad), 3)) != SR_SUCCESS)
				return ret;
			sent += 3;
			if ((ret = m_out->put_data(artist, strlen(artist))) != SR_SUCCESS)
				return ret;
			sent += strlen(artist);
		
			strncpy(id3v2frame2.id, "TIT2", 4);
			framesize = htonl(strlen(title)+1);
			// Write ID3V2 frame2 with data
			if ((ret = m_out->put_data((char *)&(id3v2frame2.id), 4)) != SR_SUCCESS)
				return ret;
			sent += 4;
			if ((ret = m_out->put_data((char *)&(framesize), sizeof(framesize))) != SR_SUCCESS)
				return ret;
			sent += sizeof(framesize);
			if ((ret = m_out->put_data((char *)&(id3v2frame2.pad), 3)) != SR_SUCCESS)
				return ret;
			sent += 3;
			if ((ret = m_out->put_data(title, strlen(title))) != SR_SUCCESS)
				return ret;
			sent += strlen(title);

			
			strncpy(id3v2frame2.id, "TENC", 4);
			framesize = htonl(strlen(comment)+1);
			// Write ID3V2 frame2 with data
			if ((ret = m_out->put_data((char *)&(id3v2frame2.id), 4)) != SR_SUCCESS)
				return ret;
			sent += 4;
			if ((ret = m_out->put_data((char *)&(framesize), sizeof(framesize))) != SR_SUCCESS)
				return ret;
			sent += sizeof(framesize);
			if ((ret = m_out->put_data((char *)&(id3v2frame2.pad), 3)) != SR_SUCCESS)
				return ret;
			sent += 3;
			sent += sizeof(id3v2frame2);
			if ((ret = m_out->put_data(comment, strlen(comment))) != SR_SUCCESS)
				return ret;
			sent += strlen(comment);
		
			memset(&id3v2frame2, '\000', sizeof(id3v2frame2));
			strncpy(id3v2frame2.id, "TALB", 4);
			framesize = htonl(strlen(album)+1);
			// Write ID3V2 frame2 with data
			if ((ret = m_out->put_data((char *)&(id3v2frame2.id), 4)) != SR_SUCCESS)
				return ret;
			sent += 4;
			if ((ret = m_out->put_data((char *)&(framesize), sizeof(framesize))) != SR_SUCCESS)
				return ret;
			sent += sizeof(framesize);
			if ((ret = m_out->put_data((char *)&(id3v2frame2.pad), 3)) != SR_SUCCESS)
				return ret;
			sent += 3;
			if ((ret = m_out->put_data(album, strlen(album))) != SR_SUCCESS)
				return ret;
			sent += strlen(album);

			if ((ret = m_out->put_data(bigbuf, 1600-sent)) != SR_SUCCESS)
				return ret;
			
		}
	}

	
	m_on_first_track = FALSE;

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
        m_cbuffer_size = m_mi_to_cbuffer_end;
	m_sw_end_to_cbuffer_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	m_sw_start_to_cbuffer_end = m_sw_end_to_cbuffer_end + sws;
    }

    /* Case 2 */
    else if (m_mi_to_cbuffer_end >= 0) {
        m_cbuffer_size = m_mi_to_cbuffer_end - m_mi_to_cbuffer_start;
	m_sw_end_to_cbuffer_end = sp_opt->xs_padding_2 
	    - sp_opt->xs_silence_length/2;
	m_sw_start_to_cbuffer_end = m_sw_end_to_cbuffer_end + sws;
    }

    /* Case 3 */
    else {
        m_cbuffer_size = - m_mi_to_cbuffer_start;
        m_sw_start_to_cbuffer_end = m_cbuffer_size 
	    + sp_opt->xs_silence_length/2 
	    - sp_opt->xs_padding_1;
	m_sw_end_to_cbuffer_end = m_sw_start_to_cbuffer_end + sws;
    }

    /* Convert these from ms to bytes */
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

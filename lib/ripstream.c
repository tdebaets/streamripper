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

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	ripstream_init(u_long buffersize_mult, IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, BOOL addID3Tag);
error_code	ripstream_rip();
void		ripstream_destroy();

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code	find_sep(u_long *pos);
static error_code end_track(u_long pos, char *trackname);
static error_code start_track(char *trackname);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static CBUFFER			m_cbuffer;
static IO_GET_STREAM	*m_in;
static IO_PUT_STREAM	*m_out;
static char				m_last_track[MAX_TRACK_LEN] = {'\0'};
static char				m_current_track[MAX_TRACK_LEN] = {'\0'};
static char				m_no_meta_name[MAX_TRACK_LEN] = {'\0'};
static int				m_buffersize_mult;
static char				*m_getbuffer = NULL;
static int				m_find_silence = -1;
static BOOL				m_on_first_track = TRUE;
static BOOL				m_addID3tag = TRUE;

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



error_code ripstream_init(u_long buffersize_mult, IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, BOOL addID3tag)
{

	if (!in || !out || buffersize_mult < 1 || !no_meta_name)
		return SR_ERROR_INVALID_PARAM;

	m_in = in;
	m_out = out;
	m_buffersize_mult = buffersize_mult;
	m_on_first_track = TRUE;
	m_addID3tag = addID3tag;
	strcpy(m_no_meta_name, no_meta_name);

	if ((m_getbuffer = malloc(in->getsize)) == NULL)
		return SR_ERROR_CANT_ALLOC_MEMORY;

	return cbuffer_init(&m_cbuffer, in->getsize * buffersize_mult);
	
}

void ripstream_destroy()
{
	if (m_getbuffer) {free(m_getbuffer); m_getbuffer = NULL;}
	m_find_silence = 0;
	m_in = NULL;
	m_out = NULL;
	m_buffersize_mult = 0;
	cbuffer_destroy(&m_cbuffer);
	m_last_track[0] = '\0';
	m_current_track[0] = '\0';
}

BOOL is_buffer_full()
{
	return (cbuffer_get_free(&m_cbuffer) - m_in->getsize) < m_in->getsize;
}

BOOL is_track_changed()
{
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
	if ((ret = m_in->get_data(m_getbuffer, m_current_track)) != SR_SUCCESS)
	{
		// If it is a single track recording, finish the track
		if (is_no_meta_track())
			if ((ret = end_track(cbuffer_get_used(&m_cbuffer), m_no_meta_name)) != SR_SUCCESS)
				return ret;
		return ret;
	}
	
	// if the current track matchs with the special no track info 
	// name we declair that we are no longer on the first track (or the last for that matter)
	// this is so end_track will actually call end.
	if (is_no_meta_track() &&
		m_on_first_track)
	{
		if ((ret = start_track(m_no_meta_name)) != SR_SUCCESS)
			return ret;
	} 
	// if this is the first time we have received a track name, then we
	// can start the track
	else if	(*m_current_track && *m_last_track == '\0')
	{
		// Done say set first track on, because the first track 
		// should not be ended. It will always be incomplete.
		if ((ret = m_out->start_track(m_current_track)) != SR_SUCCESS)
			return ret;
	}

	if ((ret = cbuffer_insert(&m_cbuffer, m_getbuffer, m_in->getsize)) != SR_SUCCESS)
		return ret;

	// The track changed, signal a findsep in a little while
	if (is_track_changed() &&
		is_buffer_full() &&
		m_find_silence < 0)
	{
		m_find_silence = (m_buffersize_mult/2);
	}													

	m_find_silence--;


	// If the buffer is not full, buffer more
	if (!is_buffer_full())
		return SR_SUCCESS_BUFFERING;

	// This means we had a track change a little while back and it's time
	// to find a silent point to seperate the track
	if (m_find_silence == 0)
	{
		u_long pos = 0;

		if ((ret = find_sep(&pos)) != SR_SUCCESS)
			real_ret = ret;

		if ((ret = end_track(pos, m_last_track)) != SR_SUCCESS)
			real_ret = ret;

		if ((ret = start_track(m_current_track)) != SR_SUCCESS)
			real_ret = ret;

		m_find_silence = -1;
	}		

	// Remove our buffer size from the buffer
	extract_size = m_in->getsize < cbuffer_get_used(&m_cbuffer) ? m_in->getsize : cbuffer_get_used(&m_cbuffer);
	if ((ret = cbuffer_extract(&m_cbuffer, m_getbuffer, extract_size)) != SR_SUCCESS)
		return ret;
	
	// And post that data to our caller
	if (*m_current_track)
		m_out->put_data(m_getbuffer, extract_size);

	return real_ret;

}


error_code find_sep(u_long *pos)
{
	int cbufsize = cbuffer_get_used(&m_cbuffer);
	u_char *buf = (u_char *)malloc(cbufsize);
	int ret;

	if (!pos)
		return SR_ERROR_INVALID_PARAM;

	if ((ret = cbuffer_peek(&m_cbuffer, buf, cbufsize)) != SR_SUCCESS)
	{
		free(buf);
		return ret;
	}

#if 0
	{
	char str[50];
	FILE *fp;
	static int tracknum;
	sprintf(str, "debug_%03d.mp3", ++tracknum);
	fp = fopen(str, "wb");
	fwrite(buf, 1, cbufsize, fp);
	fclose(fp);
	}
#endif

	*pos = 0;
	ret = findsep_silence(buf, cbufsize, pos);
	if (ret != SR_SUCCESS && ret != SR_ERROR_CANT_DECODE_MP3)
		return ret;
	
	if (*pos > cbuffer_get_used(&m_cbuffer))
	{
		debug_printf("pos bigger then buffer!!!");
		free(buf);
		return SR_ERROR_CANT_FIND_TRACK_SEPERATION;
	}

	if (*pos == 0)
	{
		//
		// I used to return can't find track sep, but people
		// don't want streamripper to stop, or restart or anything
		// when this happens.. though in theroy it prob should report it
		// as a non fatal error
		// anyway, we know that when this happens it means mpeglib couldn't
		// even find the first mpeg header, which is bad.. so lets just assume
		// the seperation is right in the middle of our buffer
		//
		*pos = cbufsize/2;
	}
	free(buf);

	return SR_SUCCESS;
}

error_code end_track(u_long pos, char *trackname)
{
	int ret;
	ID3Tag	id3;
	char	*p1;

	u_char *buf = (u_char *)malloc(pos);

    if (m_addID3tag) 
	{
            char    artist[1024] = "";
            char    title[1024] = "";

            memset(&id3, '\000',sizeof(id3));
            strncpy(id3.tag, "TAG", strlen("TAG"));
            strncpy(id3.comment, "Streamripper!", strlen("Streamripper!"));

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

	// get out the part of the last track
	if ((ret = cbuffer_extract(&m_cbuffer, buf, pos)) != SR_SUCCESS)
		goto BAIL;

	// Write that out to the current file
	if ((ret = m_out->put_data(buf, pos)) != SR_SUCCESS)
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


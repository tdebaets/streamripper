/* ripshout.c - jonclegg@yahoo.com
 * ripper for shoutcast streams, finds meta data, reports track changes
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

#include "util.h"
#include "ripshout.h"
#include "srtypes.h"
#include "debug.h"

#define DEFAULT_BUFFER_SIZE	1024


/*********************************************************************************
 * Private Functions
 *********************************************************************************/
static error_code	get_trackname(int size, char *newtrack);
static error_code ripshout_get_stream_data(char *data_buf, char *track_buf);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static int		m_meta_interval;
static int		m_buffersize;
static IO_DATA_INPUT	*m_in;
static int		m_chunkcount;	// for when we need to know what the first
					// empty metadata came for streams that
					// stream empty metadatas

error_code
ripshout_init(IO_DATA_INPUT *in, IO_GET_STREAM *getstream, int meta_interval)
{
    if (!in || meta_interval < NO_META_INTERVAL)
	return SR_ERROR_INVALID_PARAM;

    m_meta_interval = meta_interval;
    m_in = in;

    m_buffersize = (m_meta_interval == NO_META_INTERVAL) ? DEFAULT_BUFFER_SIZE : m_meta_interval;

    getstream->get_stream_data = ripshout_get_stream_data;
    getstream->getsize = m_buffersize;

    m_chunkcount = 0;
    return SR_SUCCESS;
}


void
ripshout_destroy()
{
    m_buffersize = 0;
    m_meta_interval = 0;
    m_chunkcount = 0;
    m_in = NULL;
}

error_code
ripshout_get_stream_data(char *data_buf, char *track_buf)
{
    int ret = 0;
    char c;
    char newtrack[MAX_TRACK_LEN];

    *track_buf = 0;
    m_chunkcount++;
    if ((ret = m_in->get_input_data(data_buf, m_buffersize)) <= 0)
	return ret;

    if (m_meta_interval == NO_META_INTERVAL) {
	return SR_SUCCESS;
    }

    if ((ret = m_in->get_input_data(&c, 1)) <= 0)
	return ret;

    debug_printf ("METADATA LEN: %d\n",(int)c);
    if (c < 0) {
	debug_printf ("Got invalid metadata: %d\n",c);
	return SR_ERROR_INVALID_METADATA;
    } else if (c == 0) {
	// around christmas time 2001 someone noticed that 1.8.7 shoutcast
	// none-meta servers now just send a '0' if they have no meta data
	// anyway, bassicly if the first meta capture is null then we assume
	// that the stream does not have metadata
	return SR_SUCCESS;
    } else {
	if ((ret = get_trackname(c * 16, newtrack)) != SR_SUCCESS) {
	    debug_printf("get_trackname had a bad return %d", ret);
	    return ret;
	}

	// WolfFM is a station that does not stream meta-data, but is in that format anyway...
	// StreamTitle='', so we need to pretend this has no meta data.
	if (*newtrack == '\0') {
	    return SR_SUCCESS;
	}
	/* This is the case where we got metadata. */
	strncpy(track_buf, newtrack, MAX_TRACK_LEN);
    }
    return SR_SUCCESS;
}

error_code
get_trackname(int size, char *newtrack)
{
    int i;
    int ret;
    char *namebuf;

    if ((namebuf = malloc(size)) == NULL)
	return SR_ERROR_CANT_ALLOC_MEMORY;

    if ((ret = m_in->get_input_data(namebuf, size)) <= 0) {
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
	debug_printf ("%2c ",namebuf[i]);
	if (i % 20 == 19) {
	    debug_printf ("\n");
	}
    }
    debug_printf ("\n");

    if(strstr(namebuf, "StreamTitle='") == NULL) {
	free(namebuf);
	return SR_ERROR_NO_TRACK_INFO;
    }
    subnstr_until(namebuf+strlen("StreamTitle='"), "';", newtrack, MAX_TRACK_LEN);
    trim(newtrack);

    free(namebuf);
    return SR_SUCCESS;
}

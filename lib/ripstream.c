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
#include "callback.h"

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static int
ripstream_recvall (RIP_MANAGER_INFO* rmi, char* buffer, int size);
static error_code get_track_from_metadata (RIP_MANAGER_INFO* rmi, 
					   int size, char *newtrack);

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
    debug_printf ("ripstream_recvall (%p, %d)\n", 
		  data_buf, 
		  rmi->getbuffer_size);
    ret = ripstream_recvall (rmi, data_buf, rmi->getbuffer_size);
    debug_printf ("ripstream_recvall (ret = %d)\n", ret);
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
ripstream_put_data (RIP_MANAGER_INFO *rmi, char *buf, int size)
{
#if defined (commentout)
    int ret;

    /* GCS FIX: kkk */
    if (GET_INDIVIDUAL_TRACKS (rmi->prefs->flags)) {
	if (rmi->write_data) {
	    ret = filelib_write_track (rmi, buf, size);
	    if (ret != SR_SUCCESS) {
		debug_printf ("filelib_write_track returned: %d\n",ret);
		return ret;
	    }
	}
    }
#endif

    rmi->filesize += size;	/* This is used by the GUI */
    rmi->bytes_ripped += size;	/* This is used to determine when to quit */
    while (rmi->bytes_ripped >= 1048576) {
	rmi->bytes_ripped -= 1048576;
	rmi->megabytes_ripped++;
    }

    return SR_SUCCESS;
}

error_code
ripstream_start_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    error_code rc;

    if (ti->save_track && GET_INDIVIDUAL_TRACKS(rmi->prefs->flags)) {
	rmi->write_data = 1;
    } else {
	rmi->write_data = 0;
    }

    /* Update data for callback */
    debug_printf ("calling rip_manager_start_track(#2)\n");
    rc = callback_start_track (rmi, ti);
    if (rc != SR_SUCCESS) {
	return rc;
    }

    return SR_SUCCESS;
}

error_code
ripstream_end_track (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    mchar mfullpath[SR_MAX_PATH];
    char fullpath[SR_MAX_PATH];

#if defined (commentout)
    /* GCS FIX: kkk */
    if (rmi->write_data) {
        filelib_end (rmi, ti, rmi->prefs->overwrite,
		     GET_TRUNCATE_DUPS(rmi->prefs->flags),
		     mfullpath);
    }
#endif

    callback_post_status(rmi, 0);

    string_from_gstring (rmi, fullpath, SR_MAX_PATH, mfullpath, 
			 CODESET_FILESYS);
    rmi->status_callback (rmi, RM_TRACK_DONE, (void*)fullpath);

    return SR_SUCCESS;
}


/******************************************************************************
 * Private functions
 *****************************************************************************/
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

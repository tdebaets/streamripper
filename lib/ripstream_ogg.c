/* ripstream_ogg.c
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
static error_code end_track_ogg (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
static error_code
ripstream_ogg_handle_bos (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti);
static error_code
ripstream_ogg_write_page (RIP_MANAGER_INFO *rmi, Ogg_page_reference *opr);

/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
ripstream_ogg_rip (RIP_MANAGER_INFO* rmi)
{
    int ret;
    int real_ret = SR_SUCCESS;
    GList *node;
    Cbuf3 *cbuf3 = &rmi->cbuf3;

    debug_printf ("RIPSTREAM_RIP_OGG: top of loop\n");

    if (rmi->ripstream_first_time_through) {
	/* Allocate circular buffer */
	rmi->detected_bitrate = -1;
	rmi->bitrate = -1;
	rmi->getbuffer_size = 1024;
	rmi->cbuf2_size = 128;
	ret = cbuf3_init (cbuf3, rmi->http_info.content_type, 
			  GET_MAKE_RELAY(rmi->prefs->flags),
			  rmi->getbuffer_size, rmi->cbuf2_size);
	if (ret != SR_SUCCESS) return ret;
	rmi->ogg_track_state = 0;
	rmi->ogg_fixed_page_no = 0;
	/* Warm up the ogg decoding routines */
	ripogg_init (rmi);
	/* Done! */
	rmi->ripstream_first_time_through = 0;
    }

    /* get the data from the stream */
    node = cbuf3_request_free_node (rmi, cbuf3);
    ret = ripstream_get_data (rmi, node->data, rmi->current_track.raw_metadata);
    if (ret != SR_SUCCESS) {
	debug_printf ("get_stream_data bad return code: %d\n", ret);
	return ret;
    }

    /* Copy the data into cbuffer */
    ret = cbuf3_insert_node (cbuf3, node);
    if (ret != SR_SUCCESS) {
	debug_printf ("cbuf3_insert had bad return code %d\n", ret);
	return ret;
    }

    /* Fill in this_page_list with ogg page references */
    track_info_clear (&rmi->current_track);
    ripogg_process_chunk (rmi, node->data, cbuf3->chunk_size, 
			  &rmi->current_track);

    debug_printf ("ogg_track_state[a] = %d\n", rmi->ogg_track_state);

    /* If we have unwritten pages, write them */
    do {
	GList *page_node;
	Ogg_page_reference *opr;

	/* Get an unwritten page */
	cbuf3_ogg_peek_page (cbuf3, &page_node);
	if (!page_node) {
	    break;
	}

	/* We got an unwritten page */
	opr = (Ogg_page_reference *) page_node->data;

	/* If new track, then (maybe) open a new file */
	debug_printf ("Testing unwritten page [%p, %p]\n", page_node, opr);
	if (opr->m_page_flags & OGG_PAGE_BOS) {
	    /* If the BOS page is the last page, it means 
	       we haven't yet parsed the PAGE2. */
	    if (!page_node->next) {
		break;
	    }
	    ret = ripstream_ogg_handle_bos (rmi, &rmi->current_track);
	    if (ret != SR_SUCCESS) {
		return ret;
	    }
	}

	/* Write page to disk */
	debug_printf ("Trying to write ogg page to disk\n");
	ripstream_ogg_write_page (rmi, opr);

	if (opr->m_page_flags & OGG_PAGE_EOS) {
	    rmi->ogg_track_state = 2;
	}

	/* Advance page */
	debug_printf ("Advancing ogg written page in cbuf3\n");
	cbuf3_ogg_advance_page (cbuf3);

    } while (1);

#if defined (commentout)
    ret = rip_manager_put_data (rmi, rmi->getbuffer, amt_filled);
    if (ret != SR_SUCCESS) {
	debug_printf ("rip_manager_put_data(#1): %d\n",ret);
	return ret;
    }

    debug_printf ("ogg_track_state[b] = %d\n", rmi->ogg_track_state);
    debug_track_info (&rmi->current_track, "current");

    /* If buffer almost full, advance the buffer */
    if (cbuf2_get_free(&rmi->cbuf2) < rmi->getbuffer_size) {
	debug_printf ("cbuf2_get_free < getbuffer_size\n");
	extract_size = rmi->getbuffer_size - cbuf2_get_free(&rmi->cbuf2);

        ret = cbuf2_advance_ogg (rmi, &rmi->cbuf2, rmi->getbuffer_size);

        if (ret != SR_SUCCESS) {
	    debug_printf("cbuf2_advance_ogg had bad return code %d\n", ret);
	    return ret;
	}
    }
#endif

    return real_ret;
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
static error_code
ripstream_ogg_handle_bos (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    if (!rmi->write_data) {
	return SR_SUCCESS;
    }

    debug_printf ("ripstream_ogg_handle_bos: testing track_info\n");
    if (rmi->current_track.have_track_info) {
	if (track_info_different (&rmi->old_track, &rmi->current_track)) {
	    error_code ret;

	    if (rmi->ogg_track_state == 2) {
		debug_printf ("ripstream_ogg_handle_bos: ending track\n");
		end_track_ogg (rmi, &rmi->old_track);
	    }
	    debug_printf ("ripstream_ogg_handle_bos: starting track\n");
	    /* GCS FIX: kkk */
#if defined (commentout)
	    ret = filelib_start (rmi, ti);
#endif
	    if (ret != SR_SUCCESS) {
		return ret;
	    }
	    track_info_copy (&rmi->old_track, &rmi->current_track);
	}
	rmi->ogg_track_state = 1;
    }
    return SR_SUCCESS;
}

static error_code
ripstream_ogg_write_page (RIP_MANAGER_INFO *rmi, Ogg_page_reference *opr)
{
    Cbuf3_pointer cbuf3_ptr;
    error_code ret;
    u_long bytes_remaining;

    cbuf3_ptr.node = opr->m_cbuf3_loc.node;
    cbuf3_ptr.offset = opr->m_cbuf3_loc.offset;
    bytes_remaining = opr->m_page_len;
    while (bytes_remaining > 0) {
	u_long bytes_requested;
	u_long bytes_read;

	if (bytes_remaining > rmi->getbuffer_size) {
	    bytes_requested = rmi->getbuffer_size;
	} else {
	    bytes_requested = bytes_remaining;
	}
	debug_printf ("cbuf3_extract: ptr [%p,%d], req %d\n", cbuf3_ptr.node,
		      cbuf3_ptr.offset, bytes_requested);
	ret = cbuf3_extract (&rmi->cbuf3,
			     &cbuf3_ptr,
			     rmi->getbuffer,
			     bytes_requested,
			     &bytes_read);
	if (ret != SR_SUCCESS) {
	    return ret;
	}
	bytes_remaining -= bytes_read;

	/* Do the actual write -- showfile */
	ret = filelib_write_show (rmi, rmi->getbuffer, bytes_read);
	if (ret != SR_SUCCESS) {
	    debug_printf("filelib_write_show had bad return code: %d\n", ret);
	    return ret;
	}

	/* Do the actual write -- split files */
	if (GET_INDIVIDUAL_TRACKS(rmi->prefs->flags)) {
	    if (rmi->write_data) {
#if defined (commentout)
		/* GCS FIX: kkk */
		ret = filelib_write_track (rmi, rmi->getbuffer, bytes_read);
#endif
		if (ret != SR_SUCCESS) {
		    debug_printf ("filelib_write_track returned: %d\n",ret);
		    return ret;
		}
	    }
	}
    }
    return SR_SUCCESS;
}

// Only save this track if we've skipped over enough cruft 
// at the beginning of the stream
static error_code
end_track_ogg (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    error_code ret;
    debug_printf ("Current track number %d (skipping if less than %d)\n", 
		  rmi->track_count, rmi->prefs->dropcount);
    if (rmi->track_count >= rmi->prefs->dropcount) {
	ret = ripstream_end_track (rmi, ti);
    } else {
	ret = SR_SUCCESS;
    }
    rmi->track_count ++;
    return ret;
}

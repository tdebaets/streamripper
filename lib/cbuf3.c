/* cbuf3.c
 * circular buffer lib - version 3
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
#include <stdlib.h>
#include <string.h>
#include "srtypes.h"
#include "errors.h"
#include "threadlib.h"
#include "cbuf3.h"
#include "debug.h"

static void
cbuf3_advance_relay_list (RIP_MANAGER_INFO *rmi, Cbuf3 *cbuf3);



/******************************************************************************
 * Public functions
 *****************************************************************************/
error_code
cbuf3_init (struct cbuf3 *cbuf3, 
	    int content_type, 
	    int have_relay, 
	    unsigned long chunk_size, 
	    unsigned long num_chunks)
{
    int i;

    debug_printf ("Initializing cbuf3\n");

    if (chunk_size == 0 || num_chunks == 0) {
        return SR_ERROR_INVALID_PARAM;
    }
    cbuf3->have_relay = have_relay;
    cbuf3->content_type = content_type;
    cbuf3->chunk_size = chunk_size;

    cbuf3->buf = g_queue_new ();
    cbuf3->free_list = NULL;
    cbuf3->pending = 0;

    for (i = 0; i < num_chunks; i++) {
	char* chunk = (char*) malloc (chunk_size);
	if (!chunk) {
	    return SR_ERROR_CANT_ALLOC_MEMORY;
	}
	cbuf3->free_list = g_slist_prepend (cbuf3->free_list, chunk);
    }

    cbuf3->ogg_page_refs = g_queue_new ();

    //    cbuf2->next_song = 0;        /* MP3 only */
    //    cbuf2->song_page = 0;        /* OGG only */
    //    cbuf2->song_page_done = 0;   /* OGG only */

    //    INIT_LIST_HEAD (&cbuf2->metadata_list);
    //    INIT_LIST_HEAD (&cbuf2->ogg_page_list);

    cbuf3->sem = threadlib_create_sem();
    threadlib_signal_sem (&cbuf3->sem);

    return SR_SUCCESS;
}

void
cbuf3_destroy (struct cbuf3 *cbuf3)
{
    char *c;
    GSList *n;

    /* Remove buffer */
    if (cbuf3->buf) {
	while ((c = g_queue_pop_head (cbuf3->buf)) != 0) {
	    free (c);
	}
	g_queue_free (cbuf3->buf);
	cbuf3->buf = 0;
    }

    /* Remove free_list */
    if (cbuf3->free_list) {
	for (n = cbuf3->free_list; n; n = n->next) {
	    free (n->data);
	}
	g_slist_free (cbuf3->free_list);
	cbuf3->free_list = 0;
    }

    /* Remove ogg page references */
    if (cbuf3->ogg_page_refs) {
	while ((c = g_queue_pop_head (cbuf3->ogg_page_refs)) != 0) {
	    free (c);
	}
	g_queue_free (cbuf3->ogg_page_refs);
	cbuf3->ogg_page_refs = 0;
    }
}

/* Returns a free chunk */
char*
cbuf3_request_free_chunk (RIP_MANAGER_INFO *rmi,
			  struct cbuf3 *cbuf3)
{
    char *chunk;

    /* If there is a free chunk, return it */
    /* No need to lock, only the main thread accesses free_list */
    if (cbuf3->free_list) {
	chunk = cbuf3->free_list->data;
	cbuf3->free_list = g_slist_delete_link (cbuf3->free_list, 
						cbuf3->free_list);
	return chunk;
    }

    /* Otherwise, we have to eject the oldest chunk from buf.  
       This requires updates to the relay lists. */
    if (cbuf3->have_relay) {
	debug_printf ("cbuf3_request_free_chunk is waiting for rmi->relay_list_sem\n");
	threadlib_waitfor_sem (&rmi->relay_list_sem);
	debug_printf ("cbuf3_request_free_chunk got rmi->relay_list_sem\n");
    }

    debug_printf ("cbuf3_request_free_chunk is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_request_free_chunk got cbuf3->sem\n");

    /* Remove the chunk */
    chunk = g_queue_pop_head (cbuf3->buf);

    /* Advance the chunk numbers in the relay list */
    if (cbuf3->have_relay) {
	cbuf3_advance_relay_list (rmi, cbuf3);
    }

    /* Done */
    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_request_free_chunk released cbuf3->sem\n");
    if (cbuf3->have_relay) {
	threadlib_signal_sem (&rmi->relay_list_sem);
	debug_printf ("cbuf3_request_free_chunk released rmi->relay_list_sem\n");
    }

    return chunk;
}

/* Adds chunk to buf, but relay is not yet allowed to access */
error_code
cbuf3_insert (struct cbuf3 *cbuf3, char *chunk)
{
    GList *p;

    /* Insert data */
    debug_printf ("cbuf3_insert is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_insert got cbuf3->sem\n");

    debug_printf ("CBUF_INSERT\n");
    debug_printf ("  Node       Data\n");

    g_queue_push_tail (cbuf3->buf, chunk);

    for (p = cbuf3->buf->head; p; p = p->next) {
	
	debug_printf ("  %p, %p\n", p, p->data);
    }

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_insert released cbuf3->sem\n");
    return SR_SUCCESS;
}

/* Adds chunk to buf, but relay is not yet allowed to access */
error_code
cbuf3_add (struct cbuf3 *cbuf3, 
	   struct cbuf3_pointer *out_ptr, 
	   struct cbuf3_pointer *in_ptr, 
	   u_long len)
{
    //debug_printf ("add: %p,%d + %d\n", in_ptr->chunk, in_ptr->offset, len);
    out_ptr->chunk = in_ptr->chunk;
    out_ptr->offset = in_ptr->offset;
    while (len > 0) {
	out_ptr->offset += len;
	len = 0;
	/* Check for advancing to next chunk */
	if (out_ptr->offset >= cbuf3->chunk_size) {
	    len = out_ptr->offset - cbuf3->chunk_size;
	    out_ptr->offset = 0;
	    out_ptr->chunk = out_ptr->chunk->next;
	    /* Check for overflow */
	    if (!out_ptr->chunk) {
		//debug_printf ("     (overflow)\n");
		return SR_ERROR_BUFFER_TOO_SMALL;
	    }
	}
    }
    //debug_printf ("     %p,%d\n", out_ptr->chunk, out_ptr->offset);
    return SR_SUCCESS;
}

error_code
cbuf3_set_uint32 (struct cbuf3 *cbuf3, 
		  struct cbuf3_pointer *in_ptr, 
		  uint32_t val)
{
    u_long i;
    struct cbuf3_pointer tmp;
    error_code rc;

    //debug_printf ("set: %p,%d\n", in_ptr->chunk, in_ptr->offset);
    tmp.chunk = in_ptr->chunk;
    tmp.offset = in_ptr->offset;

    for (i = 0; i < 4; i++) {
	rc = cbuf3_add (cbuf3, &tmp, in_ptr, i);
	if (rc != SR_SUCCESS) {
	    return rc;
	}
	char *buf = (char*) tmp.chunk->data;
	buf[tmp.offset] = (val & 0xff);
	val >>= 8;
    }
    
    return SR_SUCCESS;
}

void
cbuf3_debug_ogg_page_ref (struct ogg_page_reference *opr, void *user_data)
{
    debug_printf ("  chk/off/len: [%p,%5d,%5d] hdr: [%p,%5d] flg: %2d\n", 
		  opr->m_cbuf3_loc.chunk,
		  opr->m_cbuf3_loc.offset,
		  opr->m_page_len,
		  opr->m_header_buf_ptr,
		  opr->m_header_buf_len,
		  opr->m_page_flags);
}

void
cbuf3_debug_ogg_page_list (struct cbuf3 *cbuf3)
{
    debug_printf ("Ogg page references:\n");
    g_queue_foreach (cbuf3->ogg_page_refs, 
		     (GFunc) cbuf3_debug_ogg_page_ref, 0);
}

void
cbuf3_splice_page_list (struct cbuf3 *cbuf3, 
			GList **chunk_page_list)
{
    /* Do I need to lock?  If so, read access should lock too? */
    debug_printf ("cbuf3_splice_page_list is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_splice_page_list got cbuf3->sem\n");

    while (*chunk_page_list) {
	GList *ele = (*chunk_page_list);
	(*chunk_page_list) = g_list_remove_link ((*chunk_page_list), ele);
	g_queue_push_tail_link (cbuf3->ogg_page_refs, ele);
    }

    cbuf3_debug_ogg_page_list (cbuf3);
    
    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_splice_page_list released cbuf3->sem\n");
}

/* Sets (*page_node) if a page was found */
void
cbuf3_ogg_peek_page (Cbuf3 *cbuf3, 
		     GList **page_node)
{
    if (!cbuf3->written_page) {
	if (cbuf3->ogg_page_refs->head) {
	    cbuf3->written_page = cbuf3->ogg_page_refs->head;
	} else {
	    (*page_node) = 0;
	    return;
	}
    }
    
    if (cbuf3->written_page->next) {
	(*page_node) = cbuf3->written_page;
    } else {
	(*page_node) = 0;
    }
}

void
cbuf3_ogg_advance_page (Cbuf3 *cbuf3)
{
    cbuf3->written_page = cbuf3->written_page->next;
}

/* This sets the m_cbuf_ptr (and related items) within the 
   relay_client.  If it fails, m_cbuf_ptr.chunk is left unchanged 
   at zero.  */
error_code
cbuf3_initialize_relay_client_ptr (struct cbuf3 *cbuf3,
		       struct relay_client *relay_client,
		       u_long burst_request)
{
    debug_printf ("cbuf3_add_relay_entry is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_add_relay_entry got cbuf3->sem\n");

    /* Find a Cbuf3_pointer for the relay to start sending */
    if (cbuf3->content_type == CONTENT_TYPE_OGG) {
	GList *ogg_page_ptr;
	Ogg_page_reference *opr;
	u_long burst_amt = 0;

	/* For ogg, walk through the ogg pages in reverse order to 
	   find page at location > burst_request. */
	ogg_page_ptr = cbuf3->ogg_page_refs->tail;
	if (!ogg_page_ptr) {
	    debug_printf ("Error.  No data for relay\n");
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_NO_DATA_FOR_RELAY;
	}
	opr = (Ogg_page_reference *) ogg_page_ptr->data;
	burst_amt += opr->m_page_len;
	debug_printf ("OGG_ADD_RELAY_ENTRY: BA=%d\n", burst_amt);

	while (burst_amt < burst_request) {
	    if (!ogg_page_ptr->prev) {
		debug_printf ("Hit list head while spinning back\n");
		break;
	    }
	    ogg_page_ptr = ogg_page_ptr->prev;
	    opr = (Ogg_page_reference *) ogg_page_ptr->data;
	    burst_amt += opr->m_page_len;
	    debug_printf ("OGG_ADD_RELAY_ENTRY: BA=%d\n", burst_amt);
	}

	/* If the desired ogg page is a header page, spin forward 
	   to a non-header page */
	while (opr->m_page_flags & (OGG_PAGE_BOS | OGG_PAGE_2)) {
	    if (!ogg_page_ptr->next) {
		debug_printf ("Error.  No data for relay\n");
		threadlib_signal_sem (&cbuf3->sem);
		return SR_ERROR_NO_DATA_FOR_RELAY;
	    }
	    ogg_page_ptr = ogg_page_ptr->next;
	    opr = (Ogg_page_reference *) ogg_page_ptr->data;
	}
	relay_client->m_cbuf_ptr.chunk = opr->m_cbuf3_loc.chunk;
	relay_client->m_cbuf_ptr.offset = opr->m_cbuf3_loc.offset;
	relay_client->m_header_buf_ptr = opr->m_header_buf_ptr;
	relay_client->m_header_buf_len = opr->m_header_buf_len;
	relay_client->m_header_buf_off = 0;

    } else {
	GList *chunk_ptr;
	u_long burst_amt = 0;

	/* For mp3 et al., walk through chunks in reverse order */
	chunk_ptr = cbuf3->buf->tail;
	if (!chunk_ptr) {
	    debug_printf ("Error.  No data for relay\n");
	    threadlib_signal_sem (&cbuf3->sem);
	    return SR_ERROR_NO_DATA_FOR_RELAY;
	}
	burst_amt += cbuf3->chunk_size;

	while (burst_amt < burst_request) {
	    chunk_ptr = chunk_ptr->prev;
	    if (!chunk_ptr) {
		break;
	    }
	    burst_amt += cbuf3->chunk_size;
	}

	relay_client->m_cbuf_ptr.chunk = chunk_ptr;
	relay_client->m_cbuf_ptr.offset = 0;
    }

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_add_relay_entry released cbuf3->sem\n");
    return SR_SUCCESS;
}

error_code 
cbuf3_extract_relay (Cbuf3 *cbuf3,
		     Relay_client *relay_client)
{
    GList *chunk;
    char *chunk_data;
    u_long offset, remaining;
    error_code ec;

    debug_printf ("cbuf3_extract_relay is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_relay got cbuf3->sem\n");

    debug_printf ("EXTRACT_RELAY\n");

    chunk = relay_client->m_cbuf_ptr.chunk;
    offset = relay_client->m_cbuf_ptr.offset;
    chunk_data = (char*) chunk->data;
    remaining = cbuf3->chunk_size - offset;


    if (chunk->next == NULL) {
	ec = SR_ERROR_BUFFER_EMPTY;
    } else {
	debug_printf ("Client %d chunk %p\n", relay_client->m_sock, chunk);
	memcpy (relay_client->m_buffer, &chunk_data[offset], remaining);
	relay_client->m_left_to_send = remaining;
	relay_client->m_cbuf_ptr.chunk = chunk->next;
	relay_client->m_cbuf_ptr.offset = 0;
	ec = SR_SUCCESS;
    }

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract_relay released cbuf3->sem\n");
    return ec;
}

/* Copy data from the cbuf3 into the caller's buffer.  Note: This 
   updates the caller's cbuf3_ptr. */
error_code 
cbuf3_extract (Cbuf3 *cbuf3,
	       Cbuf3_pointer *cbuf3_ptr,
	       char *buf,
	       u_long req_size,
	       u_long *bytes_read)
{
    char *chunk_data;
    u_long offset, chunk_remaining;

    if (!cbuf3_ptr->chunk) {
	return SR_ERROR_BUFFER_EMPTY;
    }

    debug_printf ("cbuf3_extract is waiting for cbuf3->sem\n");
    threadlib_waitfor_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract got cbuf3->sem\n");

    chunk_data = (char*) cbuf3_ptr->chunk->data;
    offset = cbuf3_ptr->offset;
    chunk_remaining = cbuf3->chunk_size - offset;
    if (req_size < chunk_remaining) {
	(*bytes_read) = req_size;
	cbuf3_ptr->offset += req_size;
    } else {
	(*bytes_read) = chunk_remaining;
	cbuf3_ptr->chunk = cbuf3_ptr->chunk->next;
	cbuf3_ptr->offset = 0;
    }

    memcpy (buf, &chunk_data[offset], (*bytes_read));

    threadlib_signal_sem (&cbuf3->sem);
    debug_printf ("cbuf3_extract released cbuf3->sem\n");
    return SR_SUCCESS;
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
static void
cbuf3_advance_relay_list (RIP_MANAGER_INFO *rmi, Cbuf3 *cbuf3)
{

    GList *rlist_node = rmi->relay_list->head;
    GList *cbuf3_head = cbuf3->buf->head;

    while (rlist_node) {
	Relay_client *relay_client = (Relay_client *) rlist_node->data;
	GList *next = rlist_node->next;

	if (relay_client->m_cbuf_ptr.chunk == cbuf3_head) {
	    debug_printf ("Relay: Client %d couldn't keep up with cbuf\n", 
			  relay_client->m_sock);
	    relaylib_disconnect (rmi, rlist_node);
	}
	rlist_node = next;
    }
}

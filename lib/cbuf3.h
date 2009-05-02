/* cbuf3.h
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
#ifndef __CBUF3_H__
#define __CBUF3_H__

#include "srtypes.h"
#include "errors.h"

/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
error_code
cbuf3_init (struct cbuf3 *cbuf3, 
	    int content_type, 
	    int have_relay, 
	    unsigned long chunk_size, 
	    unsigned long num_chunks);
void
cbuf3_destroy (struct cbuf3 *cbuf3);
char*
cbuf3_request_free_chunk (RIP_MANAGER_INFO *rmi,
			  struct cbuf3 *cbuf3);
error_code
cbuf3_insert (struct cbuf3 *cbuf3, char *chunk);
error_code
cbuf3_add (struct cbuf3 *cbuf3, 
	   struct cbuf3_pointer *out_ptr, 
	   struct cbuf3_pointer *in_ptr, 
	   u_long len);
error_code
cbuf3_set_uint32 (struct cbuf3 *cbuf3, 
		  struct cbuf3_pointer *in_ptr, 
		  uint32_t val);
void
cbuf3_splice_page_list (struct cbuf3 *cbuf3, 
			GList **chunk_page_list);
void
cbuf3_ogg_peek_page (Cbuf3 *cbuf3, 
		     GList **page_node);
void
cbuf3_ogg_advance_page (Cbuf3 *cbuf3);
error_code
cbuf3_initialize_relay_client_ptr (struct cbuf3 *cbuf3,
				   struct relay_client *relay_client,
				   u_long burst_request);
error_code 
cbuf3_extract_relay (Cbuf3 *cbuf3,
		     Relay_client *relay_client);
error_code 
cbuf3_extract (Cbuf3 *cbuf3,
	       Cbuf3_pointer *cbuf3_ptr,
	       char *buf,
	       u_long req_size,
	       u_long *bytes_read);

#endif

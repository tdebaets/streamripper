/* sbuffer.c - jonclegg@yahoo.com
 * general use circular buffer lib
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
#include "cbuffer.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	cbuffer_init(CBUFFER *buffer, u_long size);
void		cbuffer_destroy(CBUFFER *buffer);
error_code	cbuffer_extract(CBUFFER *buffer, char *items, u_long count);
error_code	cbuffer_peek(CBUFFER *buffer, char *items, u_long count);
error_code	cbuffer_insert(CBUFFER *buffer, const char *items, u_long count);
error_code cbuffer_fastforward (CBUFFER *buffer, u_long count);

u_long		cbuffer_get_free(CBUFFER *buffer);
u_long 		cbuffer_get_used(CBUFFER *buffer);
u_long		cbuffer_get_size(CBUFFER *buffer);


/*********************************************************************************
 * Private functions
 *********************************************************************************/
static void	reset(CBUFFER *buffer);
static void	increment(CBUFFER *buffer, u_long *index);
static u_long   get_read_ptr (CBUFFER *buffer, u_long pos);


error_code
cbuffer_init(CBUFFER *buffer, unsigned long size)
{
    if (size == 0) {
        return SR_ERROR_INVALID_PARAM;
    }
    if (size == 0) {
        buffer->size = 0;
        buffer->buf = NULL;
        reset(buffer);
    } else {
        buffer->size = size;
        buffer->buf = (char *)malloc(size);
        reset(buffer);
    }

    return SR_SUCCESS;
}

void
cbuffer_destroy(CBUFFER *buffer)
{
    if (buffer->buf)
    {
	free(buffer->buf);
	buffer->buf = NULL;
    }
}

// GCS -- Inefficient
error_code
cbuffer_fastforward (CBUFFER *buffer, u_long count)
{
    u_long i;	

    for(i = 0; i < count; i++) {
	if (buffer->item_count > 0)
	    buffer->item_count--;
	else
	    return SR_ERROR_BUFFER_EMPTY;
	increment(buffer, &buffer->read_index);
    }
    return SR_SUCCESS;
}

error_code
cbuffer_peek_rgn (CBUFFER *buffer, char *out_buf, u_long start, u_long length)
{
    u_long sidx;

    if (length > buffer->item_count)
	return SR_ERROR_BUFFER_EMPTY;
    sidx = get_read_ptr (buffer, start);
    if (sidx + length > buffer->size) {
	int first_hunk = buffer->size - sidx;
	memcpy (out_buf, buffer->buf+sidx, first_hunk);
	sidx = 0;
	length -= first_hunk;
	out_buf += first_hunk;
    }
    memcpy (out_buf, buffer->buf+sidx, length);

    return SR_SUCCESS;
}


error_code cbuffer_extract(CBUFFER *buffer, char *items, u_long count)
{
	u_long i;	
	
	for(i = 0; i < count; i++)
	{
		if (buffer->item_count > 0)
			buffer->item_count--;
		else
			return SR_ERROR_BUFFER_EMPTY;

		increment(buffer, &buffer->read_index);
		items[i] = buffer->buf[buffer->read_index];
	}
	return SR_SUCCESS;
}


error_code cbuffer_peek(CBUFFER *buffer, char *items, u_long count)
{
	u_long i;	
	u_long my_read_index = buffer->read_index;
	u_long my_item_count = buffer->item_count;

	for(i = 0; i < count; i++)
	{
		if (my_item_count > 0)
			my_item_count--;
		else
			return SR_ERROR_BUFFER_EMPTY;

		increment(buffer, &my_read_index);
		items[i] = buffer->buf[my_read_index];
	}
	return SR_SUCCESS;
}


error_code cbuffer_insert(CBUFFER *buffer, const char *items, u_long count)
{
	u_long i;	
	
	for(i = 0; i < count; i++)
	{
		if (buffer->item_count < cbuffer_get_size(buffer))
			buffer->item_count++;
		else
			return SR_ERROR_BUFFER_FULL;

		increment(buffer, &buffer->write_index);
		buffer->buf[buffer->write_index] = items[i];
	}

	return SR_SUCCESS;
}

u_long cbuffer_get_size(CBUFFER *buffer)
{
	return buffer->size;

}

u_long cbuffer_get_free(CBUFFER *buffer)
{
	return cbuffer_get_size(buffer) - cbuffer_get_used(buffer);
}

u_long cbuffer_get_used(CBUFFER *buffer)
{
	return buffer->item_count;
}

void increment(CBUFFER *buffer, u_long *index)
{
   (*index)++;
   if (*index >= cbuffer_get_size(buffer))
      *index = 0;
}

void reset(CBUFFER *buffer)
{
	buffer->read_index = buffer->write_index = cbuffer_get_size(buffer) - 1;
	buffer->item_count = 0;
}

static u_long
get_read_ptr (CBUFFER *buffer, u_long pos)
{
    u_long idx;
    idx = buffer->read_index + pos + 1;
    if (idx >= buffer->size) {
	idx -= buffer->size;
    }
    return idx;
}


#ifndef __CBUF2_H__
#define __CBUF2_H__

#include "srtypes.h"
#include "list.h"

typedef struct CBUF2_struct
{
    char	*buf;
    u_long	size;
    u_long	write_index;
    u_long	read_index;
    u_long	item_count;
    LIST        meta_list;
    LIST        frame_list;
    LIST        client_list;
} CBUF2;

error_code	cbuf2_init(CBUF2 *buffer, u_long size);
void		cbuf2_destroy(CBUF2 *buffer);
error_code	cbuf2_extract(CBUF2 *buffer, char *items, u_long count);
error_code	cbuf2_peek(CBUF2 *buffer, char *items, u_long count);
error_code	cbuf2_insert(CBUF2 *buffer, const char *items, u_long count);
error_code  cbuf2_fastforward (CBUF2 *buffer, u_long count);
u_long		cbuf2_get_free(CBUF2 *buffer);
u_long 		cbuf2_get_used(CBUF2 *buffer);
u_long		cbuf2_get_size(CBUF2 *buffer);
error_code  cbuf2_peek_rgn (CBUF2 *buffer, char *out_buf, u_long start, u_long length);
error_code
cbuf2_insert_chunk (CBUF2 *buffer, const char *items, u_long count);

#endif

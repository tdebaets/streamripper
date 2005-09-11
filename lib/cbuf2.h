#ifndef __CBUF2_H__
#define __CBUF2_H__

#include "srtypes.h"
#include "threadlib.h"
#include "list.h"
#include "relaylib.h"

#define MAX_METADATA_LEN (127*16)

/* Each metadata within the cbuf gets this struct */
typedef struct METADATA_LIST_struct METADATA_LIST;
struct METADATA_LIST_struct
{
    unsigned long m_chunk;
    char m_composed_metadata[MAX_METADATA_LEN+1];   // +1 for len*16
    LIST m_list;
};

typedef struct CBUF2_struct
{
    char*	buf;
    u_long	num_chunks;
    u_long	chunk_size;
    u_long	size;        /* size is chunk_size * num_chunks */

    u_long	base_idx;
    u_long	item_count;

    u_long	next_song;   /* start of next song */

    // u_long	write_index;
    HSEM        cbuf_sem;

    LIST        metadata_list;
    LIST        frame_list;
} CBUF2;


/*****************************************************************************
 * Global variables
 *****************************************************************************/
extern CBUF2 g_cbuf2;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
error_code cbuf2_init (CBUF2 *cbuf2, unsigned long chunk_size,
		       unsigned long num_chunks);
void cbuf2_destroy(CBUF2 *buffer);
error_code cbuf2_extract (CBUF2 *cbuf2, char *data, 
			  u_long count, u_long* curr_song);
error_code cbuf2_peek(CBUF2 *buffer, char *items, u_long count);
error_code cbuf2_insert_chunk (CBUF2 *buffer, const char *items, u_long count,
			       TRACK_INFO* ti);
error_code cbuf2_fastforward (CBUF2 *buffer, u_long count);
error_code cbuf2_peek_rgn (CBUF2 *buffer, char *out_buf, 
			   u_long start, u_long length);

u_long cbuf2_get_free(CBUF2 *buffer);
u_long cbuf2_get_free_tail (CBUF2 *cbuf2);
u_long cbuf2_write_index (CBUF2 *cbuf2);

void cbuf2_set_next_song (CBUF2 *cbuf2, u_long pos);

error_code cbuf2_init_relay_entry (CBUF2 *cbuf2, RELAY_LIST* ptr, 
				   u_long burst_request);
error_code cbuf2_extract_relay (CBUF2 *cbuf2, char *data, u_long *pos, 
				u_long *len, int icy_metadata);

void cbuf2_debug_lists (CBUF2 *cbuf2);

#endif

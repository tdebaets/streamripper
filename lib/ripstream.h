#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "types.h"

#if defined (commentout)
error_code ripstream_init(u_long buffersize_mult, IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, BOOL addID3tag, BOOL pad_songs);
#endif
error_code
ripstream_init (IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, 
		SPLITPOINT_OPTIONS *sp_opt, int bitrate, BOOL addID3tag);

error_code ripstream_rip();
void ripstream_destroy();


#endif //__RIPLIB__


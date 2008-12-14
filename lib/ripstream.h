#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "types.h"

error_code
ripstream_init (IO_GET_STREAM *in, char *no_meta_name, 
		int drop_count, SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, 
		int content_type, 
		BOOL addID3tag);
error_code ripstream_rip();
void ripstream_destroy();


#endif //__RIPLIB__


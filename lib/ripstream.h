#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "srtypes.h"
#include "socklib.h"

error_code
ripstream_init (HSOCKET sock, 
		int have_relay,
		int timeout, 
		char *no_meta_name, 
		int drop_count,
		SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, 
		int meta_interval, 
		int content_type, 
		BOOL addID3tag);
error_code ripstream_rip();
void ripstream_destroy();


#endif //__RIPLIB__

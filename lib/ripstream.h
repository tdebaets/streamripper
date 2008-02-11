#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "rip_manager.h"
#include "srtypes.h"
#include "prefs.h"
#include "socklib.h"

error_code
ripstream_init (RIP_MANAGER_INFO* rmi,
		HSOCKET sock, 
		int have_relay,
		int timeout, 
		char *no_meta_name, 
		int drop_count,
		SPLITPOINT_OPTIONS *sp_opt, 
		int bitrate, 
		int meta_interval, 
		int content_type, 
		BOOL add_id3v1,
		BOOL add_id3v2,
		External_Process* ep);
error_code ripstream_rip (RIP_MANAGER_INFO* rmi);
void ripstream_clear(RIP_MANAGER_INFO* rmi);

#endif //__RIPLIB__

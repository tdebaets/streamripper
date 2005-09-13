#ifndef __FINDSEP_H__
#define __FINDSEP_H__

#include "srtypes.h"

error_code
findsep_silence (const u_char* mpgbuf, 
		 long mpgsize, 
		 long silence_length, 
		 long padding1,
		 long padding2,
		 u_long* pos1, 
		 u_long* pos2
		 );
error_code find_bitrate(unsigned long* bitrate, const u_char* mpgbuf, 
			long mpgsize);

#endif //__FINDSEP_H__

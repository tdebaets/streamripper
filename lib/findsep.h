#ifndef __FINDSEP_H__
#define __FINDSEP_H__

#include "types.h"

error_code findsep_silence(const u_char* mpgbuf, long mpgsize, 
			   long silence_length, u_long* psilence);
#endif //__FINDSEP_H__

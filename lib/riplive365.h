#ifndef __RIPLIVE365_H__
#define __RIPLIVE365_H__

#include "types.h"

extern error_code	riplive365_init(IO_DATA_INPUT *in, IO_GET_STREAM *getstream, char *stream_name, 
									char *url, char *proxyurl);
extern void			riplive365_destroy();


#endif //__RIPLIVE365_H__



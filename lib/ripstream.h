#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "types.h"

extern error_code	ripstream_init(u_long buffersize_mult, IO_GET_STREAM *in, IO_PUT_STREAM *out, char *no_meta_name, BOOL addID3Tag);
extern error_code	ripstream_rip();
extern void			ripstream_destroy();


#endif //__RIPLIB__


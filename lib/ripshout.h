#ifndef __RIPSHOUT_H__
#define __RIPSHOUT_H__

#include "types.h"

extern error_code	ripshout_init(IO_DATA_INPUT *in, IO_GET_STREAM *getstream, int meta_interval, char *no_meta_name);
extern void			ripshout_destroy();

#endif //__RIPSHOUT_H__

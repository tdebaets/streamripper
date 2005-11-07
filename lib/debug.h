#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "time.h"
#include <stdio.h>
void debug_open (void);
void debug_set_filename (char* filename);
void debug_close (void);
void debug_printf (char* fmt, ...);
void debug_enable (void);
void debug_print_error (void);

#endif

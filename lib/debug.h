#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "time.h"
#include <stdio.h>
//
// Set this 0 to 2, won't take effect unless _DEBUG_ is def'd
// setting to -1 turn everything off.
//
//#define DLEVEL	2
#define DLEVEL	-1
#define _DEBUG_

extern char* _varg2str(char *format, ...);
extern void _freevargstr(char *str);

#define _DPRINT(_x_, _level_)						\
{									\
    char *body = _varg2str _x_;						\
    char str[MAX_ERROR_STR];						\
    char datebuf[50];							\
    time_t now = time(NULL);						\
    strftime(datebuf, 50, "%m/%d/%Y:%H:%M:%S", localtime(&now));	\
    sprintf(str, "L%d [%s] %s:%d -- %s\n",				\
			 _level_,					\
			 datebuf,					\
			 __FILE__,					\
			 __LINE__,					\
			 body);						\
    if (DLEVEL >= _level_)						\
    {									\
	/* OutputDebugString(str); */					\
	fprintf(stderr, "%s", str);					\
    }									\
    _freevargstr(body);							\
}


#ifdef _DEBUG_
#define DEBUG0(_x_)	_DPRINT(_x_, 0)
#define DEBUG1(_x_)	_DPRINT(_x_, 1)
#define DEBUG2(_x_)	_DPRINT(_x_, 2)
#else
#define DEBUG0(_x_)
#define DEBUG1(_x_)
#define DEBUG2(_x_)
#endif


void debug_open (void);
void debug_close (void);
void debug_printf (char* fmt, ...);
void debug_enable (void);

#endif//__DEBUG_H__


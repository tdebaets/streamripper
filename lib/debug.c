#include "debug.h"
#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#if WIN32
	#define vsnprintf _vsnprintf
#endif

/*********************************************************************************
 * Public functions
 *********************************************************************************/
char* _varg2str(char *format, ...)
{
    va_list va;
	char *s = (char*)malloc(MAX_ERROR_STR);
	va_start(va, format);
	vsnprintf(s, MAX_ERROR_STR, format, va);
	va_end(va);
	return s;
}

void _freevargstr(char *str)
{
	if (str)
		free(str);
}

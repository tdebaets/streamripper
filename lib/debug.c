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


#define DEBUG_PRINTF_TO_FILE 1

int debug_on = 0;
int command_line_debug = 0;
FILE* gcsfp = 0;

void
debug_enable (void)
{
    debug_on = 1;
}

void
debug_open (void)
{
    char* filename = "gcs.txt";
    if (!debug_on) return;
    if (!gcsfp) {
	gcsfp = fopen(filename, "a");
    }
}

void
debug_close (void)
{
    if (!debug_on) return;
    if (gcsfp) {
	fclose(gcsfp);
	gcsfp = 0;
    }
}

void
debug_printf (char* fmt, ...)
{
#if (DEBUG_PRINTF_TO_FILE)
    static int initialized = 0;
    int was_open = 1;
#endif
    va_list argptr;

    if (!debug_on) return;

#if (DEBUG_PRINTF_TO_FILE)
    va_start (argptr, fmt);
    if (!gcsfp) {
	was_open = 0;
	debug_open();
    }
    if (!initialized) {
	initialized = 1;
	fprintf (gcsfp, "=========================\n");
    }
    vfprintf (gcsfp, fmt, argptr);
#endif

    if (command_line_debug) {
#if (!DEBUG_PRINTF_TO_FILE)
      va_start (argptr, fmt);
#endif
      vprintf (fmt, argptr);
#if (!DEBUG_PRINTF_TO_FILE)
      va_end (argptr);
#endif
    }

#if (DEBUG_PRINTF_TO_FILE)
    va_end (argptr);
    if (!was_open) {
	debug_close ();
    }
#endif
}

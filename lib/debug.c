#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "srtypes.h"
#include "threadlib.h"
#include "rip_manager.h"
#include "debug.h"

#if WIN32
	#define vsnprintf _vsnprintf
#endif

/*********************************************************************************
 * Public functions
 *********************************************************************************/
#define DEBUG_PRINTF_TO_FILE 1

int debug_on = 0;
int command_line_debug = 0;
FILE* gcsfp = 0;
static HSEM m_debug_lock;
static char* debug_filename = 0;
static char filename_buf[SR_MAX_PATH];
static char* default_filename = "gcs.txt";

void
debug_set_filename (char* filename)
{
    strcpy (filename_buf, filename);
    debug_filename = filename_buf;
}

void
debug_enable (void)
{
    debug_on = 1;
    if (!debug_filename) {
	debug_filename = default_filename;
    }
}

void
debug_open (void)
{
    if (!debug_on) return;
    if (!gcsfp) {
	gcsfp = fopen(debug_filename, "a");
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

    if (!initialized) {
        m_debug_lock = threadlib_create_sem();
        threadlib_signal_sem(&m_debug_lock);
    }
    threadlib_waitfor_sem (&m_debug_lock);

#if (DEBUG_PRINTF_TO_FILE)
    va_start (argptr, fmt);
    if (!gcsfp) {
	was_open = 0;
	debug_open();
    }
    if (!initialized) {
	initialized = 1;
	fprintf (gcsfp, "=========================\n");
	fprintf (gcsfp, "STREAMRIPPER " SRPLATFORM " " SRVERSION "\n");
    }
    vfprintf (gcsfp, fmt, argptr);
    fflush (gcsfp);
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
    threadlib_signal_sem (&m_debug_lock);
}

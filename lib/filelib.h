#ifndef __FILELIB_H__
#define __FILELIB_H__

#include "types.h"
#if WIN32
#include <windows.h>
#endif

#if WIN32
#define PATH_SLASH '\\'
#else
#define PATH_SLASH '/'
#endif


/* Pathname support.
   Copyright (C) 1995-1999, 2000-2003 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
   Licence: GNU LGPL */
/* ISSLASH(C)           tests whether C is a directory separator character.
   IS_ABSOLUTE_PATH(P)  tests whether P is an absolute path.  If it is not,
                        it may be concatenated to a directory pathname.
 */
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
# define HAS_DEVICE(P) \
    ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) \
     && (P)[1] == ':')
# define IS_ABSOLUTE_PATH(P) (ISSLASH ((P)[0]) || HAS_DEVICE (P))
#else
  /* Unix */
# define ISSLASH(C) ((C) == '/')
# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
#endif



#if defined (commentout)
#ifndef WIN32
#define MAX_PATH 256
#endif
#ifndef MAX_PATH
#define MAX_PATH		512
#endif
#define MAX_PATH_LEN		255
#define MAX_FILENAME_LEN	255
#define MAX_FILENAME		255
#endif
#define SR_MIN_FILENAME		54     /* For files in incomplete */
#define SR_MIN_COMPLETEDIR      10     /* For dir with radio station name */
#define SR_DATE_LEN		11
#define SR_MIN_COMPLETE_W_DATE	(SR_MIN_COMPLETEDIR+SR_DATE_LEN)
/* Directory lengths, including trailing slash */
#define SR_MAX_INCOMPLETE  (SR_MAX_PATH-SR_MIN_FILENAME)
#define SR_MAX_COMPLETE    (SR_MAX_INCOMPLETE-strlen("incomplete/"))
#define SR_MAX_BASE        (SR_MAX_COMPLETE-SR_MIN_COMPLETEDIR-strlen("/"))
#define SR_MAX_BASE_W_DATE (SR_MAX_BASE-SR_MIN_COMPLETE_W_DATE)


error_code filelib_init(BOOL do_count, BOOL keep_incomplete, 
			BOOL do_single_file, char* single_file_name);
error_code filelib_start(char *filename);
error_code filelib_end(char *filename, BOOL over_write_existing, 
			    /*out*/ char *fullpath);
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
void filelib_shutdown();
error_code filelib_set_output_directory (char* output_directory, 
		int get_separate_dirs, int get_date_stamp, char* icy_name);
char* filelib_get_output_directory ();
error_code filelib_remove(char *filename);
error_code filelib_write_cue(char *artist, char* title, int secs);

#endif //FILELIB

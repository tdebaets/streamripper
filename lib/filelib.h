#ifndef __FILELIB_H__
#define __FILELIB_H__

#include "srtypes.h"
#if WIN32
#include <windows.h>
#endif

#if WIN32
#define PATH_SLASH '\\'
#define PATH_SLASH_STR "\\"
#else
#define PATH_SLASH '/'
#define PATH_SLASH_STR "/"
#endif


/* Pathname support.
   Copyright (C) 1995-1999, 2000-2003 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
   Licence: GNU LGPL */
/* ISSLASH(C)           tests whether C is a directory separator character.
   IS_ABSOLUTE_PATH(P)  tests whether P is an absolute path.  If it is not,
                        it may be concatenated to a directory pathname. */
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
# define HAS_DEVICE(P) \
    ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) \
     && (P)[1] == ':')
/* GCS: This is not correct, because it could be c:foo which is relative */
/* # define IS_ABSOLUTE_PATH(P) (ISSLASH ((P)[0]) || HAS_DEVICE (P)) */
# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
#else
  /* Unix */
# define ISSLASH(C) ((C) == '/')
# define HAS_DEVICE(P) (0)
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


error_code
filelib_init (BOOL do_individual_tracks,
	      BOOL do_count,
	      int count_start,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* show_file_name,
	      char* output_directory,
	      char* output_pattern,
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name);
error_code filelib_start (TRACK_INFO* ti);
error_code filelib_end (TRACK_INFO* ti, 
	     enum OverwriteOpt overwrite,
	     BOOL truncate_dup, 
	     char *fullpath);
error_code filelib_write_track(char *buf, u_long size);
error_code filelib_write_show(char *buf, u_long size);
void filelib_shutdown();
error_code filelib_remove(char *filename);
error_code filelib_write_cue(TRACK_INFO* ti, int secs);

#endif //FILELIB

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

#ifndef MAX_PATH
#define MAX_PATH		512
#endif
#define MAX_FILENAME	255
#define MAX_DIR_LEN		248
#define MAX_BASE_DIR_LEN (248-strlen("/incomplete/"))

error_code filelib_init(BOOL do_count, BOOL keep_incomplete, 
			BOOL do_single_file, char* single_file_name);
error_code	filelib_start(char *filename);
error_code	filelib_end(char *filename, BOOL over_write_existing, /*out*/ char *fullpath);
error_code	filelib_write_track(char *buf, u_long size);
error_code	filelib_write_show(char *buf, u_long size);
void		filelib_shutdown();
error_code	filelib_set_output_directory(char *str);
error_code	filelib_remove(char *filename);
void filelib_set_max_filename_length (int mfl);
error_code filelib_write_cue(char *artist, char* title, int secs);

#endif //FILELIB

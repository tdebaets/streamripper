#ifndef __UTIL_H__
#define __UTIL_H__

#include "types.h"

char *escape_string_alloc(const char *str);
char *left_str(char *str, int len);
char *strip_last_word(char *str);
int word_count(char *str);
char *subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char *strip_invalid_chars(char *str);
char *format_byte_size(char *str, long size);
char *add_trailing_slash(char *str);
void trim(char *str);
void null_printf(char *str, ...);
int dir_max_filename_length (char* dirname);
void initialize_default_locale (CODESET_OPTIONS* cs_opt);
void set_codeset (char* codeset_type, const char* codeset);
void parse_artist_title (char* artist, char* title, char* album, 
			 int bufsize, char* trackname);

#endif //__UTIL_H__

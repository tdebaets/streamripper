#ifndef __MCHAR_H__
#define __MCHAR_H__

#include "srtypes.h"

#if HAVE_WCHAR_SUPPORT
#define m(x) L##x
#define mS L"%ls"
#define mC L"%lc"
#else
#define m(x) x
#define mS "%s"
#define mC "%c"
#endif

char *subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char *left_str(char *str, int len);
char *format_byte_size(char *str, long size);
void trim(char *str);
void sr_strncpy(char* dst, char* src, int n);

void initialize_default_locale (CODESET_OPTIONS* cs_opt);
void set_codeset (char* codeset_type, const char* codeset);

int mstring_from_string (mchar* m, int mlen, char* c, int codeset_type);
int string_from_mstring (char* c, int clen, mchar* m, int codeset_type);

mchar* mstrdup (mchar* src);
mchar* mstrcpy (mchar* dest, const mchar* src);
int msnprintf (mchar* dest, size_t n, const mchar* fmt, ...);
void mstrncpy (mchar* dst, mchar* src, int n);
size_t mstrlen (mchar* s);
mchar* mstrchr (const mchar* ws, mchar wc);
mchar* mstrncat (mchar* ws1, const mchar* ws2, size_t n);
int mstrcmp (const mchar* ws1, const mchar* ws2);
long int mtol (const mchar* string);

char *strip_invalid_chars(char *str);
mchar* strip_invalid_chars_new (mchar *str);

int is_id3_unicode (void);

#endif /*__MCHAR_H__*/

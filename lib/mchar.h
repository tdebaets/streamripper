#ifndef __MCHAR_H__
#define __MCHAR_H__

#include "srtypes.h"

#if USE_GLIB_UTF8
#define m_(x) x
#define m_S "%s"
#define m_C "%c"
#define m_s "%s"
#define m_c "%c"
#elif HAVE_WCHAR_SUPPORT
#define m_(x) L##x
#if defined (WIN32)
#define m_S L"%s"
#define m_C L"%c"
#define m_s L"%S"
#define m_c L"%C"
#else
#define m_S L"%ls"
#define m_C L"%lc"
#define m_s L"%s"
#define m_c L"%c"
#endif
#else
#define m_(x) x
#define m_S "%s"
#define m_C "%c"
#define m_s "%s"
#define m_c "%c"
#endif

char *subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char *left_str(char *str, int len);
char *format_byte_size(char *str, long size);
void trim(char *str);
void sr_strncpy(char* dst, char* src, int n);

void set_codesets_default (CODESET_OPTIONS* cs_opt);
void register_codesets (RIP_MANAGER_INFO* rmi, CODESET_OPTIONS* cs_opt);

int mstring_from_string (RIP_MANAGER_INFO* rmi, mchar* m, int mlen, 
			 char* c, int codeset_type);
int string_from_mstring (RIP_MANAGER_INFO* rmi, char* c, int clen, 
			 mchar* m, int codeset_type);

mchar* mstrdup (mchar* src);
mchar* mstrcpy (mchar* dest, const mchar* src);
int msnprintf (mchar* dest, size_t n, const mchar* fmt, ...);
void mstrncpy (mchar* dst, mchar* src, int n);
size_t mstrlen (mchar* s);
mchar* mstrchr (const mchar* ws, mchar wc);
mchar* mstrrchr (const mchar* ws, mchar wc);
mchar* mstrncat (mchar* ws1, const mchar* ws2, size_t n);
int mstrcmp (const mchar* ws1, const mchar* ws2);
long int mtol (const mchar* string);
int is_id3_unicode (RIP_MANAGER_INFO* mchar_cs);

gchar* utf8_string_from_string (char* src, char* codeset);

#endif /*__MCHAR_H__*/

/* util.c - jonclegg@yahoo.com
 * general util library
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "srconfig.h"
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined HAVE_WCHAR_T
#if defined HAVE_WCHAR_H
#include <wchar.h>
#endif
#if defined HAVE_WCTYPE_H
#include <wctype.h>
#endif
#endif
#include <locale.h>
#include <time.h>
#include <errno.h>
#if defined HAVE_ICONV
#include <iconv.h>
#endif
#if defined HAVE_LOCALE_CHARSET
#include <localcharset.h>
#elif defined HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif
#include "debug.h"
#include "types.h"

/* uncomment to use new i18n code */
/* #define NEW_I18N_CODE 1 */

/*****************************************************************************
 * Public functions
 *****************************************************************************/
char	*escape_string_alloc(const char *str);
char	*left_str(char *str, int len);
char	*strip_last_word(char *str);
int	word_count(char *str);
char	*subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char	*strip_invalid_chars(char *str);
char	*format_byte_size(char *str, long size);
char	*add_trailing_slash(char *str);
void	trim(char *str);
void 	null_printf(char *s, ...);

/*****************************************************************************
 * Private global variables
 *****************************************************************************/
const char* codeset_metadata;
const char* codeset_matchstring;
const char* codeset_relay;
const char* codeset_id3;
const char* codeset_filesys;


char*
add_trailing_slash (char *str)
{
#if WIN32
    if (str[strlen(str)-1] != '\\')
	strcat(str, "\\");
#else
    if (str[strlen(str)-1] != '/')
	strcat(str, "/");
#endif

    return str;
}

char *subnstr_until(const char *str, char *until, char *newstr, int maxlen)
{
    const char *p = str;
    int len = 0;

    for(len = 0; strncmp(p, until, strlen(until)) != 0 && len < maxlen; p++)
    {
	newstr[len] = *p;
	len++;
    }
    newstr[len] = '\0';

    return newstr;
}

char *left_str(char *str, int len)
{
    int slen = strlen(str);

    if (slen <= len)
	return str;

    str[len] = '\0';
    return str;
}

#if HAVE_WCHAR_T
# if HAVE_ICONV
int 
iconv_convert_string (char* dst, int dst_len, char* src,
		      const char* dst_codeset, const char* src_codeset)
{
    size_t rc;
    iconv_t ict;
    size_t src_left, dst_left;
    char *src_ptr, *dst_ptr;

    /* First try to convert using iconv. */
    ict = iconv_open(dst_codeset, src_codeset);
    if (ict == (iconv_t)(-1)) {
	printf ("Error on iconv_open(\"%s\",\"%s\")\n",
		      dst_codeset, src_codeset);
	return -1;
    }
    src_left = strlen(src);
    dst_left = dst_len;
    src_ptr = src;
    dst_ptr = dst;
    rc = iconv(ict,&src_ptr,&src_left,&dst_ptr,&dst_left);
    if (rc == -1) {
	if (errno == EINVAL || errno == E2BIG) {
	    /* EINVAL means the last character was truncated
	       E2BIG means the output buffer was too small.
	       Declare success and try to continue... */
	    printf ("Oops 1\n");
	} else if (errno == EILSEQ) {
	    /* Here I should advance cptr and try to continue, right? */
	    printf ("Oops 2\n");
	}
    }
    iconv_close (ict);
    return 0;
}
# endif

/* What does the rc mean here? */
int 
string_from_wstring (char* c, int clen, wchar_t* w, const char* codeset)
{
    int rc;

# if HAVE_ICONV
    rc = iconv_convert_string (c, clen, (char*) w, codeset, "WCHAR_T");
    if (rc == 0) return 0;
    /* Otherwise, fall through to wcstombs method */
# endif

    rc = wcstombs(c,w,clen);
    if (rc == -1) {
	/* Do something smart here */
    }
    return 0;
}

/* What does the rc mean here? */
int 
wstring_from_string (wchar_t* w, int wlen, char* c, const char* codeset)
{
    int rc;

# if HAVE_ICONV
    rc = iconv_convert_string ((char*) w, wlen, c, "WCHAR_T", codeset);
    if (rc == 0) return 0;
    /* Otherwise, fall through to mbstowcs method */
# endif

    rc = mbstowcs(w,c,wlen);
    if (rc == -1) {
	/* Do something smart here */
    }
    return 0;
}
#endif /* HAVE_WCHAR_T */

void
set_codeset (char* codeset_type, const char* codeset)
{
    if (!strcmp(codeset_type, "CODESET_METADATA")) {
	codeset_metadata = codeset;
	return;
    }
    if (!strcmp(codeset_type, "CODESET_MATCHSTRING")) {
	codeset_matchstring = codeset;
	return;
    }
    if (!strcmp(codeset_type, "CODESET_RELAY")) {
	codeset_relay = codeset;
	return;
    }
    if (!strcmp(codeset_type, "CODESET_ID3")) {
	codeset_id3 = codeset;
	return;
    }
    if (!strcmp(codeset_type, "CODESET_FILESYS")) {
	codeset_filesys = codeset;
	return;
    }
    if (!strcmp(codeset_type, "CODESET_ALL")) {
	codeset_metadata = codeset;
	codeset_matchstring = codeset;
	codeset_relay = codeset;
	codeset_id3 = codeset;
	codeset_filesys = codeset;
	return;
    }
}

const char*
get_default_codeset (void)
{
    const char* fromcode = 0;
#if defined HAVE_LOCALE_CHARSET
    fromcode = locale_charset ();
#elif defined HAVE_LANGINFO_CODESET
    fromcode = nl_langinfo (CODESET);
#else
    /* No way to get default codeset */
#endif
    return fromcode;
}

void
initialize_default_locale (CODESET_OPTIONS* cs_opt)
{
    const char* fromcode = 0;
    setlocale (LC_ALL, "");
    setlocale (LC_CTYPE, "");
    debug_printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));

#if defined HAVE_LOCALE_CHARSET
    debug_printf ("Using locale_charset() to get system codeset.\n");
#elif defined HAVE_LANGINFO_CODESET
    debug_printf ("Using nl_langinfo() to get system codeset.\n");
#else
    debug_printf ("No way to get system codeset.\n");
#endif

#if defined HAVE_ICONV
    debug_printf ("Found iconv.\n");
#else
    debug_printf ("No iconv.\n");
#endif

    /* Set default codesets */
    fromcode = get_default_codeset ();
    if (fromcode) {
        debug_printf ("LOCALE CODESET is %s\n", fromcode);
	set_codeset ("CODESET_ALL", fromcode);
    } else {
	set_codeset ("CODESET_ALL", 0);
    }

    /* Override from command line if requested */
    if (!cs_opt) return;
    if (cs_opt->codeset) {
	set_codeset ("CODESET_ALL", cs_opt->codeset);
    }
}

/*
  metadata     -> wchar }       { wchar -> filename
  matchstring  -> wchar } parse { wchar -> id3, cue
                                { wchar -> relay stream

  metadata_locale: use locale()
  matchstring: use locale() - same as metadata
  filename: <<special/platform-specific>> use utf8
  id3, cue: use locale()
  relay stream: use locale() - same as metadata

  if have iconv 
     && iconv has conversion to wchar_t?
     && HAVE_LOCALE_CHARSET || HAVE_LANGINFO_CODESET
  then
     use iconv for conversion to wchar
  else
     only default locale available
     use mbcstowc for conversion to wchar

  // question: what if posix wchar r.e. matching is not available?
  // answer: default to ascii r.e. matching or simple matching.
  
  default approach, use tre for regular expressions.
  question: should I use built-in posix r.e. at all?
  answer: i don't know, but check AT&T compatibility page, linked 
    off tre page.

  --codeset
  --codeset-metadata
  --codeset-matchstring
  --codeset-relay
  --codeset-id3
  --codeset-filesys

  Three places processing are needed:

  1) stream name -> parsing -> directory name
  2) meta data -> parsing -> file name, id3, relay
  3) What is the third??  

*/
/* Given a multibyte string containing the title, three names are 
   suggested.  One in utf8 encoding, one with wchar_t encoding,
   one in the multibyte encoding of the locale, and one that 
   is a "guaranteed-to-work" ascii name.

   For saving, set up filename like this:

   linux:   utf8 or locale_mbcs + open() or fopen()
   osx:     utf8 + open() or utf8 + wfopen()
   windows: utf8 + OpenFile() or locale_mbcs + OpenFile()

   But still need to convert to wchar for stripping.  Note, this 
   doesn't work all the time using mbstowcs.  

   Finally, user should be able to override.

   ------------------------------------------------------
   Pseudocode:
   ------------------------------------------------------
   convert to wchar

   if it seemed to work (non-null)
     strip using wchar
   else
     give up, and use anonymous ascii

   if target_lang user spec'd
   if have iconv
     convert wchar to user spec'd
     try to open file
     if successful, done

   target_lang is utf8
   if have iconv
     convert wchar to utf8
     try to open file
     if successful, done

   if have wchar_open
     try to open file
     if successful, done

   target_lang is locale_mbcs
   if have iconv
     convert using wchar
   else
     convert using wcstombs
   if successful, done

   give up, and use anonymous ascii
   ------------------------------------------------------
   Note that even the above doesn't consider what 
   to do about the id3 information. :-)
*/
#if defined (commentout)
void
suggest_filenames (char *input_string, char *utf8_name, char *wchar_name, 
		   char *locale_mbcs_name, char *ascii_name,
		   int buflen)
{
    static unsigned int anonymous_idx = 0;

    *utf8_name = 0;
    *wchar_name = 0;
    *locale_mbcs_name = 0;
    *ascii_name = 0;
    if (!input_string) return;
    if (buflen <= 1) return;
}
#endif

#if HAVE_WCHAR_T
char*
strip_invalid_chars_testing(char *str)
{
    /* GCS FIX: Only the leading "." should be stripped for unix. */
#if defined (WIN32)
    char invalid_chars[] = "\\/:*?\"<>|~";
#else
    char invalid_chars[] = "\\/:*?\"<>|.~";
#endif
    char* mb_in = str;
    char* strp;
    int mb_in_len = strlen(mb_in);
    wchar_t *w_in = (wchar_t*) malloc (sizeof(wchar_t)*(mb_in_len+2));
    wchar_t *w_invalid = (wchar_t*) malloc(sizeof(wchar_t)
					   *strlen(invalid_chars)+2);
    wchar_t replacement;
    wchar_t *wstrp;
    unsigned int i;
    size_t t;

    iconv_t ict;
    size_t inleft, outleft;
    const char* fromcode = 0;
#if defined HAVE_LOCALE_CHARSET
    fromcode = locale_charset ();
#elif defined HAVE_LANGINFO_CODESET
    fromcode = nl_langinfo (CODESET);
#endif

    debug_printf ("strip_invalid_chars() mb_in:\n");
    debug_printf (mb_in);
    debug_printf ("\n");
    for (strp = mb_in; *strp; strp++) {
	debug_printf ("%02x ",*strp&0x0ff);
    }
    debug_printf ("\n");

    /* Convert title string to wchar */
    t = mbstowcs(w_in,mb_in,mb_in_len+1);
    debug_printf ("Conversion #1 returned %d\n", t);

    ict = iconv_open("WCHAR_T",fromcode);
    inleft = mb_in_len+1;
    outleft = (mb_in_len+1)*sizeof(wchar_t);
    t = iconv(ict,&mb_in,&inleft,(char**)&w_in,&outleft);
    iconv_close(ict);

    debug_printf ("FROMCODE is %s\n", fromcode);
    debug_printf ("Conversion #2 returned %d,%d,%d\n", t, inleft, outleft);

    /* Convert invalid chars to wide char */
    t = mbstowcs(w_invalid,invalid_chars,strlen(invalid_chars)+1);

    /* Convert "replacement" to wide */
    mbtowc (&replacement,"-",1);

    debug_printf ("strip_invalid_chars() w_in (pre):\n");
    for (wstrp = w_in; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("strip_invalid_chars() w_in (pre #2):\n");
    for (i = 0; i < t; i++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");
    debug_printf ("strip_invalid_chars() w_invalid:\n");
    for (wstrp = w_invalid; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");

    /* Replace illegals to legal */
    for (wstrp = w_in; *wstrp; wstrp++) {
	if ((wcschr(w_invalid, *wstrp) == NULL) && (!iswcntrl(*wstrp)))
	    continue;
	*wstrp = replacement;
    }

    debug_printf ("strip_invalid_chars() w_in (post):\n");
    for (wstrp = w_in; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");

    /* Convert back to multibyte */
    wcstombs(mb_in,w_in,mb_in_len);

    debug_printf ("strip_invalid_chars() mb_in (post):\n");
    debug_printf (mb_in);
    debug_printf ("\n");
    for (strp = mb_in; *strp; strp++) {
	debug_printf ("%02x ",*strp&0x0ff);
    }
    debug_printf ("\n");

    free (w_in);
    free (w_invalid);

    return str;
}

char*
strip_invalid_chars_stable(char *str)
{
    /* GCS FIX: Only the leading "." should be stripped for unix. */
#if defined (WIN32)
    char invalid_chars[] = "\\/:*?\"<>|~";
#else
    char invalid_chars[] = "\\/:*?\"<>|.~";
#endif
    char* mb_in = str;
    char* strp;
    int mb_in_len = strlen(mb_in);
    wchar_t *w_in = (wchar_t*) malloc (sizeof(wchar_t)*mb_in_len+2);
    wchar_t *w_invalid = (wchar_t*) malloc(sizeof(wchar_t)
					   *strlen(invalid_chars)+2);
    wchar_t replacement;
    wchar_t *wstrp;
    int i;
    size_t t;

    debug_printf ("strip_invalid_chars() mb_in:\n");
    debug_printf (mb_in);
    debug_printf ("\n");
    for (strp = mb_in; *strp; strp++) {
	debug_printf ("%02x ",*strp&0x0ff);
    }
    debug_printf ("\n");

    /* Convert invalid chars to wide char */
    t = mbstowcs(w_invalid,invalid_chars,strlen(invalid_chars)+1);
    debug_printf ("Conversion returned %d\n", t);

    /* Convert title string to wchar */
    mbstowcs(w_in,mb_in,mb_in_len+1);

    /* Convert "replacement" to wide */
    mbtowc (&replacement,"-",1);

    debug_printf ("strip_invalid_chars() w_in (pre):\n");
    for (wstrp = w_in; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("strip_invalid_chars() w_in (pre #2):\n");
    for (i = 0; i < t; i++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");
    debug_printf ("strip_invalid_chars() w_invalid:\n");
    for (wstrp = w_invalid; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");

    /* Replace illegals to legal */
    for (wstrp = w_in; *wstrp; wstrp++) {
	if ((wcschr(w_invalid, *wstrp) == NULL) && (!iswcntrl(*wstrp)))
	    continue;
	*wstrp = replacement;
    }

    debug_printf ("strip_invalid_chars() w_in (post):\n");
    for (wstrp = w_in; *wstrp; wstrp++) {
	debug_printf ("%04x ",*wstrp&0x0ffff);
    }
    debug_printf ("\n");

    /* Convert back to multibyte */
    wcstombs(mb_in,w_in,mb_in_len);

    debug_printf ("strip_invalid_chars() mb_in (post):\n");
    debug_printf (mb_in);
    debug_printf ("\n");
    for (strp = mb_in; *strp; strp++) {
	debug_printf ("%02x ",*strp&0x0ff);
    }
    debug_printf ("\n");

    free (w_in);
    free (w_invalid);

    return str;
}
#endif /* HAVE_WCHAR_T */

char*
strip_invalid_chars_no_wchar(char *str)
{
#if defined (WIN32)
    char invalid_chars[] = "\\/:*?\"<>|~";
#else
    char invalid_chars[] = "\\/:*?\"<>|.~";
#endif
    char *oldstr = str;						
    char *newstr = str;

    if (!str) return NULL;

    for (;*oldstr; oldstr++) {
	if (strchr(invalid_chars, *oldstr) != NULL)
		continue;
	*newstr = *oldstr;
	newstr++;
    }
    *newstr = '\0';
    return str;
}

void
parse_artist_title (char* artist, char* title, char* album, 
		    int bufsize, char* trackname)
{
    char *p1,*p2;

    /* Parse artist, album & title. Look for a '-' in the track name,
     * i.e. Moby - sux0rs (artist/track)
     */
    memset(album, '\000', bufsize);
    memset(artist, '\000', bufsize);
    memset(title, '\000', bufsize);
    p1 = strchr(trackname, '-');
    if (p1) {
	strncpy(artist, trackname, p1-trackname);
	p1++;
	p2 = strchr(p1, '-');
	if (p2) {
	    if (*p1 == ' ') {
		p1++;
	    }
	    strncpy(album, p1, p2-p1);
	    p2++;
	    if (*p2 == ' ') {
		p2++;
	    }
	    strcpy(title, p2);
	} else {
	    if (*p1 == ' ') {
		p1++;
	    }
	    strcpy(title, p1);
	}
    } else {
        strcpy(artist, trackname);
        strcpy(title, trackname);
    }
}

char* 
strip_invalid_chars(char *str)
{
#if HAVE_WCHAR_T
#if defined (NEW_I18N_CODE)
    return strip_invalid_chars_testing(str);
#else
    return strip_invalid_chars_stable(str);
#endif
#else
    return strip_invalid_chars_no_wchar(str);
#endif
}

char *format_byte_size(char *str, long size)
{
    const long ONE_K = 1024;
    const long ONE_M = ONE_K*ONE_K;

    if (size < ONE_K)
	sprintf(str, "%ldb", size);
    else if (size < ONE_M)
	sprintf(str, "%ldkb", size/ONE_K);
    else 
	sprintf(str, "%.2fM", (float)size/(ONE_M));
	
    return str;
}

void trim(char *str)
{
    int size = strlen(str)-1;
    while(str[size] == 13 || str[size] == 10 || str[size] == ' ')
    {
	str[size] = '\0';
	size--;
    }
    size = strlen(str);
    while(str[0] == ' ')
    {
	size--;
	memmove(str, str+1, size);
    }
    str[size] = '\0';
}

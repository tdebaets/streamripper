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
#if defined (__UNIX__)
#include <unistd.h>
#endif
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <time.h>
#include <iconv.h>
#include <langinfo.h>
#include "debug.h"
#include "types.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
char		*escape_string_alloc(const char *str);
char		*left_str(char *str, int len);
char		*strip_last_word(char *str);
int			word_count(char *str);
char		*subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char		*strip_invalid_chars(char *str);
char		*format_byte_size(char *str, long size);
char		*add_trailing_slash(char *str);
void		trim(char *str);
void 		null_printf(char *s, ...);

char *add_trailing_slash(char *str)
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


char *escape_string_alloc(const char *str)
{
    static const char c2x_table[] = "0123456789abcdef";
    const unsigned char *spStr = (const unsigned char *)str;
    char *sNewStr = (char *)calloc(3 * strlen(str) + 1, sizeof(char));
    unsigned char *spNewStr = (unsigned char *)sNewStr;
    unsigned c;

    while ((c = *spStr)) {
        if (c < 'A' || c > 'z') 
		{
			*spNewStr++ = '%';
			*spNewStr++ = c2x_table[c >> 4];
			*spNewStr++ = c2x_table[c & 0xf];
        }
        else 
		{
            *spNewStr++ = c;
        }
        ++spStr;
    }
    *spNewStr = '\0';
    return sNewStr;
}

char *left_str(char *str, int len)
{
	int slen = strlen(str);

	if (slen <= len)
		return str;

	str[len] = '\0';
	return str;
}

char *strip_last_word(char *str)
{
	int len = strlen(str)-1;

	while(str[len] != ' ' && len != 0)
		len--;

	str[len] = '\0';
	return str;
}

int word_count(char *str)
{
	int n = 0;
	char *p = str;

	if (!*p)
		return 0; 

	while(*p++)
		if (*p == ' ')
			n++;

	return n+1;
}

void
initialize_locale (void)
{
    char* locale_codeset;
    setlocale (LC_ALL, "");
    setlocale (LC_CTYPE, "");
    debug_printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));
    locale_codeset = nl_langinfo(CODESET);
    debug_printf ("LOCALE CODESET is %s\n", locale_codeset);
}


/* Given a multibyte string containing the title, three names are 
   suggested.  One in utf8 encoding, one in the multibyte encoding 
   of the locale, and one is a "guaranteed-to-work" ascii name.
*/
void
suggest_filenames (char *title_string, char *utf8_name, 
		   char *locale_name, char *ascii_name)
{
}

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
    int i;
    size_t t;

    iconv_t ict;
    size_t inleft, outleft;
    char *fromcode = nl_langinfo(CODESET);

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

#if defined (commentout)
    /* GCS - This is the old code.  Keep for reference until the
       above code is well tested */
	char invalid_chars[] = "\\/:*?\"<>|.~";
	char *oldstr = str;						
	char *newstr = str;
	
	if (!str)
		return NULL;

	for(;*oldstr; oldstr++)
	{
		if (strchr(invalid_chars, *oldstr) != NULL)
			continue;

		*newstr = *oldstr;
		newstr++;
	}
	*newstr = '\0';

	return str;
#endif
}

char* 
strip_invalid_chars(char *str)
{
    return strip_invalid_chars_stable(str);
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

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
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <time.h>
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
#if defined (commentout)
    static int initialized = 0;
    if (!initialized) {
	initialized = 1;
	setlocale (LC_ALL, "");
	setlocale (LC_CTYPE, "");
	printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));
    }
#endif
    setlocale (LC_ALL, "");
    setlocale (LC_CTYPE, "");
    printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));
}

char*
strip_invalid_chars(char *str)
{
    //    int i;
    char invalid_chars[] = "\\/:*?\"<>|.~";
    char* mb_in = str;
    int mb_in_len = strlen(mb_in);
    wchar_t *w_in = (wchar_t*) malloc (2*mb_in_len+2);
    wchar_t *w_invalid = (wchar_t*) malloc(2*strlen(invalid_chars)+2);
    wchar_t replacement;
    wchar_t *wstrp;

#if defined (commentout)
    printf (mb_in);
    printf ("\n");
    for (strp = mb_in; *strp; strp++) {
	printf ("%02x ",*strp&0x0ff);
    }
    printf ("\n");
#endif

    /* Convert invalid chars to wide char */
    mbstowcs(w_invalid,invalid_chars,strlen(invalid_chars)+1);

    /* Convert title string to wchar */
    mbstowcs(w_in,mb_in,mb_in_len+1);

    /* Convert "replacement" to wide */
    mbtowc (&replacement,"-",1);

#if defined (commentout)
    for (wstrp = w_in; *wstrp; wstrp++) {
	printf ("%04x ",*wstrp&0x0ffff);
    }
    printf ("\n");
#endif

    /* Replace illegals to legal */
    for (wstrp = w_in; *wstrp; wstrp++) {
	if ((wcschr(w_invalid, *wstrp) == NULL) && (!iswcntrl(*wstrp)))
	    continue;
	*wstrp = replacement;
    }

    /* Convert back to multibyte */
    wcstombs(mb_in,w_in,mb_in_len);
    return str;

#if defined (commentout)    
    printf (mb_in);
#endif

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

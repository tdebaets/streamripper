#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include "util.h"


int
main (int argc, char* argv[])
{
    char* kanji_sjis = "\x8a\xbf\x8e\x9a";
    char cvt_buf[1024];
    initialize_default_locale ();
    set_codeset ("CODESET_ALL", "SHIFT-JIS");

    iconv_convert_string (utf_buf, 1023, kanji_sjis, 
			  "EUCJP", "SJIS");

    printf ("ori=%s\n", kanji_sjis);
    printf ("cvt=%s\n", utf_buf);
    return 0;
}


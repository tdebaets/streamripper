#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#define MINIMAL 1

#if (MINIMAL)

void
run_test (char* from_codeset, char* to_codeset) {
    GIConv giconv;
    GError *error = 0;
    gchar* out;

    printf ("Converting %s -> %s\n", from_codeset, to_codeset);
    giconv = g_iconv_open (to_codeset, from_codeset);
    if (giconv == (GIConv) -1) {
	printf ("g_iconv_open error\n");
	return;
    }

    out = g_convert_with_iconv ("A", 1, giconv, 0, 0, &error);

    if (error && error->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE) {
	printf ("Illegal sequence\n");
    } else {
	printf ("Successful conversion: %s\n", out);
    }
    g_iconv_close (giconv);
}

int main (int argc, char* argv[]) {

    if (argc == 1) {
	run_test ("CP1252", "UTF-8");
	run_test ("UTF-8", "CP1252");
	run_test ("UTF-8", "US-ASCII");
    } else if (argc == 3) {
	char* from_codeset = argv[1];
	char* to_codeset = argv[2];
	run_test (from_codeset, to_codeset);
    } else {
	printf ("Usage: glib_iconv_test [from_codeset to_codeset]\n");
	exit (0);
    }

    return 0;
}

#else

/* Convert a string, replacing unconvertable characters.
   Returns a gchar* string, which must be freed by caller using g_free. */
gchar*
convert_string (char* instring, gsize len, 
		char* from_codeset, char* to_codeset)
{
    GIConv giconv;
    GError *error = 0;
    gchar* os;
    gsize br, bw;

    giconv = g_iconv_open (to_codeset, from_codeset);
    if (giconv == (GIConv)-1) {
	printf ("g_iconv_open error\n");
	return 0;
    }

    os = g_convert_with_iconv (instring, len, 
			       giconv, &br, &bw, &error);

    if (error) {
	switch (error->code) {
	case G_CONVERT_ERROR_ILLEGAL_SEQUENCE:
	    printf ("Illegal sequence\n");
	    return 0;
	case G_CONVERT_ERROR_FAILED:
	case G_CONVERT_ERROR_PARTIAL_INPUT:
	case G_CONVERT_ERROR_NO_CONVERSION:
	default:
	    printf ("other error\n");
	    return 0;
	}
    }
    g_iconv_close (giconv);
    return os;
}

#if (0)
void
test_library (void)
    hlibiconv = LoadLibrary(dllname);
01085         if (hlibiconv != NULL)
01086         {
01087             if (hlibiconv == hwiniconv)
01088             {
01089                 FreeLibrary(hlibiconv);
01090                 hlibiconv = NULL;
01091                 continue;
01092             }
01093             break;
01094         }
#endif

main () {
    char instring[] = { 'A', '\0' };
    gchar* os;

    // "ISO-8859-15", "US-ASCII"
    char *to_codeset, *from_codeset;
    to_codeset = "US-ASCII";
    to_codeset = "UTF-8";
    to_codeset = "CP1252";
    from_codeset = "CP1252";
    from_codeset = "UTF-8";

    os = convert_string (instring, strlen(instring), 
			 from_codeset, to_codeset);
    if (os) {
	printf ("os = %s\n", os);
	g_free (os);
    }
}
#endif

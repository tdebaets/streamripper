/* prefs.c
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
#include <string.h>

#include "rip_manager.h"
#include "mchar.h"
#include "prefs.h"
#include "debug.h"

#define MAX_INI_LINE_LEN	1024

/******************************************************************************
 * Private Vars
 *****************************************************************************/

/* Return 0 for EOF.  Set key[0] to 0 if not a key/value pair. */
int 
parse_line (FILE* fp, char* section, char* key, char* value)
{
    char linebuf[MAX_INI_LINE_LEN];
    char tmp[MAX_INI_LINE_LEN];
    char *p;

    key[0] = 0;
    value[0] = 0;

    if (!fgets (linebuf, MAX_INI_LINE_LEN, fp)) {
	return 0;
    }

    /* Check for [section] */
    if (linebuf[0] == '[') {
	if ((p = strrchr (&linebuf[1], ']')) != 0) {
	    *p = 0;
	    strcpy (section, &linebuf[1]);
	}
    }

    /* Check for key=value */
    else if ((p = strchr (linebuf, '=')) != 0) {
	*p = 0;
	trim (linebuf);
	trim (++p);
	if (strlen(linebuf) > 0 && strlen(p) > 0) {
	    strcpy (key, linebuf);
	    strcpy (value, p);
	}
    }

    return 1;
}

int 
load_prefs (char* prefs_fn, GLOBAL_PREFS* gp)
{
    FILE* fp;
    char section[MAX_INI_LINE_LEN];
    char key[MAX_INI_LINE_LEN];
    char value[MAX_INI_LINE_LEN];

    if ((fp = fopen (prefs_fn, "r")) == 0) {
        return 0;
    }

    strcpy (section, "global");
    while (parse_line (fp, section, key, value)) {
	if (!key[0]) {
	    /* Might be new section */
	} else {
	    /* Change value for existing section */
	}
    }
    return 1;
}

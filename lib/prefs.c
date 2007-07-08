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

/* Return 0 for EOF.
   Return 1 for new section.
   Return 2 for key/value pair.
   Return 3 for no data (blank line/comment).
*/
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
	    return 1;
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
	    return 2;
	}
    }

    return 3;
}

int 
load_prefs (char* prefs_fn, PREFS_LIST* prefs)
{
    int rc;
    FILE* fp;
    char section[MAX_INI_LINE_LEN];
    char key[MAX_INI_LINE_LEN];
    char value[MAX_INI_LINE_LEN];
    GList *i;
    PREFS *cur;

    if ((fp = fopen (prefs_fn, "r")) == 0) {
        return 0;
    }

    /* Initialize linked list */
    prefs->m_list.head = 0;
    prefs->m_list.tail = 0;
    prefs->m_list.length = 0;

    strcpy (section, "global");  /* If there is no section, it is global */
    while (1) {
	switch (parse_line (fp, section, key, value)) {
	case 0:
	    /* End of file */
	    fclose (fp);
	    return 0;
	case 1:
	    /* New section heading */
	    /* GCS FIX: check for global */
	    cur = 0;
	    for (i = prefs->m_list.head; i; g_slist_next (i)) {
		PREFS* p = (PREFS*) i->data;
		if (!strcmp (p->label, section)) {
		    cur = p;
		    break;
		}
	    }
	    if (!cur) {
		PREFS* cur = (PREFS*) g_memdup (&prefs->m_global_prefs, 
						(sizeof (PREFS)));
		g_queue_push_tail (&prefs->m_list, cur);
	    }
	    break;
	case 2:
	    /* Key value pair */
	    break;
	case 3:
	    /* No data, try next line */
	    break;
	}
    }
    return 0;
}

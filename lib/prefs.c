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
 *
 * REFS
 * http://www.gtkbook.com/tutorial.php?page=keyfile
 */
#include <stdio.h>
#include <string.h>
//#include <glib.h>

#include "rip_manager.h"
#include "mchar.h"
#include "prefs.h"
#include "debug.h"

/******************************************************************************
 * Private Vars
 *****************************************************************************/
static GKeyFile *g_key_file = NULL;

/******************************************************************************
 * Private Vars
 *****************************************************************************/
void
prefs_load (char* prefs_fn)
{
    gboolean rc;
    GKeyFileFlags flags;
    GError *error = NULL;

    if (!g_key_file) {
	g_key_file = g_key_file_new ();
    }
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
    rc = g_key_file_load_from_file (g_key_file, prefs_fn,
				    flags, &error);
    if (!rc) {
	/* No key file */
	// g_error (error->message);
    }
}

void
prefs_get_defaults (PREFS* prefs)
{
    debug_printf ("- set_rip_manager_options_defaults -\n");
    prefs->url[0] = 0;
    prefs->proxyurl[0] = 0;
    strcpy(prefs->output_directory, "./");
    prefs->output_pattern[0] = 0;
    prefs->showfile_pattern[0] = 0;
    prefs->if_name[0] = 0;
    prefs->rules_file[0] = 0;
    prefs->pls_file[0] = 0;
    prefs->relay_ip[0] = 0;
    prefs->relay_port = 8000;
    prefs->max_port = 18000;
    prefs->max_connections = 1;
    prefs->maxMB_rip_size = 0;
    prefs->flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPERATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    /* OPT_ADD_ID3V1 | -- removed starting 1.62-beta-2 */
	    OPT_ADD_ID3V2 |
	    OPT_INDIVIDUAL_TRACKS;
    strcpy(prefs->useragent, "sr-POSIX/" SRVERSION);

    // Defaults for splitpoint - times are in ms
    prefs->sp_opt.xs = 1;
    prefs->sp_opt.xs_min_volume = 1;
    prefs->sp_opt.xs_silence_length = 1000;
    prefs->sp_opt.xs_search_window_1 = 6000;
    prefs->sp_opt.xs_search_window_2 = 6000;
    prefs->sp_opt.xs_offset = 0;
    prefs->sp_opt.xs_padding_1 = 300;
    prefs->sp_opt.xs_padding_2 = 300;

    /* GCS FIX: What is the difference between this timeout 
       and the one used in setsockopt()? */
#if defined (commentout)
    prefs->timeout = 0;
#endif
    prefs->timeout = 15;
    prefs->dropcount = 0;

    // Defaults for codeset
    memset (&prefs->cs_opt, 0, sizeof(CODESET_OPTIONS));
    set_codesets_default (&prefs->cs_opt);

    prefs->count_start = 0;
    prefs->overwrite = OVERWRITE_LARGER;
    prefs->ext_cmd[0] = 0;
}

void
prefs_get_string (char* dest, gsize dest_size, char* group, char* key)
{
    GError *error = NULL;
    gchar *value;
    
    value = g_key_file_get_string (g_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	// g_error (error->message);
	return;
    }
    if (g_strlcpy (dest, value, dest_size) >= dest_size) {
	/* Value too loing, silently truncate */
    }
    g_free (value);
}

void
prefs_get_ushort (u_short *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (g_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	// g_error (error->message);
	return;
    }
    if (value < 0 || value > G_MAXUSHORT) {
	/* Bad value, silently ignore (?) */
	return;
    }
    *dest = (u_short) value;
}

void
prefs_get_ulong (u_long *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (g_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	// g_error (error->message);
	return;
    }
    if (value < 0) {
	/* Bad value, silently ignore (?) */
	return;
    }
    *dest = (u_long) value;
}

void
prefs_get_section (PREFS* prefs, char* label)
{
    char* group = 0;
    if (!g_key_file) return;
    if (g_key_file_has_group (g_key_file, label)) {
	group = label;
    } else {
	/* Look at all groups for matching URL */
	return;
    }

    prefs_get_string (prefs->url, MAX_URL_LEN, group, "url");
    prefs_get_string (prefs->proxyurl, MAX_URL_LEN, group, "proxyurl");
    prefs_get_string (prefs->output_directory, SR_MAX_PATH, group, "output_directory");
    prefs_get_string (prefs->output_pattern, SR_MAX_PATH, group, "output_pattern");
    prefs_get_string (prefs->showfile_pattern, SR_MAX_PATH, group, "showfile_pattern");
    prefs_get_string (prefs->if_name, SR_MAX_PATH, group, "if_name");
    prefs_get_string (prefs->rules_file, SR_MAX_PATH, group, "rules_file");
    prefs_get_string (prefs->pls_file, SR_MAX_PATH, group, "pls_file");
    prefs_get_string (prefs->relay_ip, SR_MAX_PATH, group, "relay_ip");
    prefs_get_ushort (&prefs->relay_port, group, "relay_port");
    prefs_get_ushort (&prefs->max_port, group, "max_port");
    prefs_get_ulong (&prefs->max_connections, group, "max_connections");
    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_rip_size");

    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_rip_size");
    /* Flags */
    prefs->flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPERATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    /* OPT_ADD_ID3V1 | -- removed starting 1.62-beta-2 */
	    OPT_ADD_ID3V2 |
	    OPT_INDIVIDUAL_TRACKS;

    prefs_get_string (prefs->useragent, MAX_USERAGENT_STR, group, "useragent");

    // Defaults for splitpoint - times are in ms
    prefs->sp_opt.xs = 1;
    prefs->sp_opt.xs_min_volume = 1;
    prefs->sp_opt.xs_silence_length = 1000;
    prefs->sp_opt.xs_search_window_1 = 6000;
    prefs->sp_opt.xs_search_window_2 = 6000;
    prefs->sp_opt.xs_offset = 0;
    prefs->sp_opt.xs_padding_1 = 300;
    prefs->sp_opt.xs_padding_2 = 300;

    /* GCS FIX: What is the difference between this timeout 
       and the one used in setsockopt()? */
#if defined (commentout)
    prefs->timeout = 0;
#endif
    prefs->timeout = 15;
    prefs->dropcount = 0;

    // Defaults for codeset
    memset (&prefs->cs_opt, 0, sizeof(CODESET_OPTIONS));
    set_codesets_default (&prefs->cs_opt);

    prefs->count_start = 0;
    prefs->overwrite = OVERWRITE_LARGER;
    prefs->ext_cmd[0] = 0;
}

void
prefs_get (PREFS* prefs, char* label)
{
    prefs_get_defaults (prefs);
    prefs_get_section (prefs, "global");
    prefs_get_section (prefs, label);
}

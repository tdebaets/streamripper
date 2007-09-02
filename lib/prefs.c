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
#include <stdlib.h>
#include <string.h>
//#include <glib.h>

#include "rip_manager.h"
#include "mchar.h"
#include "prefs.h"
#include "debug.h"

/******************************************************************************
 * Private variables
 *****************************************************************************/
static GKeyFile *m_key_file = NULL;

/******************************************************************************
 * Private function protoypes
 *****************************************************************************/
void prefs_get_defaults (PREFS* prefs);
void prefs_get_section (PREFS* prefs, char* label);
static gchar* prefs_get_config_dir (void);

/******************************************************************************
 * Public functions
 *****************************************************************************/
void
prefs_load (void)
{
    gboolean rc;
    GKeyFileFlags flags;
    GError *error = NULL;
    gchar* prefs_dir;
    gchar* prefs_fn;

    prefs_dir = prefs_get_config_dir ();
    prefs_fn = g_build_filename (prefs_dir,
				 "streamripper.ini",
				 NULL);

    if (!m_key_file) {
	m_key_file = g_key_file_new ();
    }
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
    rc = g_key_file_load_from_file (m_key_file, prefs_fn,
				    flags, &error);

    g_free (prefs_fn);
    g_free (prefs_dir);
}

void
prefs_set (PREFS* prefs, char* label)
{
}

void
prefs_get (PREFS* prefs, char* label)
{
    prefs_get_defaults (prefs);
    prefs_get_section (prefs, "global");
    prefs_get_section (prefs, label);

    //    printf ("Home dir is: %s\n", g_get_home_dir());
    //    printf ("Config dir is: %s\n", g_get_user_config_dir ());
    //    printf ("Data dir is: %s\n", g_get_user_data_dir ());
    exit (0);
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
/* Calling routine must free */
static gchar* 
prefs_get_config_dir (void)
{
    return g_build_filename (g_get_user_config_dir(), 
			     "streamripper",
			     NULL);
}

void
prefs_get_defaults (PREFS* prefs)
{
    debug_printf ("- set_rip_manager_options_defaults -\n");
    prefs->url[0] = 0;
    prefs->proxyurl[0] = 0;
    strcpy(prefs->output_directory, "./");
    /* GCS FIX: Plugin defaults to desktop */
#if defined (commentout)
    if (rmo->output_directory[0] == 0) {
	strcpy (rmo->output_directory, desktop_path);
    }
#endif
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

/* Return 0 if value not found, 1 if value found */
int
prefs_get_string (char* dest, gsize dest_size, char* group, char* key)
{
    GError *error = NULL;
    gchar *value;
    
    value = g_key_file_get_string (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	return 0;
    }
    if (g_strlcpy (dest, value, dest_size) >= dest_size) {
	/* Value too loing, silently truncate */
    }
    g_free (value);
    return 1;
}

void
prefs_get_ushort (u_short *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
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

/* Return 0 if value not found, 1 if value found */
int
prefs_get_ulong (u_long *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	return 0;
    }
    if (value < 0) {
	/* Bad value, silently ignore (?) */
	return 0;
    }
    *dest = (u_long) value;
    return 1;
}

void
prefs_get_int (int *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	return;
    }
    *dest = (u_long) value;
}

/* Return 0 if value not found, 1 if value found */
int
prefs_get_bool (u_long *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist */
	return 0;
    }
    *dest = (value != 0);
    return 1;
}

#if defined (commentout)
BOOL
options_load (RIP_MANAGER_OPTIONS *rmo, GUI_OPTIONS *guiOpt)
{
    int i, p;
    char desktop_path[MAX_INI_LINE_LEN];
    char filename[MAX_INI_LINE_LEN];
    BOOL    auto_reconnect,
	    make_relay,
	    add_id3_sr161,    /* For loading prefs from 1.61 and earlier */
	    add_id3v1,
	    add_id3v2,
	    check_max_btyes,
	    keep_incomplete,
	    rip_individual_tracks,
	    rip_single_file,
	    use_ext_cmd;
    char overwrite_string[MAX_INI_LINE_LEN];

    if (!get_desktop_folder(desktop_path)) {
	debug_printf ("get_desktop_folder() failed\n");
	desktop_path[0] = '\0';
    }

    if (!winamp_get_path(filename)) {
	debug_printf ("winamp_get_path failed #2\n");
	return FALSE;
    }

    strcat (filename, "Plugins\\sripper.ini");
    debug_printf ("Loading filename is %s\n", filename);

    /* PLUGIN SPECIFIC STUFF */
    GetPrivateProfileString(APPNAME, "localhost", "localhost", guiOpt->localhost, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "default_skin", DEFAULT_SKINFILE, guiOpt->default_skin, MAX_INI_LINE_LEN, filename);
    if (guiOpt->default_skin[0] == 0) {
        strcpy(guiOpt->default_skin, DEFAULT_SKINFILE);
    }
    guiOpt->m_add_finshed_tracks_to_playlist = GetPrivateProfileInt(APPNAME, "add_tracks_to_playlist", FALSE, filename);
    guiOpt->m_start_minimized = GetPrivateProfileInt(APPNAME, "start_minimized", FALSE, filename);
    guiOpt->oldpos.x = GetPrivateProfileInt(APPNAME, "window_x", 0, filename);
    guiOpt->oldpos.y = GetPrivateProfileInt(APPNAME, "window_y", 0, filename);
    guiOpt->m_enabled = GetPrivateProfileInt(APPNAME, "enabled", 1, filename);
    guiOpt->use_old_playlist_ret = GetPrivateProfileInt(APPNAME, "use_old_playlist_ret", 0, filename);
    if (guiOpt->oldpos.x < 0 || guiOpt->oldpos.y < 0)
	guiOpt->oldpos.x = guiOpt->oldpos.y = 0;

    /* UNDECIDED */
    rmo->max_port = rmo->relay_port+1000;
    debug_printf ("Got PPS: %s\n", overwrite_string);
    rmo->flags |= OPT_SEARCH_PORTS;	// not having this caused a bad bug, must remember this.

    /* Note, there is no way to change the rules file location (for now) */
    if (!winamp_get_path(rmo->rules_file)) {
	debug_printf ("winamp_get_path failed #3\n");
	return FALSE;
    }
    strcat (rmo->rules_file, "Plugins\\parse_rules.txt");
    debug_printf ("RULES: %s\n", rmo->rules_file);

    /* Get history */
    for (i = 0, p = 0; i < RIPLIST_LEN; i++) {
	char profile_name[128];
	sprintf (profile_name, "riplist%d", i);
	GetPrivateProfileString (APPNAME, profile_name, "", guiOpt->riplist[p], MAX_INI_LINE_LEN, filename);
	if (guiOpt->riplist[p][0]) {
	    p++;
	}
    }


    return TRUE;
}
#endif

void
prefs_get_section (PREFS* prefs, char* label)
{
    char* group = 0;
    char overwrite_str[128];
    u_long temp;

    if (!m_key_file) return;
    if (g_key_file_has_group (m_key_file, label)) {
	group = label;
    } else {
	/* Look at all groups for matching URL */
	return;
    }

    prefs_get_string (prefs->url, MAX_URL_LEN, group, "url");
    prefs_get_string (prefs->proxyurl, MAX_URL_LEN, group, "proxy");
    prefs_get_string (prefs->output_directory, SR_MAX_PATH, group, "output_dir");
    prefs_get_string (prefs->output_pattern, SR_MAX_PATH, group, "output_pattern");
    prefs_get_string (prefs->showfile_pattern, SR_MAX_PATH, group, "showfile_pattern");
    prefs_get_string (prefs->if_name, SR_MAX_PATH, group, "if_name");
    prefs_get_string (prefs->rules_file, SR_MAX_PATH, group, "rules_file");
    prefs_get_string (prefs->pls_file, SR_MAX_PATH, group, "pls_file");
    prefs_get_string (prefs->relay_ip, SR_MAX_PATH, group, "relay_ip");
    prefs_get_string (prefs->useragent, MAX_USERAGENT_STR, group, "useragent");
    prefs_get_string (prefs->ext_cmd, SR_MAX_PATH, group, "ext_cmd");
    prefs_get_ushort (&prefs->relay_port, group, "relay_port");
    prefs_get_ushort (&prefs->max_port, group, "max_port");
    prefs_get_ulong (&prefs->max_connections, group, "max_connections");
    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_bytes");
    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_bytes");
    prefs_get_ulong (&prefs->dropcount, group, "dropcount");

    /* Overwrite */
    if (prefs_get_string (overwrite_str, 128, group, "over_write_complete")) {
	prefs->overwrite = string_to_overwrite_opt (overwrite_str);
    }

    /* Flags */
    if (prefs_get_ulong (&temp, group, "auto_reconnect")) {
	OPT_FLAG_SET (prefs->flags, OPT_AUTO_RECONNECT, temp);
    }
    if (prefs_get_ulong (&temp, group, "make_relay")) {
	OPT_FLAG_SET (prefs->flags, OPT_MAKE_RELAY, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3")) {
	/* Obsolete */
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V1, temp);
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V2, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3v1")) {
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V1, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3v2")) {
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V2, temp);
    }
    if (prefs_get_ulong (&temp, group, "check_max_bytes")) {
	OPT_FLAG_SET (prefs->flags, OPT_CHECK_MAX_BYTES, temp);
    }
    if (prefs_get_ulong (&temp, group, "keep_incomplete")) {
	OPT_FLAG_SET (prefs->flags, OPT_KEEP_INCOMPLETE, temp);
    }
    if (prefs_get_ulong (&temp, group, "rip_individual_tracks")) {
	OPT_FLAG_SET (prefs->flags, OPT_INDIVIDUAL_TRACKS, temp);
    }
    if (prefs_get_ulong (&temp, group, "rip_single_file")) {
	OPT_FLAG_SET (prefs->flags, OPT_SINGLE_FILE_OUTPUT, temp);
    }
    if (prefs_get_ulong (&temp, group, "use_ext_cmd")) {
	OPT_FLAG_SET (prefs->flags, OPT_EXTERNAL_CMD, temp);
    }

    /* Splitpoint options */
    prefs_get_int (&prefs->sp_opt.xs_offset, group, "xs_offset");
    prefs_get_int (&prefs->sp_opt.xs_silence_length, group, "xs_silence_length");
    prefs_get_int (&prefs->sp_opt.xs_search_window_1, group, "xs_search_window_1");
    prefs_get_int (&prefs->sp_opt.xs_search_window_2, group, "xs_search_window_2");
    prefs_get_int (&prefs->sp_opt.xs_padding_1, group, "xs_padding_1");
    prefs_get_int (&prefs->sp_opt.xs_padding_2, group, "xs_padding_2");

    /* Codesets */
    prefs_get_string (prefs->cs_opt.codeset_metadata, MAX_CODESET_STRING, group, "codeset_metadata");
    prefs_get_string (prefs->cs_opt.codeset_relay, MAX_CODESET_STRING, group, "codeset_relay");
    prefs_get_string (prefs->cs_opt.codeset_id3, MAX_CODESET_STRING, group, "codeset_id3");
    prefs_get_string (prefs->cs_opt.codeset_filesys, MAX_CODESET_STRING, group, "codeset_filesys");
}

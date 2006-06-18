/* winamp.c
 * gets info from winamp in various hacky ass ways
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

#include <windows.h>
#include <stdio.h>

#include "srtypes.h"
#include "winamp.h"
#include "debug.h"
#include "util.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL winamp_init();			
BOOL winamp_get_info(WINAMP_INFO *info, BOOL useoldway);
BOOL winamp_add_track_to_playlist(char *track);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static char m_winamps_path[SR_MAX_PATH] = {'\0'};

// Get's winamp's path from the reg key ..
// HKEY_CLASSES_ROOT\Applications\winamp.exe\shell\Enqueue\command
BOOL
get_string_from_registry (char *path, HKEY hkey, LPCTSTR subkey, LPTSTR name)
{
    LONG rc;
    HKEY hkey_result;
    DWORD size = SR_MAX_PATH;
    char strkey[SR_MAX_PATH];
    int i;
    DWORD type;
    int k = REG_SZ;

    debug_printf ("Trying RegOpenKeyEx: 0x%08x %s\n", hkey, subkey);
    rc = RegOpenKeyEx(hkey, subkey, 0, KEY_QUERY_VALUE, &hkey_result);
    if (rc != ERROR_SUCCESS) {
	return FALSE;
    }

    debug_printf ("Trying RegQueryValueEx: %s\n", name);
    rc = RegQueryValueEx (hkey_result, name, NULL, &type, (LPBYTE)strkey, &size);
    if (rc != ERROR_SUCCESS) {
	debug_printf ("Return code = %d\n", rc);
	RegCloseKey (hkey_result);
	return FALSE;
    }

    debug_printf ("RegQueryValueEx succeeded: %d\n", type);
    for(i = 0; strkey[i]; i++) {
	path[i] = toupper(strkey[i]);
    }

    RegCloseKey (hkey_result);

    return TRUE;
}

BOOL
strip_registry_path (char* path, char* tail)
{
    int i = 0;
    int tail_len = strlen(tail);

    /* Skip the leading quote */
    debug_printf ("Stripping registry path: %s\n", path);
    if (path[0] == '\"') {
	for (i = 1; path[i]; i++) {
	    path[i-1] = path[i];
	}
	path[i-1] = path[i];
        debug_printf ("Stripped quote mark: %s\n", path);
    }

    /* Search for, and strip, the tail */
    i = 0;
    while (path[i]) {
	if (strncmp (&path[i], tail, tail_len) == 0) {
	    path[i] = 0;
	    debug_printf ("Found path: %s (%s)\n", path, tail);
	    return TRUE;
	}
	i++;
    }

    debug_printf ("Did not find path\n");
    return FALSE;
}

BOOL
winamp_get_path(char *path)
{
    BOOL rc;
    char* sr_winamp_home_env;

    sr_winamp_home_env = getenv ("STREAMRIPPER_WINAMP_HOME");
    debug_printf ("STREAMRIPPER_WINAMP_HOME = %s\n", sr_winamp_home_env);
    if (sr_winamp_home_env) {
	sr_strncpy (path, sr_winamp_home_env, SR_MAX_PATH);
	return TRUE;
    }

    rc = get_string_from_registry (path, HKEY_LOCAL_MACHINE, 
	    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Winamp"),
	    TEXT("UninstallString"));
    if (rc == TRUE) {
	rc = strip_registry_path (path, "UNINSTWA.EXE");
	if (rc == TRUE) return TRUE;
    }

    rc = get_string_from_registry (path, HKEY_CLASSES_ROOT, 
	    "Winamp.File\\shell\\Enqueue\\command", NULL);
    if (rc == TRUE) {
	rc = strip_registry_path (path, "WINAMP.EXE");
	if (rc == TRUE) return TRUE;
    }

    /* The alternative is: */
/* SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINIDIRECTORY);
    #define IPC_GETINIDIRECTORY 335
    (Requires winamp 5.0)
**
** This returns a pointer to the directory where winamp.ini can be found and is
** useful if you want store config files but you don't want to use winamp.ini.
*/

    return FALSE;
}

BOOL
winamp_init ()
{
    // Not implemented
    return winamp_get_path(m_winamps_path);
}

#define DbgBox(_x_)	MessageBox(NULL, _x_, "Debug", 0)

BOOL
winamp_get_info (WINAMP_INFO *info, BOOL useoldway)
{
    HWND hwndWinamp;
    info->url[0] = '\0';

    hwndWinamp = FindWindow("Winamp v1.x", NULL);

    /* Get winamp path */
    if (!m_winamps_path[0])
	return FALSE;

    if (!hwndWinamp) {
	info->is_running = FALSE;
	return FALSE;
    } else {
	info->is_running = TRUE;
    }

    if (useoldway) {
	// Send a message to winamp to save the current playlist
	// to a file, 'n' is the index of the currently selected item
	int n  = SendMessage (hwndWinamp, WM_USER, (WPARAM)NULL, 120); 
	char m3u_path[SR_MAX_PATH];
	char buf[4096] = {'\0'};
	FILE *fp;

	sprintf (m3u_path, "%s%s", m_winamps_path, "winamp.m3u");	
	if ((fp = fopen (m3u_path, "r")) == NULL)
	    return FALSE;

	while(!feof(fp) && n >= 0)
	{
	    fgets(buf, 4096, fp);
	    if (*buf != '#')
		n--;
	}
	fclose(fp);
	buf[strlen(buf)-1] = '\0';

	// Make sure it's a URL
	if (strncmp (buf, "http://", strlen("http://")) == 0)
	    strcpy (info->url, buf);
    } else {
	// Much better way to get the filename
	int get_filename = 211;
	int get_position = 125;
	int data = 0;
	int pos;
	char* fname;
	char *purl;

	pos = (int)SendMessage (hwndWinamp, WM_USER, data, get_position);
	fname = (char*)SendMessage (hwndWinamp, WM_USER, pos, get_filename);
	if (fname == NULL)
	    return FALSE;
	purl = strstr (fname, "http://");
	if (purl)
	    strncpy (info->url, purl, MAX_URL_LEN);
    }

    return TRUE;
}

BOOL
winamp_add_relay_to_playlist (char *host, u_short port, int content_type)
{
    char relay_file[SR_MAX_PATH];
    char winamp_path[SR_MAX_PATH];

    sprintf (winamp_path, "%s%s", m_winamps_path, "winamp.exe");
    if (content_type == CONTENT_TYPE_OGG) {
	sprintf (relay_file, "/add http://%s:%d/.ogg", host, port);
    } else if (content_type == CONTENT_TYPE_NSV) {
	sprintf (relay_file, "/add http://%s:%d/;stream.nsv", host, port);
    } else {
	sprintf (relay_file, "/add http://%s:%d", host, port);
    }
    ShellExecute (NULL, "open", winamp_path, relay_file, NULL, SW_SHOWNORMAL);

    return TRUE;
}


BOOL
winamp_add_track_to_playlist (char *fullpath)
{
    char add_track[SR_MAX_PATH];
    char winamp_path[SR_MAX_PATH];

    sprintf (winamp_path, "%s%s", m_winamps_path, "winamp.exe");
    sprintf (add_track, "/add \"%s\"", fullpath);
    ShellExecute (NULL, "open", winamp_path, add_track, NULL, SW_SHOWNORMAL);
    return TRUE;
}

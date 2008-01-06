/* registry.c
 * look into the registry
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

//#include "dsp_sripper.h"
#include "srtypes.h"
#include "wa_ipc.h"
#include "ipc_pe.h"
//#include "winamp.h"
#include "debug.h"
#include "mchar.h"

#define DbgBox(_x_)	MessageBox(NULL, _x_, "Debug", 0)

/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL winamp_init();			
BOOL winamp_add_track_to_playlist(char *track);
void winamp_add_rip_to_menu (void);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
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
    rc = RegOpenKeyEx (hkey, subkey, 0, KEY_QUERY_VALUE, &hkey_result);
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
    for (i = 0; strkey[i] && i < SR_MAX_PATH-1; i++) {
	path[i] = toupper(strkey[i]);
    }
    path[i] = 0;

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


/* winamp.c - jonclegg@yahoo.com
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

#include "types.h"
#include "winamp.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL winamp_init();			
BOOL winamp_get_info(WINAMP_INFO *info, BOOL useoldway);
BOOL winamp_add_relay_to_playlist(char *host, u_short port);
BOOL winamp_add_track_to_playlist(char *track);
BOOL winamp_get_path(char *path);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static char m_winamps_path[SR_MAX_PATH] = {'\0'};

// Get's winamp's path from the reg key ..
// HKEY_CLASSES_ROOT\Applications\winamp.exe\shell\Enqueue\command
BOOL winamp_get_path(char *path)
{
	DWORD size = SR_MAX_PATH;
	char strkey[SR_MAX_PATH];
	char winampstr[] = "WINAMP.EXE";
	int winampstrlen = strlen(winampstr);
	int i;
	HKEY hKey;

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT,
        TEXT("Winamp.File\\shell\\Enqueue\\command"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) != ERROR_SUCCESS) 
	{
		return FALSE;
	}

    if (RegQueryValue(hKey,
					NULL,
                    (LPBYTE)strkey,
                    &size) != ERROR_SUCCESS)
	{
		return FALSE;
	}


	// get the path out
	for(i = 0; strkey[i]; i++)	
		strkey[i] = toupper(strkey[i]);
	
	for(i = 1; strncmp(strkey+i, winampstr, winampstrlen) != 0; i++)
	{
		path[i-1] = strkey[i];
	}
	path[i-1] = '\0';
    RegCloseKey(hKey);

	return TRUE;
}



BOOL winamp_init()
{
	// Not implemented
	return winamp_get_path(m_winamps_path);
}

#define DbgBox(_x_)	MessageBox(NULL, _x_, "Debug", 0)

BOOL winamp_get_info(WINAMP_INFO *info, BOOL useoldway)
{
	HWND hwndWinamp;
	info->url[0] = '\0';

	hwndWinamp = FindWindow("Winamp v1.x", NULL);

	// Get winamps path
	if (!m_winamps_path[0])
		return FALSE;

	if (!hwndWinamp)
	{
		info->is_running = FALSE;
		return FALSE;
	}
	else
		info->is_running = TRUE;

	if (useoldway)
	{
		//
		// Send a message to winamp to save the current playlist
		// to a file, 'n' is the index of the currently selected item
		// 
		int n  = SendMessage(hwndWinamp, WM_USER, (WPARAM)NULL, 120); 
		char m3u_path[SR_MAX_PATH];
		char buf[4096] = {'\0'};
		FILE *fp;

		sprintf(m3u_path, "%s%s", m_winamps_path, "winamp.m3u");	
		if ((fp = fopen(m3u_path, "r")) == NULL)
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
		if (strncmp(buf, "http://", strlen("http://")) == 0)
			strcpy(info->url, buf);
	}
	else
	{
	// Much better way to get the filename
		int get_filename = 211;
		int get_position = 125;
		int data = 0;
		int pos;
		char* fname;
		char *purl;

		pos = (int)SendMessage(hwndWinamp, WM_USER, data, get_position);
		fname = (char*)SendMessage(hwndWinamp, WM_USER, pos, get_filename);
		if (fname == NULL)
			return FALSE;
		purl = strstr(fname, "http://");
		if (purl)
			strncpy(info->url, purl, MAX_URL_LEN);
	}

	return TRUE;
}


BOOL winamp_add_relay_to_playlist(char *host, u_short port)
{
//	char host[] = "localhost";		// needs to use the machines name, for proxys
	char relay_file[SR_MAX_PATH];
	char winamp_path[SR_MAX_PATH];

	sprintf(winamp_path, "%s%s", m_winamps_path, "winamp.exe");
	sprintf(relay_file, "/add http://%s:%d", host, port);
	ShellExecute(NULL, "open", winamp_path,	relay_file, NULL, SW_SHOWNORMAL);

	return TRUE;
}


BOOL winamp_add_track_to_playlist(char *fullpath)
{
	char add_track[SR_MAX_PATH];
	char winamp_path[SR_MAX_PATH];

	sprintf(winamp_path, "%s%s", m_winamps_path, "winamp.exe");
	sprintf(add_track, "/add \"%s\"", fullpath);
	ShellExecute(NULL, "open", winamp_path,	add_track, NULL, SW_SHOWNORMAL);
	return TRUE;
}

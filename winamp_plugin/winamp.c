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
BOOL winamp_get_info(WINAMP_INFO *info);
BOOL winamp_add_relay_to_playlist(char *host, u_short port);
BOOL winamp_add_track_to_playlist(char *track);
BOOL winamp_get_path(char *path);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static char m_winamps_path[MAX_PATH_LEN] = {'\0'};

// Get's winamp's path from the reg key ..
// HKEY_CLASSES_ROOT\Applications\winamp.exe\shell\Enqueue\command
BOOL winamp_get_path(char *path)
{
	DWORD size = MAX_PATH_LEN;
	char strkey[MAX_PATH_LEN];
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
	for(i = 0; strkey[i]; i++)	strkey[i] = toupper(strkey[i]);
	for(i = 1; strncmp(strkey+i, "WINAMP.EXE", strlen("WINAMP.EXE")) != 0; i++)
		path[i-1] = strkey[i];
	path[i-1] = '\0';

    RegCloseKey(hKey);

	return TRUE;
}



BOOL winamp_init()
{
	// Not implemented
	return winamp_get_path(m_winamps_path);
}


BOOL winamp_get_info(WINAMP_INFO *info)
{
	HWND hwndWinamp = FindWindow("Winamp v1.x", NULL);
	int n;

	info->url[0] = '\0';

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

	//
	// Send a message to winamp to save the current playlist
	// to a file, 'n' is the index of the currently selected item
	// 
//	n  = SendMessage(hwndWinamp, WM_USER, (WPARAM)NULL, 120); 
//
//	{
//		char m3u_path[MAX_PATH_LEN];
//		char buf[4096] = {'\0'};
//		FILE *fp;
//
//		sprintf(m3u_path, "%s%s", m_winamps_path, "winamp.m3u");	
//		if ((fp = fopen(m3u_path, "r")) == NULL)
//			return FALSE;
//
//		while(!feof(fp) && n >= 0)
//		{
//			fgets(buf, 4096, fp);
//			if (*buf != '#')
//				n--;
//		}
//		fclose(fp);
//		buf[strlen(buf)-1] = '\0';
//
//		// Make sure it's a URL
//		if (strncmp(buf, "http://", strlen("http://")) == 0)
//			strcpy(info->url, buf);
//	}

	// Much better way to get the filename
	{
		int get_filename = 211;
		int get_position = 125;

		int data = 0;
		int pos = (int)SendMessage(hwndWinamp, WM_USER, data, get_position);
		char* fname = (char*)SendMessage(hwndWinamp, WM_USER, pos, get_filename);
		char *purl = strstr(fname, "http://");
		if (purl)
			strcpy(info->url, purl);
	}

	return TRUE;
}


BOOL winamp_add_relay_to_playlist(char *host, u_short port)
{
//	char host[] = "localhost";		// needs to use the machines name, for proxys
	char relay_file[MAX_PATH_LEN];
	char winamp_path[MAX_PATH_LEN];

	sprintf(winamp_path, "%s%s", m_winamps_path, "winamp.exe");
	sprintf(relay_file, "/add http://%s:%d", host, port);
	ShellExecute(NULL, "open", winamp_path,	relay_file, NULL, SW_SHOWNORMAL);

	return TRUE;

}


BOOL winamp_add_track_to_playlist(char *fullpath)
{
	char add_track[MAX_PATH_LEN];
	char winamp_path[MAX_PATH_LEN];

	sprintf(winamp_path, "%s%s", m_winamps_path, "winamp.exe");
	sprintf(add_track, "/add \"%s\"", fullpath);
	ShellExecute(NULL, "open", winamp_path,	add_track, NULL, SW_SHOWNORMAL);
	return TRUE;
}

/* options.c - jonclegg@yahoo.com
 * handles the options popup dialog
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
#include <windowsx.h>
#include <shlobj.h>
#include <stdio.h>
//#include <shfolder.h>

#include "rip_manager.h"
#include "options.h"
#include "resource.h"
#include "winamp.h"

#define MAX_INI_LINE_LEN	1024
#define DEFAULT_RELAY_PORT	8000
#define APPNAME		"sripper"

/**********************************************************************************
 * Public functions
 **********************************************************************************/
void options_dialog_show(HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_load(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_save(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);


/**********************************************************************************
 * Private functions
 **********************************************************************************/
static LRESULT CALLBACK	options_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL				browse_for_folder(HWND hwnd, const char *title, UINT flags, char *folder);
static BOOL				get_desktop_folder(char *path);

/**********************************************************************************
 * Private Vars
 **********************************************************************************/

// This is what we need from shfolder.h, but thats not on my home system
typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHA)(HWND, int, HANDLE, DWORD, LPSTR);  

static RIP_MANAGER_OPTIONS *m_opt;
static GUI_OPTIONS *m_guiOpt;


BOOL get_desktop_folder(char *path)
{
	static HMODULE hMod = NULL;
	PFNSHGETFOLDERPATHA pSHGetFolderPath = NULL;
	
	if (!path)
		return FALSE;

	path[0] = '\0';

   // Load SHFolder.dll only once
	if (!hMod)
		hMod = LoadLibrary("SHFolder.dll");

	if (hMod == NULL)
		return FALSE;

	// Obtain a pointer to the SHGetFolderPathA function
	pSHGetFolderPath = (PFNSHGETFOLDERPATHA)GetProcAddress(hMod, "SHGetFolderPathA");
	pSHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, (HANDLE)0, (DWORD)0, path);

	return path[0] != '\0';
} 


BOOL browse_for_folder(HWND hwnd, const char *title, UINT flags, char *folder)
{
	char *strResult = NULL;
	LPITEMIDLIST lpItemIDList;
	LPMALLOC lpMalloc;  // pointer to IMalloc
	BROWSEINFO browseInfo;
	char display_name[_MAX_PATH];
	char buffer[_MAX_PATH];
	BOOL retval = FALSE;


	if (SHGetMalloc(&lpMalloc) != NOERROR)
	 return FALSE;


	browseInfo.hwndOwner = hwnd;
	// set root at Desktop
	browseInfo.pidlRoot = NULL; 
	browseInfo.pszDisplayName = display_name;
	browseInfo.lpszTitle = title;   // passed in
	browseInfo.ulFlags = flags;   // also passed in
	browseInfo.lpfn = NULL;      // not used
	browseInfo.lParam = 0;      // not used   


	if ((lpItemIDList = SHBrowseForFolder(&browseInfo)) == NULL)
		goto BAIL;


	 // Get the path of the selected folder from the
	 // item ID list.
	if (!SHGetPathFromIDList(lpItemIDList, buffer))
		goto BAIL;

	// At this point, szBuffer contains the path 
	// the user chose.
	if (buffer[0] == '\0')
		goto BAIL;
	
	// We have a path in szBuffer!
	// Return it.
	strncpy(folder, buffer, _MAX_PATH);
	retval = TRUE;

BAIL:
	lpMalloc->lpVtbl->Free(lpMalloc, lpItemIDList);
	lpMalloc->lpVtbl->Release(lpMalloc);      

	// If we made it this far, SHBrowseForFolder failed.
	return retval;
}


void options_dialog_show(HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt)
{
	m_opt = opt;
	m_guiOpt = guiOpt;
	DialogBox(inst, (LPCTSTR)IDD_OPTIONS, parent, options_dlg);
}


void set_checkbox(HWND parent, int id, BOOL checked)
{
	HWND hwndctl = GetDlgItem(parent, id);
	SendMessage(hwndctl, BM_SETCHECK, 
		(LPARAM)checked ? BST_CHECKED : BST_UNCHECKED,
		(WPARAM)NULL);
}

BOOL get_checkbox(HWND parent, int id)
{
	HWND hwndctl = GetDlgItem(parent, id);
	return SendMessage(hwndctl, BM_GETCHECK, (LPARAM)NULL, (WPARAM)NULL);
}

LRESULT CALLBACK options_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch(message)
	{
		case WM_INITDIALOG:
			options_load(m_opt, m_guiOpt);
			SetDlgItemText(hWnd, IDC_PROXY, m_opt->proxyurl);
			SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory);

			set_checkbox(hWnd, IDC_SEPERATE_DIRS, OPT_FLAG_ISSET(m_opt->flags, OPT_SEPERATE_DIRS));
			set_checkbox(hWnd, IDC_RECONNECT, OPT_FLAG_ISSET(m_opt->flags, OPT_AUTO_RECONNECT));
			set_checkbox(hWnd, IDC_OVER_WRITE, OPT_FLAG_ISSET(m_opt->flags, OPT_OVER_WRITE_TRACKS));
			set_checkbox(hWnd, IDC_MAKE_RELAY, !OPT_FLAG_ISSET(m_opt->flags, OPT_NO_RELAY));
			set_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST, m_guiOpt->m_add_finshed_tracks_to_playlist);
			set_checkbox(hWnd, IDC_ALLOW_TOUCH, m_guiOpt->m_allow_touch);
			set_checkbox(hWnd, IDC_COUNT_FILES, OPT_FLAG_ISSET(m_opt->flags, OPT_COUNT_FILES));
			set_checkbox(hWnd, IDC_DATE_STAMP, OPT_FLAG_ISSET(m_opt->flags, OPT_DATE_STAMP));
			set_checkbox(hWnd, IDC_ADD_ID3, OPT_FLAG_ISSET(m_opt->flags, OPT_ADD_ID3));
			set_checkbox(hWnd, IDC_CHECK_MAX_BYTES, OPT_FLAG_ISSET(m_opt->flags, OPT_CHECK_MAX_BYTES));

			if (OPT_FLAG_ISSET(m_opt->flags, OPT_CHECK_MAX_BYTES))
			{
				SetDlgItemInt(hWnd, IDC_MAX_BYTES, m_opt->maxMB_rip_size, FALSE);
			}

			return TRUE;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch(wmId)
			{
				case IDC_OK:
					GetDlgItemText(hWnd, IDC_PROXY, m_opt->proxyurl, MAX_URL_LEN);
					GetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory, MAX_PATH_LEN);
					m_guiOpt->m_allow_touch = get_checkbox(hWnd, IDC_ALLOW_TOUCH);

					m_opt->flags = 0;
					if (get_checkbox(hWnd, IDC_SEPERATE_DIRS))
						m_opt->flags ^= OPT_SEPERATE_DIRS;

					if (get_checkbox(hWnd, IDC_RECONNECT))
						m_opt->flags ^= OPT_AUTO_RECONNECT;

					if (get_checkbox(hWnd, IDC_OVER_WRITE))
						m_opt->flags ^= OPT_OVER_WRITE_TRACKS;

					if (!get_checkbox(hWnd, IDC_MAKE_RELAY))
						m_opt->flags ^= OPT_NO_RELAY;

					if (get_checkbox(hWnd, IDC_COUNT_FILES))
						m_opt->flags ^= OPT_COUNT_FILES;

					if (get_checkbox(hWnd, IDC_DATE_STAMP))
						m_opt->flags ^= OPT_DATE_STAMP;

					if (get_checkbox(hWnd, IDC_ADD_ID3))
						m_opt->flags ^= OPT_ADD_ID3;

					if (get_checkbox(hWnd, IDC_CHECK_MAX_BYTES))
					{
						m_opt->flags ^= OPT_CHECK_MAX_BYTES;
						m_opt->maxMB_rip_size = GetDlgItemInt(hWnd, IDC_MAX_BYTES, FALSE, FALSE);
					}

				

					m_guiOpt->m_add_finshed_tracks_to_playlist = get_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST);

					options_save(m_opt, m_guiOpt);
					/* FALLTHROUGH */
				case IDC_CANCEL:
					EndDialog(hWnd, 0);
					return TRUE;

				case IDC_BROWSE:
					{
						char temp_dir[MAX_PATH_LEN];
						if (browse_for_folder(hWnd, "bla", 0, temp_dir))
						{
							strcpy(m_opt->output_directory, temp_dir);
							SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory);
						}
						return TRUE;
					}

			}
			break;
		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
	}
	
	return FALSE;
}

BOOL options_load(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt)
{
	char desktop_path[MAX_PATH];
	char filename[MAX_INI_LINE_LEN];
	BOOL	seperate_dirs,
			auto_reconnect,
			over_write_tracks,
			no_relay,
			count_files,
			date_stamp,
			add_id3,
			check_max_btyes;

	if (!get_desktop_folder(desktop_path))
	{
		// Maybe an error message? nahhh..
		desktop_path[0] = '\0';
	}

	if (!winamp_get_path(filename))
		return FALSE;

	strcat(filename, "Plugins\\sripper.ini");

	GetPrivateProfileString(APPNAME, "url", "", opt->url, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "proxy", "", opt->proxyurl, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "output_dir", desktop_path, opt->output_directory, MAX_INI_LINE_LEN, filename);
	seperate_dirs = GetPrivateProfileInt(APPNAME, "seperate_dirs", TRUE, filename);
	opt->relay_port = GetPrivateProfileInt(APPNAME, "relay_port", 8000, filename);
	opt->max_port = opt->relay_port+1000;
	auto_reconnect = GetPrivateProfileInt(APPNAME, "auto_reconnect", TRUE, filename);
	over_write_tracks = GetPrivateProfileInt(APPNAME, "over_write_tracks", FALSE, filename);
	no_relay = GetPrivateProfileInt(APPNAME, "no_relay", FALSE, filename);
	count_files = GetPrivateProfileInt(APPNAME, "count_files", FALSE, filename);
	date_stamp = GetPrivateProfileInt(APPNAME, "date_stamp", FALSE, filename);
	add_id3 = GetPrivateProfileInt(APPNAME, "add_id3", TRUE, filename);
	check_max_btyes = GetPrivateProfileInt(APPNAME, "check_max_bytes", FALSE, filename);
	opt->maxMB_rip_size = GetPrivateProfileInt(APPNAME, "maxMB_bytes", 0, filename);

	guiOpt->m_add_finshed_tracks_to_playlist = GetPrivateProfileInt(APPNAME, "add_tracks_to_playlist", FALSE, filename);
	guiOpt->m_start_minimized = GetPrivateProfileInt(APPNAME, "start_minimized", FALSE, filename);
	guiOpt->m_allow_touch = GetPrivateProfileInt(APPNAME, "allow_touch", TRUE, filename);
	guiOpt->oldpos.x = GetPrivateProfileInt(APPNAME, "window_x", 0, filename);
	guiOpt->oldpos.y = GetPrivateProfileInt(APPNAME, "window_y", 0, filename);
	guiOpt->m_enabled = GetPrivateProfileInt(APPNAME, "enabled", 1, filename);

	if (guiOpt->oldpos.x < 0 || guiOpt->oldpos.y < 0)
		guiOpt->oldpos.x = guiOpt->oldpos.y = 0;

	opt->flags = 0;
	opt->flags |= OPT_SEARCH_PORTS;
	if (seperate_dirs) opt->flags |= OPT_SEPERATE_DIRS;
	if (auto_reconnect) opt->flags |= OPT_AUTO_RECONNECT;
	if (over_write_tracks) opt->flags |= OPT_OVER_WRITE_TRACKS;
	if (no_relay) opt->flags |= OPT_NO_RELAY;
	if (count_files) opt->flags |= OPT_COUNT_FILES;
	if (date_stamp) opt->flags |= OPT_DATE_STAMP;
	if (add_id3) opt->flags |= OPT_ADD_ID3;
	if (check_max_btyes) opt->flags |= OPT_CHECK_MAX_BYTES;

	return TRUE;
}


BOOL options_save(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt)
{

	char filename[MAX_INI_LINE_LEN];
	FILE *fp;
	winamp_get_path(filename);
	strcat(filename, "Plugins\\sripper.ini");
	fp = fopen(filename, "w");
	if (!fp)
		return FALSE;

	fprintf(fp, "[%s]\n", APPNAME);
	fprintf(fp, "url=%s\n", opt->url);
	fprintf(fp, "proxy=%s\n", opt->proxyurl);
	fprintf(fp, "output_dir=%s\n", opt->output_directory);
	fprintf(fp, "seperate_dirs=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_SEPERATE_DIRS));
	fprintf(fp, "relay_port=%d\n", opt->relay_port);
	fprintf(fp, "auto_reconnect=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_AUTO_RECONNECT));
	fprintf(fp, "over_write_tracks=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_OVER_WRITE_TRACKS));
	fprintf(fp, "no_relay=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_NO_RELAY));
	fprintf(fp, "count_files=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_COUNT_FILES));
	fprintf(fp, "date_stamp=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_DATE_STAMP));
	fprintf(fp, "add_id3=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_ADD_ID3));
	fprintf(fp, "check_max_bytes=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_CHECK_MAX_BYTES));
	fprintf(fp, "maxMB_bytes=%d\n", opt->maxMB_rip_size);


	fprintf(fp, "add_tracks_to_playlist=%d\n", guiOpt->m_add_finshed_tracks_to_playlist);
	fprintf(fp, "start_minimized=%d\n", guiOpt->m_start_minimized);
	fprintf(fp, "allow_touch=%d\n", guiOpt->m_allow_touch);
	fprintf(fp, "window_x=%d\n", guiOpt->oldpos.x);
	fprintf(fp, "window_y=%d\n", guiOpt->oldpos.y);
	fprintf(fp, "enabled=%d\n", guiOpt->m_enabled);

	fclose(fp);

	return TRUE;
}

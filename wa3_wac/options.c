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
#include <prsht.h>

#include "lib/rip_manager.h"
#include "lib/debug.h"
#include "options.h"
#include "resource.h"
//#include "winamp.h"
#include <assert.h>

#define MAX_INI_LINE_LEN	1024
#define DEFAULT_RELAY_PORT	8000
#define APPNAME		"sripper"
#define DEFAULT_USERAGENT	"FreeAmp/2.x"
#define NUM_PROP_PAGES	2
#define PROP_SHEET_CON		0
#define PROP_SHEET_FILE		1
#define SRVERSION			"1.52"

/**********************************************************************************
 * Public functions
 **********************************************************************************/
void options_dialog_show(HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_load(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_save(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);


/**********************************************************************************
 * Private functions
 **********************************************************************************/
static BOOL				browse_for_folder(HWND hwnd, const char *title, UINT flags, char *folder, long );
static BOOL				get_desktop_folder(char *path);
static LRESULT CALLBACK file_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK con_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//static LRESULT CALLBACK skin_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK options_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL confile);
static void				saveload_fileopts(HWND hWnd, BOOL saveload);
static void				saveload_conopts(HWND hWnd, BOOL saveload);
static HPROPSHEETPAGE	create_prop_sheet_page(HINSTANCE inst, DWORD iddres, DLGPROC dlgproc);
static void				add_useragent_strings();
static BOOL				get_skin_list();
static void				free_skin_list();

/**********************************************************************************
 * Private Vars
 **********************************************************************************/

// This is what we need from shfolder.h, but thats not on my home system
typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHA)(HWND, int, HANDLE, DWORD, LPSTR);  

static RIP_MANAGER_OPTIONS *m_opt;
static GUI_OPTIONS *m_guiOpt;

//JCBUG not yet implemented (need to figure out how to get this)
static int m_last_sheet = 0;	

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


BOOL browse_for_folder(HWND hwnd, const char *title, UINT flags, char *folder, long foldersize)
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
	strncpy(folder, buffer, foldersize);
	retval = TRUE;

BAIL:
	lpMalloc->lpVtbl->Free(lpMalloc, lpItemIDList);
	lpMalloc->lpVtbl->Release(lpMalloc);      

	return retval;
}


HPROPSHEETPAGE create_prop_sheet_page(HINSTANCE inst, DWORD iddres, DLGPROC dlgproc)
{
	PROPSHEETPAGE  psp;
	memset(&psp, 0, sizeof(PROPSHEETPAGE));
	psp.dwSize      = sizeof(PROPSHEETPAGE);
	psp.dwFlags     = PSP_DEFAULT;
	psp.hInstance   = inst;
	psp.pszTemplate = MAKEINTRESOURCE(iddres);
	psp.pfnDlgProc  = dlgproc;
	return CreatePropertySheetPage(&psp);
}

BOOL CALLBACK MyWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HBRUSH hBrush = NULL;

	switch (uMsg)
	{
		case WM_CREATE:
			return 0;
		case WM_PAINT:
			{
				PAINTSTRUCT pt;
				HDC hdc = BeginPaint(hWnd, &pt);
				TextOut(hdc, 0, 0, "what the fuck", strlen("what the fuck"));
				EndPaint(hWnd, &pt);
			}
			return 0;

	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
void options_dialog_show(HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt)
{
	HPROPSHEETPAGE hPage[NUM_PROP_PAGES];	
	int ret;
	char szCaption[256];
	PROPSHEETHEADER psh;

	m_opt = opt;
	m_guiOpt = guiOpt;

	sprintf(szCaption, "Streamripper WA3 Settings v%s", SRVERSION);
	options_load(m_opt, m_guiOpt);
	hPage[0] = create_prop_sheet_page(inst, IDD_PROPPAGE_CON, con_dlg);
	hPage[1] = create_prop_sheet_page(inst, IDD_PROPPAGE_FILE, file_dlg);
//	hPage[2] = create_prop_sheet_page(inst, IDD_PROPPAGE_SKIN, skin_dlg);
	memset(&psh, 0, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT;
	psh.hwndParent = parent;
	psh.hInstance = inst;
	psh.pszCaption = szCaption;
	psh.nPages = NUM_PROP_PAGES;
	psh.nStartPage = m_last_sheet;
	psh.phpage = hPage;
	ret = PropertySheet(&psh);
	if (ret == -1)
	{
		char s[255];
		sprintf(s, "There was an error while attempting to load the options dialog\r\n"
				   "Please check http://streamripper.sourceforge.net for updates\r\n"
				   "Error: %d\n", GetLastError());
		MessageBox(parent, s, "Can't load options dialog", MB_ICONEXCLAMATION);
		return; //JCBUG

	}
	else if (ret != 0)
	{
		options_save(m_opt, m_guiOpt);
	}

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

void set_to_checkbox(HWND parent, int id, u_short* popt, u_short flag)
{
	if (get_checkbox(parent, id))
		*popt |= flag;
	else
	{
		if (OPT_FLAG_ISSET(flag, *popt))
			*popt ^= flag;
	}
}

void add_useragent_strings(HWND hdlg)
{
	HWND hcombo = GetDlgItem(hdlg, IDC_USERAGENT);

	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"Streamripper/1.x");
//	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"WinampMPEG/2.7");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"FreeAmp/2.x");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"XMMS/1.x");
	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"UnknownPlayer/1.x");
}

void saveload_fileopts(HWND hWnd, BOOL saveload)
{

	/*
	- seperate dir
	- overwrite
	- add finished to winamp
	- add seq numbers/count files
	- append date
	- add id3
	- output dir
	- keep incomplete
	*/

	if (saveload)
	{
		set_to_checkbox(hWnd, IDC_SEPERATE_DIRS, &m_opt->flags, OPT_SEPERATE_DIRS);
		set_to_checkbox(hWnd, IDC_OVER_WRITE, &m_opt->flags, OPT_OVER_WRITE_TRACKS);
		m_guiOpt->m_add_finshed_tracks_to_playlist =	get_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST);
		set_to_checkbox(hWnd, IDC_COUNT_FILES, &m_opt->flags, OPT_COUNT_FILES);
		set_to_checkbox(hWnd, IDC_DATE_STAMP, &m_opt->flags, OPT_DATE_STAMP);
		set_to_checkbox(hWnd, IDC_ADD_ID3, &m_opt->flags, OPT_ADD_ID3);
		GetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory, MAX_PATH_LEN);
		set_to_checkbox(hWnd, IDC_KEEP_INCOMPLETE, &m_opt->flags, OPT_KEEP_INCOMPLETE);
	}
	else
	{
		set_checkbox(hWnd, IDC_SEPERATE_DIRS, OPT_FLAG_ISSET(m_opt->flags, OPT_SEPERATE_DIRS));
		set_checkbox(hWnd, IDC_OVER_WRITE, OPT_FLAG_ISSET(m_opt->flags, OPT_OVER_WRITE_TRACKS));
		set_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST, m_guiOpt->m_add_finshed_tracks_to_playlist);
		set_checkbox(hWnd, IDC_COUNT_FILES, OPT_FLAG_ISSET(m_opt->flags, OPT_COUNT_FILES));
		set_checkbox(hWnd, IDC_DATE_STAMP, OPT_FLAG_ISSET(m_opt->flags, OPT_DATE_STAMP));
		set_checkbox(hWnd, IDC_ADD_ID3, OPT_FLAG_ISSET(m_opt->flags, OPT_ADD_ID3));
		SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory);
		set_checkbox(hWnd, IDC_KEEP_INCOMPLETE, OPT_FLAG_ISSET(m_opt->flags, OPT_KEEP_INCOMPLETE));
	}

}


void saveload_conopts(HWND hWnd, BOOL saveload)
{

	/*
	- reconnect
	- relay
	- rip max
	- anon usage
	- proxy server
	- local machine name
	- useragent
	- use old way
	*/

	if (saveload)
	{
		set_to_checkbox(hWnd, IDC_RECONNECT, &m_opt->flags, OPT_AUTO_RECONNECT);
		set_to_checkbox(hWnd, IDC_MAKE_RELAY, &m_opt->flags, OPT_MAKE_RELAY);
		set_to_checkbox(hWnd, IDC_CHECK_MAX_BYTES, &m_opt->flags, OPT_CHECK_MAX_BYTES);
		m_opt->maxMB_rip_size = GetDlgItemInt(hWnd, IDC_MAX_BYTES, FALSE, FALSE);
		GetDlgItemText(hWnd, IDC_PROXY, m_opt->proxyurl, MAX_URL_LEN);
		GetDlgItemText(hWnd, IDC_LOCALHOST, m_guiOpt->localhost, MAX_HOST_LEN);
		GetDlgItemText(hWnd, IDC_USERAGENT, m_opt->useragent, MAX_USERAGENT_STR);
		m_guiOpt->use_old_playlist_ret = get_checkbox(hWnd, IDC_USE_OLD_PLAYLIST_RET);
	}
	else
	{
		set_checkbox(hWnd, IDC_RECONNECT, OPT_FLAG_ISSET(m_opt->flags, OPT_AUTO_RECONNECT));
		set_checkbox(hWnd, IDC_MAKE_RELAY, OPT_FLAG_ISSET(m_opt->flags, OPT_MAKE_RELAY));
		if (OPT_FLAG_ISSET(m_opt->flags, OPT_CHECK_MAX_BYTES))
		{
			set_checkbox(hWnd, IDC_CHECK_MAX_BYTES, OPT_FLAG_ISSET(m_opt->flags, OPT_CHECK_MAX_BYTES));
			SetDlgItemInt(hWnd, IDC_MAX_BYTES, m_opt->maxMB_rip_size, FALSE);
		}
		SetDlgItemText(hWnd, IDC_PROXY, m_opt->proxyurl);
		SetDlgItemText(hWnd, IDC_LOCALHOST, m_guiOpt->localhost);
		if (!m_opt->useragent[0])
			strcpy(m_opt->useragent, DEFAULT_USERAGENT);
		SetDlgItemText(hWnd, IDC_USERAGENT, m_opt->useragent);
		set_checkbox(hWnd, IDC_USE_OLD_PLAYLIST_RET, m_guiOpt->use_old_playlist_ret);
	}

}

//
// These are wrappers for property page callbacks
// bassicly i didn't want to copy past the entire dialog proc 
// it'll dispatch the calls forward with a confile boolean which tells if
// it's the connection of file pages 
//
LRESULT CALLBACK file_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return options_dlg(hWnd, message, wParam, lParam, FALSE);
}
LRESULT CALLBACK con_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return options_dlg(hWnd, message, wParam, lParam, TRUE);
}

LRESULT CALLBACK options_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL confile)
{
	int wmId, wmEvent;

	switch(message)
	{
		case WM_INITDIALOG:
			if (confile)
				saveload_conopts(hWnd, FALSE);
			else
				saveload_fileopts(hWnd, FALSE);

			PropSheet_UnChanged(GetParent(hWnd), hWnd);

			add_useragent_strings(hWnd);

			return TRUE;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			switch(wmId)
			{
				case IDC_BROWSE:
					{
						char temp_dir[MAX_PATH_LEN];
						if (browse_for_folder(hWnd, "Select a folder", 0, temp_dir, MAX_PATH_LEN))
						{
							strncpy(m_opt->output_directory, temp_dir, MAX_PATH_LEN);
							SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, m_opt->output_directory);
						}
						return TRUE;
					}
				case IDC_CHECK_MAX_BYTES:
					{
					BOOL isset = get_checkbox(hWnd, IDC_CHECK_MAX_BYTES);
					HWND hwndEdit = GetDlgItem(hWnd, IDC_MAX_BYTES);
					SendMessage(hwndEdit, EM_SETREADONLY, !isset, 0);
					}

				case IDC_SEPERATE_DIRS:
				case IDC_RECONNECT:
				case IDC_OVER_WRITE:
				case IDC_MAKE_RELAY:
				case IDC_COUNT_FILES:
				case IDC_DATE_STAMP:
				case IDC_ADD_ID3:
				case IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST:
				case IDC_PROXY:
				case IDC_OUTPUT_DIRECTORY:
				case IDC_LOCALHOST:
				case IDC_KEEP_INCOMPLETE:
				case IDC_USERAGENT:
				case IDC_USE_OLD_PLAYLIST_RET:
					PropSheet_Changed(GetParent(hWnd), hWnd);
					break;
			}
			break;
		case WM_NOTIFY:
		{
			NMHDR* phdr = (NMHDR*) lParam;
			switch ( phdr->code )
			{
			case PSN_APPLY:
				if (confile)
					saveload_conopts(hWnd, TRUE);
				else
					saveload_fileopts(hWnd, TRUE);
				break;
			}
		}
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
			make_relay,
			count_files,
			date_stamp,
			add_id3,
			check_max_btyes,
			keep_incomplete;

	if (!get_desktop_folder(desktop_path))
	{
		// Maybe an error message? nahhh..
		desktop_path[0] = '\0';
	}

//	if (!winamp_get_path(filename))
//		return FALSE;

	GetCurrentDirectory(255, filename);
	strcat(filename, "\\sripper.ini");

	opt->flags = 0;
	opt->flags |= OPT_SEARCH_PORTS;			// not having this caused a bad bug, must remember this.

	GetPrivateProfileString(APPNAME, "url", "", opt->url, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "proxy", "", opt->proxyurl, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "output_dir", desktop_path, opt->output_directory, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "localhost", "localhost", guiOpt->localhost, MAX_INI_LINE_LEN, filename);
	GetPrivateProfileString(APPNAME, "useragent", DEFAULT_USERAGENT, opt->useragent, MAX_INI_LINE_LEN, filename);
//	GetPrivateProfileString(APPNAME, "default_skin", DEFAULT_SKINFILE, guiOpt->default_skin, MAX_INI_LINE_LEN, filename);

	seperate_dirs = GetPrivateProfileInt(APPNAME, "seperate_dirs", TRUE, filename);
	opt->relay_port = GetPrivateProfileInt(APPNAME, "relay_port", 8000, filename);
	opt->max_port = opt->relay_port+1000;
	auto_reconnect = GetPrivateProfileInt(APPNAME, "auto_reconnect", TRUE, filename);
	over_write_tracks = GetPrivateProfileInt(APPNAME, "over_write_tracks", FALSE, filename);
	make_relay = GetPrivateProfileInt(APPNAME, "make_relay", TRUE, filename);
	count_files = GetPrivateProfileInt(APPNAME, "count_files", FALSE, filename);
	date_stamp = GetPrivateProfileInt(APPNAME, "date_stamp", FALSE, filename);
	add_id3 = GetPrivateProfileInt(APPNAME, "add_id3", TRUE, filename);
	check_max_btyes = GetPrivateProfileInt(APPNAME, "check_max_bytes", FALSE, filename);
	opt->maxMB_rip_size = GetPrivateProfileInt(APPNAME, "maxMB_bytes", 0, filename);
	keep_incomplete = GetPrivateProfileInt(APPNAME, "keep_incomplete", TRUE, filename);

	guiOpt->m_add_finshed_tracks_to_playlist = GetPrivateProfileInt(APPNAME, "add_tracks_to_playlist", FALSE, filename);
//	guiOpt->m_start_minimized = GetPrivateProfileInt(APPNAME, "start_minimized", FALSE, filename);
//	guiOpt->oldpos.x = GetPrivateProfileInt(APPNAME, "window_x", 0, filename);
//	guiOpt->oldpos.y = GetPrivateProfileInt(APPNAME, "window_y", 0, filename);
//	guiOpt->m_enabled = GetPrivateProfileInt(APPNAME, "enabled", 1, filename);
	guiOpt->use_old_playlist_ret = GetPrivateProfileInt(APPNAME, "use_old_playlist_ret", 0, filename);


//	if (guiOpt->oldpos.x < 0 || guiOpt->oldpos.y < 0)
//		guiOpt->oldpos.x = guiOpt->oldpos.y = 0;

	opt->flags = 0;
	opt->flags |= OPT_SEARCH_PORTS;
	if (seperate_dirs) opt->flags |= OPT_SEPERATE_DIRS;
	if (auto_reconnect) opt->flags |= OPT_AUTO_RECONNECT;
	if (over_write_tracks) opt->flags |= OPT_OVER_WRITE_TRACKS;
	if (make_relay) opt->flags |= OPT_MAKE_RELAY;
	if (count_files) opt->flags |= OPT_COUNT_FILES;
	if (date_stamp) opt->flags |= OPT_DATE_STAMP;
	if (add_id3) opt->flags |= OPT_ADD_ID3;
	if (check_max_btyes) opt->flags |= OPT_CHECK_MAX_BYTES;
	if (keep_incomplete) opt->flags |= OPT_KEEP_INCOMPLETE;

	return TRUE;
}


BOOL options_save(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt)
{

	char filename[MAX_INI_LINE_LEN];
	FILE *fp;
//	winamp_get_path(filename);
	GetCurrentDirectory(255, filename);
	strcat(filename, "\\sripper.ini");
	fp = fopen(filename, "w");
	if (!fp)
		return FALSE;

	fprintf(fp, "[%s]\n", APPNAME);
	fprintf(fp, "url=%s\n", opt->url);
	fprintf(fp, "proxy=%s\n", opt->proxyurl);
	fprintf(fp, "output_dir=%s\n", opt->output_directory);
	fprintf(fp, "localhost=%s\n", guiOpt->localhost);
	fprintf(fp, "useragent=%s\n", opt->useragent);
	fprintf(fp, "seperate_dirs=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_SEPERATE_DIRS));
	fprintf(fp, "relay_port=%d\n", opt->relay_port);
	fprintf(fp, "auto_reconnect=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_AUTO_RECONNECT));
	fprintf(fp, "over_write_tracks=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_OVER_WRITE_TRACKS));
	fprintf(fp, "make_relay=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_MAKE_RELAY));
	fprintf(fp, "count_files=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_COUNT_FILES));
	fprintf(fp, "date_stamp=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_DATE_STAMP));
	fprintf(fp, "add_id3=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_ADD_ID3));
	fprintf(fp, "check_max_bytes=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_CHECK_MAX_BYTES));
	fprintf(fp, "maxMB_bytes=%d\n", opt->maxMB_rip_size);
	fprintf(fp, "keep_incomplete=%d\n", OPT_FLAG_ISSET(opt->flags, OPT_KEEP_INCOMPLETE));


	fprintf(fp, "add_tracks_to_playlist=%d\n", guiOpt->m_add_finshed_tracks_to_playlist);
//	fprintf(fp, "start_minimized=%d\n", guiOpt->m_start_minimized);
//	fprintf(fp, "window_x=%d\n", guiOpt->oldpos.x);
//	fprintf(fp, "window_y=%d\n", guiOpt->oldpos.y);
//	fprintf(fp, "enabled=%d\n", guiOpt->m_enabled);
//	fprintf(fp, "default_skin=%s\n", guiOpt->default_skin);
	fprintf(fp, "use_old_playlist_ret=%d", guiOpt->use_old_playlist_ret);

	fclose(fp);

	return TRUE;
}
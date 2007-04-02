/* options.c
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
#include <prsht.h>

#include "rip_manager.h"
#include "options.h"
#include "resource.h"
#include "winamp.h"
#include "debug.h"
#include "render.h"
#include <assert.h>

#define MAX_INI_LINE_LEN	1024
#define DEFAULT_RELAY_PORT	8000
#define APPNAME			"sripper"
#define DEFAULT_USERAGENT	"FreeAmp/2.x"
#define NUM_PROP_PAGES		6
#define DEFAULT_SKINFILE	"srskin.bmp"
#define SKIN_PREV_LEFT		(2*110)
#define SKIN_PREV_TOP		(2*50)
#define	MAX_SKINS		256

/**********************************************************************************
 * Private functions
 **********************************************************************************/
static LRESULT CALLBACK con_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK file_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK pat_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK skin_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK splitting_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK external_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**********************************************************************************
 * Private Vars
 **********************************************************************************/
// This is what we need from shfolder.h, but thats not on my home system
typedef HRESULT (__stdcall * PFNSHGETFOLDERPATHA)(HWND, int, HANDLE, DWORD, LPSTR);  

//static RIP_MANAGER_OPTIONS *m_opt;
static GUI_OPTIONS *m_guiOpt;
static char *m_pskin_list[MAX_SKINS];
static int m_skin_list_size = 0;
static int m_curskin = 0;
static int m_last_sheet = 0;    // not yet implemented

BOOL
get_skin_list()
{
    WIN32_FIND_DATA	filedata; 
    HANDLE			hsearch = NULL;
    char			temppath[MAX_PATH];

    m_skin_list_size = 0;
    memset(m_pskin_list, 0, sizeof(m_pskin_list));
		
    if (!winamp_get_path(temppath)) {
	debug_printf ("winamp_get_path failed #1\n");
	return FALSE;
    }

    debug_printf ("temppath = %s\n", temppath);
    strcat(temppath, SKIN_PATH);
    strcat(temppath, "*.bmp");
    debug_printf ("temppath = %s\n", temppath);

    hsearch = FindFirstFile(temppath, &filedata);
    if (hsearch == INVALID_HANDLE_VALUE) 
	return FALSE;

    m_pskin_list[m_skin_list_size] = strdup(filedata.cFileName);
    debug_printf ("GetSkinList: %d = %s\n", m_skin_list_size,
	m_pskin_list[m_skin_list_size]);
    m_skin_list_size++;

    while(TRUE) {
	if (FindNextFile(hsearch, &filedata)) {
	    m_pskin_list[m_skin_list_size] = strdup(filedata.cFileName);
	    debug_printf ("GetSkinList: %d = %s\n", m_skin_list_size,
		m_pskin_list[m_skin_list_size]);
	    m_skin_list_size++;
	    continue;
	}
	if (GetLastError() == ERROR_NO_MORE_FILES) {
	    break;
	} else {
	    if (hsearch) {
		FindClose(hsearch);
	    }
	    return FALSE;
	}
    }
	
    FindClose(hsearch);
    return TRUE;
}

void
free_skin_list()
{
    if (m_skin_list_size == 0)
	return;

    while (m_skin_list_size--) {
	assert(m_pskin_list[m_skin_list_size]);
	free(m_pskin_list[m_skin_list_size]);
	m_pskin_list[m_skin_list_size] = NULL;
    }
    m_skin_list_size = 0;
}

BOOL
get_desktop_folder (char *path)
{
    HRESULT hresult;
    static HMODULE hMod = NULL;
    PFNSHGETFOLDERPATHA pSHGetFolderPath = NULL;

    if (!path)
	return FALSE;

    path[0] = '\0';

    // Load SHFolder.dll only once
    if (!hMod) {
	hMod = LoadLibrary("SHFolder.dll");
    }

    if (hMod == NULL) {
	debug_printf ("Failed to load SHFolder.dll\n");
	return FALSE;
    }

    // Obtain a pointer to the SHGetFolderPathA function
    pSHGetFolderPath = (PFNSHGETFOLDERPATHA) GetProcAddress (hMod, "SHGetFolderPathA");
    if (pSHGetFolderPath == 0) {
	debug_printf ("Failed to get SHGetFolderPathA from SHFolder\n");
	return FALSE;
    }

    // Use SHGetFolderPathA to get desktop folder
    hresult = pSHGetFolderPath (NULL, CSIDL_COMMON_DESKTOPDIRECTORY, (HANDLE)0, (DWORD)0, path);
    if (SUCCEEDED(hresult)) {
	debug_printf ("SHGetFolderPathA succeeded %s (%08x)\n", path, hresult);
    } else {
	debug_printf ("SHGetFolderPathA failed %s (%08x)\n", path, hresult);
	/* If failed (win98), try again */
	hresult = pSHGetFolderPath (NULL, CSIDL_DESKTOPDIRECTORY, (HANDLE)0, (DWORD)0, path);
	if (SUCCEEDED(hresult)) {
	    debug_printf ("SHGetFolderPathA (2) succeeded %s (%08x)\n", path, hresult);
	} else {
	    debug_printf ("SHGetFolderPathA (2) failed %s (%08x)\n", path, hresult);
	}
    }
    return path[0] != '\0';
}

BOOL
browse_for_folder (HWND hwnd, const char *title, UINT flags, char *folder, 
		   long foldersize)
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

BOOL
browse_for_file (HWND hwnd, const char *title, UINT flags, 
		 char *file_name, long file_size)
{
    OPENFILENAME ofn;
    char szFile[SR_MAX_PATH] = "rip_to_single_file.mp3";

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0MP3 files\0*.MP3\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

    // Display the Open dialog box. 
    if (GetOpenFileName(&ofn)) {
	strncpy (file_name, ofn.lpstrFile, file_size);
	return TRUE;
    } else {
	return FALSE;
    }
}

HPROPSHEETPAGE
create_prop_sheet_page (HINSTANCE inst, DWORD iddres, DLGPROC dlgproc)
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

void
options_dialog_show (HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *rmo, GUI_OPTIONS *guiOpt)
{
    HPROPSHEETPAGE hPage[NUM_PROP_PAGES];	
    int ret;
    char szCaption[256];
    PROPSHEETHEADER psh;

    //m_opt = opt;
    m_guiOpt = guiOpt;

    debug_printf ("options_dialog_show checkpoint 1\n");
    sprintf(szCaption, "Streamripper Settings v%s", SRVERSION);
    //options_load (m_opt, m_guiOpt);
    hPage[0] = create_prop_sheet_page(inst, IDD_PROPPAGE_CON, con_dlg);
    hPage[1] = create_prop_sheet_page(inst, IDD_PROPPAGE_FILE, file_dlg);
    hPage[2] = create_prop_sheet_page(inst, IDD_PROPPAGE_PATTERN, pat_dlg);
    hPage[3] = create_prop_sheet_page(inst, IDD_PROPPAGE_SKIN, skin_dlg);
    hPage[4] = create_prop_sheet_page(inst, IDD_PROPPAGE_SPLITTING, splitting_dlg);
    hPage[5] = create_prop_sheet_page(inst, IDD_PROPPAGE_EXTERNAL, external_dlg);
    debug_printf ("options_dialog_show checkpoint 2a\n");
    memset (&psh, 0, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = parent;
    psh.hInstance = inst;
    psh.pszCaption = szCaption;
    psh.nPages = NUM_PROP_PAGES;
    psh.nStartPage = m_last_sheet;
    psh.phpage = hPage;
    debug_printf ("options_dialog_show checkpoint 2b\n");
    ret = PropertySheet (&psh);
    debug_printf ("options_dialog_show checkpoint 3\n");
    if (ret == -1) {
	char s[255];
	sprintf(s, "There was an error while attempting to load the options dialog\r\n"
		"Please check http://streamripper.sourceforge.net for updates\r\n"
		"Error: %d\n", GetLastError());
	MessageBox(parent, s, "Can't load options dialog", MB_ICONEXCLAMATION);
	return; //JCBUG
    }
    if (ret) {
	if (m_pskin_list[m_curskin]) {
	    strcpy (m_guiOpt->default_skin, m_pskin_list[m_curskin]);
	    //JCBUG font color doesn't change until restart.
	    render_change_skin (m_guiOpt->default_skin);
	}
	options_save (rmo, m_guiOpt);
    }
    debug_printf ("options_dialog_show checkpoint 4\n");
    free_skin_list ();
    debug_printf ("options_dialog_show checkpoint 5\n");
}

void
set_checkbox(HWND parent, int id, BOOL checked)
{
    HWND hwndctl = GetDlgItem(parent, id);
    SendMessage(hwndctl, BM_SETCHECK, 
		(LPARAM)checked ? BST_CHECKED : BST_UNCHECKED,
		(WPARAM)NULL);
}

BOOL
get_checkbox(HWND parent, int id)
{
    HWND hwndctl = GetDlgItem(parent, id);
    return SendMessage(hwndctl, BM_GETCHECK, (LPARAM)NULL, (WPARAM)NULL);
}

void
set_from_checkbox(HWND parent, int id, u_long* popt, u_long flag)
{
    if (get_checkbox(parent, id)) {
	*popt |= flag;
    } else {
	if (OPT_FLAG_ISSET(flag, *popt))
	    *popt ^= flag;
    }
}

void
add_useragent_strings(HWND hdlg)
{
    HWND hcombo = GetDlgItem(hdlg, IDC_USERAGENT);

    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"Streamripper/1.x");
    //	SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"WinampMPEG/2.7");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"FreeAmp/2.x");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"XMMS/1.x");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"UnknownPlayer/1.x");
}

static void
set_overwrite_combo (HWND hdlg, RIP_MANAGER_OPTIONS* rmo)
{
    HWND hcombo = GetDlgItem(hdlg, IDC_OVERWRITE_COMPLETE);
    int combo_idx = rmo->overwrite - 1;

    LRESULT rc = SendMessage(hcombo, CB_SETCURSEL, combo_idx, 0);
    debug_printf("Setting combo, idx=%d, rc=%d\n", combo_idx, rc);
}

static void
get_overwrite_combo (HWND hdlg, RIP_MANAGER_OPTIONS* rmo)
{
    HWND hcombo = GetDlgItem (hdlg, IDC_OVERWRITE_COMPLETE);
    int combo_idx = SendMessage (hcombo, CB_GETCURSEL, 0, 0);
    debug_printf("Getting combo, idx=%d\n", combo_idx);
    rmo->overwrite = combo_idx + 1;
}

static void
add_codeset_strings_1 (HWND hdlg, RIP_MANAGER_OPTIONS* rmo, int id)
{
    HWND hcombo = GetDlgItem(hdlg, id);
    char default_locale_string[1024];
    _snprintf (default_locale_string, 1024, "%s (Locale)", rmo->cs_opt.codeset_locale);
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM) default_locale_string);
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"UTF-8 (Unicode)");
    if (id == IDC_CODESET_ID3) {
        SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"UTF-16 (Unicode)");
    }
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-1 (Western Europe)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-2 (Central/East Europe)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-4 (Baltic)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-5 (Cyrillic)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-6 (Arabic)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-7 (Greek)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-8 (Hebrew)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"ISO-8859-9 (Turkish)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"KOI8-R (Russian)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"KOI8-U (Ukranian)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"GB2312 (Simp. Chinese)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"BIG5 (Trad. Chinese)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"EUC-JP (Japanese)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"SHIFT_JIS (Japanese)");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"CP949 (Korean)");
}

static void
add_codeset_strings (HWND hdlg, RIP_MANAGER_OPTIONS* rmo)
{
    add_codeset_strings_1(hdlg, rmo, IDC_CODESET_METADATA);
    add_codeset_strings_1(hdlg, rmo, IDC_CODESET_RELAY);
    add_codeset_strings_1(hdlg, rmo, IDC_CODESET_ID3);
    add_codeset_strings_1(hdlg, rmo, IDC_CODESET_FILESYS);
}

void
parse_gui_codeset (HWND hWnd, char* new_codeset, char* default_codeset, int id)
{
    char *p;
    new_codeset[0] = 0;
    GetDlgItemText (hWnd, id, new_codeset, MAX_CODESET_STRING);
    p = new_codeset + strlen(new_codeset);
    /* Empty string, set to default */
    if (p == new_codeset) {
	strcpy (new_codeset, default_codeset);
	return;
    }
    /* Strip off the " (Ukranian)" stuff from codeset string */
    if (*(--p) == ')') {
	p = strrchr (new_codeset, '(');
	if (p && p != new_codeset && *(--p) == ' ') {
	    *p = 0;
	}
    }
}

void
set_codesets_from_gui (HWND hWnd, RIP_MANAGER_OPTIONS* rmo)
{
    parse_gui_codeset (hWnd, rmo->cs_opt.codeset_metadata, 
			rmo->cs_opt.codeset_locale, IDC_CODESET_METADATA);
    parse_gui_codeset (hWnd, rmo->cs_opt.codeset_relay, 
			rmo->cs_opt.codeset_locale, IDC_CODESET_RELAY);
    parse_gui_codeset (hWnd, rmo->cs_opt.codeset_id3, 
			rmo->cs_opt.codeset_locale, IDC_CODESET_ID3);
    parse_gui_codeset (hWnd, rmo->cs_opt.codeset_filesys, 
			rmo->cs_opt.codeset_locale, IDC_CODESET_FILESYS);
}

void
set_gui_from_codesets (HWND hWnd, RIP_MANAGER_OPTIONS* rmo)
{
    SetDlgItemText (hWnd, IDC_CODESET_METADATA, 
		    rmo->cs_opt.codeset_metadata);
    SetDlgItemText (hWnd, IDC_CODESET_RELAY, 
		    rmo->cs_opt.codeset_relay);
    SetDlgItemText (hWnd, IDC_CODESET_ID3, 
		    rmo->cs_opt.codeset_id3);
    SetDlgItemText (hWnd, IDC_CODESET_FILESYS, 
		    rmo->cs_opt.codeset_filesys);
}

void
add_overwrite_complete_strings(HWND hdlg)
{
    HWND hcombo = GetDlgItem(hdlg, IDC_OVERWRITE_COMPLETE);

    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"Always");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"Never");
    SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM)"When larger");
}

void
gui_set_conn_opts (HWND hWnd, RIP_MANAGER_OPTIONS* rmo, BOOL from_gui)
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
    if (from_gui) {
	set_from_checkbox(hWnd, IDC_RECONNECT, &rmo->flags, OPT_AUTO_RECONNECT);
	set_from_checkbox(hWnd, IDC_MAKE_RELAY, &rmo->flags, OPT_MAKE_RELAY);
	debug_printf ("from_gui: make_relay=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_MAKE_RELAY));
	rmo->relay_port = GetDlgItemInt(hWnd, IDC_RELAY_PORT_EDIT, FALSE, FALSE);
        rmo->max_port = rmo->relay_port+1000;
	set_from_checkbox(hWnd, IDC_CHECK_MAX_BYTES, &rmo->flags, OPT_CHECK_MAX_BYTES);
	rmo->maxMB_rip_size = GetDlgItemInt(hWnd, IDC_MAX_BYTES, FALSE, FALSE);
	GetDlgItemText(hWnd, IDC_PROXY, rmo->proxyurl, MAX_URL_LEN);
	GetDlgItemText(hWnd, IDC_LOCALHOST, m_guiOpt->localhost, MAX_HOST_LEN);
	GetDlgItemText(hWnd, IDC_USERAGENT, rmo->useragent, MAX_USERAGENT_STR);
	m_guiOpt->use_old_playlist_ret = get_checkbox(hWnd, IDC_USE_OLD_PLAYLIST_RET);
    } else {
	set_checkbox(hWnd, IDC_RECONNECT, OPT_FLAG_ISSET(rmo->flags, OPT_AUTO_RECONNECT));
	set_checkbox(hWnd, IDC_MAKE_RELAY, OPT_FLAG_ISSET(rmo->flags, OPT_MAKE_RELAY));
	debug_printf ("to_gui: make_relay=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_MAKE_RELAY));
	SetDlgItemInt(hWnd, IDC_RELAY_PORT_EDIT, rmo->relay_port, FALSE);
	set_checkbox(hWnd, IDC_CHECK_MAX_BYTES, OPT_FLAG_ISSET(rmo->flags, OPT_CHECK_MAX_BYTES));
	SetDlgItemInt(hWnd, IDC_MAX_BYTES, rmo->maxMB_rip_size, FALSE);
	SetDlgItemText(hWnd, IDC_PROXY, rmo->proxyurl);
	SetDlgItemText(hWnd, IDC_LOCALHOST, m_guiOpt->localhost);
	if (!rmo->useragent[0])
	    strcpy(rmo->useragent, DEFAULT_USERAGENT);
	SetDlgItemText(hWnd, IDC_USERAGENT, rmo->useragent);
	set_checkbox(hWnd, IDC_USE_OLD_PLAYLIST_RET, m_guiOpt->use_old_playlist_ret);
    }
}

void
gui_set_file_opts(HWND hWnd, RIP_MANAGER_OPTIONS* rmo, BOOL from_gui)
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
      - rip single file check
      - rip single file filename
    */
    if (from_gui) {
	get_overwrite_combo(hWnd, rmo);
	m_guiOpt->m_add_finshed_tracks_to_playlist = get_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST);
	set_from_checkbox(hWnd, IDC_ADD_ID3V1, &rmo->flags, OPT_ADD_ID3V1);
	set_from_checkbox(hWnd, IDC_ADD_ID3V2, &rmo->flags, OPT_ADD_ID3V2);
	GetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, rmo->output_directory, SR_MAX_PATH);
	set_from_checkbox(hWnd, IDC_KEEP_INCOMPLETE, &rmo->flags, OPT_KEEP_INCOMPLETE);
	set_from_checkbox(hWnd, IDC_RIP_INDIVIDUAL_CHECK, &rmo->flags, OPT_INDIVIDUAL_TRACKS);
	set_from_checkbox(hWnd, IDC_RIP_SINGLE_CHECK, &rmo->flags, OPT_SINGLE_FILE_OUTPUT);
	GetDlgItemText(hWnd, IDC_RIP_SINGLE_EDIT, rmo->showfile_pattern, SR_MAX_PATH);
	rmo->dropcount = GetDlgItemInt(hWnd, IDC_DROP_COUNT, FALSE, FALSE);
    } else {
	set_overwrite_combo(hWnd, rmo);
	set_checkbox(hWnd, IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST, m_guiOpt->m_add_finshed_tracks_to_playlist);
	set_checkbox(hWnd, IDC_ADD_ID3V1, OPT_FLAG_ISSET(rmo->flags, OPT_ADD_ID3V1));
	set_checkbox(hWnd, IDC_ADD_ID3V2, OPT_FLAG_ISSET(rmo->flags, OPT_ADD_ID3V2));
	SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, rmo->output_directory);
	set_checkbox(hWnd, IDC_KEEP_INCOMPLETE, OPT_FLAG_ISSET(rmo->flags, OPT_KEEP_INCOMPLETE));
	set_checkbox(hWnd, IDC_RIP_INDIVIDUAL_CHECK, OPT_FLAG_ISSET(rmo->flags, OPT_INDIVIDUAL_TRACKS));
	set_checkbox(hWnd, IDC_RIP_SINGLE_CHECK, OPT_FLAG_ISSET(rmo->flags, OPT_SINGLE_FILE_OUTPUT));
	SetDlgItemText(hWnd, IDC_RIP_SINGLE_EDIT, rmo->showfile_pattern);
	SetDlgItemInt(hWnd, IDC_DROP_COUNT, rmo->dropcount, FALSE);
    }
}

void
gui_set_pat_opts(HWND hWnd, RIP_MANAGER_OPTIONS* rmo, BOOL from_gui)
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
      - rip single file check
      - rip single file filename
    */
    if (from_gui) {
	GetDlgItemText(hWnd, IDC_PATTERN_EDIT, rmo->output_pattern, SR_MAX_PATH);
    } else {
	SetDlgItemText(hWnd, IDC_PATTERN_EDIT, rmo->output_pattern);
    }
}

void
gui_set_splitting_opts(HWND hWnd, RIP_MANAGER_OPTIONS* rmo, BOOL from_gui)
{
    /*
      - skew
      - silence_len
      - search_pre
      - search_post
      - padding_pre
      - padding_post
    */
    if (from_gui) {
	rmo->sp_opt.xs_offset = GetDlgItemInt(hWnd, IDC_XS_OFFSET, FALSE, TRUE);
	rmo->sp_opt.xs_silence_length = GetDlgItemInt(hWnd, IDC_XS_SILENCE_LENGTH, FALSE, FALSE);
	rmo->sp_opt.xs_search_window_1 = GetDlgItemInt(hWnd, IDC_XS_SEARCH_WIN_PRE, FALSE, TRUE);
	rmo->sp_opt.xs_search_window_2 = GetDlgItemInt(hWnd, IDC_XS_SEARCH_WIN_POST, FALSE, TRUE);
	rmo->sp_opt.xs_padding_1 = GetDlgItemInt(hWnd, IDC_XS_PADDING_PRE, FALSE, TRUE);
	rmo->sp_opt.xs_padding_2 = GetDlgItemInt(hWnd, IDC_XS_PADDING_POST, FALSE, TRUE);
    } else {
	SetDlgItemInt(hWnd, IDC_XS_OFFSET, rmo->sp_opt.xs_offset, TRUE);
	SetDlgItemInt(hWnd, IDC_XS_SILENCE_LENGTH, rmo->sp_opt.xs_silence_length, FALSE);
	SetDlgItemInt(hWnd, IDC_XS_SEARCH_WIN_PRE, rmo->sp_opt.xs_search_window_1, TRUE);
	SetDlgItemInt(hWnd, IDC_XS_SEARCH_WIN_POST, rmo->sp_opt.xs_search_window_2, TRUE);
	SetDlgItemInt(hWnd, IDC_XS_PADDING_PRE, rmo->sp_opt.xs_padding_1, TRUE);
	SetDlgItemInt(hWnd, IDC_XS_PADDING_POST, rmo->sp_opt.xs_padding_2, TRUE);
    }
}

void
gui_set_external_opts (HWND hWnd, RIP_MANAGER_OPTIONS* rmo, BOOL from_gui)
{
    /*
      - external check
      - external cmd
    */
    if (from_gui) {
	set_from_checkbox(hWnd, IDC_EXTERNAL_COMMAND_CHECK, &rmo->flags, OPT_EXTERNAL_CMD);
	GetDlgItemText(hWnd, IDC_EXTERNAL_COMMAND, rmo->ext_cmd, SR_MAX_PATH);
	set_codesets_from_gui (hWnd, rmo);
    } else {
	set_checkbox(hWnd, IDC_EXTERNAL_COMMAND_CHECK, OPT_FLAG_ISSET(rmo->flags, OPT_EXTERNAL_CMD));
	SetDlgItemText(hWnd, IDC_EXTERNAL_COMMAND, rmo->ext_cmd);
	set_gui_from_codesets (hWnd, rmo);
    }
}

/* confile == 1  -> con_dlg */
/* confile == 2  -> file_dlg */
/* confile == 3  -> pat_dlg */
/* confile == 4  -> splitting_dlg */
/* confile == 5  -> external_dlg */
LRESULT CALLBACK
options_dlg (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, 
	     RIP_MANAGER_OPTIONS* rmo, int confile)
{
    int wmId, wmEvent;

    switch(message)
    {
    case WM_INITDIALOG:
	debug_printf ("options_dlg:WM_INITDIALOG (1)\n");
	add_useragent_strings (hWnd);
	add_codeset_strings (hWnd, rmo);
	add_overwrite_complete_strings (hWnd);
	debug_printf ("options_dlg:WM_INITDIALOG (2)\n");

	switch (confile) {
	case 1:
	    gui_set_conn_opts(hWnd, rmo, FALSE);
	    break;
	case 2:
	    gui_set_file_opts(hWnd, rmo, FALSE);
	    break;
	case 3:
	    gui_set_pat_opts(hWnd, rmo, FALSE);
	    break;
	case 4:
	    gui_set_splitting_opts(hWnd, rmo, FALSE);
	    break;
	case 5:
	    gui_set_external_opts(hWnd, rmo, FALSE);
	    break;
	}
	PropSheet_UnChanged(GetParent(hWnd), hWnd);

	return TRUE;
    case WM_COMMAND:
	debug_printf ("options_dlg:WM_COMMAND\n");
	wmId    = LOWORD(wParam); 
	wmEvent = HIWORD(wParam); 
	switch(wmId)
	{
	case IDC_BROWSE_OUTDIR:
	    {
		char temp_dir[SR_MAX_PATH];
		if (browse_for_folder(hWnd, "Select a folder", 0, temp_dir, SR_MAX_PATH))
		{
		    strncpy(rmo->output_directory, temp_dir, SR_MAX_PATH);
		    SetDlgItemText(hWnd, IDC_OUTPUT_DIRECTORY, rmo->output_directory);
		}
		return TRUE;
	    }
	case IDC_BROWSE_RIPSINGLE:
	    {
		char temp_fn[SR_MAX_PATH];
		if (browse_for_file(hWnd, "Select a file", 0, temp_fn, SR_MAX_PATH))
		{
		    strncpy(rmo->showfile_pattern, temp_fn, SR_MAX_PATH);
		    SetDlgItemText(hWnd, IDC_RIP_SINGLE_EDIT, rmo->showfile_pattern);
		}
		return TRUE;
	    }
	case IDC_CHECK_MAX_BYTES:
	    {
		BOOL isset = get_checkbox(hWnd, IDC_CHECK_MAX_BYTES);
		HWND hwndEdit = GetDlgItem(hWnd, IDC_MAX_BYTES);
		SendMessage(hwndEdit, EM_SETREADONLY, !isset, 0);
		PropSheet_Changed(GetParent(hWnd), hWnd);
		break;
	    }
	case IDC_MAKE_RELAY:
	    {
		BOOL isset = get_checkbox(hWnd, IDC_MAKE_RELAY);
		HWND hwndEdit = GetDlgItem(hWnd, IDC_RELAY_PORT_EDIT);
		SendMessage(hwndEdit, EM_SETREADONLY, !isset, 0);
		PropSheet_Changed(GetParent(hWnd), hWnd);
		break;
	    }

	case IDC_RECONNECT:
	case IDC_RELAY_PORT_EDIT:
	case IDC_MAX_BYTES:
	case IDC_ADD_ID3V1:
	case IDC_ADD_ID3V2:
	case IDC_ADD_FINSHED_TRACKS_TO_PLAYLIST:
	case IDC_PROXY:
	case IDC_OUTPUT_DIRECTORY:
	case IDC_LOCALHOST:
	case IDC_KEEP_INCOMPLETE:
	case IDC_USERAGENT:
	case IDC_USE_OLD_PLAYLIST_RET:
	case IDC_RIP_SINGLE_CHECK:
	case IDC_RIP_SINGLE_EDIT:
	case IDC_XS_OFFSET:
	case IDC_XS_SILENCE_LENGTH:
	case IDC_XS_SEARCH_WIN_PRE:
	case IDC_XS_SEARCH_WIN_POST:
	case IDC_XS_PADDING_PRE:
	case IDC_XS_PADDING_POST:
	case IDC_OVERWRITE_COMPLETE:
	case IDC_RIP_INDIVIDUAL_CHECK:
	case IDC_PATTERN_EDIT:
	case IDC_EXTERNAL_COMMAND:
	case IDC_EXTERNAL_COMMAND_CHECK:
	    PropSheet_Changed(GetParent(hWnd), hWnd);
	    break;
	}
	break;
    case WM_NOTIFY:
	{
	    NMHDR* phdr = (NMHDR*) lParam;
	    debug_printf ("options_dlg:WM_NOTIFY\n");
	    switch (phdr->code)
	    {
	    case PSN_APPLY:
		switch (confile) {
		case 1:
		    gui_set_conn_opts (hWnd, rmo, TRUE);
		    break;
		case 2:
		    gui_set_file_opts (hWnd, rmo, TRUE);
		    break;
		case 3:
		    gui_set_pat_opts (hWnd, rmo, TRUE);
		    break;
		case 4:
		    gui_set_splitting_opts (hWnd, rmo, TRUE);
		    break;
		case 5:
		    gui_set_external_opts (hWnd, rmo, TRUE);
		    break;
		}
	    }
	}
    }
    return FALSE;
}

// These are wrappers for property page callbacks
// bassicly i didn't want to copy past the entire dialog proc 
// it'll dispatch the calls forward with a confile boolean which tells if
// it's the connection of file pages 
//
LRESULT CALLBACK
con_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RIP_MANAGER_OPTIONS* rmo = &g_rmo;
    debug_printf ("Calling options_dlg(1)\n");
    return options_dlg(hWnd, message, wParam, lParam, rmo, 1);
}
LRESULT CALLBACK
file_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RIP_MANAGER_OPTIONS* rmo = &g_rmo;
    debug_printf ("Calling options_dlg(2)\n");
    return options_dlg(hWnd, message, wParam, lParam, rmo, 2);
}
LRESULT CALLBACK
pat_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RIP_MANAGER_OPTIONS* rmo = &g_rmo;
    debug_printf ("Calling options_dlg(3)\n");
    return options_dlg(hWnd, message, wParam, lParam, rmo, 3);
}
LRESULT CALLBACK
splitting_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RIP_MANAGER_OPTIONS* rmo = &g_rmo;
    debug_printf ("Calling options_dlg(4)\n");
    return options_dlg(hWnd, message, wParam, lParam, rmo, 4);
}
LRESULT CALLBACK
external_dlg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RIP_MANAGER_OPTIONS* rmo = &g_rmo;
    debug_printf ("Calling options_dlg(5)\n");
    return options_dlg(hWnd, message, wParam, lParam, rmo, 5);
}

BOOL
populate_skin_list (HWND dlg)
{
    int i;
    HWND hlist = GetDlgItem(dlg, IDC_SKIN_LIST);

    if (!hlist)
	return FALSE;

    for (i = 0; i < m_skin_list_size; i++) {
	assert(m_pskin_list[i]);
	debug_printf ("pop_skin_list: %d = %s\n", i, m_pskin_list[i]);
	SendMessage (hlist, LB_ADDSTRING, 0, (LPARAM)m_pskin_list[i]);
	if (strcmp(m_pskin_list[i], m_guiOpt->default_skin) == 0)
	    m_curskin = i;
    }
    SendMessage(hlist, LB_SETCURSEL, (WPARAM)m_curskin, 0);
    return TRUE;
}

LRESULT CALLBACK
skin_dlg (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
	debug_printf( "skin:WM_INITDIALOG\n" );

	if (!get_skin_list() || 
	    !populate_skin_list(hWnd))	// order of exec important!
	{
	    MessageBox(hWnd, 
		       "Failed to find any skins in the SrSkin directory\r\n"
		       "Please make verifiy that the Winamp/Skins/SrSkins/"
		       "contins some Streamripper skins",
		       "Can't load skins", MB_ICONEXCLAMATION);
	    return FALSE;
	}

	return TRUE;
    case WM_PAINT:
	{
	    PAINTSTRUCT pt;
	    RECT size_rect;
	    RECT pos_rect;
	    POINT pos_point;
	    BOOL rc;

	    HDC hdc = BeginPaint(hWnd, &pt);
	    debug_printf ("skin:WM_PAINT\n");

	    rc = GetClientRect (GetDlgItem (hWnd, IDC_SKIN_PREVIEW), &size_rect);
	    debug_printf ("skin:Got client rect rc = %d, "
			    "(b,l,r,t) = (%d %d %d %d)\n", 
			    rc,
			    size_rect.bottom, size_rect.left,
			    size_rect.right, size_rect.top);
	    rc = GetWindowRect (GetDlgItem (hWnd, IDC_SKIN_PREVIEW), &pos_rect);
	    debug_printf ("skin:Got window rect rc = %d, "
			    "(b,l,r,t) = (%d %d %d %d)\n", 
			    rc,
			    pos_rect.bottom, pos_rect.left,
			    pos_rect.right, pos_rect.top);
	    pos_point.x = pos_rect.left;
	    pos_point.y = pos_rect.top;
	    rc = ScreenToClient (hWnd, &pos_point);
	    debug_printf ("skin:mapped screen to client rc = %d, "
			    "(x,y) = (%d %d)\n", 
			    rc, pos_point.x, pos_point.y);
 
	    render_create_preview (m_pskin_list[m_curskin], hdc, 
		pos_point.x, pos_point.y, 
		size_rect.right, size_rect.bottom);

	    EndPaint(hWnd, &pt);
	}
	return FALSE;

    case WM_COMMAND: 
	switch (LOWORD(wParam)) 
	{ 
	case IDC_SKIN_LIST: 
	    switch (HIWORD(wParam)) 
	    { 
	    case LBN_SELCHANGE:
		{
		    HWND hwndlist = GetDlgItem (hWnd, IDC_SKIN_LIST); 
		    m_curskin = SendMessage (hwndlist, LB_GETCURSEL, 0, 0); 
		    assert (m_curskin >= 0 && m_curskin < m_skin_list_size);
		    UpdateWindow (hWnd);
		    InvalidateRect (hWnd, NULL, FALSE);
		}
	    }
	}
    }
    return FALSE;
}

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

    GetPrivateProfileString(APPNAME, "url", "", rmo->url, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "proxy", "", rmo->proxyurl, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "output_dir", desktop_path, rmo->output_directory, MAX_INI_LINE_LEN, filename);
    if (rmo->output_directory[0] == 0) {
	strcpy (rmo->output_directory, desktop_path);
    }
    GetPrivateProfileString(APPNAME, "localhost", "localhost", guiOpt->localhost, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "useragent", DEFAULT_USERAGENT, rmo->useragent, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "default_skin", DEFAULT_SKINFILE, guiOpt->default_skin, MAX_INI_LINE_LEN, filename);
    if (guiOpt->default_skin[0] == 0) {
        strcpy(guiOpt->default_skin, DEFAULT_SKINFILE);
    }
    GetPrivateProfileString(APPNAME, "rip_single_path", "", rmo->showfile_pattern, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "output_pattern", "%S/%A - %T", rmo->output_pattern, MAX_INI_LINE_LEN, filename);
    GetPrivateProfileString(APPNAME, "over_write_complete", "larger", overwrite_string, MAX_INI_LINE_LEN, filename);
    debug_printf ("Got PPS: %s\n", overwrite_string);

    rmo->relay_port = GetPrivateProfileInt(APPNAME, "relay_port", DEFAULT_RELAY_PORT, filename);
    rmo->max_port = rmo->relay_port+1000;
    auto_reconnect = GetPrivateProfileInt(APPNAME, "auto_reconnect", TRUE, filename);
    make_relay = GetPrivateProfileInt(APPNAME, "make_relay", FALSE, filename);
    debug_printf ("GetPrivateProfileInt (make_relay) =? %d\n",
	GetPrivateProfileInt(APPNAME, "make_relay", 2, filename));
    add_id3_sr161 = GetPrivateProfileInt(APPNAME, "add_id3", FALSE, filename);
    add_id3v1 = GetPrivateProfileInt(APPNAME, "add_id3v1", TRUE, filename);
    add_id3v2 = GetPrivateProfileInt(APPNAME, "add_id3v2", TRUE, filename);
    check_max_btyes = GetPrivateProfileInt(APPNAME, "check_max_bytes", FALSE, filename);
    rmo->maxMB_rip_size = GetPrivateProfileInt(APPNAME, "maxMB_bytes", 0, filename);
    keep_incomplete = GetPrivateProfileInt(APPNAME, "keep_incomplete", TRUE, filename);
    rip_individual_tracks = GetPrivateProfileInt(APPNAME, "rip_individual_tracks", TRUE, filename);
    rip_single_file = GetPrivateProfileInt(APPNAME, "rip_single_file", FALSE, filename);
    rmo->dropcount = GetPrivateProfileInt(APPNAME, "dropcount", FALSE, filename);

    rmo->sp_opt.xs_offset = GetPrivateProfileInt(APPNAME, "xs_offset", 
	rmo->sp_opt.xs_offset, filename);
    rmo->sp_opt.xs_silence_length = GetPrivateProfileInt(APPNAME, "xs_silence_length", 
	rmo->sp_opt.xs_silence_length, filename);
    rmo->sp_opt.xs_search_window_1 = GetPrivateProfileInt(APPNAME, "xs_search_window_1", 
	rmo->sp_opt.xs_search_window_1, filename);
    rmo->sp_opt.xs_search_window_2 = GetPrivateProfileInt(APPNAME, "xs_search_window_2", 
	rmo->sp_opt.xs_search_window_2, filename);
    rmo->sp_opt.xs_padding_1 = GetPrivateProfileInt(APPNAME, "xs_padding_1", 
	rmo->sp_opt.xs_padding_1, filename);
    rmo->sp_opt.xs_padding_2 = GetPrivateProfileInt(APPNAME, "xs_padding_2", 
	rmo->sp_opt.xs_padding_2, filename);

    use_ext_cmd = GetPrivateProfileInt(APPNAME, "use_ext_cmd", FALSE, filename);
    GetPrivateProfileString(APPNAME, "ext_cmd", "", rmo->ext_cmd, MAX_INI_LINE_LEN, filename);

    guiOpt->m_add_finshed_tracks_to_playlist = GetPrivateProfileInt(APPNAME, "add_tracks_to_playlist", FALSE, filename);
    guiOpt->m_start_minimized = GetPrivateProfileInt(APPNAME, "start_minimized", FALSE, filename);
    guiOpt->oldpos.x = GetPrivateProfileInt(APPNAME, "window_x", 0, filename);
    guiOpt->oldpos.y = GetPrivateProfileInt(APPNAME, "window_y", 0, filename);
    guiOpt->m_enabled = GetPrivateProfileInt(APPNAME, "enabled", 1, filename);
    guiOpt->use_old_playlist_ret = GetPrivateProfileInt(APPNAME, "use_old_playlist_ret", 0, filename);

    if (guiOpt->oldpos.x < 0 || guiOpt->oldpos.y < 0)
	guiOpt->oldpos.x = guiOpt->oldpos.y = 0;

    rmo->overwrite = string_to_overwrite_opt (overwrite_string);

    rmo->flags = 0;
    rmo->flags |= OPT_SEARCH_PORTS;	// not having this caused a bad bug, must remember this.
    if (auto_reconnect) rmo->flags |= OPT_AUTO_RECONNECT;
    if (make_relay) rmo->flags |= OPT_MAKE_RELAY;
    if (add_id3_sr161) rmo->flags |= OPT_ADD_ID3V1;
    if (add_id3_sr161) rmo->flags |= OPT_ADD_ID3V2;
    if (add_id3v1) rmo->flags |= OPT_ADD_ID3V1;
    if (add_id3v2) rmo->flags |= OPT_ADD_ID3V2;
    if (check_max_btyes) rmo->flags |= OPT_CHECK_MAX_BYTES;
    if (keep_incomplete) rmo->flags |= OPT_KEEP_INCOMPLETE;
    if (rip_individual_tracks) rmo->flags |= OPT_INDIVIDUAL_TRACKS;
    if (rip_single_file) rmo->flags |= OPT_SINGLE_FILE_OUTPUT;
    if (use_ext_cmd) rmo->flags |= OPT_EXTERNAL_CMD;

    debug_printf ("loading: flags = 0x%04x\n", rmo->flags);
    debug_printf ("make_relay = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_MAKE_RELAY));
    debug_printf ("id3v1 flag = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_ADD_ID3V1));
    debug_printf ("id3v2 flag = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_ADD_ID3V2));

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

    /* Get codesets */
    GetPrivateProfileString (APPNAME, "codeset_metadata", rmo->cs_opt.codeset_locale, rmo->cs_opt.codeset_metadata, MAX_CODESET_STRING, filename);
    GetPrivateProfileString (APPNAME, "codeset_relay", rmo->cs_opt.codeset_locale, rmo->cs_opt.codeset_relay, MAX_CODESET_STRING, filename);
    GetPrivateProfileString (APPNAME, "codeset_id3", rmo->cs_opt.codeset_locale, rmo->cs_opt.codeset_id3, MAX_CODESET_STRING, filename);
    GetPrivateProfileString (APPNAME, "codeset_filesys", rmo->cs_opt.codeset_locale, rmo->cs_opt.codeset_filesys, MAX_CODESET_STRING, filename);

    return TRUE;
}

BOOL
options_save (RIP_MANAGER_OPTIONS *rmo, GUI_OPTIONS *guiOpt)
{
    int i, p;
    char filename[MAX_INI_LINE_LEN];
    FILE *fp;
    if (!winamp_get_path(filename)) {
	debug_printf ("winamp_get_path failed #4\n");
	return FALSE;
    }
    strcat(filename, "Plugins\\sripper.ini");
    fp = fopen(filename, "w");
    if (!fp)
	return FALSE;

    debug_printf ("Saving filename is %s\n", filename);

    fprintf(fp, "[%s]\n", APPNAME);
    fprintf(fp, "url=%s\n", rmo->url);
    fprintf(fp, "proxy=%s\n", rmo->proxyurl);
    fprintf(fp, "output_dir=%s\n", rmo->output_directory);
    fprintf(fp, "localhost=%s\n", guiOpt->localhost);
    fprintf(fp, "useragent=%s\n", rmo->useragent);
#if defined (commentout)
    fprintf(fp, "seperate_dirs=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_SEPERATE_DIRS));
#endif
    fprintf(fp, "relay_port=%d\n", rmo->relay_port);
    fprintf(fp, "auto_reconnect=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_AUTO_RECONNECT));
    fprintf(fp, "over_write_complete=%s\n", overwrite_opt_to_string (rmo->overwrite));
    fprintf(fp, "make_relay=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_MAKE_RELAY));
#if defined (commentout)
    fprintf(fp, "count_files=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_COUNT_FILES));
    fprintf(fp, "date_stamp=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_DATE_STAMP));
#endif
    fprintf(fp, "add_id3v1=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_ADD_ID3V1));
    fprintf(fp, "add_id3v2=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_ADD_ID3V2));
    fprintf(fp, "check_max_bytes=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_CHECK_MAX_BYTES));
    fprintf(fp, "maxMB_bytes=%d\n", rmo->maxMB_rip_size);
    fprintf(fp, "keep_incomplete=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_KEEP_INCOMPLETE));
    fprintf(fp, "rip_individual_tracks=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_INDIVIDUAL_TRACKS));
    fprintf(fp, "rip_single_file=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_SINGLE_FILE_OUTPUT));
    fprintf(fp, "rip_single_path=%s\n", rmo->showfile_pattern);
    fprintf(fp, "dropcount=%d\n", rmo->dropcount);

    debug_printf ("saving: flags = 0x%04x\n", rmo->flags);
    debug_printf ("make_relay = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_MAKE_RELAY));
    debug_printf ("id3v1 flag = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_ADD_ID3V1));
    debug_printf ("id3v2 flag = %d\n", OPT_FLAG_ISSET (rmo->flags, OPT_ADD_ID3V2));

    fprintf(fp, "output_pattern=%s\n", rmo->output_pattern);

    fprintf(fp, "xs_offset=%d\n", rmo->sp_opt.xs_offset);
    fprintf(fp, "xs_silence_length=%d\n", rmo->sp_opt.xs_silence_length);
    fprintf(fp, "xs_search_window_1=%d\n", rmo->sp_opt.xs_search_window_1);
    fprintf(fp, "xs_search_window_2=%d\n", rmo->sp_opt.xs_search_window_2);
    fprintf(fp, "xs_padding_1=%d\n", rmo->sp_opt.xs_padding_1);
    fprintf(fp, "xs_padding_2=%d\n", rmo->sp_opt.xs_padding_2);

    fprintf(fp, "use_ext_cmd=%d\n", OPT_FLAG_ISSET(rmo->flags, OPT_EXTERNAL_CMD));
    fprintf(fp, "ext_cmd=%s\n", rmo->ext_cmd);

    fprintf(fp, "add_tracks_to_playlist=%d\n", guiOpt->m_add_finshed_tracks_to_playlist);
    fprintf(fp, "start_minimized=%d\n", guiOpt->m_start_minimized);
    fprintf(fp, "window_x=%d\n", guiOpt->oldpos.x);
    fprintf(fp, "window_y=%d\n", guiOpt->oldpos.y);
    fprintf(fp, "enabled=%d\n", guiOpt->m_enabled);
    fprintf(fp, "default_skin=%s\n", guiOpt->default_skin);
    fprintf(fp, "use_old_playlist_ret=%d\n", guiOpt->use_old_playlist_ret);

    /* Save history */
    for (i = 0, p = 0; i < RIPLIST_LEN; i++) {
	if (guiOpt->riplist[i][0]) {
	    fprintf (fp, "riplist%d=%s\n", p, guiOpt->riplist[i]);
	    p++;
	}
    }

    fprintf (fp, "codeset_metadata=%s\n", rmo->cs_opt.codeset_metadata);
    fprintf (fp, "codeset_relay=%s\n", rmo->cs_opt.codeset_relay);
    fprintf (fp, "codeset_id3=%s\n", rmo->cs_opt.codeset_id3);
    fprintf (fp, "codeset_filesys=%s\n", rmo->cs_opt.codeset_filesys);

    fclose(fp);

    return TRUE;
}

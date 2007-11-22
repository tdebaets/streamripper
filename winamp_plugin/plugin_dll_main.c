/* plugin_dll_main.c
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
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <windows.h>
#include "resource.h"
#include "winamp.h"
#include "gen.h"
#include "dsp_sripper.h"

static int  init();
static void config(); 
static void quit();

static TCHAR m_szToopTip[] = "Streamripper For Winamp";
static int m_enabled;

winampGeneralPurposePlugin g_plugin = {
    GPPHDR_VER,
    m_szToopTip,
    init,
    config,
    quit
};

BOOL WINAPI
_DllMainCRTStartup (HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

__declspec(dllexport) winampGeneralPurposePlugin*
winampGetGeneralPurposePlugin ()
{
    return &g_plugin;
}

int
init ()
{
    /* Create a thread which will communicate with the exe */
#if defined (commentout)
    WNDCLASS wc;
    char* sr_debug_env;
    sr_debug_env = getenv ("STREAMRIPPER_DEBUG");
    if (sr_debug_env) {
	debug_enable();
	debug_set_filename (sr_debug_env);
    }

    winamp_init (g_plugin.hDllInstance);

#if defined (commentout)
    prefs_load ();
    prefs_get_stream_prefs (prefs, prefs->url);
    prefs_save ();
    set_rip_manager_options_defaults (&g_rmo);
    options_load (&g_rmo, &m_guiOpt);
#endif

    prefs_load ();
    prefs_get_stream_prefs (&g_rmo, "");
    prefs_save ();
    m_guiOpt.m_enabled = 1;

    debug_printf ("Checking if enabled.\n");
    if (!m_guiOpt.m_enabled)
	return 0;

    debug_printf ("Was enabled.\n");
    memset (&wc,0,sizeof(wc));
    wc.lpfnWndProc = WndProc;			// our window procedure
    wc.hInstance = g_plugin.hDllInstance;	// hInstance of DLL
    wc.lpszClassName = m_szWindowClass;		// our window class name
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);

    // Load systray popup menu
    m_hmenu_systray = LoadMenu (g_plugin.hDllInstance, MAKEINTRESOURCE(IDR_TASKBAR_POPUP));
    m_hmenu_systray_sub = GetSubMenu (m_hmenu_systray, 0);
    SetMenuDefaultItem (m_hmenu_systray_sub, 0, TRUE);

    if (!RegisterClass(&wc)) {
	MessageBox (g_plugin.hwndParent,"Error registering window class","blah",MB_OK);
	return 1;
    }
    m_hwnd = CreateWindow (m_szWindowClass, g_plugin.description, WS_POPUP,
			   m_guiOpt.oldpos.x, m_guiOpt.oldpos.y, WINDOW_WIDTH, WINDOW_HEIGHT, 
			   g_plugin.hwndParent, NULL, g_plugin.hDllInstance, NULL);

    // Create a systray icon
    memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hIcon = LoadImage(g_plugin.hDllInstance, MAKEINTRESOURCE(IDI_SR_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    m_nid.hWnd = m_hwnd;
    strcpy(m_nid.szTip, m_szToopTip);
    m_nid.uCallbackMessage = WM_MY_TRAY_NOTIFICATION;
    m_nid.uFlags =  NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uID = 1;
    Shell_NotifyIcon(NIM_ADD, &m_nid);

    // Load main popup menu 
    m_hmenu_context = LoadMenu (g_plugin.hDllInstance, MAKEINTRESOURCE(IDR_HISTORY_POPUP));
    m_hmenu_context_sub = GetSubMenu (m_hmenu_context, 0);
    SetMenuDefaultItem (m_hmenu_context_sub, 0, TRUE);

    // Populate main popup menu
    populate_history_popup ();

    if (!m_guiOpt.m_start_minimized)
	dock_show_window(m_hwnd, SW_SHOWNORMAL);
    else
	dock_show_window(m_hwnd, SW_HIDE);
#endif	
    return 0;
}

INT_PTR CALLBACK
EnableDlgProc (HWND hwndDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    switch (umsg)
    {

    case WM_INITDIALOG:
	{
	    HWND hwndctl = GetDlgItem(hwndDlg, IDC_ENABLE);
	    SendMessage(hwndctl, BM_SETCHECK, 
			(LPARAM)m_enabled ? BST_CHECKED : BST_UNCHECKED,
			(WPARAM)NULL);
	}

    case WM_COMMAND:
	switch(LOWORD(wParam))
	{
	case IDC_OK:
	    {
		HWND hwndctl = GetDlgItem(hwndDlg, IDC_ENABLE);
		m_enabled = SendMessage(hwndctl, BM_GETCHECK, (LPARAM)NULL, (WPARAM)NULL);
		EndDialog(hwndDlg, 0);
	    }
	}
	break;
    case WM_CLOSE:
	EndDialog(hwndDlg, 0);
	break;
    }

    return 0;
}

void
config ()
{
    BOOL prev_enabled = m_enabled;

    DialogBox(g_plugin.hDllInstance, 
	      MAKEINTRESOURCE(IDD_ENABLE), 
	      g_plugin.hwndParent,
	      EnableDlgProc);

//    options_save(&g_rmo, &m_guiOpt);
    if (m_enabled) {
	if (!prev_enabled) {
	    init();
	}
    }
    else if(prev_enabled) {
	quit();
    }
}

void
quit()
{
}

/* dsp_sripper.c
 * should be called gen_sripper.c, main plugin file for streamripper for winamp
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
#include "srtypes.h"
#include "rip_manager.h"
#include "winamp.h"
#include "gen.h"
#include "options.h"
#include "shellapi.h"
#include "dsp_sripper.h"
#include "render.h"
#include "mchar.h"
#include "dock.h"
#include "filelib.h"
#include "commctrl.h"
#include "debug.h"

#define ID_TRAY	1

/* GCS -- why? */
#undef MAX_FILENAME
#define MAX_FILENAME	1024

#define WINDOW_WIDTH		276
#define WINDOW_HEIGHT		150
#define WM_MY_TRAY_NOTIFICATION WM_USER+0

static int  init();
static void config(); 
static void quit();
static BOOL CALLBACK WndProc(HWND hwnd,UINT umsg,WPARAM wParam,LPARAM lParam);
static void RipCallback(int message, void *data);
static void stop_button_pressed();
static void populate_history_popup (void);
static void insert_riplist (char* url, int pos);

RIP_MANAGER_OPTIONS	g_rmo;

static GUI_OPTIONS		m_guiOpt;
static RIP_MANAGER_INFO		m_rmiInfo;
static HWND			m_hwnd;
static BOOL			m_bRipping = FALSE;
static TCHAR 			m_szToopTip[] = "Streamripper For Winamp";
static NOTIFYICONDATA		m_nid;
static char			m_output_dir[MAX_FILENAME];
static HBUTTON			m_startbut;
static HBUTTON			m_stopbut;
static HBUTTON			m_relaybut;
static char			m_szWindowClass[] = "sripper";
static HMENU			m_hmenu_systray = NULL;
static HMENU			m_hmenu_systray_sub = NULL;
static HMENU			m_hmenu_context = NULL;
static HMENU			m_hmenu_context_sub = NULL;
// a hack to make sure the options dialog is not
// open if the user trys to disable streamripper
static BOOL			m_doing_options_dialog = FALSE;
// Ugh, this is awful.  Cache the stream url when requesting 
// from winamp, so that we can detect when there was a change
static char m_winamp_stream_cache[MAX_URL_LEN] = "0";

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
    WNDCLASS wc;
    char* sr_debug_env;

    sr_debug_env = getenv ("STREAMRIPPER_DEBUG");
    if (sr_debug_env) {
	debug_enable();
	debug_set_filename (sr_debug_env);
    }

    winamp_init (g_plugin.hDllInstance);
    set_rip_manager_options_defaults (&g_rmo);
    options_load (&g_rmo, &m_guiOpt);

    if (!m_guiOpt.m_enabled)
	return 0;

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
			(LPARAM)m_guiOpt.m_enabled ? BST_CHECKED : BST_UNCHECKED,
			(WPARAM)NULL);
	}

    case WM_COMMAND:
	switch(LOWORD(wParam))
	{
	case IDC_OK:
	    {
		HWND hwndctl = GetDlgItem(hwndDlg, IDC_ENABLE);
		m_guiOpt.m_enabled = SendMessage(hwndctl, BM_GETCHECK, (LPARAM)NULL, (WPARAM)NULL);
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
    BOOL prev_enabled = m_guiOpt.m_enabled;

    if (m_doing_options_dialog) {
	MessageBox(m_hwnd, "Please close the options dialog in Streamripper before you try this", 
		   "Can not open config dialog", 
		   MB_ICONINFORMATION);
	return;
    }

    DialogBox(g_plugin.hDllInstance, 
	      MAKEINTRESOURCE(IDD_ENABLE), 
	      g_plugin.hwndParent,
	      EnableDlgProc);

    options_save(&g_rmo, &m_guiOpt);
    if (m_guiOpt.m_enabled) {
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
    options_save (&g_rmo, &m_guiOpt);
    if (m_bRipping)
	rip_manager_stop();

    dock_unhook_winamp();

    debug_printf ("Going to render_destroy()...\n");
    render_destroy();
    debug_printf ("Going to DestroyWindow()...\n");
    DestroyWindow(m_hwnd);
    Shell_NotifyIcon(NIM_DELETE, &m_nid);
    DestroyIcon(m_nid.hIcon);
    debug_printf ("Going to UnregisterClass()...\n");
    UnregisterClass(m_szWindowClass, g_plugin.hDllInstance); // unregister window class
    debug_printf ("Finished UnregisterClass()\n");
}

void
start_button_enable()
{
    render_set_button_enabled(m_startbut, TRUE);
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STARTRIPPING, MF_ENABLED);
}

void
start_button_disable()
{
    render_set_button_enabled(m_startbut, FALSE);
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STARTRIPPING, MF_DISABLED|MF_GRAYED);
}

void
stop_button_enable()
{
    render_set_button_enabled(m_stopbut, TRUE);
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STOPRIPPING, MF_ENABLED);
}

void
stop_button_disable()
{
    render_set_button_enabled(m_stopbut, FALSE);
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STOPRIPPING, MF_DISABLED|MF_GRAYED);
}

void
compose_relay_url (char* relay_url, char *host, u_short port, int content_type)
{
    if (content_type == CONTENT_TYPE_OGG) {
	sprintf (relay_url, "http://%s:%d/.ogg", host, port);
    } else if (content_type == CONTENT_TYPE_NSV) {
	sprintf (relay_url, "http://%s:%d/;stream.nsv", host, port);
    } else {
	sprintf (relay_url, "http://%s:%d", host, port);
    }
}

BOOL
url_is_relay (char* url)
{
    char relay_url[SR_MAX_PATH];
    compose_relay_url (relay_url, m_guiOpt.localhost, 
			g_rmo.relay_port, 
			rip_manager_get_content_type());
    debug_printf ("Comparing %s vs rly %s\n", url, relay_url);
    return (!strcmp(relay_url, url));
}

BOOL
url_is_stream (char* url)
{
    return (strchr(url, ':') != 0);
}

void
set_ripping_url (char* url)
{
    if (url) {
	debug_printf ("IDR_STREAMNAME:Press start to rip %s\n", url);
	render_set_display_data (IDR_STREAMNAME, "Press start to rip %s", url);
	start_button_enable ();
    } else {
	debug_printf ("IDR_STREAMNAME:No stream loaded\n");
	render_set_display_data (IDR_STREAMNAME, "No stream loaded");
	start_button_disable ();
    }
}

void
UpdateNotRippingDisplay (HWND hwnd)
{
    WINAMP_INFO winfo;

    debug_printf ("UNRD begin\n");
    if (winamp_get_info (&winfo, m_guiOpt.use_old_playlist_ret)) {
        debug_printf ("UNRD got winamp stream: %s\n", winfo.url);
	if (!strcmp (winfo.url, m_winamp_stream_cache)) {
	    debug_printf ("UNRD return - cached\n");
	    return;
	}
	strcpy (m_winamp_stream_cache, winfo.url);
	
	if (!url_is_stream(winfo.url) || url_is_relay (winfo.url)) {
	    debug_printf ("UNRD not_stream/is_relay: %d\n", g_rmo.url[0]);
	    if (g_rmo.url[0]) {
		set_ripping_url (g_rmo.url);
	    } else {
		set_ripping_url (0);
	    }
	} else {
	    debug_printf ("UNRD setting g_rmo.url: %s\n", g_rmo.url);
	    strcpy(g_rmo.url, winfo.url);
	    insert_riplist (winfo.url, 0);
	    set_ripping_url (winfo.url);
	}
    }
}

void
UpdateRippingDisplay ()
{
    static int buffering_tick = 0;
    char sStatusStr[50];

    if (m_rmiInfo.status == 0)
	return;

    switch(m_rmiInfo.status)
    {
    case RM_STATUS_BUFFERING:
	buffering_tick++;
	if (buffering_tick == 30)
	    buffering_tick = 0;
	strcpy(sStatusStr,"Buffering... ");
	break;
    case RM_STATUS_RIPPING:
	strcpy(sStatusStr, "Ripping...    ");
	break;
    case RM_STATUS_RECONNECTING:
	strcpy(sStatusStr, "Re-connecting..");
	break;
    default:
	debug_printf("************ what am i doing here?");
    }
    render_set_display_data(IDR_STATUS, "%s", sStatusStr);

    if (!m_rmiInfo.streamname[0]) {
	return;
    }

    debug_printf ("IDR_STREAMNAME:%s\n", m_rmiInfo.streamname);
    render_set_display_data(IDR_STREAMNAME, "%s", m_rmiInfo.streamname);
    render_set_display_data(IDR_BITRATE, "%dkbit", m_rmiInfo.bitrate);
    render_set_display_data(IDR_SERVERTYPE, "%s", m_rmiInfo.server_name);

    if ((m_rmiInfo.meta_interval == -1) && 
	(strstr(m_rmiInfo.server_name, "Nanocaster") != NULL))
    {
	render_set_display_data(IDR_METAINTERVAL, "Live365 Stream");
    } else if(m_rmiInfo.meta_interval) {
	render_set_display_data(IDR_METAINTERVAL, "MetaInt:%d", m_rmiInfo.meta_interval);
    } else {
	render_set_display_data(IDR_METAINTERVAL, "No track data");
    }

    if (m_rmiInfo.filename[0]) {
	char strsize[50];
	format_byte_size(strsize, m_rmiInfo.filesize);
	render_set_display_data(IDR_FILENAME, "[%s] - %s", strsize, m_rmiInfo.filename);
    } else {
	render_set_display_data(IDR_FILENAME, "Getting track data...");
    }
}

VOID CALLBACK
UpdateDisplay(HWND hwnd, UINT umsg, UINT_PTR idEvent,DWORD dwTime)
{

    if (m_bRipping)
	UpdateRippingDisplay();
    else
	UpdateNotRippingDisplay(hwnd);

    InvalidateRect(m_hwnd, NULL, FALSE);
}

void
RipCallback (int message, void *data)
{
    RIP_MANAGER_INFO *info;
    ERROR_INFO *err;
    switch(message)
    {
    case RM_UPDATE:
	info = (RIP_MANAGER_INFO*)data;
	memcpy(&m_rmiInfo, info, sizeof(RIP_MANAGER_INFO));
	break;
    case RM_ERROR:
	err = (ERROR_INFO*)data;
	debug_printf("***RipCallback: about to post error dialog");
	MessageBox(m_hwnd, err->error_str, "Streamripper", MB_SETFOREGROUND);
	debug_printf("***RipCallback: done posting error dialog");
	break;
    case RM_DONE:
	//stop_button_pressed();
	//
	// calling the stop button in here caused all kinds of stupid problems
	// so, fuck it. call the clearing button shit here. 
	//
	render_clear_all_data();
	m_bRipping = FALSE;
	start_button_enable();
	stop_button_disable();
	render_set_prog_bar(FALSE);

	break;
    case RM_TRACK_DONE:
	if (m_guiOpt.m_add_finshed_tracks_to_playlist)
	    winamp_add_track_to_playlist((char*)data);
	break;
    case RM_NEW_TRACK:
	break;
    case RM_STARTED:
	stop_button_enable();
	break;
#if defined (commentout)
    /* GCS: This no longer applies as of adding the filename patterns */
    case RM_OUTPUT_DIR:
	strcpy(m_output_dir, (char*)data);
#endif
    }
}

void
start_button_pressed()
{
    int ret;
    debug_printf ("Start button pressed\n");
    insert_riplist (g_rmo.url, 0);

    assert(!m_bRipping);
    render_clear_all_data();
    debug_printf ("IDR_STREAMNAME:Connecting...\n");
    render_set_display_data(IDR_STREAMNAME, "Connecting...");
    start_button_disable();

    if ((ret = rip_manager_start(RipCallback, &g_rmo)) != SR_SUCCESS)
    {
	MessageBox(m_hwnd, rip_manager_get_error_str(ret), "Failed to connect to stream", MB_ICONSTOP);
	start_button_enable();
	return;
    }
    m_bRipping = TRUE;

    render_set_prog_bar(TRUE);
    PostMessage(m_hwnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
}

void
stop_button_pressed()
{
    debug_printf ("Stop button pressed\n");

    stop_button_disable();
    assert(m_bRipping);
    render_clear_all_data();

    rip_manager_stop();

    m_bRipping = FALSE;
    start_button_enable();
    stop_button_disable();
    set_ripping_url(g_rmo.url);
    render_set_prog_bar(FALSE);
}

void
options_button_pressed()
{
    debug_printf ("Options button pressed\n");

    m_doing_options_dialog = TRUE;
    options_dialog_show (g_plugin.hDllInstance, m_hwnd, &g_rmo, &m_guiOpt);

    render_set_button_enabled (m_relaybut, OPT_FLAG_ISSET(g_rmo.flags, OPT_MAKE_RELAY)); 			
    m_doing_options_dialog = FALSE;
}

void
close_button_pressed()
{
    dock_show_window(m_hwnd, SW_HIDE);
    m_guiOpt.m_start_minimized = TRUE;
}

void
relay_pressed()
{
    winamp_add_relay_to_playlist (m_guiOpt.localhost, 
	g_rmo.relay_port, 
	rip_manager_get_content_type());
}

void
debug_riplist (void)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	debug_printf ("riplist%d=%s\n", i, m_guiOpt.riplist[i]);
    }
}

/* return -1 if not in riplist */
int
find_url_in_riplist (char* url)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	if (!strcmp(m_guiOpt.riplist[i],url)) {
	    return i;
	}
    }
    return -1;
}

/* pos is 0 to put at top, or 1 to put next to top */
void
insert_riplist (char* url, int pos)
{
    int i;
    int oldpos;

    debug_printf ("Insert riplist (0)\n");

    /* Don't add if it's the relay stream */
    if (url_is_relay (url)) return;

    debug_printf ("Insert riplist (1): %d %s\n", pos, url);
    debug_riplist ();

    /* oldpos is previous position of this url. Don't shift if 
       it's already at that position or above. */
    oldpos = find_url_in_riplist (url);
    if (oldpos == -1) {
	oldpos = RIPLIST_LEN - 1;
    }
    if (oldpos <= pos) return;

    /* Shift the url to the correct position */
    for (i = oldpos; i > pos; i--) {
	strcpy(m_guiOpt.riplist[i], m_guiOpt.riplist[i-1]);
    }
    strcpy(m_guiOpt.riplist[pos], url);

    debug_printf ("Insert riplist (2): %d %s\n", pos, url);
    debug_riplist ();

    /* Rebuild the history menu */
    populate_history_popup ();
}

void
populate_history_popup (void)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	RemoveMenu (m_hmenu_context_sub, ID_MENU_HISTORY_LIST+i, MF_BYCOMMAND);
    }
    for (i = 0; i < RIPLIST_LEN; i++) {
	if (m_guiOpt.riplist[i][0]) {
	    AppendMenu (m_hmenu_context_sub, MF_ENABLED | MF_STRING, ID_MENU_HISTORY_LIST+i, m_guiOpt.riplist[i]);
	}
    }
}

BOOL CALLBACK
WndProc (HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hBrush = NULL;

    switch (umsg)
    {
    case WM_CREATE:
	if (!render_init(g_plugin.hDllInstance, hwnd, m_guiOpt.default_skin))
	{
	    MessageBox(hwnd, "Failed to find the skin bitmap", "Error", 0);
	    break;
	}
	{
	    RECT rt = {0, 0, 276, 150};	// background

	    // points for the shape of the main window
	    POINT rgn_points[] = {
		{2,		0},
		{273+1,	0},
		{273+1,	1},
		{274+1,	1},
		{274+1,	2},
		{275+1,	2},
		{275+1,	147+1},
		{274+1,	147+1},
		{274+1,	148+1},
		{273+1,	148+1},
		{273+1,	149+1},
		{2,		149+1},
		{2,		148+1},
		{1,		148+1},
		{1,		147+1},
		{0,		147+1},
		{0,		2},
		{1,		2},
		{1,		1},
		{2,		1}
	    };

	    render_set_background(&rt, rgn_points, sizeof(rgn_points)/sizeof(POINT));
	}

	// Start button
	{
	    RECT rt[] = {
		{277, 1, 325, 21},		// Noraml
		{327, 1, 375, 21},		// Pressed
		{277, 67, 325, 87},		// Over
		{327, 67, 375, 87},		// Grayed
		{12, 123, 60, 143}		// Dest
	    };
	    m_startbut = render_add_button(&rt[0],&rt[1], &rt[2], &rt[3], &rt[4], start_button_pressed);
	}

	// Stop button
	{
	    RECT rt[] = {
		{277, 23, 325, 43},
		{327, 23, 375, 43},
		{277, 89, 325, 109},
		{327, 89, 375, 109},
		{68, 123, 116, 143} // dest
	    };
	    m_stopbut = render_add_button(&rt[0],&rt[1], &rt[2], &rt[3], &rt[4], stop_button_pressed);
	    stop_button_disable();
	}

	// Options button
	{
	    RECT rt[] = {
		{277, 45, 325, 65},
		{327, 45, 375, 65},
		{277, 111, 325, 131},
		{327, 111, 375, 131},
		{215, 123, 263, 143} // dest
	    };
	    render_add_button(&rt[0],&rt[1], &rt[2], &rt[3], &rt[4], options_button_pressed);
	}

	// Min  button
	{
	    RECT rt[] = {
		{373, 133, 378, 139},
		{387, 133, 392, 139},	//pressed
		{380, 133, 385, 139},	//over
		{373, 133, 378, 139},	//gray
		{261, 4, 266, 10}
	    };
	    render_add_button(&rt[0],&rt[1], &rt[2], &rt[3], &rt[4], close_button_pressed);
	}
			
	// Relay button
	{
	    RECT rt[] = { 
		{277, 133, 299, 148},	// normal
		{325, 133, 347, 148},	// pressed
		{301, 133, 323, 148},
		{349, 133, 371, 148},   // Grayed
		{10, 24, 32, 39}
	    };
	    m_relaybut = render_add_button(&rt[0],&rt[1], &rt[2], &rt[3], &rt[4], relay_pressed);
	    render_set_button_enabled(m_relaybut, OPT_FLAG_ISSET(g_rmo.flags, OPT_MAKE_RELAY));
	}

	// Set the progress bar
	{
	    RECT rt = {373, 141, 378, 148};	// progress bar image
	    POINT pt = {105, 95};
	    render_set_prog_rects(&rt, pt, RGB(100, 111, 132));
	}


	// Set positions for out display text
	{

	    int i;
	    RECT rt[] = {
		{42, 95, 88, 95+12},	// Status
		{42, 49, 264, 49+12},	// Filename
		{42, 26, 264, 26+12},	// Streamname
		{42, 66, 264, 66+12},	// Server Type
		{42, 78, 85, 78+12},	// Bitrate
		{92, 78, 264, 78+12}	// MetaInt
	    };
				
	    for(i = 0; i < IDR_NUMFIELDS; i++)
		render_set_display_data_pos(i, &rt[i]);
	}
	{
	    POINT pt = {380, 141};
	    render_set_text_color(pt);
	}

	{
	    debug_printf ("IDR_STREAMNAME:Loading please wait...\n");
	    render_set_display_data(IDR_STREAMNAME, "Loading please wait...");
	}
			
	SetTimer (hwnd, 1, 500, (TIMERPROC)UpdateDisplay);
	dock_init (hwnd);

	return 0;

    case WM_PAINT:
	{
	    PAINTSTRUCT pt;
	    HDC hdc = BeginPaint(hwnd, &pt);
	    render_do_paint(hdc);
	    EndPaint(hwnd, &pt);
	}
	return 0;

		
    case WM_MOUSEMOVE:
	render_do_mousemove(hwnd, wParam, lParam);
	dock_do_mousemove(hwnd, wParam, lParam);
	break;

    case WM_COMMAND:
	switch(wParam)
	{
	case ID_MENU_STARTRIPPING:
	    start_button_pressed();
	    break;
	case ID_MENU_STOPRIPPING:
	    stop_button_pressed();
	    break;
	case ID_MENU_OPTIONS:
	    options_button_pressed();
	    break;
	case ID_MENU_OPEN:
	    PostMessage(hwnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
	    break;
	case ID_MENU_RESET_URL:
	    strcpy(g_rmo.url, "");
	    set_ripping_url (0);
	    break;
	default:
	    if (wParam >= ID_MENU_HISTORY_LIST && wParam < ID_MENU_HISTORY_LIST + RIPLIST_LEN) {
		int i = wParam - ID_MENU_HISTORY_LIST;
		char* url = m_guiOpt.riplist[i];
		debug_printf ("Setting URL through history list\n");
		strcpy(g_rmo.url, url);
		set_ripping_url (url);
	    }
	    break;
	}
	break;

    case WM_MY_TRAY_NOTIFICATION:
	switch(lParam)
	{
	case WM_LBUTTONDBLCLK:
	    dock_show_window(m_hwnd, SW_NORMAL);
	    SetForegroundWindow(hwnd);
	    m_guiOpt.m_start_minimized = FALSE;
	    break;

	case WM_RBUTTONDOWN:
	    {
		int item;
		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow(hwnd);
		item = TrackPopupMenu(m_hmenu_systray_sub, 
				      0,
				      pt.x,
				      pt.y,
				      (int)NULL,
				      hwnd,
				      NULL);
	    }
	    break;
	}
	break;

    case WM_LBUTTONDOWN:
	dock_do_lbuttondown(hwnd, wParam, lParam);
	render_do_lbuttondown(hwnd, wParam, lParam);
	break;

    case WM_LBUTTONUP:
	dock_do_lbuttonup(hwnd, wParam, lParam);
	render_do_lbuttonup(hwnd, wParam, lParam);
	{
	    BOOL rc;
	    RECT rt;
	    rc = GetWindowRect(hwnd, &rt);
	    if (rc) {
		m_guiOpt.oldpos.x = rt.left;
		m_guiOpt.oldpos.y = rt.top;
	    }
	}
	break;
	
    case WM_RBUTTONDOWN:
	{
	    int item;
	    POINT pt;
	    if (!m_bRipping) {
		GetCursorPos (&pt);
		SetForegroundWindow (hwnd);
		item = TrackPopupMenu (m_hmenu_context_sub, 
					0,
					pt.x,
					pt.y,
					(int)NULL,
					hwnd,
					NULL);
	    }
	}
	break;


    }
    return DefWindowProc (hwnd, umsg, wParam, lParam);
}

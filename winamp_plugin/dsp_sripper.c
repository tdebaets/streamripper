/* dsp_sripper.c - jonclegg@yahoo.com
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
#include "types.h"
#include "rip_manager.h"
#include "winamp.h"
#include "gen.h"
#include "options.h"
#include "shellapi.h"
#include "dsp_sripper.h"
#include "render.h"
#include "../lib/util.h"	// because windows has a util.h, and gets very confused.
#include "dock.h"
#include "filelib.h"
#include "commctrl.h"
#include "debug.h"

#define ID_TRAY	1

#undef MAX_FILENAME
#define MAX_FILENAME	1024

#define WINDOW_WIDTH		276
#define WINDOW_HEIGHT		150
#define WM_MY_TRAY_NOTIFICATION WM_USER+0


static int	init();
static void	config(); 
static void	quit();
static BOOL	CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
static int	dont_modify_samples(struct winampDSPModule *this_mod, short int *samples, int numsamples, int bps, int nch, int srate);
static void	RipCallback(int message, void *data);
static void stop_button_pressed();
static void rotate_files(char *trackname);


BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{return TRUE;}


static RIP_MANAGER_OPTIONS	m_rmoOpt;
static GUI_OPTIONS			m_guiOpt;
static RIP_MANAGER_INFO		m_rmiInfo;
static HWND					m_hWnd;
static BOOL					m_bRipping = FALSE;
static TCHAR 				m_szToopTip[] = "Streamripper For Winamp";
static NOTIFYICONDATA		m_nid;
static char					m_output_dir[MAX_FILENAME];
static HBUTTON				m_startbut;
static HBUTTON				m_stopbut;
static HBUTTON				m_relaybut;
static char					m_szWindowClass[] = "sripper";
static HMENU				m_hMenu = NULL;
static HMENU				m_hPopupMenu = NULL;
static BOOL					m_doing_options_dialog = FALSE;		// a hack to make sure the options dialog is not
																// open if the user trys to disable streamripper													

winampGeneralPurposePlugin m_plugin= {GPPHDR_VER,
									  m_szToopTip,
									  init,
									  config,
									  quit};

int dont_modify_samples(struct winampDSPModule *this_mod, short int *samples, int numsamples, int bps, int nch, int srate)
{
	return numsamples;
}

int init()
{
	WNDCLASS wc;
	
	winamp_init(m_plugin.hDllInstance);
	options_load(&m_rmoOpt, &m_guiOpt);

	if (!m_guiOpt.m_enabled)
		return 0;

	memset(&wc,0,sizeof(wc));
	wc.lpfnWndProc = WndProc;				// our window procedure
	wc.hInstance = m_plugin.hDllInstance;	// hInstance of DLL
	wc.lpszClassName = m_szWindowClass;			// our window class name
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	
	// Load popup menu 
	m_hMenu = LoadMenu(m_plugin.hDllInstance, MAKEINTRESOURCE(IDR_POPUP_MENU));
	m_hPopupMenu = GetSubMenu(m_hMenu, 0);
	SetMenuDefaultItem(m_hPopupMenu, 0, TRUE);

	if (!RegisterClass(&wc)) 
	{
		MessageBox(m_plugin.hwndParent,"Error registering window class","blah",MB_OK);
		return 1;
	}
	m_hWnd = CreateWindow(m_szWindowClass, m_plugin.description, WS_POPUP,
	  m_guiOpt.oldpos.x, m_guiOpt.oldpos.y, WINDOW_WIDTH, WINDOW_HEIGHT, 
	  m_plugin.hwndParent, NULL, m_plugin.hDllInstance, NULL);



	// Create a systray icon
	memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hIcon = LoadImage(m_plugin.hDllInstance, MAKEINTRESOURCE(IDI_SR_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_nid.hWnd = m_hWnd;
	strcpy(m_nid.szTip, m_szToopTip);
	m_nid.uCallbackMessage = WM_MY_TRAY_NOTIFICATION;
	m_nid.uFlags =  NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_nid.uID = 1;
	Shell_NotifyIcon(NIM_ADD, &m_nid);

	if (!m_guiOpt.m_start_minimized)
		dock_show_window(m_hWnd, SW_SHOWNORMAL);
	else
		dock_show_window(m_hWnd, SW_HIDE);
	
	return 0;
}

INT_PTR CALLBACK EnableDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
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

void config()
{
	BOOL prev_enabled = m_guiOpt.m_enabled;

	if (m_doing_options_dialog)
	{
		MessageBox(m_hWnd, "Please close the options dialog in Streamripper before you try this", 
						   "Can not open config dialog", 
						   MB_ICONINFORMATION);
		return;
	}

	DialogBox(m_plugin.hDllInstance, 
			  MAKEINTRESOURCE(IDD_ENABLE), 
			  m_plugin.hwndParent,
			  EnableDlgProc);

	options_save(&m_rmoOpt, &m_guiOpt);
	if (m_guiOpt.m_enabled)
	{
		if (!prev_enabled)
		{
			init();
		}
	}
	else if(prev_enabled)
		quit();
}

void quit()
{
	options_save(&m_rmoOpt, &m_guiOpt);
	if (m_bRipping)
		rip_manager_stop();

	dock_unhook_winamp();


	render_destroy();
	DestroyWindow(m_hWnd);
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
	DestroyIcon(m_nid.hIcon);
	UnregisterClass(m_szWindowClass, m_plugin.hDllInstance); // unregister window class

}


void start_button_enable()
{
	render_set_button_enabled(m_startbut, TRUE);
	EnableMenuItem(m_hPopupMenu, ID_MENU_STARTRIPPING, MF_ENABLED);
}

void start_button_disable()
{
	render_set_button_enabled(m_startbut, FALSE);
	EnableMenuItem(m_hPopupMenu, ID_MENU_STARTRIPPING, MF_DISABLED|MF_GRAYED);
}

void stop_button_enable()
{
	render_set_button_enabled(m_stopbut, TRUE);
	EnableMenuItem(m_hPopupMenu, ID_MENU_STOPRIPPING, MF_ENABLED);
}

void stop_button_disable()
{
	render_set_button_enabled(m_stopbut, FALSE);
	EnableMenuItem(m_hPopupMenu, ID_MENU_STOPRIPPING, MF_DISABLED|MF_GRAYED);
}


void UpdateNotRippingDisplay(HWND hwnd)
{
	WINAMP_INFO winfo;
//	HWND hwndStart = GetDlgItem(hwnd, IDC_START);		// JCBUG, why was this here?
	winamp_get_info(&winfo, m_guiOpt.use_old_playlist_ret);
	assert(winfo.is_running);
	strcpy(m_rmoOpt.url, winfo.url);

	if (strchr(m_rmoOpt.url, ':'))
	{
//		EnableWindow(hwndStart, TRUE);
		render_set_display_data(IDR_STREAMNAME, "Press start to rip %s", m_rmoOpt.url);
		start_button_enable();
	}
	else
	{
//		EnableWindow(hwndStart, FALSE);
		render_set_display_data(IDR_STREAMNAME, "Winamp is not listening to a stream");
		start_button_disable();
	}
}

void UpdateRippingDisplay()
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
			DEBUG1(("************ what am i doing here?"));
	}
	render_set_display_data(IDR_STATUS, "%s", sStatusStr);

	if (!m_rmiInfo.streamname[0])
	{
		return;
	}

	render_set_display_data(IDR_STREAMNAME, "%s", m_rmiInfo.streamname);
	render_set_display_data(IDR_BITRATE, "%dkbit", m_rmiInfo.bitrate);
	render_set_display_data(IDR_SERVERTYPE, "%s", m_rmiInfo.server_name);

	if ((m_rmiInfo.meta_interval == -1) && 
		(strstr(m_rmiInfo.server_name, "Nanocaster") != NULL))
	{
		render_set_display_data(IDR_METAINTERVAL, "Live365 Stream");
	}
	else if(m_rmiInfo.meta_interval)
	{
		render_set_display_data(IDR_METAINTERVAL, "MetaInt:%d", m_rmiInfo.meta_interval);
	}
	else
	{
		render_set_display_data(IDR_METAINTERVAL, "No track data");
	}

	if (m_rmiInfo.filename[0])
	{
		char strsize[50];
		format_byte_size(strsize, m_rmiInfo.filesize);
		render_set_display_data(IDR_FILENAME, "[%s] - %s", strsize, m_rmiInfo.filename);
	}
	else
		render_set_display_data(IDR_FILENAME, "Getting track data...");

}

VOID CALLBACK UpdateDisplay(HWND hwnd, UINT uMsg, UINT_PTR idEvent,DWORD dwTime)
{

	if (m_bRipping)
		UpdateRippingDisplay();
	else
		UpdateNotRippingDisplay(hwnd);

	InvalidateRect(m_hWnd, NULL, FALSE);
	
}

void RipCallback(int message, void *data)
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
			DEBUG1(("***RipCallback: about to post error dialog"));
			MessageBox(m_hWnd, err->error_str, "Streamripper", MB_SETFOREGROUND);
			DEBUG1(("***RipCallback: done posting error dialog"));
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
		case RM_OUTPUT_DIR:
			strcpy(m_output_dir, (char*)data);
	}
}


void start_button_pressed()
{
	int ret;
	DEBUG0(("start"));

	assert(!m_bRipping);
	render_clear_all_data();
	render_set_display_data(IDR_STREAMNAME, "Connecting...");
	start_button_disable();

	if ((ret = rip_manager_start(RipCallback, &m_rmoOpt)) != SR_SUCCESS)
	{
		MessageBox(m_hWnd, rip_manager_get_error_str(ret), "Failed to connect to stream", MB_ICONSTOP);
		start_button_enable();
		return;
	}
	m_bRipping = TRUE;

	render_set_prog_bar(TRUE);
	PostMessage(m_hWnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
}

void stop_button_pressed()
{
	DEBUG0(("stop"));

	stop_button_disable();
	assert(m_bRipping);
	render_clear_all_data();

	rip_manager_stop();

	m_bRipping = FALSE;
	start_button_enable();
	stop_button_disable();
	render_set_prog_bar(FALSE);
}

void options_button_pressed()
{
	m_doing_options_dialog = TRUE;
	options_dialog_show(m_plugin.hDllInstance, m_hWnd, &m_rmoOpt, &m_guiOpt);

	render_set_button_enabled(m_relaybut, OPT_FLAG_ISSET(m_rmoOpt.flags, OPT_MAKE_RELAY)); 			
	m_doing_options_dialog = FALSE;

}

void close_button_pressed()
{
	dock_show_window(m_hWnd, SW_HIDE);
	m_guiOpt.m_start_minimized = TRUE;
}


void relay_pressed()
{
	winamp_add_relay_to_playlist(m_guiOpt.localhost, rip_mananger_get_relay_port());
}

BOOL CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HBRUSH hBrush = NULL;

	switch (uMsg)
	{
		case WM_CREATE:
			if (!render_init(m_plugin.hDllInstance, hWnd, m_guiOpt.default_skin))
			{
				MessageBox(hWnd, "Failed to find the skin bitmap", "Error", 0);
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
				render_set_button_enabled(m_relaybut, OPT_FLAG_ISSET(m_rmoOpt.flags, OPT_MAKE_RELAY));
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
				render_set_display_data(IDR_STREAMNAME, "Loading please wait...");
			}
			
			SetTimer(hWnd, 1, 500, (TIMERPROC)UpdateDisplay);
			dock_hook_winamp(hWnd);

			return 0;

		case WM_PAINT:
			{
				PAINTSTRUCT pt;
				HDC hdc = BeginPaint(hWnd, &pt);
				render_do_paint(hdc);
				EndPaint(hWnd, &pt);
			}
			return 0;

		
		case WM_MOUSEMOVE:
			render_do_mousemove(hWnd, wParam, lParam);
			dock_do_mousemove(hWnd, wParam, lParam);
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
					PostMessage(hWnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
					break;
			}
			break;

		case WM_MY_TRAY_NOTIFICATION:
			switch(lParam)
			{
				case WM_LBUTTONDBLCLK:
					dock_show_window(m_hWnd, SW_NORMAL);
					SetForegroundWindow(hWnd);
					m_guiOpt.m_start_minimized = FALSE;
					break;

                case WM_RBUTTONDOWN:
					{
						int item;
						POINT pt;
						GetCursorPos(&pt);
						SetForegroundWindow(hWnd);
						item = TrackPopupMenu(m_hPopupMenu, 
									   0,
									   pt.x,
									   pt.y,
									   (int)NULL,
									   hWnd,
									   NULL);
					}
					break;
			}
			break;

		case WM_LBUTTONDOWN:
			dock_do_lbuttondown(hWnd, wParam, lParam);
			render_do_lbuttondown(hWnd, wParam, lParam);
			break;

		case WM_LBUTTONUP:
			dock_do_lbuttonup(hWnd, wParam, lParam);
			render_do_lbuttonup(hWnd, wParam, lParam);
			{
				RECT rt;
				GetWindowRect(hWnd, &rt);
				m_guiOpt.oldpos.x = rt.left;
				m_guiOpt.oldpos.y = rt.top;
			}
			break;
	

	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &m_plugin;
}


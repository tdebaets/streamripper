/* dock.c
 * handles hooking winamp, and making the streamripper window "dock"
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

#include "windows.h"
#include <stdio.h>
#include "dock.h"
#include "debug.h"

#define SNAP_OFFSET		10	
#define WINAMP_WINDOWS		4

#define DOCKED_TOP_LL		1	// Top Left Left
#define DOCKED_TOP_LR		2	// Top Left Right
#define DOCKED_TOP_RL		3	// Top Right Left
#define DOCKED_LEFT_TT		4
#define DOCKED_LEFT_TB		5
#define DOCKED_LEFT_BT		6
#define DOCKED_BOTTOM_LL	7
#define DOCKED_BOTTOM_RL	8
#define DOCKED_BOTTOM_LR	9
#define DOCKED_RIGHT_TT		10
#define DOCKED_RIGHT_TB		11
#define DOCKED_RIGHT_BT		12

// My extensions
#define DOCKED_LEFT		100
#define DOCKED_TOP		101
#define DOCKED_RIGHT		102
#define DOCKED_BOTTOM		103


#define RTWIDTH(rect)	((rect).right-(rect).left)
#define RTHEIGHT(rect)	((rect).bottom-(rect).top)
#define SETWINDOWPOS(left, top)	\
		SetWindowPos(hWnd, NULL, left, top, RTWIDTH(rt), RTHEIGHT(rt), SWP_SHOWWINDOW);

/*****************************************************************************
 * Public functions
 *****************************************************************************/
BOOL dock_hook_winamp(HWND hwnd);
VOID dock_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_show_window(HWND hWnd, int nCmdShow);
BOOL dock_unhook_winamp();

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static LRESULT CALLBACK	hook_winamp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID			dock_window();
static BOOL			set_dock_side(RECT *rtnew);
static BOOL			find_winamp_windows(HWND hWnd);
static VOID			get_new_rect(HWND hWnd, POINTS cur, POINTS last, RECT *rtnew);


/*****************************************************************************
 * Private Vars
 *****************************************************************************/
static POINTS		m_drag_from = {0, 0};
static BOOL		m_dragging = FALSE;
static BOOL		m_docked = FALSE;
static int		m_docked_side;
static HWND		m_hwnd = NULL;
static POINT		m_docked_diff = {0, 0};
static int		m_docked_index;
static struct PARENTS
{		
    HWND hwnd;
    BOOL visible;
    WNDPROC orig_proc;
} m_winamp_wins[WINAMP_WINDOWS];


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    char classname[256];
    HWND bigowner = (HWND)lParam;
    HWND owner = GetWindow(hwnd, GW_OWNER);

    GetClassName(hwnd, classname, 256); 

    if (owner != bigowner )
	return TRUE;

    debug_printf ("Enum found: %s\n", classname);

    if (strcmp(classname, "Winamp PE") == 0) {
	m_winamp_wins[1].hwnd = hwnd;
    } else if (strcmp(classname, "Winamp EQ") == 0) {
	m_winamp_wins[2].hwnd = hwnd;
    } else if (strcmp(classname, "Winamp Video") == 0) {
	m_winamp_wins[3].hwnd = hwnd;
    }
#if defined (commentout)
    if (m_winamp_wins[1].hwnd != NULL &&
	m_winamp_wins[2].hwnd != NULL &&
	m_winamp_wins[3].hwnd != NULL)
    {
	return FALSE;
    }
#endif
    return TRUE;
}

BOOL find_winamp_windows(HWND hWnd)
{
    int i;
    long style = 0;

    m_winamp_wins[1].hwnd = m_winamp_wins[2].hwnd = m_winamp_wins[3].hwnd = NULL;
    m_winamp_wins[0].hwnd = GetParent(hWnd);
    debug_printf ("Starting enumeration of windows\n");
    EnumWindows (EnumWindowsProc, (LPARAM)m_winamp_wins[0].hwnd);

    for (i = 0; i < WINAMP_WINDOWS; i++) {
    	if (m_winamp_wins[i].hwnd == NULL)
	    return FALSE;

	style = GetWindowLong(m_winamp_wins[i].hwnd, GWL_STYLE);
	m_winamp_wins[i].visible = style & WS_VISIBLE;
	debug_printf ("Win %d, visibility %d\n", i, m_winamp_wins[i].visible);
    }
    debug_printf ("Found all the windows\n");
    return TRUE;
}

BOOL
dock_hook_winamp (HWND hwnd)
{
    int i;

    if (!find_winamp_windows(hwnd))
	return FALSE;

    // hook em'
    for (i = 0; i < WINAMP_WINDOWS; i++) {
	m_winamp_wins[i].orig_proc = NULL;
#if defined (commentout)
	/* GCS not sure why this visibility test is needed */
	if (!m_winamp_wins[i].visible)
	    continue;
#endif

	debug_printf ("Hooking...\n");
	m_winamp_wins[i].orig_proc = (WNDPROC)SetWindowLong(m_winamp_wins[i].hwnd, GWL_WNDPROC, (LONG)hook_winamp);
	if (m_winamp_wins[i].orig_proc == NULL)
	    return FALSE;
    }
    m_hwnd = hwnd;
    return TRUE;
}

LRESULT CALLBACK
hook_winamp (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;

    if (uMsg == WM_MOVE) 
	dock_window();

    for (i = 0; i < WINAMP_WINDOWS; i++) {
	if (m_winamp_wins[i].hwnd == hwnd)
	    return CallWindowProc(m_winamp_wins[i].orig_proc, hwnd, uMsg, wParam, lParam); 
    }

    //	MessageBox(hwnd, "Shit, this shouldn't happen", "doh", (UINT)0);
    return FALSE;
}


VOID
dock_show_window(HWND hWnd, int nCmdShow)
{
    RECT rt;

    // Make sure we know about all the windows
    find_winamp_windows(hWnd);

    // Have it set the docking point to where-ever the window is
    GetWindowRect(hWnd, &rt);
    set_dock_side(&rt);

    ShowWindow(hWnd, nCmdShow);
}


BOOL
dock_unhook_winamp()
{
    int i;

    for(i = 0; i < WINAMP_WINDOWS; i++)
	if (m_winamp_wins[i].orig_proc)
	    SetWindowLong(m_winamp_wins[i].hwnd, GWL_WNDPROC,(LONG)m_winamp_wins[i].orig_proc); 

    m_dragging = FALSE;
    m_docked = FALSE;
    m_docked_side = 0;
    m_hwnd = NULL;

    return TRUE;
}

// This taken from James Spibey's <spib@bigfoot.com> code example for 
// how to do a winamp plugin the goto's are mine :)
VOID
dock_window()
{
    int i;
    RECT rt;
    int left, top;
    RECT	rtparents[4];

    if(!m_docked)
	return;

    GetWindowRect(m_hwnd, &rt);

    for(i = 0; i < WINAMP_WINDOWS; i++)
	GetWindowRect(m_winamp_wins[i].hwnd, &rtparents[i]);

    i = m_docked_index;
    switch(m_docked_side)
    {
    case DOCKED_TOP_LL:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].left;
	goto SETWINDOW;
    case DOCKED_TOP_LR:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].right;
	goto SETWINDOW;
    case DOCKED_TOP_RL:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_LEFT_TT:
	top = rtparents[i].top;
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_LEFT_TB:
	top = rtparents[i].bottom;
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_LEFT_BT:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_BOTTOM_LL:
	top = rtparents[i].bottom;
	left = rtparents[i].left;
	goto SETWINDOW;
    case DOCKED_BOTTOM_RL:
	top = rtparents[i].bottom;
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_BOTTOM_LR:
	top = rtparents[i].bottom;
	left = rtparents[i].right;
	goto SETWINDOW;
    case DOCKED_RIGHT_TT:
	top = rtparents[i].top;
	left = rtparents[i].right;
	goto SETWINDOW;
    case DOCKED_RIGHT_TB:
	top = rtparents[i].bottom;
	left = rtparents[i].right;
	goto SETWINDOW;
    case DOCKED_RIGHT_BT:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].right;
	goto SETWINDOW;

	// My extensions
    case DOCKED_LEFT:
	top = rtparents[i].top + m_docked_diff.y;
	left = rtparents[i].left - RTWIDTH(rt);
	goto SETWINDOW;
    case DOCKED_RIGHT:
	top = rtparents[i].top + m_docked_diff.y;
	left = rtparents[i].right;
	goto SETWINDOW;
    case DOCKED_TOP:
	top = rtparents[i].top - RTHEIGHT(rt);
	left = rtparents[i].left + m_docked_diff.x;
	goto SETWINDOW;
    case DOCKED_BOTTOM:
	top = rtparents[i].bottom;
	left = rtparents[i].left + m_docked_diff.x;
	goto SETWINDOW;

    default:
	return;
    }
 SETWINDOW:
    SetWindowPos(m_hwnd, NULL, left, top, RTWIDTH(rt), RTHEIGHT(rt), SWP_NOACTIVATE);
}

VOID
get_new_rect (HWND hWnd, POINTS cur, POINTS last, RECT *rtnew)
{
    RECT rt;
    POINT diff = {cur.x-last.x, cur.y-last.y};

    GetWindowRect(hWnd, &rt);
    rtnew->left = rt.left+diff.x;
    rtnew->top = rt.top+diff.y;
    rtnew->right = rt.right+diff.x;
    rtnew->bottom = rt.bottom+diff.y;
}

VOID
dock_do_mousemove (HWND hWnd, LONG wParam, LONG lParam)
{
    RECT rtnew;

    // Simulate the the caption bar
    if (!(m_dragging && (wParam & MK_LBUTTON)))
	return;

    get_new_rect(hWnd, MAKEPOINTS(lParam), m_drag_from, &rtnew);

    if (!set_dock_side(&rtnew)) {
	SetWindowPos(hWnd, NULL, rtnew.left, rtnew.top, RTWIDTH(rtnew), RTHEIGHT(rtnew), SWP_SHOWWINDOW);
	m_docked = FALSE;
    } else {
	dock_window();
    }
}

BOOL
set_dock_side(RECT *rtnew)
{
    int i;
    RECT	rtparents[4];

    // This taken from James Spibey's <spib@bigfoot.com> code example for how to do a winamp plugin
    // however, the goto's are mine :)
    for(i = 0; i < WINAMP_WINDOWS; i++)
    {

	if (m_winamp_wins[i].visible == FALSE)
	    continue;

	GetWindowRect(m_winamp_wins[i].hwnd, &rtparents[i]);

	/*********************************
	 ** Dock to Right Side of Winamp **
	 *********************************/
	if((rtnew->left <= rtparents[i].right + SNAP_OFFSET) && (rtnew->left >= rtparents[i].right - SNAP_OFFSET) &&
	   ((rtnew->top >= rtparents[i].bottom && rtnew->bottom <= rtparents[i].top) ||
	    (rtnew->top <= rtparents[i].bottom && rtnew->bottom >= rtparents[i].top)))
	{
	    // Dock top to top
	    if((rtnew->top <= rtparents[i].top + SNAP_OFFSET) && (rtnew->top >= rtparents[i].top - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_RIGHT_TT;
	    }
	    // Dock top to bottom
	    else if((rtnew->top <= rtparents[i].bottom + SNAP_OFFSET) && (rtnew->top >= rtparents[i].bottom - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_RIGHT_TB;
	    }
	    // Dock bottom to top
	    else if((rtnew->bottom <= rtparents[i].top + SNAP_OFFSET) && (rtnew->bottom >= rtparents[i].top - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_RIGHT_BT;
	    }
	    else
	    {
		m_docked_diff.y = rtnew->top - rtparents[i].top;
		m_docked_side = DOCKED_RIGHT;
	    }
	    goto DOCKED;
	}
	/********************************
	 ** Dock to Left Side of Winamp **
	 ********************************/
	else if((rtnew->right <= rtparents[i].left + SNAP_OFFSET) && (rtnew->right >= rtparents[i].left - SNAP_OFFSET) &&
		((rtnew->top >= rtparents[i].bottom && rtnew->bottom <= rtparents[i].top) ||
		 (rtnew->top <= rtparents[i].bottom && rtnew->bottom >= rtparents[i].top)))

	{
	    // Dock top to top
	    if((rtnew->top <= rtparents[i].top + SNAP_OFFSET) && (rtnew->top >= rtparents[i].top - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_LEFT_TT;
	    }
	    // Dock top to bottom
	    else if((rtnew->top <= rtparents[i].bottom + SNAP_OFFSET) && (rtnew->top >= rtparents[i].bottom - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_LEFT_TB;	
	    }
	    // Dock bottom to top
	    else if((rtnew->bottom <= rtparents[i].top + SNAP_OFFSET) && (rtnew->bottom >= rtparents[i].top - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_LEFT_BT;
	    }
	    else
	    {
		m_docked_diff.y = rtnew->top - rtparents[i].top;
		m_docked_side = DOCKED_LEFT;

	    }
	    goto DOCKED;
	}
	/*******************************
	 ** Dock to Top Side of Winamp **
	 *******************************/
	else if((rtnew->bottom <= rtparents[i].top + SNAP_OFFSET) && (rtnew->bottom >= rtparents[i].top - SNAP_OFFSET) &&
		((rtnew->left >= rtparents[i].right && rtnew->right <= rtparents[i].left) ||
		 (rtnew->left <= rtparents[i].right && rtnew->right >= rtparents[i].left)))
	{
	    // Dock left to left
	    if((rtnew->left <= rtparents[i].left + SNAP_OFFSET) && (rtnew->left >= rtparents[i].left - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_TOP_LL;
	    }
	    // Dock left to right
	    else if((rtnew->left <= rtparents[i].right + SNAP_OFFSET) && (rtnew->left >= rtparents[i].right - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_TOP_LR;
	    }
	    // Dock right to left
	    else if((rtnew->right <= rtparents[i].left + SNAP_OFFSET) && (rtnew->right >= rtparents[i].left - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_TOP_RL;
	    }
	    else
	    {
		m_docked_diff.x = rtnew->left - rtparents[i].left;
		m_docked_side = DOCKED_TOP;

	    }
	    goto DOCKED;
	}
	/**********************************
	 ** Dock to Bottom Side of Winamp **
	 **********************************/
	else if((rtnew->top <= rtparents[i].bottom + SNAP_OFFSET) && (rtnew->top >= rtparents[i].bottom - SNAP_OFFSET) &&
		((rtnew->left >= rtparents[i].right && rtnew->right <= rtparents[i].left) ||
		 (rtnew->left <= rtparents[i].right && rtnew->right >= rtparents[i].left)))
	{
	    // Dock left to left
	    if((rtnew->left <= rtparents[i].left + SNAP_OFFSET) && (rtnew->left >= rtparents[i].left - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_BOTTOM_LL;
	    }
	    // Dock left to right
	    else if((rtnew->left <= rtparents[i].right + SNAP_OFFSET) && (rtnew->left >= rtparents[i].right - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_BOTTOM_LR;
	    }
	    // Dock right to left
	    else if((rtnew->right <= rtparents[i].left + SNAP_OFFSET) && (rtnew->right >= rtparents[i].left - SNAP_OFFSET))
	    {
		m_docked_side = DOCKED_BOTTOM_RL;
	    }
	    else
	    {
		m_docked_diff.x = rtnew->left - rtparents[i].left;
		m_docked_side = DOCKED_BOTTOM;
	    }
	    goto DOCKED;
	}
    }

    return FALSE;

 DOCKED:
    m_docked_index = i;
    m_docked = TRUE;
    return TRUE;
	
}

VOID
dock_do_lbuttondown (HWND hWnd, LONG wParam, LONG lParam)
{
    RECT rt;
    POINT cur = {LOWORD(lParam), HIWORD(lParam)};

    find_winamp_windows(hWnd);
    GetClientRect(hWnd, &rt);
    rt.bottom = rt.top + 120;
    if (PtInRect(&rt, cur)) {
	SetCapture(hWnd);
	m_dragging = TRUE;
	m_drag_from.x = (short)cur.x;
	m_drag_from.y = (short)cur.y;
    }
}

VOID
dock_do_lbuttonup (HWND hWnd, LONG wParam, LONG lParam)
{
    if (m_dragging) {
	ReleaseCapture();
	m_dragging = FALSE;
    }
}


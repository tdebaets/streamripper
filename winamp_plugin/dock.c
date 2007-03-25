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
#include "wa_ipc.h"
#include "dock.h"
#include "debug.h"

#define SNAP_OFFSET		10	
#define WINAMP_CLASSIC_WINS	4
#define WINAMP_MODERN_WINS	8

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

/*****************************************************************************
 * Public functions
 *****************************************************************************/
BOOL dock_hook_winamp(HWND hwnd);
VOID dock_do_mousemove(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_do_lbuttondown(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_do_lbuttonup(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_show_window(HWND hwnd, int nCmdShow);
BOOL dock_unhook_winamp();

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static LRESULT CALLBACK	hook_winamp_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
static VOID dock_window();
static BOOL set_dock_side(RECT *rtnew);
static BOOL find_winamp_windows ();
static VOID get_new_rect(HWND hwnd, POINTS cur, POINTS last, RECT *rtnew);


/*****************************************************************************
 * Private Vars
 *****************************************************************************/
struct WINAMP_WINS
{		
    HWND hwnd;
    BOOL visible;
    WNDPROC orig_proc;
};

static POINTS		m_drag_from = {0, 0};
static BOOL		m_dragging = FALSE;
static BOOL		m_docked = FALSE;
static int		m_docked_side;
static HWND		m_hwnd = NULL;
static POINT		m_docked_diff = {0, 0};
static int		m_docked_index;
static struct WINAMP_WINS m_winamp_classic_wins [WINAMP_CLASSIC_WINS];
static struct WINAMP_WINS m_winamp_modern_wins [WINAMP_MODERN_WINS];
static int m_num_modern_wins = 0;
static int m_skin_is_modern = 0;

BOOL CALLBACK
EnumWindowsProc (HWND hwnd, LPARAM lparam)
{
    int i;
    char classname[256];
    char title[256];
    HWND bigowner = (HWND) lparam;
    HWND owner = GetWindow (hwnd, GW_OWNER);
    
    LONG win_id, style;
    HINSTANCE hinstance;
    WNDCLASSEX wcx;

    if (owner != bigowner) {
#if defined (commentout)
	debug_printf ("IGNORING: %s\n", classname);
#endif
	return TRUE;
    }

    GetClassName (hwnd, classname, 256); 
    if (!GetWindowText (hwnd, title, 256)) title[0] = 0;
    win_id = GetWindowLong (hwnd, GWL_ID);
    hinstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE);
    style = GetWindowLong (hwnd, GWL_STYLE);
    GetClassInfoEx (hinstance, classname, &wcx);
    debug_printf ("%s/%s [%d]: %d %d %0x04x %0x04x\n", classname, title, hwnd, win_id, hinstance, style, wcx.style);

    if (strcmp (classname, "BaseWindow_RootWnd") == 0) {
	m_skin_is_modern = 1;
	for (i = 0; i < WINAMP_MODERN_WINS; i++) {
	    if (m_winamp_modern_wins[i].hwnd == hwnd) {
		//debug_printf ("%s (repeat[%d]) = %d\n", classname, i, hwnd);
		return TRUE;
	    }
	}
	if (m_num_modern_wins < WINAMP_MODERN_WINS) {
	    //debug_printf ("%s [%d] = %d\n", classname, m_num_modern_wins, hwnd);
	    m_winamp_modern_wins[m_num_modern_wins++].hwnd = hwnd;
	} else {
	    //debug_printf ("%s [RAN OUT OF SLOTS]\n", classname);
	}
    } else if (strcmp(classname, "Winamp PE") == 0) {
	if (!m_winamp_classic_wins[1].hwnd) {
	    m_winamp_classic_wins[1].hwnd = hwnd;
	    //debug_printf ("%s [1] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[1]) = %d\n", classname, hwnd);
	}
    } else if (strcmp(classname, "Winamp EQ") == 0) {
	if (!m_winamp_classic_wins[2].hwnd) {
	    m_winamp_classic_wins[2].hwnd = hwnd;
	    //debug_printf ("%s [2] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[2]) = %d\n", classname, hwnd);
	}
    } else if (strcmp(classname, "Winamp Video") == 0) {
	if (!m_winamp_classic_wins[3].hwnd) {
	    m_winamp_classic_wins[3].hwnd = hwnd;
	    //debug_printf ("%s [3] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[3]) = %d\n", classname, hwnd);
	}
    } else {
	//debug_printf ("%s (other) = %d\n", classname, hwnd);
    }

    /* Always enumerate until the end */
    return TRUE;
}

BOOL
find_winamp_windows (void)
{
    int i;
    long style = 0;

    debug_printf ("-------------------------------\n");
    m_skin_is_modern = 0;
    EnumWindows (EnumWindowsProc, (LPARAM)m_winamp_classic_wins[0].hwnd);
    debug_printf ("-------------------------------\n");

    debug_printf ("is_modern = %d\n", m_skin_is_modern);
    debug_printf ("par o par = %d\n", GetParent(m_winamp_classic_wins[0].hwnd));

    {
    void winamp_test_stuff (void);
    winamp_test_stuff();
    }

    if (m_skin_is_modern) {
	for (i = 0; i < m_num_modern_wins; i++) {
	    /* Set visible flag and hook if unhooked */
	    style = GetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_STYLE);
	    m_winamp_modern_wins[i].visible = style & WS_VISIBLE;
	    if (!m_winamp_modern_wins[i].orig_proc) {
		m_winamp_modern_wins[i].orig_proc = (WNDPROC) SetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_WNDPROC, (LONG) hook_winamp_callback);
		if (m_winamp_modern_wins[i].orig_proc == NULL) {
		    debug_printf ("Hooking failure?\n");
		    return FALSE;
		}
	    }
	}
    } else {
	/* Verify all windows present */
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
    	    if (m_winamp_classic_wins[i].hwnd == NULL)
		return FALSE;
	}
	/* Set visible flag and hook if unhooked */
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	    style = GetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_STYLE);
	    m_winamp_classic_wins[i].visible = style & WS_VISIBLE;
	    if (!m_winamp_classic_wins[i].orig_proc) {
		m_winamp_classic_wins[i].orig_proc = (WNDPROC) SetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_WNDPROC, (LONG) hook_winamp_callback);
		if (m_winamp_classic_wins[i].orig_proc == NULL) {
		    debug_printf ("Hooking failure?\n");
		    return FALSE;
		}
	    }
	}
    }

#if defined (commentout)
    for (i = -1; i <= 10; i++) {
	HWND h = (HWND) SendMessage (m_winamp_classic_wins[0].hwnd,WM_WA_IPC,i,IPC_GETWND);
	int ws = (int) SendMessage (m_winamp_classic_wins[0].hwnd,WM_WA_IPC,i,IPC_IS_WNDSHADE);
	debug_printf ("HWND(%d) = %d (%d)\n", i, h, ws);
    }
#endif

    debug_printf ("Found all the windows\n");
    return TRUE;
}

void
dock_init (HWND hwnd)
{
    int i;

    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	m_winamp_classic_wins[i].hwnd = NULL;
	m_winamp_classic_wins[i].orig_proc = NULL;
    }
    for (i = 0; i < WINAMP_MODERN_WINS; i++) {
	m_winamp_modern_wins[i].hwnd = NULL;
	m_winamp_modern_wins[i].orig_proc = NULL;
    }
    m_hwnd = hwnd;
    m_winamp_classic_wins[0].hwnd = GetParent (hwnd);

    debug_printf ("myself = %d\n", hwnd);
    debug_printf ("parent = %d\n", m_winamp_classic_wins[0].hwnd);
    debug_printf ("par o par = %d\n", GetParent(m_winamp_classic_wins[0].hwnd));

    find_winamp_windows ();

    return;
}

void
switch_docking_index (void)
{
    int i;
    HWND hwnd = m_winamp_modern_wins[m_docked_index].hwnd;
    char title_1[256], title_2[256];
    LONG style;
    
    if (!GetWindowText (hwnd, title_1, 256)) return;

    for (i = 0; i < m_num_modern_wins; i++) {
	if (i == m_docked_index) continue;
	hwnd = m_winamp_modern_wins[i].hwnd;
	if (!GetWindowText (hwnd, title_2, 256)) continue;
	if (strcmp (title_1, title_2) == 0) {
	    style = GetWindowLong (hwnd, GWL_STYLE);
	    if (style & WS_VISIBLE) {
		debug_printf ("Switching %d -> %d\n", m_docked_index, i);
		m_docked_index = i;
		break;
	    }
	}
    }
}

LRESULT CALLBACK
hook_winamp_callback (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    int i;

#if defined (commentout)
    if (umsg == WM_USER || umsg == WM_COPYDATA) {
	debug_printf ("callback: %d/0x%04x/0x%04x/0x%08x\n",hwnd,umsg,wparam,lparam);
    }
#endif

    if (umsg == WM_SHOWWINDOW && wparam == FALSE) {
	if (m_docked && m_skin_is_modern) {
	    if (m_winamp_modern_wins[m_docked_index].hwnd == hwnd) {
		switch_docking_index ();
		dock_window();
	    }
	}
    }

    if (umsg == WM_MOVE) {
	/* Do nothing */
    }

    if (umsg == WM_WINDOWPOSCHANGED) {
	if ((m_skin_is_modern && m_winamp_modern_wins[m_docked_index].hwnd == hwnd)
	    || (!m_skin_is_modern && m_winamp_classic_wins[m_docked_index].hwnd == hwnd))
	{
	    UINT wpf = ((WINDOWPOS*)lparam)->flags;
	    debug_printf ("WM_WINDOWPOSCHANGED: %d/0x%04x/0x%04x/0x%04x/0x%04x\n",hwnd,umsg,wparam,lparam,((WINDOWPOS*)lparam)->flags);
	    debug_printf ("%s %s %s %s %s %s %s %s %s %s %s %s %s\n",
		(wpf & SWP_DRAWFRAME)     ? "DRAWF" : "-----",
		(wpf & SWP_FRAMECHANGED)  ? "FRAME" : "-----",
		(wpf & SWP_HIDEWINDOW)    ? "HIDEW" : "-----",
		(wpf & SWP_NOACTIVATE)    ? "NOACT" : "-----",
		(wpf & SWP_NOCOPYBITS)    ? "NOCOP" : "-----",
		(wpf & SWP_NOMOVE)        ? "NOMOV" : "-----",
		(wpf & SWP_NOOWNERZORDER) ? "NOOWN" : "-----",
		(wpf & SWP_NOREDRAW)      ? "NORED" : "-----",
		(wpf & SWP_NOREPOSITION)  ? "NOREP" : "-----",
		(wpf & SWP_NOSENDCHANGING)? "NOSEN" : "-----",
		(wpf & SWP_NOSIZE)        ? "NOSIZ" : "-----",
		(wpf & SWP_NOZORDER)      ? "NOZOR" : "-----",
		(wpf & SWP_SHOWWINDOW)    ? "SHOWW" : "-----");
	    dock_window();
	}
    }

    //debug_printf ("callback: %d/0x%04x/0x%04x/0x%08x\n",hwnd,umsg,wparam,lparam);
    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	if (m_winamp_classic_wins[i].hwnd == hwnd)
	    return CallWindowProc (m_winamp_classic_wins[i].orig_proc, hwnd, umsg, wparam, lparam); 
    }
    for (i = 0; i < m_num_modern_wins; i++) {
	if (m_winamp_modern_wins[i].hwnd == hwnd)
	    return CallWindowProc (m_winamp_modern_wins[i].orig_proc, hwnd, umsg, wparam, lparam); 
    }

    debug_printf ("hook_callback problem: %d/0x%04x/0x%04x/0x%04x\n",hwnd,umsg,wparam,lparam);
    return FALSE;
}

VOID
dock_show_window (HWND hwnd, int nCmdShow)
{
    RECT rt;

    // Make sure we know about all the windows
    find_winamp_windows ();

    // Have it set the docking point to where-ever the window is
    GetWindowRect (hwnd, &rt);
    set_dock_side (&rt);

    ShowWindow (hwnd, nCmdShow);
}

BOOL
dock_unhook_winamp ()
{
    int i;

    debug_printf ("Unhooking...\n");
    for (i = 0; i < WINAMP_CLASSIC_WINS; i++)
	if (m_winamp_classic_wins[i].orig_proc)
	    SetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_WNDPROC, (LONG) m_winamp_classic_wins[i].orig_proc);

    for (i = 0; i < m_num_modern_wins; i++)
	if (m_winamp_modern_wins[i].orig_proc)
	    SetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_WNDPROC, (LONG) m_winamp_modern_wins[i].orig_proc);

    m_dragging = FALSE;
    m_docked = FALSE;
    m_docked_side = 0;
    m_hwnd = NULL;

    return TRUE;
}

// This taken from James Spibey's <spib@bigfoot.com> code example for 
// how to do a winamp plugin the goto's are mine :)
VOID
dock_window ()
{
    int i;
    RECT rt;
    int left, top;
    RECT rtparents[WINAMP_MODERN_WINS];

    if (!m_docked)
	return;

    GetWindowRect (m_hwnd, &rt);

    if (m_skin_is_modern) {
	for (i = 0; i < m_num_modern_wins; i++) {
	    GetWindowRect (m_winamp_modern_wins[i].hwnd, &rtparents[i]);
	}
    } else {
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	    GetWindowRect (m_winamp_classic_wins[i].hwnd, &rtparents[i]);
	}
    }

    i = m_docked_index;
    switch (m_docked_side)
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
    debug_printf ("SetWindowPos-a [%d] (%d %d %d %d)\n", m_hwnd, left, top, RTWIDTH(rt), RTHEIGHT(rt));
    SetWindowPos (m_hwnd, NULL, left, top, RTWIDTH(rt), RTHEIGHT(rt), SWP_NOACTIVATE);
}

VOID
get_new_rect (HWND hwnd, POINTS cur, POINTS last, RECT *rtnew)
{
    RECT rt;
    POINT diff = {cur.x-last.x, cur.y-last.y};

    GetWindowRect(hwnd, &rt);
    rtnew->left = rt.left+diff.x;
    rtnew->top = rt.top+diff.y;
    rtnew->right = rt.right+diff.x;
    rtnew->bottom = rt.bottom+diff.y;
}

VOID
dock_do_mousemove (HWND hwnd, LONG wparam, LONG lparam)
{
    RECT rtnew;

    // Simulate the the caption bar
    if (!(m_dragging && (wparam & MK_LBUTTON)))
	return;

    get_new_rect(hwnd, MAKEPOINTS(lparam), m_drag_from, &rtnew);

    if (!set_dock_side (&rtnew)) {
	SetWindowPos (hwnd, NULL, rtnew.left, rtnew.top, RTWIDTH(rtnew), RTHEIGHT(rtnew), SWP_SHOWWINDOW);
	debug_printf ("SetWindowPos-b [%d] (%d %d %d %d)\n", hwnd, rtnew.left, rtnew.top, RTWIDTH(rtnew), RTHEIGHT(rtnew));
	m_docked = FALSE;
    } else {
	dock_window ();
    }
}

BOOL
set_dock_side (RECT *rtnew)
{
    int i;
    RECT rtparents[WINAMP_MODERN_WINS];
    int nwin;
    struct WINAMP_WINS* wins;

    // This taken from James Spibey's <spib@bigfoot.com> code example for how to do a winamp plugin
    // however, the goto's are mine :)

    if (m_skin_is_modern) {
	nwin = m_num_modern_wins;
	wins = m_winamp_modern_wins;
    } else {
	nwin = WINAMP_CLASSIC_WINS;
	wins = m_winamp_classic_wins;
    }

    for (i = 0; i < nwin; i++) {
	if (wins[i].visible == FALSE)
	    continue;

	GetWindowRect (wins[i].hwnd, &rtparents[i]);

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
dock_do_lbuttondown (HWND hwnd, LONG wparam, LONG lparam)
{
    RECT rt;
    POINT cur = {LOWORD(lparam), HIWORD(lparam)};

    find_winamp_windows ();
    GetClientRect (hwnd, &rt);
    rt.bottom = rt.top + 120;
    if (PtInRect (&rt, cur)) {
	SetCapture(hwnd);
	m_dragging = TRUE;
	m_drag_from.x = (short)cur.x;
	m_drag_from.y = (short)cur.y;
    }
}

VOID
dock_do_lbuttonup (HWND hwnd, LONG wparam, LONG lparam)
{
    if (m_dragging) {
	ReleaseCapture ();
	m_dragging = FALSE;
    }
}

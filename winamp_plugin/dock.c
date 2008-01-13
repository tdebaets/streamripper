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
#include "wstreamripper.h"
#include "dock.h"
#include "debug.h"

#define SNAP_OFFSET		10	
//#define WINAMP_CLASSIC_WINS	4
//#define WINAMP_MODERN_WINS	8
#define WINAMP_MAX_WINS		8

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
VOID dock_do_mousemove(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_do_lbuttondown(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_do_lbuttonup(HWND hwnd, LONG wparam, LONG lparam);
VOID dock_show_window(HWND hwnd, int nCmdShow);


/*****************************************************************************
 * Private functions
 *****************************************************************************/
static LRESULT CALLBACK	hook_winamp_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
static VOID dock_window();
static BOOL set_dock_side(RECT *rtnew);
static VOID get_new_rect(HWND hwnd, POINTS cur, POINTS last, RECT *rtnew);


/*****************************************************************************
 * Private Vars
 *****************************************************************************/
struct WINAMP_WINS
{		
    HWND hwnd;
    BOOL visible;
    RECT rect;
#if defined (commentout)
    WNDPROC orig_proc;
#endif
};


static POINTS		m_drag_from = {0, 0};
static BOOL		m_dragging = FALSE;
static BOOL		m_docked = FALSE;
static int		m_docked_side;
//static HWND		m_hwnd = NULL;
static POINT		m_docked_diff = {0, 0};
static HWND		m_docked_hwnd = 0;
//static struct WINAMP_WINS m_winamp_classic_wins [WINAMP_CLASSIC_WINS];
//static struct WINAMP_WINS m_winamp_modern_wins [WINAMP_MODERN_WINS];
static struct WINAMP_WINS m_winamp_wins[WINAMP_MAX_WINS];
//static int m_num_modern_wins = 0;
//static int m_skin_is_modern = 0;


/*****************************************************************************
 * Functions
 *****************************************************************************/
void
dock_init (HWND hwnd)
{
    int i;

    for (i = 0; i < WINAMP_MAX_WINS; i++) {
	m_winamp_wins[i].hwnd = 0;
    }
}

#if defined (commentout)
/* GCS FIX: DO NOT DELETE THIS CODE.  THIS IS FOR DOCKING TO MODERN SKIN. */
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
#endif


/* This is called whenever the window transitions between 
   visible and hidden */
VOID
dock_show_window (HWND hwnd, int nCmdShow)
{
    RECT rt;

    /* Have it set the docking point to where-ever the window is */
    GetWindowRect (hwnd, &rt);
    set_dock_side (&rt);

    ShowWindow (hwnd, nCmdShow);
}

// This taken from James Spibey's <spib@bigfoot.com> code example for 
// how to do a winamp plugin the goto's are mine :)
VOID
dock_window (HWND hwnd)
{
    int i;
    RECT rt;
    int left, top;
    RECT rtparents[WINAMP_MAX_WINS];

    if (!m_docked)
	return;

    GetWindowRect (hwnd, &rt);

#if defined (commentout)
    if (m_skin_is_modern) {
	for (i = 0; i < m_num_modern_wins; i++) {
	    GetWindowRect (m_winamp_modern_wins[i].hwnd, &rtparents[i]);
	}
    } else {
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	    GetWindowRect (m_winamp_classic_wins[i].hwnd, &rtparents[i]);
	}
    }
#endif

    i = 0;
    GetWindowRect (m_docked_hwnd, &rtparents[i]);
    debug_printf ("Docking against [%d] (%d %d %d %d)\n", m_docked_hwnd, rtparents[i].top, rtparents[i].left, rtparents[i].bottom, rtparents[i].right);

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
	debug_printf ("DOCKED_RIGHT\n");
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
    debug_printf ("SetWindowPos (dock) [%d] (%d %d %d %d)\n", hwnd, left, top, RTWIDTH(rt), RTHEIGHT(rt));
    SetWindowPos (hwnd, NULL, left, top, RTWIDTH(rt), RTHEIGHT(rt), SWP_NOACTIVATE);
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
    int rc;
    RECT rtnew;

    /* Only drag for left mouse */
    if (!(m_dragging && (wparam & MK_LBUTTON)))
	return;

    /* Compute new location where the window rectangle will
       go if it doesn't dock.  */
    get_new_rect(hwnd, MAKEPOINTS(lparam), m_drag_from, &rtnew);

    /* Check to see if the new location should dock */
    rc = set_dock_side (&rtnew);

    if (rc) {
	dock_window (hwnd);
    } else {
	/* No docking, so move the window normally */
	SetWindowPos (hwnd, NULL, rtnew.left, rtnew.top, RTWIDTH(rtnew), RTHEIGHT(rtnew), SWP_SHOWWINDOW);
	debug_printf ("SetWindowPos (no dock) [%d] (%d %d %d %d)\n", hwnd, rtnew.left, rtnew.top, RTWIDTH(rtnew), RTHEIGHT(rtnew));
	m_docked = FALSE;
    }
}

void
debug_winamp_wins (void)
{
    int w;

    debug_printf ("debug_winamp_wins()\n");
    for (w = 0; w < WINAMP_MAX_WINS; w++) {
	if (m_winamp_wins[w].hwnd) {
	    debug_printf ("%d %d %d %d %d %d\n",
			    m_winamp_wins[w].hwnd,
			    m_winamp_wins[w].visible,
			    m_winamp_wins[w].rect.left,
			    m_winamp_wins[w].rect.top,
			    m_winamp_wins[w].rect.right,
			    m_winamp_wins[w].rect.bottom);
	}
    }
}

void
dock_resize (HWND hwnd, char* buf)
{
    if (*buf == '0') {
	debug_printf ("Hiding because of winamp minimize.\n");
	ShowWindow (hwnd, SW_HIDE);
    } else {
	if (!g_gui_prefs.m_start_minimized) {
	    debug_printf ("Showing because of winamp restore.\n");
	    ShowWindow (hwnd, SW_SHOW);
	} else {
	    debug_printf ("Ignoring winamp window restore.\n");
	}
    }
}

void
dock_update_winamp_wins (HWND hwnd, char* buf)
{
    int rc;
    int h, v, l, r, t, b, n;
    int i = 0, w = 0;

    while (1) {
	rc = sscanf (buf+i, " %d %d %d %d %d %d%n", 
			&h, &v, &l, &t, &r, &b, &n);
	if (rc < 0) break;
	if (rc != 6) {
	    debug_printf ("Error parsing message from dll: %d\n", rc);
	    break;
	}
	m_winamp_wins[w].hwnd = (HWND) h;
	m_winamp_wins[w].visible = v;
	m_winamp_wins[w].rect.left = l;
	m_winamp_wins[w].rect.top = t;
	m_winamp_wins[w].rect.right = r;
	m_winamp_wins[w].rect.bottom = b;
	w++;
	i += n;
    }
    while (w < WINAMP_MAX_WINS) {
	m_winamp_wins[w].hwnd = 0;
	w++;
    }
    debug_winamp_wins ();

    /* Dock window */
    dock_window (hwnd);
}


BOOL
set_dock_side (RECT *rtnew)
{
    int i;
    RECT rtparents[WINAMP_MAX_WINS];
    int nwin;
    struct WINAMP_WINS* wins;

    // This taken from James Spibey's <spib@bigfoot.com> code example for how to do a winamp plugin
    // however, the goto's are mine :)

#if defined (commentout)
    if (m_skin_is_modern) {
	nwin = m_num_modern_wins;
	wins = m_winamp_modern_wins;
    } else {
	nwin = WINAMP_CLASSIC_WINS;
	wins = m_winamp_classic_wins;
    }
#endif

    nwin = WINAMP_MAX_WINS;
    wins = m_winamp_wins;

    for (i = 0; i < nwin; i++) {
	//if (wins[i].visible == FALSE)
	//    continue;

	if (!wins[i].hwnd) continue;

	GetWindowRect (wins[i].hwnd, &rtparents[i]);
	debug_printf ("Got window rect: %d %d %d %d %d\n",
			wins[i].hwnd, 
			wins[i].rect.top, wins[i].rect.left, 
			wins[i].rect.bottom, wins[i].rect.right);
	debug_printf ("Got window rect[2]:      %d %d %d %d\n",
			rtparents[i].top, rtparents[i].left, 
			rtparents[i].bottom, rtparents[i].right);

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

    debug_printf ("NOT DOCKED (%d)\n", m_docked);
    return FALSE;

 DOCKED:
    m_docked_hwnd = wins[i].hwnd;
    m_docked = TRUE;
    debug_printf ("DOCKED [%d]\n", m_docked_hwnd);
    return TRUE;
}

VOID
dock_do_lbuttondown (HWND hwnd, LONG wparam, LONG lparam)
{
    RECT rt;
    POINT cur = {LOWORD(lparam), HIWORD(lparam)};

#if defined (commentout)
    find_winamp_windows ();
#endif
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

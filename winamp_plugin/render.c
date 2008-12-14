/* render.c - jonclegg@yahoo.com
 * renders the streamripper for winamp screen. skinning and stuff.
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
#include <time.h>
#include <stdio.h>
#include "render.h"
#include "winamp.h"
#include "debug.h"

#define BIG_IMAGE_WIDTH		400
#define BIG_IMAGE_HEIGHT	150
#define WIDTH(rect)			((rect).right-(rect).left+1)
#define HEIGHT(rect)		((rect).bottom-(rect).top+1)

#define HOT_TIMEOUT			1

#define SR_BLACK			RGB(10, 42, 31)
#define	TIMER_ID			101

#define BTN_MODE_COUNT		0x04
#define BTN_MODE_NORMAL		0x00
#define BTN_MODE_PRESSED	0x01
#define BTN_MODE_HOT		0x02
#define BTN_MODE_GRAYED		0x03


/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL	render_init(HINSTANCE hInst, HWND hWnd, LPCTSTR szBmpFile);
BOOL	render_destroy();
BOOL	render_change_skin(LPCTSTR szBmpFile);

BOOL	render_set_background(RECT *rt, POINT *rgn_points, int num_points);
VOID	render_set_prog_rects(RECT *imagert, POINT dest, COLORREF line_color);
VOID	render_set_prog_bar(BOOL on_off);
BOOL	render_set_display_data(int idr, char *format, ...);
BOOL	render_set_display_data_pos(int idr, RECT *rt);
VOID	render_set_button_enabled(HBUTTON hbut, BOOL enabled);
VOID	render_set_text_color(POINT pt);
VOID	render_clear_all_data();
HBUTTON	render_add_button(RECT *normal, RECT *pressed, RECT *hot, RECT *grayed, RECT *dest, void (*clicked)());
BOOL	render_add_bar(RECT *rt, POINT dest);
BOOL	render_do_paint(HDC hdc);
BOOL	render_create_preview(char* skinfile, HDC hdc, long left, long right);
VOID	render_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
VOID	render_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
VOID	render_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);

#define do_refresh(rect)	(InvalidateRect(m_hwnd, rect, FALSE))

/*********************************************************************************
 * Private structs
 *********************************************************************************/
typedef struct BUTTONst
{
	RECT	dest;
	RECT	rt[BTN_MODE_COUNT];
	short	mode;
	void	(*clicked)();
	time_t	hot_timeout;
	BOOL	enabled;
} BUTTON;

typedef struct DISPLAYDATAst
{
	char	str[MAX_RENDER_LINE];
	RECT	rt;
} DISPLAYDATA;

typedef struct BITMAPDCst
{
	HBITMAP	bm;
	HDC		hdc;
} BITMAPDC;

typedef struct SKINDATAst
{
	BITMAPDC bmdc;
	COLORREF textcolor;
	HBRUSH	 hbrush;
} SKINDATA;

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static BOOL internal_render_do_paint(SKINDATA skind, HDC outhdc);

static VOID CALLBACK on_timer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static BOOL TrimTextOut(HDC hdc, int x, int y, int maxwidth, char *str);

static BOOL skindata_from_file(const char* skinfile, SKINDATA* pskind);
static void skindata_close(SKINDATA skind);
static BOOL bitmapdc_from_file(const char* skinfile, BITMAPDC* bmdc);
static void bitmapdc_close(BITMAPDC b);



/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static BITMAPDC		m_tempdc;
static SKINDATA		m_offscreenskind;
static HFONT		m_tempfont;

static int			m_num_buttons;
static BUTTON		m_buttons[MAX_BUTTONS];
static RECT			m_rect_background;
static RECT			m_prog_rect;
static POINT		m_pt_color;
static time_t		m_time_start;
static BOOL			m_prog_on;
static POINT		m_prog_point = {0, 0};
static COLORREF		m_prog_color;
static HWND			m_hwnd;
static DISPLAYDATA	m_ddinfo[IDR_NUMFIELDS];

BOOL render_init(HINSTANCE hInst, HWND hWnd, LPCTSTR szBmpFile)
{
	if (!skindata_from_file(szBmpFile, &m_offscreenskind))
		return FALSE;

//	m_num_buttons = 0;
	m_tempdc.hdc = NULL;
	m_tempdc.bm = NULL;
	
	// Set a timer for mouseovers
	SetTimer(hWnd, TIMER_ID, 100, (TIMERPROC)on_timer);
	m_hwnd = hWnd;

	return TRUE;	
}

BOOL render_change_skin(LPCTSTR szBmpFile)
{
	skindata_close(m_offscreenskind);
	if (!skindata_from_file(szBmpFile, &m_offscreenskind))
		return FALSE;
	return TRUE;
}

VOID render_set_text_color(POINT pt)
{
	m_pt_color = pt;
	m_offscreenskind.textcolor = GetPixel(m_offscreenskind.bmdc.hdc, pt.x, pt.y);

	if (m_offscreenskind.hbrush)
		DeleteObject(m_offscreenskind.hbrush);
	m_offscreenskind.textcolor = GetPixel(m_offscreenskind.bmdc.hdc, pt.x, pt.y);
	m_offscreenskind.hbrush = CreateSolidBrush(m_offscreenskind.textcolor);
}

BOOL render_set_background(RECT *rt, POINT *rgn_points, int num_points)
{
	HRGN rgn = CreatePolygonRgn(rgn_points, num_points, WINDING);
	
	if (!rgn)
		return FALSE;

	if (!SetWindowRgn(m_hwnd, rgn, TRUE))
		return FALSE;

	memcpy(&m_rect_background, rt, sizeof(RECT));
	
	return TRUE;
}

VOID render_set_prog_rects(RECT *imagert, POINT dest, COLORREF line_color)
{
	memcpy(&m_prog_rect, imagert, sizeof(RECT));
	m_prog_color= line_color;
	m_prog_point = dest;
}

VOID render_set_prog_bar(BOOL on)
{
	m_prog_on = on;
	time(&m_time_start);
	do_refresh(NULL);
}


VOID render_set_button_enabled(HBUTTON hbut, BOOL enabled)
{
	m_buttons[hbut].enabled = enabled;
	if (enabled)
	{
		if (m_buttons[hbut].mode != BTN_MODE_HOT)
			m_buttons[hbut].mode = BTN_MODE_NORMAL;
	}
	else
		m_buttons[hbut].mode = BTN_MODE_GRAYED;

	do_refresh(NULL);
}


HBUTTON	render_add_button(RECT *normal, RECT *pressed, RECT *hot, RECT *grayed, RECT *dest, void (*clicked)())
{
	BUTTON *b;
	if (m_num_buttons > MAX_BUTTONS || !clicked)
		return FALSE;

	b = &m_buttons[m_num_buttons];
	memcpy(&b->rt[BTN_MODE_NORMAL], normal, sizeof(RECT));
	memcpy(&b->rt[BTN_MODE_PRESSED], pressed, sizeof(RECT));
	memcpy(&b->rt[BTN_MODE_HOT], hot, sizeof(RECT));
	memcpy(&b->rt[BTN_MODE_GRAYED], grayed, sizeof(RECT));
	memcpy(&b->dest, dest, sizeof(RECT));
	b->mode = BTN_MODE_NORMAL;
	b->clicked = clicked;
	b->enabled = TRUE;
	m_num_buttons++;
	return (HBUTTON)m_num_buttons-1;
}

VOID CALLBACK on_timer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	POINT pt;
	RECT winrt;
	BUTTON *b = &m_buttons[0];
	time_t now;
	int i;

	GetCursorPos(&pt);
	GetWindowRect(hWnd, &winrt);

	pt.x -= winrt.left;
	pt.y -= winrt.top;

	time(&now);

	// Turn off a hot button if it hasn't been mouse over'd for a while 
	for(i = 0; i < m_num_buttons; i++)
	{
		b = &m_buttons[i];
		if (!PtInRect(&b->dest, pt) && 
			b->hot_timeout >= now &&
			b->mode == BTN_MODE_HOT)
		{
			b->mode = BTN_MODE_NORMAL;
			do_refresh(&b->dest);
		}
	}

	// refresh display every second.
	{
		static time_t last_diff = 0;

		if ((now-m_time_start) != last_diff &&
			m_prog_on)
		{
			do_refresh(NULL);
		}
		last_diff = now-m_time_start;
	}

}


VOID render_do_mousemove(HWND hWnd, LONG wParam, LONG lParam)
{
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};
	BUTTON *b;
	int i;

	for(i = 0; i < m_num_buttons; i++)
	{
		b = &m_buttons[i];
		if (PtInRect(&b->dest, pt) &&
			b->enabled)
		{
			b->mode = BTN_MODE_HOT;
			time(&b->hot_timeout);
			b->hot_timeout += HOT_TIMEOUT;
			do_refresh(&b->dest);
		}
	}
}

VOID render_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam)
{
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};
	BUTTON *b;
	int i;

	for(i = 0; i < m_num_buttons; i++)
	{
		b = &m_buttons[i];
		if (PtInRect(&b->dest, pt) &&
			b->enabled)
		{
			b->mode = BTN_MODE_HOT;
			b->clicked();
		}
		do_refresh(&b->dest);
	}
}

VOID render_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam)
{
	POINT pt = {LOWORD(lParam), HIWORD(lParam)};
	BUTTON *b;
	int i;

	for(i = 0; i < m_num_buttons; i++)
	{
		b = &m_buttons[i];

		if (PtInRect(&b->dest, pt) &&
			b->enabled)
		{
			b->mode = BTN_MODE_PRESSED;
		}
		do_refresh(&b->dest);
	}
}

VOID render_clear_all_data()
{
	int i;
	for(i = 0; i < IDR_NUMFIELDS; i++)
		memset(m_ddinfo[i].str, 0, MAX_RENDER_LINE);
}

BOOL render_set_display_data(int idr, char *format, ...)
{
    va_list va;

	if (idr < 0 || idr > IDR_NUMFIELDS || !format)
		return FALSE;

    va_start (va, format);
	_vsnprintf(m_ddinfo[idr].str, MAX_RENDER_LINE, format, va);
	va_end(va);
	return TRUE;
}

BOOL render_set_display_data_pos(int idr, RECT *rt)
{
	if (idr < 0 || idr > IDR_NUMFIELDS || !rt)
		return FALSE;

	memcpy(&m_ddinfo[idr].rt, rt, sizeof(RECT));

	return TRUE;
}


BOOL TrimTextOut(HDC hdc, int x, int y, int maxwidth, char *str)
{
	char buf[MAX_RENDER_LINE];
	SIZE size;

	strcpy(buf, str);
	GetTextExtentPoint32(hdc, buf, strlen(buf), &size); 
	if (size.cx > maxwidth)
	{
		while(size.cx > maxwidth && 
			  strlen(buf))
		{
			GetTextExtentPoint32(hdc, buf, strlen(buf), &size); 
			buf[strlen(buf)-1] = '\0';
		} 
		strcat(buf, "...");
	}

	return TextOut(hdc, x, y, buf, strlen(buf));
}

BOOL render_destroy()
{

	DeleteObject(m_tempfont);
	skindata_close(m_offscreenskind);
	bitmapdc_close(m_tempdc);
	return TRUE;
}

BOOL skindata_from_file(const char* skinfile, SKINDATA* pskind)
{
	if (!pskind)
		return FALSE;
	if (!bitmapdc_from_file(skinfile, &pskind->bmdc))
		return FALSE;
	pskind->textcolor = GetPixel(pskind->bmdc.hdc, m_pt_color.x, m_pt_color.y);
	pskind->hbrush = CreateSolidBrush(pskind->textcolor);
	return pskind->hbrush ? TRUE : FALSE;
}

void skindata_close(SKINDATA skind)
{
	bitmapdc_close(skind.bmdc);
	DeleteObject(skind.hbrush);
	skind.hbrush = NULL;
}

BOOL bitmapdc_from_file(const char* skinfile, BITMAPDC* bmdc)
{
	char tempfile[SR_MAX_PATH*5];
	
	memset(tempfile, 0, SR_MAX_PATH*5);
	if (!winamp_get_path(tempfile))
		return FALSE;
	strcat(tempfile, SKIN_PATH);
	strcat(tempfile, skinfile);

	bmdc->bm = (HBITMAP) LoadImage(0, tempfile, IMAGE_BITMAP, 
						BIG_IMAGE_WIDTH, BIG_IMAGE_HEIGHT, 
						LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	if (bmdc->bm == NULL)
		return FALSE;

	bmdc->hdc = CreateCompatibleDC(NULL);
	SelectObject(bmdc->hdc, bmdc->bm);

	return TRUE;
}

void bitmapdc_close(BITMAPDC b)
{
	DeleteDC(b.hdc);
	DeleteObject(b.bm);
	b.hdc = NULL;
	b.bm = NULL;
}

BOOL render_create_preview(char* skinfile, HDC hdc, long left, long top)
{
    BOOL b;
    long orig_width = WIDTH(m_rect_background);
    long orig_hight = HEIGHT(m_rect_background);
    SKINDATA skind;
    BITMAPDC tempdc;

    if (!skindata_from_file(skinfile, &skind))
	return FALSE;

    tempdc.hdc = CreateCompatibleDC(skind.bmdc.hdc);
    tempdc.bm = CreateCompatibleBitmap(skind.bmdc.hdc, orig_width, orig_hight);
    SelectObject(tempdc.hdc, tempdc.bm);

    if (!internal_render_do_paint(skind, tempdc.hdc))
	return FALSE;
    b = StretchBlt(hdc, left, top, orig_width / 2, orig_hight / 2,
	tempdc.hdc, 0, 0, orig_width, orig_hight, 
	SRCCOPY);
    bitmapdc_close(tempdc);
    skindata_close(skind);
    if (!b)
	return FALSE;
    return TRUE;
}

BOOL render_do_paint(HDC hdc)
{
	return internal_render_do_paint(m_offscreenskind, hdc);
}


BOOL internal_render_do_paint(SKINDATA skind, HDC outhdc)
{
	BUTTON *b;
	RECT *prt;
	int i;
	HDC thdc = skind.bmdc.hdc;

//	DEBUG2(( "render_do_paint: %d\n", thdc ));

	// Create out temp dc if we haven't made it yet
	if (m_tempdc.hdc == NULL)
	{
		LOGFONT ft;

		m_tempdc.hdc = CreateCompatibleDC(thdc);
		m_tempdc.bm = CreateCompatibleBitmap(thdc, WIDTH(m_rect_background), 
						HEIGHT(m_rect_background));
		SelectObject(m_tempdc.hdc, m_tempdc.bm);

		// And the font
		memset(&ft, 0, sizeof(LOGFONT));
//		strcpy(ft.lfFaceName, "Microsoft Sans pSerif");
		strcpy(ft.lfFaceName, "Verdana");
		ft.lfHeight = 12;
		m_tempfont = CreateFontIndirect(&ft);
		SelectObject(m_tempdc.hdc, m_tempfont);

	}


	// Do the background
	BitBlt(m_tempdc.hdc, 
		   0, 
		   0, 
		   WIDTH(m_rect_background),
		   HEIGHT(m_rect_background),
		   thdc,
		   m_rect_background.left,
		   m_rect_background.top,
		   SRCCOPY);

	// Draw buttons
	for(i = 0; i < m_num_buttons; i++)
	{
		b = &m_buttons[i];
		prt = &b->rt[b->mode];
		BitBlt(m_tempdc.hdc,
			   b->dest.left,
			   b->dest.top,
			   WIDTH(b->dest),
			   HEIGHT(b->dest),
			   thdc,
			   prt->left,
			   prt->top,
			   SRCCOPY);
	}
	
	// Draw progress bar
	if (m_prog_on)
	{
		RECT rt = {m_prog_point.x, 
				   m_prog_point.y, 
				   m_prog_point.x + (WIDTH(m_prog_rect)*15) + 11,   // yummy magic numbers
				   m_prog_point.y + HEIGHT(m_prog_rect)+4};
		time_t now;
		int num_bars;
		int i;

		time(&now);
		num_bars = (now-m_time_start) % 15;	// number of bars to draw
		FrameRect(m_tempdc.hdc, &rt, skind.hbrush);
		
		for(i = 0; i < num_bars; i++)
		{
			for(i = 0; i < num_bars; i++)
			{
				BitBlt(m_tempdc.hdc,
					   rt.left + (i * (WIDTH(m_prog_rect)+1)+2),
					   rt.top+2,
					   WIDTH(m_prog_rect),
					   HEIGHT(m_prog_rect),
					   thdc,
					   m_prog_rect.left,
					   m_prog_rect.top,
					   SRCCOPY);
			}
		}
	}


	// Draw text data on the screen
	{
		SetBkMode(m_tempdc.hdc, TRANSPARENT); 
		
		// Draw text
 		SetTextColor(m_tempdc.hdc, skind.textcolor);

		for(i = 0; i < IDR_NUMFIELDS; i++)
		{		
			TrimTextOut(m_tempdc.hdc, m_ddinfo[i].rt.left, 
						m_ddinfo[i].rt.top, 
						WIDTH(m_ddinfo[i].rt), 
						m_ddinfo[i].str); 
		}
	}
	

	// onto the actual screen 
	BitBlt(outhdc,
		   0,
		   0,
		   WIDTH(m_rect_background),
		   HEIGHT(m_rect_background),
		   m_tempdc.hdc,
		   0,
		   0,
		   SRCCOPY);


	return TRUE;
}


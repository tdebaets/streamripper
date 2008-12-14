#ifndef __RENDER_H__
#define __RENDER_H__


#define REN_STATUS_BUFFERING	0x01
#define REN_STATUS_RIPPING		0x02
#define REN_STATUS_RECONNECTING	0x03

#define MAX_RENDER_LINE		1024
#define IDR_NUMFIELDS		0x06

#define IDR_STATUS			0x00
#define IDR_FILENAME		0x01
#define IDR_STREAMNAME		0x02
#define IDR_SERVERTYPE		0x03
#define IDR_BITRATE			0x04
#define IDR_METAINTERVAL	0x05

#define MAX_BUTTONS			10

typedef int	HBUTTON;

BOOL render_init(HINSTANCE hInst, HWND hWnd, LPCTSTR szBmpFile);
VOID render_clear_all_data();

extern BOOL		render_set_display_data(int idr, char *format, ...);
extern BOOL		render_set_display_data_pos(int idr, RECT *rt);
extern VOID		render_clear_all_data();
extern BOOL		render_set_background(RECT *rt, POINT *rgn_points, int num_points);
extern VOID		render_set_prog_rects(RECT *imagert, POINT dest, COLORREF line_color);
extern VOID		render_set_prog_bar(BOOL on_off);
extern VOID		render_set_button_enabled(HBUTTON hbut, BOOL enabled);
extern VOID		render_set_text_color(POINT pt);

extern HBUTTON	render_add_button(RECT *normal, RECT *pressed, RECT *hot, RECT *grayed, RECT *dest, void (*clicked)());
extern BOOL		render_do_paint(HDC hdc);
extern VOID		render_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
extern VOID		render_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
extern VOID		render_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);
extern BOOL		render_destroy();

extern BOOL		render_create_preview(char* skinfile, HDC hdc, long left, long top);
extern BOOL		render_change_skin(LPCTSTR szBmpFile);

#endif //__RENDER_H__
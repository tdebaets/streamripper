#ifndef __DOCK_H__
#define __DOCK_H__

extern BOOL dock_hook_winamp(HWND hwnd);
extern VOID dock_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
extern VOID	dock_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);
extern VOID	dock_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
extern VOID dock_show_window(HWND hWnd, int nCmdShow);
extern BOOL dock_unhook_winamp();

#endif //__DOCK_H__
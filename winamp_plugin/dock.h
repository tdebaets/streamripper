#ifndef __DOCK_H__
#define __DOCK_H__

void dock_init (HWND hwnd);
BOOL dock_hook_winamp(HWND hwnd);
VOID dock_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_show_window(HWND hWnd, int nCmdShow);
BOOL dock_unhook_winamp();

#endif //__DOCK_H__
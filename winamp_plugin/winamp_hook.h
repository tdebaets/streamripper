#ifndef __WINAMP_HOOK_H__
#define __WINAMP_HOOK_H__

#include <windows.h>

void dock_init (HWND hwnd);
BOOL dock_unhook_winamp (void);
BOOL hook_winamp (void);

#endif

#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <windows.h>

BOOL get_string_from_registry (char *path, HKEY hkey, LPCTSTR subkey, LPTSTR name);
BOOL strip_registry_path (char* path, char* tail);

#endif

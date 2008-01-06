#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <windows.h>

BOOL
get_string_from_registry (char *path, HKEY hkey, LPCTSTR subkey, LPTSTR name);

#endif

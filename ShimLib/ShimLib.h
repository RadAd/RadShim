#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IDS_VERSION                     101
#define IDS_SHIM                        102
#define IDS_TARGET                      103

void Error(const TCHAR* const format, ...);
DWORD Launch(const HINSTANCE hInstance, const LPCTSTR lpCmdLine);

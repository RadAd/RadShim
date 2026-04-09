#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IDS_COMMAND                     101

void Error(const TCHAR* const format, ...);
DWORD Launch(const HINSTANCE hInstance, const LPCTSTR lpCmdLine);

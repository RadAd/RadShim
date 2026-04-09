#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IDS_COMMAND                     101

void Error(const TCHAR* const format, ...);
DWORD Launch(const HINSTANCE hInstance, const LPCTSTR lpCmdLine);

struct Win32Error
{
    DWORD code;
    LPCTSTR msg;
};
void ReportError(const Win32Error& e);
#define CHECK_RET(x, r) if (!(x)) return (r)
#define CHECK_LE(x) if (!(x)) throw Win32Error({ GetLastError(), TEXT(#x) })


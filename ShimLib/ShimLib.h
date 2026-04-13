#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IDS_VERSION                     101
#define IDS_SHIM                        102
#define IDS_TARGET                      103

void Error(const TCHAR* const format, ...);
DWORD Launch(const HINSTANCE hInstance, const LPCTSTR lpCmdLine);

struct Win32Error
{
    DWORD code;
    LPCTSTR msg;
    LPCTSTR file;
    int line;
};
void ReportError(const Win32Error& e);

template <class T>
T Check(T value, const TCHAR* const expression)
{
    if (!value)
        throw Win32Error({ GetLastError(), expression });
    return value;
}

#define CHECK_RET(x, r) if (!(x)) return (r)
#define CHECK_LE(x) if (!(x)) throw Win32Error({ GetLastError(), TEXT(#x), TEXT(__FILE__), __LINE__ })
#define CHECK_LE_F(x) Check(x, TEXT(#x))


#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct Win32Error
{
    DWORD code;
    LPCTSTR msg;
    LPCTSTR context;
    LPCTSTR file;
    int line;
};
void ReportError(const Win32Error& e);

template <class T>
const T& Check(const T& value, LPCTSTR expression, LPCTSTR context, LPCTSTR file, int line)
{
    if (!value)
        throw Win32Error({ GetLastError(), expression, context, file, line });
    return value;
}

#define CHECK_RET(x, r) if (!(x)) return (r)
#define CHECK_LE(x) Check(x, TEXT(#x), nullptr, TEXT(__FILE__), __LINE__)
#define CHECK_LE_CTX(x, y) Check(x, TEXT(#x), y, TEXT(__FILE__), __LINE__)

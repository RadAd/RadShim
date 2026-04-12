#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include "..\ShimLib\ShimLib.h"
#include "resource.h"

int WINAPI _tWinMain(_In_ const HINSTANCE hInstance, _In_opt_ const HINSTANCE hPrevInstance, _In_ const LPTSTR lpCmdLine, _In_ const int nShowCmd)
{
    return Launch(hInstance, lpCmdLine);
    //return EXIT_SUCCESS;
}

void Error(const TCHAR* const format, ...)
{
    va_list args;
    va_start(args, format);
    TCHAR buffer[1024];
    _vsntprintf_s(buffer, sizeof(buffer) / sizeof(TCHAR), format, args);
    va_end(args);
    MessageBox(NULL, buffer, _T("RadShim Error"), MB_OK | MB_ICONERROR);
}

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include "..\ShimLib\ShimLib.h"

#include <cstdio>

LPCTSTR FindNext(LPCTSTR lpText)
{
    while (_istspace(*lpText))
        ++lpText;

    if (*lpText != _T('\0'))
    {
        bool notfound = true;
        bool inquotes = false;
        while (notfound && *lpText != _T('\0'))
        {
            switch (*lpText)
            {
            case _T('\"'):
                inquotes = !inquotes;
                ++lpText;
                break;
            case _T(' '):
            case _T('\f'):
            case _T('\t'):
            case _T('\v'):
                if (inquotes)
                    ++lpText;
                else
                    notfound = false;
                break;
            default:
                ++lpText;
                break;
            }
        }
    }

    while (_istspace(*lpText))
        ++lpText;

    return lpText;
}

int _tmain(const int argc, const TCHAR* const argv[])
{
    const HINSTANCE hInstance = GetModuleHandleA(NULL);
    const LPCTSTR lpCmdLine = FindNext(GetCommandLine());
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
    _fputts(buffer, stderr);
    _fputts(TEXT("\n"), stderr);
}

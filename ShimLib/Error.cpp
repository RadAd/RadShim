#include "ShimLib.h"
#include "Error.h"
#include <tchar.h>

void ReportError(const Win32Error& e)
{
    const HMODULE hLibrary = NULL;
    LPTSTR pMessage = nullptr;
    if (FormatMessage((hLibrary == NULL ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE) |
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        hLibrary,
        e.code,
        0,
        (LPTSTR) &pMessage,
        0,
        NULL) == 0)
    {
        Error(_T("Format message failed with 0x%08x"), GetLastError());
    }
    else
    {
        const size_t len = lstrlen(pMessage);
        pMessage[len - 2] = TEXT('\0');
        if (e.context)
        {
            if (e.msg)
                Error(_T("%s -> 0x%08x %s (%s) at %s:%d"), e.msg, e.code, pMessage, e.context, e.file, e.line);
            else
                Error(_T("0x%08x %s (%s) at %s:%d"), e.code, pMessage, e.context, e.file, e.line);
        }
        else
        {
            if (e.msg)
                Error(_T("%s -> 0x%08x %s at %s:%d"), e.msg, e.code, pMessage, e.file, e.line);
            else
                Error(_T("0x%08x %s at %s:%d"), e.code, pMessage, e.file, e.line);
        }

        LocalFree(pMessage);
    }
}

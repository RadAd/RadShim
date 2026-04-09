#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shellapi.h>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "..\ShimLib\ShimLib.h"
#include "Resources.h"

void CopyResources(LPCTSTR file, LPCTSTR target)
{
    _ASSERTE(!PathIsRelative(target));
    HMODULE hModule;
    CHECK_LE(hModule = LoadLibraryEx(target, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE));
    LPCTSTR group_icon_name = FindFirstResourceName(hModule, RT_GROUP_ICON);
    LPCTSTR version_name = FindFirstResourceName(hModule, RT_VERSION);

    std::vector<WORD> icon_data;
    if (group_icon_name)
        icon_data = GetIconResourceIDs(hModule, group_icon_name);

    HANDLE hUpdate;
    CHECK_LE(hUpdate = BeginUpdateResource(file, FALSE));
    if (group_icon_name)
        CopyResource(hModule, RT_GROUP_ICON, group_icon_name, hUpdate);
    for (WORD id : icon_data)
        CopyResource(hModule, RT_ICON, MAKEINTRESOURCE(id), hUpdate);
    if (version_name)
        CopyResource(hModule, RT_VERSION, version_name, hUpdate);
    CHECK_LE(EndUpdateResource(hUpdate, FALSE));

    FreeLibrary(hModule);
}

int _tmain(const int argc, const TCHAR* const argv[])
try
{
    LPCTSTR command = argv[1];
    if (lstrcmpi(command, _T("create")) == 0)
    {
        LPCTSTR target = argv[2];
        if (PathIsRelative(target))
        {
            Error(_T("Target cannot be relative: %s"), target);
            return EXIT_FAILURE;
        }
        if (!PathFileExists(target))
        {
            Error(_T("Target file does not exist: %s"), target);
            return EXIT_FAILURE;
        }

        DWORD_PTR exe_type;
        CHECK_LE(exe_type = SHGetFileInfo(target, 0, nullptr, 0, SHGFI_EXETYPE));
        const bool isConsole = HIWORD(exe_type) == 0;

        LPCTSTR filename = PathFindFileName(target);
        TCHAR file[MAX_PATH];
        GetFullPathName(filename, ARRAYSIZE(file), file, nullptr);

        TCHAR shim[MAX_PATH];
        GetModuleFileName(NULL, shim, ARRAYSIZE(shim));
        LPTSTR shimname = PathFindFileName(shim);
        lstrcpy(shimname, isConsole ? TEXT("CShim.exe") : TEXT("WShim.exe"));
        CHECK_LE(CopyFile(shim, file, FALSE));

        LPCWSTR lpName = MAKEINTRESOURCE(IDS_COMMAND / STRINGTABLE_SIZE + 1);
        const WORD wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        StringTable stringtable = LoadStringTable(file, lpName, wLanguage);
        stringtable.item[IDS_COMMAND % STRINGTABLE_SIZE] = target;
        SaveStringTable(file, lpName, wLanguage, stringtable);

        CopyResources(file, target);

        _tprintf(_T("RadShim: %s ==> %s\n"), file, target);
    }
    else
    {
        _tprintf(_T("RadShim\n"));
        _tprintf(_T("Unknown command: %s\n"), command);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
catch (const Win32Error& e)
{
    ReportError(e);
    return e.code;
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

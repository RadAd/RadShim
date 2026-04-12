#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shellapi.h>

#include <cstdlib>
#include <cstdio>
#include <vector>

#include "..\ShimLib\ShimLib.h"
#include "Resources.h"
#include "resource.h"

void CopyResources(LPCTSTR file, LPCTSTR target)
{
    _ASSERTE(!PathIsRelative(target));
    UniqueModule hModule(InitUniqueModule());
    CHECK_LE(hModule = InitUniqueModule(LoadLibraryEx(target, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)));
    LPCTSTR group_icon_name = FindFirstResourceName(hModule.get(), RT_GROUP_ICON);
    LPCTSTR version_name = FindFirstResourceName(hModule.get(), RT_VERSION);

    std::vector<WORD> icon_data;
    if (group_icon_name)
        icon_data = GetIconResourceIDs(hModule.get(), group_icon_name);

    UniqueUpdateResource hUpdate;
    CHECK_LE(hUpdate = UniqueUpdateResource(BeginUpdateResource(file, FALSE)));
    if (group_icon_name)
        CHECK_LE(CopyResource(hModule.get(), RT_GROUP_ICON, group_icon_name, hUpdate.get()));
    for (WORD id : icon_data)
        CHECK_LE(CopyResource(hModule.get(), RT_ICON, MAKEINTRESOURCE(id), hUpdate.get()));
    if (version_name)
        CHECK_LE(CopyResource(hModule.get(), RT_VERSION, version_name, hUpdate.get()));
    hUpdate.get_deleter().discard = FALSE;
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
        CHECK_LE(GetFullPathName(filename, ARRAYSIZE(file), file, nullptr));

#if 0
        TCHAR shim[MAX_PATH];
        GetModuleFileName(NULL, shim, ARRAYSIZE(shim));
        LPTSTR shimname = PathFindFileName(shim);
        lstrcpy(shimname, isConsole ? TEXT("CShim.exe") : TEXT("WShim.exe"));
        CHECK_LE(CopyFile(shim, file, FALSE));
#else
        ExtractResource(NULL, MAKEINTRESOURCE(isConsole ? IDR_CSHIM_EXE : IDR_WSHIM_EXE), RT_RCDATA, file);
#endif

        LPCWSTR lpName = MAKEINTRESOURCE(IDS_TARGET / STRINGTABLE_SIZE + 1);
        const WORD wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        StringTable stringtable = LoadStringTable(file, lpName, wLanguage);
        stringtable.item[IDS_TARGET % STRINGTABLE_SIZE] = target;
        SaveStringTable(file, lpName, wLanguage, stringtable);

        CopyResources(file, target);

        _tprintf(_T("RadShim: %s ==> %s\n"), file, target);
    }
    else if (lstrcmpi(command, _T("details")) == 0)
    {
        LPCTSTR shimname = argv[2];
        TCHAR shim[MAX_PATH];
        CHECK_LE(GetFullPathName(shimname, ARRAYSIZE(shim), shim, nullptr));

        UniqueModule hModule(InitUniqueModule());
        CHECK_LE(hModule = InitUniqueModule(LoadLibraryEx(shim, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)));

        // TODO Verify it is a shim file
        TCHAR verify[256];
        if (!LoadString(hModule.get(), IDS_SHIM, verify, ARRAYSIZE(verify)) || lstrcmpi(verify, _T("RadShim")) != 0)
        {
            Error(_T("Not a valid RadShim file: %s"), shim);
            return EXIT_FAILURE;
        }

        TCHAR version[256];
        CHECK_LE(LoadString(hModule.get(), IDS_VERSION, version, ARRAYSIZE(version)));
        TCHAR target[MAX_PATH];
        CHECK_LE(LoadString(hModule.get(), IDS_TARGET, target, ARRAYSIZE(target)));

        _tprintf(_T("Version: %s\n"), version);
        _tprintf(_T("Target: %s\n"), target);

        if (!PathFileExists(target))
        {
            Error(_T("Target file does not exist: %s"), target);
            return EXIT_FAILURE;
        }
    }
    else
    {
        _tprintf(_T("RadShim\n"));
        Error(_T("Unknown command: %s"), command);
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

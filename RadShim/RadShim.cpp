#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shellapi.h>

#include <cstdlib>
#include <cstdio>
#include <io.h>
#include <vector>

#include "..\ShimLib\Error.h"
#include "..\ShimLib\ShimLib.h"
#include "AnsiPrint.h"
#include "Resources.h"
#include "arg.h"
#include "resource.h"

#define COLOR(n) "\x1B[" #n "m"

inline HANDLE FixHandle(HANDLE h)
{
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

typedef std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&CloseHandle)> UniqueHandle;

inline UniqueHandle InitUniqueHandle(HANDLE hHandle = NULL)
{
    return UniqueHandle(FixHandle(hHandle), CloseHandle);
}

typedef std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> UniqueModule;

inline UniqueModule InitUniqueModule(HMODULE hModule = NULL)
{
    return UniqueModule(hModule, FreeLibrary);
}

void GetShimPath(LPTSTR path, DWORD size)
{
    if (GetEnvironmentVariable(TEXT("RADSHIMDIR"), path, size) == 0)
        ExpandEnvironmentStrings(TEXT("%LOCALAPPDATA%\\RadShim"), path, size);
}

void CopyShim(LPCTSTR file, const bool isConsole)
{
    TCHAR shim[MAX_PATH];
    GetModuleFileName(NULL, shim, ARRAYSIZE(shim));
    LPTSTR shimname = PathFindFileName(shim);
    lstrcpy(shimname, isConsole ? TEXT("CShim.exe") : TEXT("WShim.exe"));
    CHECK_LE_CTX(CopyFile(shim, file, FALSE), shim);
}

void ExtractShim(LPCTSTR file, const bool isConsole)
{
    ResData resdata = GetResource(NULL, MAKEINTRESOURCE(isConsole ? IDR_CSHIM_EXE : IDR_WSHIM_EXE), RT_RCDATA);
    UniqueHandle hFile = InitUniqueHandle();
    CHECK_LE_CTX(hFile = InitUniqueHandle(CreateFile(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)), file);
    _Analysis_assume_(hFile != NULL);
    DWORD bytesWritten = 0;
    CHECK_LE(WriteFile(hFile.get(), resdata.data, resdata.size, &bytesWritten, NULL));
    _ASSERTE(bytesWritten == resdata.size);
}

void UpdateShimStringTable(LPCTSTR file, LPCTSTR target)
{
    _ASSERTE(!PathIsRelative(file));
    LPCTSTR lpName = MAKEINTRESOURCE(IDS_TARGET / STRINGTABLE_SIZE + 1);
    const WORD wLanguage = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    UniqueModule hModule(InitUniqueModule());
    CHECK_LE_CTX(hModule = InitUniqueModule(LoadLibraryEx(file, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)), file);
    StringTable stringtable = LoadStringTable(hModule.get(), lpName, wLanguage);
    hModule.reset();

    stringtable.item[IDS_TARGET % STRINGTABLE_SIZE] = target;

    UniqueUpdateResource hUpdate;
    CHECK_LE_CTX(hUpdate = UniqueUpdateResource(BeginUpdateResource(file, FALSE)), file);
    SaveStringTable(hUpdate.get(), lpName, wLanguage, stringtable);
    hUpdate.get_deleter().discard = FALSE;
}

void CopyResources(LPCTSTR file, LPCTSTR target)
{
    _ASSERTE(!PathIsRelative(target));
    UniqueModule hModule(InitUniqueModule());
    CHECK_LE_CTX(hModule = InitUniqueModule(LoadLibraryEx(target, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)), target);
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

bool AnsiColor(LPCTSTR color)
{
    if (lstrcmpi(color, _T("true")) == 0)
        return true;
    else if (lstrcmpi(color, _T("false")) == 0)
        return false;
    else if (lstrcmpi(color, _T("auto")) == 0)
        return _isatty(_fileno(stdout));
    else
        return true;
}

int _tmain(const int argc, const TCHAR* const argv[])
try
{
    arginit(argc, argv, _T("Shim management"));
    g_Ansi = AnsiColor(argvalue(TEXT("/Color"), TEXT("auto"), TEXT("true|false|auto"), TEXT("Use ansi color output")));
    LPCTSTR command = argnext(nullptr, TEXT("command"), _T("Command to execute"));
    if (lstrcmpi(command, _T("create")) == 0)
    {
        LPCTSTR target = argnext(nullptr, TEXT("target"), _T("Shim target executable"));
        if (!argcleanup())
            return EXIT_FAILURE;
        if (argusage())
        {
            _tprintf(TEXT("\n"));
            _tprintf(TEXT("If RADSHIMDIR is defined then shim is created in that directory\n"));
            _tprintf(TEXT("otherwise shim is created in %%LOCALAPPDATA%%\\RadShim\n"));
            return EXIT_SUCCESS;
        }

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
        CHECK_LE_CTX(exe_type = SHGetFileInfo(target, 0, nullptr, 0, SHGFI_EXETYPE), target);
        const bool isConsole = HIWORD(exe_type) == 0;

        TCHAR file[MAX_PATH];
        GetShimPath(file, ARRAYSIZE(file));
        if (!PathFileExists(file))
        {
            Error(_T("Shim directory doesn't exist: %s"), file);
            return EXIT_FAILURE;
        }
        CHECK_LE(PathCombine(file, file, PathFindFileName(target)));

        //CopyShim(file, isConsole);
        ExtractShim(file, isConsole);

        UpdateShimStringTable(file, target);
        CopyResources(file, target);

        AnsiTPrintf(_T("RadShim: " COLOR(34) "%s" COLOR(0) " ==> " COLOR(33) "%s" COLOR(0) "\n"), file, target);
    }
    else if (lstrcmpi(command, _T("details")) == 0)
    {
        LPCTSTR shimname = argnext(nullptr, TEXT("shim"), _T("Name of shim"));
        if (!argcleanup())
            return EXIT_FAILURE;
        if (argusage())
            return EXIT_SUCCESS;

        if (_tcschr(shimname, TEXT('\\')) != nullptr)
        {
            Error(_T("Shim name cannot contain path separators: %s"), shimname);
            return EXIT_FAILURE;
        }
        TCHAR shim[MAX_PATH];
        GetShimPath(shim, ARRAYSIZE(shim));
        if (!PathFileExists(shim))
        {
            Error(_T("Shim directory doesn't exist: %s"), shim);
            return EXIT_FAILURE;
        }
        CHECK_LE(PathCombine(shim, shim, shimname));

        AnsiTPrintf(_T(COLOR(37) "Shim:" COLOR(0) " %s\n"), shim);

        UniqueModule hModule(InitUniqueModule());
        CHECK_LE_CTX(hModule = InitUniqueModule(LoadLibraryEx(shim, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)), shim);

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

        AnsiTPrintf(_T(COLOR(37) "Version:" COLOR(0) " %s\n"), version);

        if (!PathFileExists(target))
        {
            AnsiTPrintf(_T(COLOR(37) "Target:" COLOR(31) " %s" COLOR(0) "\n"), target);
            Error(_T("Target file does not exist: %s"), target);
            return EXIT_FAILURE;
        }
        else
            AnsiTPrintf(_T(COLOR(37) "Target:" COLOR(34) " %s" COLOR(0) "\n"), target);
    }
    else
    {
        if (!argcleanup(true))
            return EXIT_FAILURE;
        if (argusage(true))
        {
            _tprintf(TEXT("\n"));
            AnsiTPrintf(TEXT("Where " COLOR(33) "command" COLOR(0) " is one of:\n"));
            AnsiTPrintf(TEXT("  " COLOR(36) "create" COLOR(0) "  - create a new shim\n"));
            AnsiTPrintf(TEXT("  " COLOR(36) "details" COLOR(0) " - show the details of a shim\n"));
            return EXIT_SUCCESS;
        }
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
    AnsiFTPrintf(stderr, TEXT(COLOR(31) "Error:" COLOR(0) " %s\n"), buffer);
}

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

#define STRINGTABLE_SIZE 16

struct StringTable
{
    std::wstring item[STRINGTABLE_SIZE];
};

StringTable LoadStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage)
{
    _ASSERTE(!PathIsRelative(file));
    HMODULE hModule;
    CHECK_LE(hModule = LoadLibraryEx(file, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE));
    HRSRC hResInfo;
    CHECK_LE(hResInfo = FindResourceEx(hModule, RT_STRING, lpName, wLanguage));
    HGLOBAL hRes;
    CHECK_LE(hRes = LoadResource(hModule, hResInfo));
    DWORD sz = SizeofResource(hModule, hResInfo);
    StringTable stringtable;
    {
        BYTE* data = (BYTE*) LockResource(hRes);
        for (UINT i = 0; i < STRINGTABLE_SIZE; ++i)
        {
            WORD nLength = *((WORD*) data);
            data += sizeof(nLength);
            if (nLength > 0)
            {
                LPCWSTR str = (LPCWSTR) data;
                data += nLength * sizeof(WCHAR);
                stringtable.item[i] = std::wstring(str, nLength);
            }
        }
        _ASSERTE((DWORD) (data - (BYTE*) hRes) == sz);
    }
    FreeLibrary(hModule);
    return stringtable;
}

void pack(std::vector<BYTE>& data, WORD value)
{
    data.push_back((BYTE) (value & 0xFF));
    data.push_back((BYTE) ((value >> 8) & 0xFF));
}

void pack(std::vector<BYTE>& data, LPCWSTR str, WORD nLength)
{
    for (UINT i = 0; i < nLength; ++i)
    {
        WORD ch = str[i];
        pack(data, ch);
    }
}

void SaveStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage, const StringTable& stringtable)
{
    std::vector<BYTE> data;
    for (UINT i = 0; i < STRINGTABLE_SIZE; ++i)
    {
        WORD nLength = (WORD) stringtable.item[i].length();
        pack(data, nLength);
        pack(data, stringtable.item[i].data(), nLength);
    }
    HANDLE hUpdate;
    CHECK_LE(hUpdate = BeginUpdateResource(file, FALSE));
    CHECK_LE(UpdateResource(hUpdate, RT_STRING, lpName, wLanguage, data.data(), (DWORD) data.size() * sizeof(BYTE)));
    CHECK_LE(EndUpdateResource(hUpdate, FALSE));
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
            _tprintf(_T("Target cannot be relative: %s\n"), target);
            return EXIT_FAILURE;
        }
        if (!PathFileExists(target))
        {
            _tprintf(_T("Target file does not exist: %s\n"), target);
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

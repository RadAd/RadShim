#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Resources.h"

#include <fstream>
#include <memory>
#include <vector>
#include <shlwapi.h>
#include "..\ShimLib\ShimLib.h"

inline HANDLE FixHandle(HANDLE h)
{
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

void UpdateResourceDeleter::operator()(HANDLE hUpdate) const
{
    if (hUpdate)
        CHECK_LE(EndUpdateResource(hUpdate, discard));
}

StringTable LoadStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage)
{
    _ASSERTE(!PathIsRelative(file));
    UniqueModule hModule(InitUniqueModule());
    CHECK_LE(hModule = InitUniqueModule(LoadLibraryEx(file, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE)));
    HRSRC hResInfo;

    CHECK_LE(hResInfo = FindResourceEx(hModule.get(), RT_STRING, lpName, wLanguage));
    HGLOBAL hRes;
    CHECK_LE(hRes = LoadResource(hModule.get(), hResInfo));
    const DWORD sz = SizeofResource(hModule.get(), hResInfo);
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
    return stringtable;
}

static void pack(std::vector<BYTE>& data, WORD value)
{
    data.push_back((BYTE) (value & 0xFF));
    data.push_back((BYTE) ((value >> 8) & 0xFF));
}

static void pack(std::vector<BYTE>& data, LPCWSTR str, WORD nLength)
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
    UniqueUpdateResource hUpdate;
    CHECK_LE(hUpdate = UniqueUpdateResource(BeginUpdateResource(file, FALSE)));
    CHECK_LE(UpdateResource(hUpdate.get(), RT_STRING, lpName, wLanguage, data.data(), (DWORD) data.size() * sizeof(BYTE)));
    hUpdate.get_deleter().discard = FALSE;
}

void ExtractResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType, LPCTSTR output)
{
    HRSRC hResInfo;
    CHECK_LE(hResInfo = FindResource(hModule, lpName, lpType));
    HGLOBAL hRes;
    CHECK_LE(hRes = LoadResource(hModule, hResInfo));
    const DWORD sz = SizeofResource(hModule, hResInfo);
    const char* data = (const char*) LockResource(hRes);
#if 0
    std::ofstream f;
    f.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    f.open(output, std::ios::out | std::ios::binary);
    f.write(data, sz);
    f.close();
#else
    HANDLE hFile;
    CHECK_LE(hFile = FixHandle(CreateFile(output, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)));
    DWORD bytesWritten = 0;
    CHECK_LE(WriteFile(hFile, data, sz, &bytesWritten, NULL));
    _ASSERTE(bytesWritten == sz);
    CHECK_LE(CloseHandle(hFile));
#endif
}

// https://docs.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)
// https://stackoverflow.com/questions/20729156/find-out-number-of-icons-in-an-icon-resource-using-win32-api
// https://devblogs.microsoft.com/oldnewthing/?p=7083

#pragma pack( push )
#pragma pack( 2 )
typedef struct GRPIconDirEntry
{
    BYTE   bWidth;               // Width, in pixels, of the image
    BYTE   bHeight;              // Height, in pixels, of the image
    BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE   bReserved;            // Reserved
    WORD   wPlanes;              // Color Planes
    WORD   wBitCount;            // Bits per pixel
    DWORD  dwBytesInRes;         // how many bytes in this resource?
    WORD   nID;                  // the ID
} GRPICONDIRENTRY, * LPGRPICONDIRENTRY;
#pragma pack( pop )

#pragma pack( push )
#pragma pack( 2 )
#pragma warning( push )
#pragma warning( disable : 4200 ) // nonstandard extension used: zero-sized array in struct/union
typedef struct GRPIconDir
{
    WORD            idReserved;   // Reserved (must be 0)
    WORD            idType;       // Resource type (1 for icons)
    WORD            idCount;      // How many images?
    GRPICONDIRENTRY idEntries[]; // The entries for each image
} GRPICONDIR, * LPGRPICONDIR;
#pragma warning( pop )
#pragma pack( pop )

static BOOL CALLBACK EnumLangsGroupIconFunc(
    HMODULE hModule, // module handle
    LPCTSTR lpType,  // address of resource type
    LPCTSTR lpName,  // address of resource name
    WORD wLang,      // resource language
    LONG_PTR lParam)     // extra parameter, could be used for error checking
{
    _ASSERTE(lpType == RT_GROUP_ICON);
    std::vector<WORD>* picon_data = (std::vector<WORD>*) lParam;

    HRSRC hResInfo;
    CHECK_RET(hResInfo = FindResourceEx(hModule, lpType, lpName, wLang), TRUE);
    HGLOBAL hRes;
    CHECK_RET(hRes = LoadResource(hModule, hResInfo), TRUE);

    const GRPICONDIR* lpGrpIconDir = (GRPICONDIR*) LockResource(hRes);
    for (size_t i = 0; i < lpGrpIconDir->idCount; ++i)
    {
        const GRPICONDIRENTRY& DirEntry = lpGrpIconDir->idEntries[i];
        picon_data->push_back(DirEntry.nID);
    }

    return TRUE;
}

std::vector<WORD> GetIconResourceIDs(HMODULE hModule, LPCWSTR lpName)
{
    std::vector<WORD> icon_data;
    EnumResourceLanguages(hModule, RT_GROUP_ICON, lpName, EnumLangsGroupIconFunc, (LPARAM) &icon_data);
    return icon_data;
}

static BOOL CALLBACK EnumNamesFindFirstFunc(
    HMODULE hModule,  // module handle
    LPCTSTR lpType,   // address of resource type
    LPTSTR lpName,    // address of resource name
    LONG_PTR lParam)      // extra parameter, could be used for error checking
{
    LPCTSTR* pname = (LPCTSTR*) lParam;
    *pname = lpName;
    return FALSE;
}

LPCTSTR FindFirstResourceName(HMODULE hModule, LPCWSTR lpType)
{
    LPCTSTR name = nullptr;
    EnumResourceNames(hModule, lpType, EnumNamesFindFirstFunc, (LPARAM) &name);
    return name;
}

static BOOL CALLBACK EnumLangsCopyResourceFunc(
    HMODULE hModule, // module handle
    LPCTSTR lpType,  // address of resource type
    LPCTSTR lpName,  // address of resource name
    WORD wLang,      // resource language
    LONG_PTR lParam)     // extra parameter, could be used for error checking
{
    HANDLE hUpdate = (HANDLE) lParam;

    HRSRC hResInfo;
    CHECK_RET(hResInfo = FindResourceEx(hModule, lpType, lpName, wLang), TRUE);
    HGLOBAL hRes;
    CHECK_RET(hRes = LoadResource(hModule, hResInfo), TRUE);

    const DWORD sz = SizeofResource(hModule, hResInfo);
    LPVOID pData = LockResource(hRes);

    return UpdateResource(hUpdate, lpType, lpName, wLang, pData, sz);
}

BOOL CopyResource(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, HANDLE hUpdate)
{
    return EnumResourceLanguages(hModule, lpType, lpName, EnumLangsCopyResourceFunc, (LPARAM) hUpdate);
}

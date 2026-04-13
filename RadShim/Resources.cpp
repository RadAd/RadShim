#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Resources.h"

#include "..\ShimLib\Error.h"

void UpdateResourceDeleter::operator()(_Notnull_ HANDLE hUpdate) const
{
    CHECK_LE(EndUpdateResource(hUpdate, discard));
}

StringTable LoadStringTable(HMODULE hModule, LPCTSTR lpName, WORD wLanguage)
{
    HRSRC hResInfo;
    CHECK_LE(hResInfo = FindResourceEx(hModule, RT_STRING, lpName, wLanguage));
    _Analysis_assume_(hResInfo != NULL);
    HGLOBAL hRes;
    CHECK_LE(hRes = LoadResource(hModule, hResInfo));
    _Analysis_assume_(hRes != NULL);
    const DWORD sz = SizeofResource(hModule, hResInfo);
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

static void pack(std::vector<BYTE>& data, LPCTSTR str, WORD nLength)
{
    for (UINT i = 0; i < nLength; ++i)
    {
        WORD ch = str[i];
        pack(data, ch);
    }
}

void SaveStringTable(HANDLE hUpdate, LPCTSTR lpName, WORD wLanguage, const StringTable& stringtable)
{
    std::vector<BYTE> data;
    for (UINT i = 0; i < STRINGTABLE_SIZE; ++i)
    {
        WORD nLength = (WORD) stringtable.item[i].length();
        pack(data, nLength);
        pack(data, stringtable.item[i].data(), nLength);
    }
    CHECK_LE(UpdateResource(hUpdate, RT_STRING, lpName, wLanguage, data.data(), (DWORD) data.size() * sizeof(BYTE)));
}

ResData GetResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType)
{
    HRSRC hResInfo;
    CHECK_LE(hResInfo = FindResource(hModule, lpName, lpType));
    _Analysis_assume_(hResInfo != NULL);
    HGLOBAL hRes;
    CHECK_LE(hRes = LoadResource(hModule, hResInfo));
    _Analysis_assume_(hRes != NULL);
    return { LockResource(hRes), SizeofResource(hModule, hResInfo) };
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

std::vector<WORD> GetIconResourceIDs(HMODULE hModule, LPCTSTR lpName)
{
    std::vector<WORD> icon_data;
    EnumResourceLanguages(hModule, RT_GROUP_ICON, lpName,
        [&icon_data](HMODULE hModule, _Notnull_ LPCTSTR lpType, _Notnull_ LPCTSTR lpName, WORD wLang)->BOOL
        {
            _ASSERTE(lpType == RT_GROUP_ICON);

            HRSRC hResInfo;
            CHECK_RET(hResInfo = FindResourceEx(hModule, lpType, lpName, wLang), TRUE);
            _Analysis_assume_(hResInfo != NULL);
            HGLOBAL hRes;
            CHECK_RET(hRes = LoadResource(hModule, hResInfo), TRUE);

            const GRPICONDIR* lpGrpIconDir = (GRPICONDIR*) LockResource(hRes);
            for (size_t i = 0; i < lpGrpIconDir->idCount; ++i)
            {
                const GRPICONDIRENTRY& DirEntry = lpGrpIconDir->idEntries[i];
                icon_data.push_back(DirEntry.nID);
            }

            return TRUE;
        });
    return icon_data;
}

LPCTSTR FindFirstResourceName(HMODULE hModule, LPCTSTR lpType)
{
    LPCTSTR name = nullptr;
    EnumResourceNames(hModule, lpType,
        [&name](HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName)->BOOL
        {
            name = lpName;
            return FALSE;
        });
    return name;
}

BOOL CopyResource(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, HANDLE hUpdate)
{
    return EnumResourceLanguages(hModule, lpType, lpName,
        [hUpdate](HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wLang) -> BOOL
        {
            HRSRC hResInfo;
            CHECK_RET(hResInfo = FindResourceEx(hModule, lpType, lpName, wLang), TRUE);
            HGLOBAL hRes;
            CHECK_RET(hRes = LoadResource(hModule, hResInfo), TRUE);

            const DWORD sz = SizeofResource(hModule, hResInfo);
            LPVOID pData = LockResource(hRes);

            return UpdateResource(hUpdate, lpType, lpName, wLang, pData, sz);
        });
}

BOOL CALLBACK EnumResourceNamesHelper(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam)
{
    EnumResNamesFunc* pFunc = (EnumResNamesFunc*) lParam;
    return (*pFunc)(hModule, lpType, lpName);
}

BOOL CALLBACK EnumResourceLanguagesHelper(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wLang, LONG_PTR lParam)
{
    EnumResLangFunc* pFunc = (EnumResLangFunc*) lParam;
    return (*pFunc)(hModule, lpType, lpName, wLang);
}

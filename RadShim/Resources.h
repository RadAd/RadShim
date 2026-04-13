#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

#define STRINGTABLE_SIZE 16

struct UpdateResourceDeleter
{
    using pointer = HANDLE;
    void operator()(_Notnull_ HANDLE hUpdate) const;
    BOOL discard = TRUE;
};

typedef std::unique_ptr<std::remove_pointer_t<HANDLE>, UpdateResourceDeleter> UniqueUpdateResource;

struct StringTable
{
    std::wstring item[STRINGTABLE_SIZE];
};

StringTable LoadStringTable(HMODULE hModule, LPCTSTR lpName, WORD wLanguage);
void SaveStringTable(HANDLE hUpdate, LPCTSTR lpName, WORD wLanguage, const StringTable& stringtable);

struct ResData
{
    LPVOID data;
    DWORD size;
};
ResData GetResource(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);

std::vector<WORD> GetIconResourceIDs(HMODULE hModule, LPCTSTR lpName);

LPCTSTR FindFirstResourceName(HMODULE hModule, LPCTSTR lpType);
BOOL CopyResource(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, HANDLE hUpdate);


#include <functional>

typedef std::function<BOOL CALLBACK(HMODULE, LPCTSTR, LPCTSTR)> EnumResNamesFunc;

BOOL CALLBACK EnumResourceNamesHelper(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam);

template <typename F>
inline BOOL EnumResourceNames(HMODULE hModule, LPCTSTR lpType, F f)
{
    EnumResNamesFunc func(f);
    return EnumResourceNames(hModule, lpType, EnumResourceNamesHelper, (LPARAM) &func);
}

typedef std::function<BOOL CALLBACK(HMODULE, LPCTSTR, LPCTSTR, WORD)> EnumResLangFunc;

BOOL CALLBACK EnumResourceLanguagesHelper(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wLang, LONG_PTR lParam);

template <typename F>
inline BOOL EnumResourceLanguages(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, F f)
{
    EnumResLangFunc func(f);
    return EnumResourceLanguages(hModule, lpType, lpName, EnumResourceLanguagesHelper, (LPARAM) &func);
}

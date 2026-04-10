#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

#define STRINGTABLE_SIZE 16

typedef std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> UniqueModule;

inline UniqueModule InitUniqueModule(HMODULE hModule = NULL)
{
    return UniqueModule(hModule, FreeLibrary);
}

struct UpdateResourceDeleter
{
    using pointer = HANDLE;
    void operator()(HANDLE hUpdate) const;
    BOOL discard = TRUE;
};

typedef std::unique_ptr<std::remove_pointer_t<HANDLE>, UpdateResourceDeleter> UniqueUpdateResource;

struct StringTable
{
    std::wstring item[STRINGTABLE_SIZE];
};

StringTable LoadStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage);
void SaveStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage, const StringTable& stringtable);

std::vector<WORD> GetIconResourceIDs(HMODULE hModule, LPCWSTR lpName);

LPCTSTR FindFirstResourceName(HMODULE hModule, LPCWSTR lpType);
BOOL CopyResource(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, HANDLE hUpdate);

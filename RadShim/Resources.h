#pragma once
#include <string>
#include <vector>

#define STRINGTABLE_SIZE 16

struct StringTable
{
    std::wstring item[STRINGTABLE_SIZE];
};

StringTable LoadStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage);
void SaveStringTable(LPCTSTR file, LPCWSTR lpName, WORD wLanguage, const StringTable& stringtable);

std::vector<WORD> GetIconResourceIDs(HMODULE hModule, LPCWSTR lpName);

LPCTSTR FindFirstResourceName(HMODULE hModule, LPCWSTR lpType);
void CopyResource(HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, HANDLE hUpdate);

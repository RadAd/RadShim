#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool g_Ansi;

void AnsiPrintf(_In_z_ _Printf_format_string_ const char* format, ...);
void AnsiWPrintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...);
void AnsiFPrintf(FILE* o, _In_z_ _Printf_format_string_ const char* format, ...);
void AnsiFWPrintf(FILE* o, _In_z_ _Printf_format_string_ const wchar_t* format, ...);
void AnsiFVPrintf(FILE* o, _In_z_ _Printf_format_string_ const char* format, va_list args);
void AnsiFVWPrintf(FILE* o, _In_z_ _Printf_format_string_ const wchar_t* format, va_list args);

#ifdef UNICODE
#define AnsiTPrintf AnsiWPrintf
#define AnsiFTPrintf AnsiFWPrintf
#define AnsiFVTPrintf AnsiFVWPrintf
#else
#define AnsiTPrintf AnsiPrintf
#define AnsiFTPrintf AnsiFPrintf
#define AnsiFVTPrintf AnsiFVPrintf
#endif

#ifdef __cplusplus
}
#endif

#include "AnsiPrint.h"

#include <string>
#include <string_view>

#define ESC '\x1B'

namespace {

template <class CharT, class F>
typename std::basic_string_view<CharT>::size_type find_if(std::basic_string_view<CharT> s, typename std::basic_string_view<CharT>::size_type pos, F f)
{
    auto it = std::find_if(s.begin() + pos, s.end(), f);
    return it == s.end() ? std::string::npos : it - s.begin();
}

template <class CharT>
std::basic_string<CharT> StripAnsi(std::basic_string_view<CharT> s)
{
    std::basic_string<CharT> r;
    if (!s.empty())
    {
        typename std::basic_string_view<CharT>::size_type pos = 0;
        while (true)
        {
            if (const std::basic_string_view<CharT>::size_type b = s.find(ESC, pos); b == std::basic_string_view<CharT>::npos)
            {
                r.append(s.substr(pos));
                break;
            }
            else if (const std::basic_string_view<CharT>::size_type e = find_if(s, b + 1, std::isalpha); e == std::basic_string_view<CharT>::npos)
            {
                r.append(s.substr(pos));
                break;
            }
            else
            {
                r.append(s.substr(pos, b - pos));
                pos = e + 1;
            }
        }
    }
    return r;
}

}

extern "C" {

    bool g_Ansi = true; // TODO Do we need one for stderr?


    void AnsiFVPrintf(FILE* o, _In_z_ _Printf_format_string_ const char* format, va_list args)
    {
        std::string_view s(format);
        if (g_Ansi || s.find(ESC, 0) == std::string_view::npos)
            _vfprintf_p(o, format, args);
        else
            _vfprintf_p(o, StripAnsi<char>(format).c_str(), args);
    }

    void AnsiFVWPrintf(FILE* o, _In_z_ _Printf_format_string_ const wchar_t* format, va_list args)
    {
        std::wstring_view s(format);
        if (g_Ansi || s.find(ESC, 0) == std::wstring_view::npos)
            _vfwprintf_p(o, format, args);
        else
            _vfwprintf_p(o, StripAnsi<wchar_t>(format).c_str(), args);
    }

    void AnsiFPrintf(FILE* o, _In_z_ _Printf_format_string_ const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AnsiFVPrintf(o, format, args);
        va_end(args);
    }

    void AnsiPrintf(_In_z_ _Printf_format_string_ const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        AnsiFVPrintf(stdout, format, args);
        va_end(args);
    }

    void AnsiFWPrintf(FILE* o, _In_z_ _Printf_format_string_ const wchar_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        AnsiFVWPrintf(o, format, args);
        va_end(args);
    }

    void AnsiWPrintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        AnsiFVWPrintf(stdout, format, args);
        va_end(args);
    }

}

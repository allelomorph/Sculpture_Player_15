#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>

// ===================== char / numeric predicates =====================

inline bool IsSpace(unsigned char c) noexcept
{
    return c == ' '; // or: return std::isspace(c) != 0;
}

inline bool IsAlnum(unsigned char c) noexcept
{
    return std::isalnum(c) != 0; // safe with unsigned char
}

inline bool IsNotAlnum(unsigned char c) noexcept
{
    return !IsAlnum(c);
}

inline bool IsAnInteger(const std::string &s) noexcept
{
    if (s.empty())
        return false;
    std::size_t start = 0;
    if (s[0] == '-' || s[0] == '+')
    {
        if (s.size() == 1)
            return false; // "+" or "-" alone is not an int
        start = 1;
    }
    return std::all_of(s.begin() + start, s.end(),
                       [](unsigned char ch)
                       { return std::isdigit(ch) != 0; });
}

inline std::string Check_Numeric(const std::string &value)
{
    return IsAnInteger(value) ? value : std::string("0");
}

inline std::string Check_Numeric(const std::string &value,
                                 const std::string &error_return)
{
    return IsAnInteger(value) ? value : error_return;
}

// remove all chars of a type from a given string (by value)
inline std::string removeChar(std::string str, char c) noexcept
{
    str.erase(std::remove(str.begin(), str.end(), c), str.end());
    return str;
}

inline constexpr bool isComma(char c) noexcept { return c == ','; }

// ===================== trim helpers =====================

// trim from left
inline std::string &ltrim(std::string &s,
                          std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    if (s.empty())
        return s;
    const auto pos = s.find_first_not_of(t);
    if (pos == std::string::npos)
        s.clear();
    else
        s.erase(0, pos);
    return s;
}

// trim from right
inline std::string &rtrim(std::string &s,
                          std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    if (s.empty())
        return s;
    const auto pos = s.find_last_not_of(t);
    s.erase((pos == std::string::npos) ? 0 : pos + 1);
    return s;
}

// trim from both sides
inline std::string &trim(std::string &s,
                         std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    return ltrim(rtrim(s, t), t);
}

// copying versions
inline std::string ltrim_copy(std::string s,
                              std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    return ltrim(s, t);
}
inline std::string rtrim_copy(std::string s,
                              std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    return rtrim(s, t);
}
inline std::string trim_copy(std::string s,
                             std::string_view t = " \"\t\n\r\f\v{}[]") noexcept
{
    return trim(s, t);
}

// ===================== vectorize text array =====================

// ["3,4,5"] or "[3, 4, 5]" or "3,4,5"  ->  {"3","4","5"}
inline std::vector<std::string> Vectorize_Text_Array(std::string s)
{
    // strip optional outer brackets only
    if (!s.empty() && s.front() == '[')
        s.erase(s.begin());
    if (!s.empty() && s.back() == ']')
        s.pop_back();

    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ','))
    {
        // trim per token; keep your default trim set or use whitespace-only
        tok.erase(0, tok.find_first_not_of(" \t\r\n"));
        tok.erase(tok.find_last_not_of(" \t\r\n") + 1);
        if (!tok.empty())
            result.push_back(tok);
    }
    return result;
}

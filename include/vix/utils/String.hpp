#ifndef VIX_STRING_HPP
#define VIX_STRING_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Vix::utils
{
    inline std::string ltrim(std::string s)
    {
        s.erase(
            s.begin(),
            std::find_if(s.begin(),
                         s.end(),
                         [](unsigned char c)
                         { return !std::isspace(c); }));
        return s;
    }
    inline std::string rtrim(std::string s)
    {
        s.erase(
            std::find_if(s.rbegin(),
                         s.rend(),
                         [](unsigned char c)
                         { return !std::isspace(c); })
                .base(),
            s.end());
        return s;
    }

    inline std::string trim(std::string s)
    {
        return rtrim(ltrim(std::move(s)));
    }

    inline std::string to_lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        return s;
    }
    inline bool starts_with(std::string_view s, std::string_view p)
    {
        return s.rfind(p, 0) == 0;
    }
    inline bool ends_with(std::string_view s, std::string_view p)
    {
        return s.size() >= p.size() && 0 == s.compare(s.size() - p.size(), p.size(), p);
    }

    inline std::vector<std::string> split(std::string_view s, char sep)
    {
        std::vector<std::string> out;
        std::string cur;
        std::stringstream ss{std::string(s)};
        while (std::getline(ss, cur, sep))
            out.push_back(cur);
        return out;
    }

    inline std::string join(const std::vector<std::string> &v, std::string_view sep)
    {
        std::string out;
        for (size_t i = 0; i < v.size(); ++i)
        {
            if (i)
                out += sep;
            out += v[i];
        }
        return out;
    }

}

#endif
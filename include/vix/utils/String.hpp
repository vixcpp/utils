#ifndef VIX_STRING_HPP
#define VIX_STRING_HPP

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>

namespace Vix::utils
{
    // --- helpers internes ---
    inline constexpr bool _is_space(unsigned char c) noexcept
    {
        return std::isspace(c) != 0; // C-locale; sufficient for generic trimming
    }

    // left-trim (in-place, returns s by value for chaining)
    inline std::string ltrim(std::string s) noexcept
    {
        auto it = std::find_if(s.begin(), s.end(), [](unsigned char c)
                               { return !_is_space(c); });
        s.erase(s.begin(), it);
        return s;
    }

    // right-trim (in-place, returns s by value for chaining)
    inline std::string rtrim(std::string s) noexcept
    {
        auto it = std::find_if(s.rbegin(), s.rend(), [](unsigned char c)
                               { return !_is_space(c); })
                      .base();
        s.erase(it, s.end());
        return s;
    }

    inline std::string trim(std::string s) noexcept
    {
        return rtrim(ltrim(std::move(s)));
    }

    inline std::string to_lower(std::string s) noexcept
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    inline bool starts_with(std::string_view s, std::string_view p) noexcept
    {
        return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
    }

    inline bool ends_with(std::string_view s, std::string_view p) noexcept
    {
        return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
    }

    // split without stringstream (faster, zero allocation except outputs)
    inline std::vector<std::string> split(std::string_view s, char sep)
    {
        std::vector<std::string> out;

        // cast explicite pour éviter le -Wsign-conversion
        {
            const auto cnt = static_cast<std::size_t>(std::count(s.begin(), s.end(), sep));
            out.reserve(cnt + 1);
        }

        std::size_t start = 0;
        while (start <= s.size())
        {
            const std::size_t pos = s.find(sep, start);
            if (pos == std::string_view::npos)
            {
                out.emplace_back(s.substr(start));
                break;
            }
            out.emplace_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        return out;
    }

    // Count non-overlapping occurrences of `needle` in `haystack`
    inline std::size_t count_nonoverlap(std::string_view haystack, std::string_view needle) noexcept
    {
        if (needle.empty())
            return 0;
        std::size_t count = 0, pos = 0;
        while (true)
        {
            pos = haystack.find(needle, pos);
            if (pos == std::string_view::npos)
                break;
            ++count;
            pos += needle.size();
        }
        return count;
    }

    // multi-character split: keep empty segments
    inline std::vector<std::string> split(std::string_view s, std::string_view sep)
    {
        if (sep.empty())
        {
            // same convention as many APIs: empty sep => no split
            return {std::string(s)};
        }

        std::vector<std::string> out;
        out.reserve(count_nonoverlap(s, sep) + 1);

        std::size_t start = 0;
        while (start <= s.size())
        {
            const std::size_t pos = s.find(sep, start);
            if (pos == std::string_view::npos)
            {
                out.emplace_back(s.substr(start));
                break;
            }
            out.emplace_back(s.substr(start, pos - start));
            start = pos + sep.size();
        }
        return out;
    }

    // join with capacity reservation
    inline std::string join(const std::vector<std::string> &v, std::string_view sep)
    {
        if (v.empty())
            return {};

        std::size_t total = (v.size() - 1) * sep.size();
        for (const auto &s : v)
            total += s.size();

        std::string out;
        out.reserve(total);

        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i)
                out += sep;
            out += v[i];
        }
        return out;
    }

} // namespace Vix::utils

#endif // VIX_STRING_HPP

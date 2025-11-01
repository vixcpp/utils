#ifndef VIX_STRING_HPP
#define VIX_STRING_HPP

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>

/**
 * @file VIX_STRING_HPP
 * @brief Small string helpers (trim, case transform, prefix/suffix checks, split/join).
 *
 * Header-only utilities designed for performance and clarity:
 *  - In-place-by-value trims (return-by-value for chaining)
 *  - ASCII-only case transform (C locale)
 *  - `split` without stringstream (fewer allocations)
 *  - `join` with pre-reservation
 *
 * @note Whitespace detection uses `std::isspace` in the C locale.
 * @note All functions are exception-free and `noexcept` where applicable.
 */

namespace vix::utils
{
    // ---------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------

    /**
     * @brief Tests if a byte is whitespace (C locale).
     * @param c Unsigned character byte.
     * @return `true` if `std::isspace(c)` is non-zero.
     */
    inline constexpr bool _is_space(unsigned char c) noexcept
    {
        return std::isspace(c) != 0; // C-locale; sufficient for generic trimming
    }

    // ---------------------------------------------------------------------
    // Trimming utilities
    // ---------------------------------------------------------------------

    /**
     * @brief Left-trim leading whitespace (C locale).
     *
     * Returns a new string with leading spaces removed. Operates on a copy
     * and returns by value for easy chaining.
     *
     * @param s Input string (copied).
     * @return String without leading whitespace.
     *
     * @code
     * auto x = ltrim("   hello ");  // -> "hello "
     * @endcode
     */
    inline std::string ltrim(std::string s) noexcept
    {
        auto it = std::find_if(s.begin(), s.end(), [](unsigned char c)
                               { return !_is_space(c); });
        s.erase(s.begin(), it);
        return s;
    }

    /**
     * @brief Right-trim trailing whitespace (C locale).
     *
     * Returns a new string with trailing spaces removed. Operates on a copy
     * and returns by value for easy chaining.
     *
     * @param s Input string (copied).
     * @return String without trailing whitespace.
     *
     * @code
     * auto x = rtrim("   hello ");  // -> "   hello"
     * @endcode
     */
    inline std::string rtrim(std::string s) noexcept
    {
        auto it = std::find_if(s.rbegin(), s.rend(), [](unsigned char c)
                               { return !_is_space(c); })
                      .base();
        s.erase(it, s.end());
        return s;
    }

    /**
     * @brief Trim both ends (C locale).
     * @param s Input string (copied).
     * @return String without leading nor trailing whitespace.
     *
     * @code
     * auto x = trim("  hello  "); // -> "hello"
     * @endcode
     */
    inline std::string trim(std::string s) noexcept
    {
        return rtrim(ltrim(std::move(s)));
    }

    // ---------------------------------------------------------------------
    // Case transform
    // ---------------------------------------------------------------------

    /**
     * @brief Convert to lowercase (ASCII only).
     *
     * Uses `std::tolower` (C locale) per byte. Non-ASCII characters
     * are transformed byte-wise (no unicode folding).
     *
     * @param s Input string (copied).
     * @return Lowercased string.
     */
    inline std::string to_lower(std::string s) noexcept
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // ---------------------------------------------------------------------
    // Prefix/Suffix checks
    // ---------------------------------------------------------------------

    /**
     * @brief Checks if `s` starts with prefix `p`.
     * @param s Full string (view).
     * @param p Prefix (view).
     * @return `true` if `s` begins with `p`.
     *
     * @code
     * starts_with("vix-core", "vix"); // true
     * @endcode
     */
    inline bool starts_with(std::string_view s, std::string_view p) noexcept
    {
        return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
    }

    /**
     * @brief Checks if `s` ends with suffix `p`.
     * @param s Full string (view).
     * @param p Suffix (view).
     * @return `true` if `s` ends with `p`.
     *
     * @code
     * ends_with("config.json", ".json"); // true
     * @endcode
     */
    inline bool ends_with(std::string_view s, std::string_view p) noexcept
    {
        return s.size() >= p.size() && s.compare(s.size() - p.size(), p.size(), p) == 0;
    }

    // ---------------------------------------------------------------------
    // Split / Join
    // ---------------------------------------------------------------------

    /**
     * @brief Split by a single-character separator.
     *
     * Keeps empty segments (e.g., splitting `",,a"` by `','` yields `["", "", "a"]`).
     * Uses linear scanning with capacity reservation; avoids `stringstream`.
     *
     * @param s Input string view.
     * @param sep Separator character.
     * @return Vector of substrings (materialized as `std::string`).
     *
     * @code
     * auto parts = split("a,b,,c", ','); // ["a","b","","c"]
     * @endcode
     */
    inline std::vector<std::string> split(std::string_view s, char sep)
    {
        std::vector<std::string> out;

        // explicit cast to avoid -Wsign-conversion
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

    /**
     * @brief Count non-overlapping occurrences of `needle` in `haystack`.
     * @param haystack Text to search.
     * @param needle Substring to count.
     * @return Number of non-overlapping matches.
     *
     * @code
     * count_nonoverlap("aaaa", "aa"); // 2
     * @endcode
     */
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

    /**
     * @brief Split by a multi-character separator. Keeps empty segments.
     *
     * If `sep` is empty, returns `{ std::string(s) }` (no split), matching many APIs.
     *
     * @param s Input string view.
     * @param sep Separator substring.
     * @return Vector of substrings (materialized as `std::string`).
     *
     * @code
     * auto parts = split("a--b----c", "--"); // ["a","b","","c"]
     * @endcode
     */
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

    /**
     * @brief Join strings with a separator, reserving capacity upfront.
     *
     * @param v Vector of strings to join.
     * @param sep Separator string (may be multi-character).
     * @return Concatenated string.
     *
     * @code
     * std::vector<std::string> v = {"a","b","","c"};
     * auto s = join(v, "::"); // "a::b::::c"
     * @endcode
     */
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

} // namespace vix::utils

#endif // VIX_STRING_HPP

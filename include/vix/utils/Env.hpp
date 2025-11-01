#ifndef VIX_ENV_HPP
#define VIX_ENV_HPP

#include <string>
#include <string_view>
#include <cstdlib>
#include <cctype>
#include <charconv>

/**
 * @file VIX_ENV_HPP
 * @brief Environment variable helpers for Vix.cpp utilities.
 *
 * This header provides simple, type-safe, and dependency-free functions
 * to read environment variables and convert them into useful types
 * (`std::string`, `bool`, `int`, `unsigned`, and `double`).
 *
 * These functions are:
 *  - Header-only and exception-free
 *  - Whitespace-tolerant (trim leading/trailing spaces)
 *  - Case-insensitive for boolean parsing
 *  - Fail-safe: return a default value on missing or invalid input
 *
 * @code
 * using namespace Vix::utils;
 *
 * std::string db = env_or("DB_URL", "mysql://localhost");
 * bool debug    = env_bool("APP_DEBUG", false);
 * int port      = env_int("PORT", 8080);
 * unsigned thr  = env_uint("WORKERS", 4);
 * double ratio  = env_double("CACHE_RATIO", 0.25);
 * @endcode
 *
 * @note Thread-safe for read-only access.
 * @note Works on POSIX and Windows.
 */

namespace vix::utils
{
    namespace detail
    {
        /**
         * @brief Converts a character to lowercase (ASCII only).
         * @param c The input character.
         * @return Lowercase version of `c`.
         */
        inline char to_lower_ascii(unsigned char c) noexcept
        {
            return static_cast<char>(std::tolower(c));
        }

        /**
         * @brief Performs a case-insensitive comparison between two strings (ASCII only).
         * @param a First string.
         * @param b Second string.
         * @return `true` if equal ignoring case, otherwise `false`.
         */
        inline bool iequals(std::string_view a, std::string_view b) noexcept
        {
            if (a.size() != b.size())
                return false;
            for (std::size_t i = 0; i < a.size(); ++i)
            {
                if (to_lower_ascii(static_cast<unsigned char>(a[i])) !=
                    to_lower_ascii(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }
    } // namespace detail

    /**
     * @brief Returns the value of an environment variable, or a default if not found.
     *
     * @param key The environment variable name.
     * @param def The default value to return if not found.
     * @return The variable’s value or the default string.
     *
     * @code
     * std::string host = env_or("APP_HOST", "127.0.0.1");
     * @endcode
     */
    inline std::string env_or(std::string_view key, std::string_view def = "")
    {
        if (const char *v = std::getenv(std::string(key).c_str()))
            return std::string(v);
        return std::string(def);
    }

    /**
     * @brief Reads an environment variable and interprets it as a boolean.
     *
     * Recognized truthy values (case-insensitive): `1`, `true`, `yes`, `on`.
     * Any other value, or missing variable, returns the default.
     *
     * @param key The environment variable name.
     * @param def Default value if missing or invalid.
     * @return Boolean value derived from the variable.
     *
     * @code
     * bool debug = env_bool("APP_DEBUG", false);
     * // APP_DEBUG=true → debug == true
     * // APP_DEBUG=no   → debug == false
     * @endcode
     */
    inline bool env_bool(std::string_view key, bool def = false)
    {
        using namespace std::literals;
        const auto s = env_or(key, def ? "1"sv : "0"sv);

        // Trim leading/trailing spaces
        std::size_t b = 0, e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
            ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
            --e;
        const std::string_view v{s.data() + b, e - b};

        return v == "1"sv ||
               detail::iequals(v, "true") ||
               detail::iequals(v, "yes") ||
               detail::iequals(v, "on");
    }

    /**
     * @brief Reads an environment variable as a signed integer (base 10).
     *
     * Leading/trailing spaces are trimmed. Returns the default if parsing fails.
     *
     * @param key The environment variable name.
     * @param def Default integer value if missing or invalid.
     * @return Parsed integer or default.
     *
     * @code
     * int port = env_int("PORT", 8080);
     * // PORT="9090" → 9090
     * // PORT="abc"  → 8080
     * @endcode
     */
    inline int env_int(std::string_view key, int def = 0)
    {
        const auto s = env_or(key);
        if (s.empty())
            return def;

        const char *first = s.data();
        const char *last = s.data() + s.size();
        while (first < last && std::isspace(static_cast<unsigned char>(*first)))
            ++first;
        while (last > first && std::isspace(static_cast<unsigned char>(*(last - 1))))
            --last;
        if (first >= last)
            return def;

        int value = def;
        const auto [ptr, ec] = std::from_chars(first, last, value, 10);
        if (ec != std::errc{} || ptr != last)
            return def;
        return value;
    }

    /**
     * @brief Reads an environment variable as an unsigned integer (base 10).
     *
     * Leading/trailing spaces are trimmed. Returns the default if parsing fails
     * or if the value is negative.
     *
     * @param key The environment variable name.
     * @param def Default unsigned integer if missing or invalid.
     * @return Parsed unsigned integer or default.
     *
     * @code
     * unsigned threads = env_uint("WORKERS", 4u);
     * @endcode
     */
    inline unsigned env_uint(std::string_view key, unsigned def = 0u)
    {
        const auto s = env_or(key);
        if (s.empty())
            return def;

        const char *first = s.data();
        const char *last = s.data() + s.size();
        while (first < last && std::isspace(static_cast<unsigned char>(*first)))
            ++first;
        while (last > first && std::isspace(static_cast<unsigned char>(*(last - 1))))
            --last;
        if (first >= last)
            return def;

        unsigned value = def;
        const auto [ptr, ec] = std::from_chars(first, last, value, 10);
        if (ec != std::errc{} || ptr != last)
            return def;
        return value;
    }

    /**
     * @brief Reads an environment variable as a floating-point value.
     *
     * Uses `std::strtod` for conversion, supports `.` as decimal separator only.
     * Returns the default if parsing fails or unparsed characters remain.
     *
     * @param key The environment variable name.
     * @param def Default double value if missing or invalid.
     * @return Parsed double or default.
     *
     * @code
     * double ratio = env_double("CACHE_RATIO", 0.25);
     * @endcode
     */
    inline double env_double(std::string_view key, double def = 0.0)
    {
        const auto s = env_or(key);
        if (s.empty())
            return def;

        char *endp = nullptr;
        const double v = std::strtod(s.c_str(), &endp);
        if (!endp || *endp != '\0')
            return def; // invalid (unparsed remainder)
        return v;
    }

} // namespace Vix::utils

#endif // VIX_ENV_HPP

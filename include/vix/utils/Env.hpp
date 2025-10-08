#ifndef VIX_ENV_HPP
#define VIX_ENV_HPP

#include <string>
#include <string_view>
#include <cstdlib>
#include <cctype>
#include <charconv>

namespace Vix::utils
{
    namespace detail
    {
        inline char to_lower_ascii(unsigned char c) noexcept
        {
            return static_cast<char>(std::tolower(c));
        }
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

    // Renvoie la valeur de la variable d'environnement, sinon def
    inline std::string env_or(std::string_view key, std::string_view def = "")
    {
        // std::getenv exige un C-string null-terminé
        if (const char *v = std::getenv(std::string(key).c_str()))
            return std::string(v);
        return std::string(def);
    }

    // Interprète une env var comme booléen (1/true/yes/on)
    inline bool env_bool(std::string_view key, bool def = false)
    {
        using namespace std::literals;
        const auto s = env_or(key, def ? "1"sv : "0"sv);
        // trim léger (espaces de tête/fin)
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

    // Lit un int en base 10 ; renvoie def si absent/invalide
    inline int env_int(std::string_view key, int def = 0)
    {
        const auto s = env_or(key);
        if (s.empty())
            return def;

        // trim léger
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

    // (Optionnel) entier non signé
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

    // (Optionnel) double ; tolère séparateur '.' ; renvoie def si invalide
    inline double env_double(std::string_view key, double def = 0.0)
    {
        const auto s = env_or(key);
        if (s.empty())
            return def;

        char *endp = nullptr;
        const double v = std::strtod(s.c_str(), &endp);
        if (!endp || *endp != '\0')
            return def; // invalide (reste non parsé)
        return v;
    }

} // namespace Vix::utils

#endif // VIX_ENV_HPP

#ifndef VIX_ENV_HPP
#define VIX_ENV_HPP

#include <string>
#include <cstdlib>
#include <cctype>

namespace Vix::utils
{

    inline std::string env_or(std::string_view key, std::string def = "")
    {
        if (const char *v = std::getenv(std::string(key).c_str()))
            return std::string(v);
        return def;
    }

    inline bool env_bool(std::string_view key, bool def = false)
    {
        auto v = env_or(key, def ? "1" : "0");
        for (auto &c : v)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return (v == "1" || v == "true" || v == "yes" || v == "on");
    }

    inline int env_int(std::string_view key, int def = 0)
    {
        auto v = env_or(key, "");
        if (v.empty())
            return def;
        try
        {
            return std::stoi(v);
        }
        catch (...)
        {
            return def;
        }
    }

}

#endif

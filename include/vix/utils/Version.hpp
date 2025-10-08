#ifndef VIX_VERSION_HPP
#define VIX_VERSION_HPP

#include <string>
#include <string_view>

namespace Vix::utils
{
    /// Retourne la version courante de Vix.cpp (ex: "0.2.0")
    [[nodiscard]] constexpr std::string_view version() noexcept
    {
        return "0.2.0";
    }

    /// Retourne un numéro de build complet incluant hash git, si dispo
    std::string build_info();
}

#endif // VIX_VERSION_HPP

/**
 * @file Env.hpp
 * @brief Deprecated Vix2 environment-helper compatibility surface.
 *
 * Canonical environment ownership is vix::env. These wrappers preserve the
 * historical vix::utils call surface without retaining lookup or parsing
 * policy in utils.
 */
#ifndef VIX_UTILS_ENV_HPP
#define VIX_UTILS_ENV_HPP

#include <string>
#include <string_view>

#include <vix/env/Legacy.hpp>

namespace vix::utils
{

  [[nodiscard]] inline const char *vix_getenv(const char *name) noexcept
  {
    return vix::env::legacy::getenv(name);
  }

  [[nodiscard]] inline std::string env_or(std::string_view key,
                                          std::string_view fallback = "")
  {
    return vix::env::legacy::env_or(key, fallback);
  }

  [[nodiscard]] inline bool env_bool(std::string_view key, bool fallback = false)
  {
    return vix::env::legacy::env_bool(key, fallback);
  }

  [[nodiscard]] inline int env_int(std::string_view key, int fallback = 0)
  {
    return vix::env::legacy::env_int(key, fallback);
  }

  [[nodiscard]] inline unsigned env_uint(std::string_view key,
                                         unsigned fallback = 0u)
  {
    return vix::env::legacy::env_uint(key, fallback);
  }

  [[nodiscard]] inline double env_double(std::string_view key,
                                         double fallback = 0.0)
  {
    return vix::env::legacy::env_double(key, fallback);
  }

} // namespace vix::utils

#endif // VIX_UTILS_ENV_HPP

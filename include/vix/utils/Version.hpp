#ifndef VIX_VERSION_HPP
#define VIX_VERSION_HPP

#include <string>
#include <string_view>

/**
 * @file VIX_VERSION_HPP
 * @brief Version and build information utilities for Vix.cpp.
 *
 * Provides compile-time access to the framework version and
 * runtime build metadata (Git hash, date, etc.).
 *
 * This header allows users and maintainers to embed precise
 * version information into logs, HTTP responses, or CLI outputs.
 *
 * ### Features
 * - Compile-time version constant (`version()`)
 * - Runtime build string with Git hash and build date (`build_info()`)
 *
 * @note The build metadata is injected via CMake using definitions like:
 * @code
 * add_compile_definitions(VIX_GIT_HASH="abcdef1" VIX_BUILD_DATE="Oct 10 2025 11:42:00")
 * @endcode
 *
 * ### Example
 * @code
 * #include <vix/utils/Version.hpp>
 * using namespace Vix::utils;
 *
 * std::cout << "Vix.cpp version: " << version() << std::endl;
 * std::cout << "Build info: " << build_info() << std::endl;
 * // Output: v0.2.0 (abcdef1, Oct 10 2025 11:42:00)
 * @endcode
 */

namespace vix::utils
{
  /**
   * @brief Get the current Vix.cpp version.
   *
   * Returned as a compile-time constant (e.g., `"0.2.0"`).
   * This version is updated manually in the codebase or via
   * release automation.
   *
   * @return Version string view (semantic version format).
   */
  [[nodiscard]] constexpr std::string_view version() noexcept
  {
    return "0.2.0";
  }

  /**
   * @brief Get the detailed build info (version + Git hash + build date).
   *
   * This combines the static version with optional build metadata injected
   * at compile time via preprocessor macros:
   * - `VIX_GIT_HASH`
   * - `VIX_BUILD_DATE`
   *
   * If unavailable, defaults to `"unknown"` and the current compile time.
   *
   * @return A formatted string like `"v0.2.0 (abcdef1, Oct 10 2025 11:42:00)"`.
   *
   * @see version()
   */
  std::string build_info();
} // namespace vix::utils

#endif // VIX_VERSION_HPP

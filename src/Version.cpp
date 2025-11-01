#include <vix/utils/Version.hpp>

#ifndef VIX_GIT_HASH
#define VIX_GIT_HASH "unknown"
#endif

#ifndef VIX_BUILD_DATE
#define VIX_BUILD_DATE __DATE__ " " __TIME__
#endif

namespace vix::utils
{
    /**
     * @brief Compose full build information string.
     *
     * The resulting format is:
     * ```
     * v<version> (<git-hash>, <build-date>)
     * ```
     *
     * For example:
     * ```
     * v0.2.0 (abcdef1, Oct 10 2025 11:42:00)
     * ```
     *
     * @return Full build descriptor for logging or CLI display.
     *
     * @code
     * std::cout << build_info();
     * // v0.2.0 (abcdef1, Oct 10 2025 11:42:00)
     * @endcode
     */
    std::string build_info()
    {
        return std::string("v") + std::string(version()) +
               " (" + VIX_GIT_HASH + ", " + VIX_BUILD_DATE + ")";
    }
}

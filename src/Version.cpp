#include "vix/utils/Version.hpp"

#ifndef VIX_GIT_HASH
#define VIX_GIT_HASH "unknown"
#endif

#ifndef VIX_BUILD_DATE
#define VIX_BUILD_DATE __DATE__ " " __TIME__
#endif

namespace Vix::utils
{
    std::string build_info()
    {
        // format simple : "v0.2.0 (abcdef1, Oct 08 2025 11:42:00)"
        return std::string("v") + std::string(version()) +
               " (" + VIX_GIT_HASH + ", " + VIX_BUILD_DATE + ")";
    }
}

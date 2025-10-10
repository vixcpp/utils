/**
 * @file utils_demo.cpp
 * @brief Example usage of Vix.cpp utility functions.
 *
 * Demonstrates how to use the core utility headers from `vix/utils`,
 * including environment handling, time/date formatting, UUID generation,
 * and build metadata.
 *
 * ### Modules demonstrated
 * - **Env.hpp** — Environment variable helpers
 * - **Time.hpp** — ISO-8601, RFC-1123, and timestamp utilities
 * - **UUID.hpp** — RFC 4122–compliant UUIDv4 generation
 * - **Version.hpp** — Framework version and build information
 *
 * ### Example output
 * ```
 * version=0.2.0
 * build_info=v0.2.0 (abcdef1, Oct 10 2025 11:42:00)
 * APP_ENV=dev
 * APP_DEBUG=false
 * APP_PORT=8080
 * iso8601_now=2025-10-10T18:32:25Z
 * rfc1123_now=Fri, 10 Oct 2025 18:32:25 GMT
 * unix_ms=1733884345123
 * now_ms=123456789
 * uuid4=550e8400-e29b-41d4-a716-446655440000
 * ```
 *
 * @note All functions are header-only except `build_info()`, which links to
 *       `src/Version.cpp`.
 *
 * @see Vix::utils::env_or
 * @see Vix::utils::env_bool
 * @see Vix::utils::env_int
 * @see Vix::utils::iso8601_now
 * @see Vix::utils::rfc1123_now
 * @see Vix::utils::uuid4
 * @see Vix::utils::build_info
 */

#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Version.hpp>
#include <iostream>

using namespace Vix::utils;

/**
 * @brief Demonstrates the use of Vix utility modules in a single executable.
 *
 * Shows how to:
 * - Retrieve environment variables with defaults
 * - Interpret boolean and integer values from the environment
 * - Fetch formatted timestamps
 * - Generate unique UUIDv4 identifiers
 * - Display framework version and build metadata
 *
 * @return int Always returns 0.
 */
int main()
{
    // --- Version information ---
    std::cout << "version=" << version() << "\n";
    std::cout << "build_info=" << build_info() << "\n";

    // --- Environment variables ---
    std::cout << "APP_ENV=" << env_or("APP_ENV", "dev") << "\n";
    std::cout << "APP_DEBUG=" << (env_bool("APP_DEBUG", false) ? "true" : "false") << "\n";
    std::cout << "APP_PORT=" << env_int("APP_PORT", 8080) << "\n";

    // --- Time utilities ---
    std::cout << "iso8601_now=" << iso8601_now() << "\n";
    std::cout << "rfc1123_now=" << rfc1123_now() << "\n";
    std::cout << "unix_ms=" << unix_ms() << "\n"; // epoch ms (persistable)
    std::cout << "now_ms=" << now_ms() << "\n";   // steady_clock (durations)

    // --- UUID generation ---
    std::cout << "uuid4=" << uuid4() << "\n";

    return 0;
}

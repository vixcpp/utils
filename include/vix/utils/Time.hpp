/**
 *
 *  @file Time.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#ifndef VIX_UTILS_TIME_HPP
#define VIX_UTILS_TIME_HPP

#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdint>

/**
 * @brief Time and date utilities (UTC, ISO-8601, RFC-1123, monotonic, UNIX ms).
 *
 * Provides safe and portable utilities for formatting and retrieving
 * timestamps in UTC, as well as high-resolution monotonic clocks for measuring
 * durations. All functions are `noexcept` and header-only.
 *
 * ### Features
 * - Thread-safe UTC time extraction (`gmtime_r` / `gmtime_s`)
 * - ISO-8601 and RFC-1123 formatted timestamps
 * - Monotonic millisecond timer (`steady_clock`)
 * - UNIX epoch milliseconds (`system_clock`)
 *
 * @note Designed for logging, metrics, and timestamp generation.
 * @note No dynamic allocations except for string formatting.
 *
 * ### Example
 * @code
 * using namespace Vix::utils;
 *
 * std::string iso = iso8601_now();  // "2025-10-10T16:03:20Z"
 * std::string rfc = rfc1123_now();  // "Fri, 10 Oct 2025 16:03:20 GMT"
 *
 * auto start = now_ms();
 * heavy_task();
 * auto elapsed = now_ms() - start;
 * std::cout << "Task took " << elapsed << " ms\n";
 *
 * std::uint64_t ts = unix_ms(); // e.g., 1733881600000
 * @endcode
 */

namespace vix::utils
{
  /**
   * @brief Convert a `time_t` value to UTC (`std::tm`), thread-safe on all major platforms.
   *
   * On POSIX systems uses `gmtime_r`, on Windows `gmtime_s`.
   * Falls back to non-thread-safe `std::gmtime()` on exotic platforms.
   *
   * @param t Time in seconds since the UNIX epoch.
   * @return `std::tm` structure in UTC.
   */
  inline std::tm utc_tm(std::time_t t) noexcept
  {
    std::tm tm{};
#if defined(_WIN32)
    ::gmtime_s(&tm, &t);
#elif defined(__unix__) || defined(__APPLE__)
    ::gmtime_r(&t, &tm);
#else
    // Fallback (non-thread-safe)
    tm = *std::gmtime(&t);
#endif
    return tm;
  }

  /**
   * @brief Get the current UTC time in ISO-8601 format.
   *
   * Format: `"YYYY-MM-DDTHH:MM:SSZ"`
   *
   * @return Current UTC timestamp as an ISO-8601 string.
   *
   * @code
   * std::string ts = iso8601_now();
   * // Example: "2025-10-10T14:07:12Z"
   * @endcode
   */
  inline std::string iso8601_now() noexcept
  {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const time_t t = clock::to_time_t(now);
    const std::tm tm = utc_tm(t);

    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
  }

  /**
   * @brief Get the current UTC time in RFC-1123 format (used in HTTP headers).
   *
   * Format: `"Wed, 08 Oct 2025 14:07:12 GMT"`
   *
   * @return Current UTC timestamp as an RFC-1123 string.
   *
   * @code
   * std::string header = "Date: " + rfc1123_now();
   * // Example: "Date: Wed, 08 Oct 2025 14:07:12 GMT"
   * @endcode
   */
  inline std::string rfc1123_now() noexcept
  {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const time_t t = clock::to_time_t(now);
    const std::tm tm = utc_tm(t);

    std::ostringstream os;
    os << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
    return os.str();
  }

  /**
   * @brief Get the current monotonic timestamp in milliseconds.
   *
   * Uses `std::chrono::steady_clock`, which is **monotonic** and immune to system clock changes.
   *
   * @return Milliseconds from an arbitrary starting point.
   *
   * @code
   * auto t1 = now_ms();
   * do_something();
   * auto elapsed = now_ms() - t1;
   * @endcode
   */
  inline std::uint64_t now_ms() noexcept
  {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
  }

  /**
   * @brief Get the current UNIX time in milliseconds since epoch (UTC).
   *
   * Uses `std::chrono::system_clock` and is suitable for persisted timestamps,
   * event logs, or distributed system synchronization.
   *
   * @return UNIX epoch timestamp in milliseconds.
   *
   * @code
   * std::uint64_t ts = unix_ms(); // e.g., 1733862450000
   * @endcode
   */
  inline std::uint64_t unix_ms() noexcept
  {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
  }

} // namespace vix::utils

#endif // VIX_TIME_HPP

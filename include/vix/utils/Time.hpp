#ifndef VIX_TIME_HPP
#define VIX_TIME_HPP

#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdint>

namespace Vix::utils
{
    // -- -helpers : get UTC tm in a threadsafe way-- -
    inline std::tm utc_tm(std::time_t t) noexcept
    {
        std::tm tm{};
#if defined(_WIN32)
        ::gmtime_s(&tm, &t);
#elif defined(__unix__) || defined(__APPLE__)
        ::gmtime_r(&t, &tm);
#else
        // fallback non thread-safe si plateforme exotique
        tm = *std::gmtime(&t);
#endif
        return tm;
    }

    // ISO-8601 instant en UTC: "YYYY-MM-DDTHH:MM:SSZ"
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

    // RFC 1123 (HTTP-date), ex: "Wed, 08 Oct 2025 14:07:12 GMT"
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

    // Monotonic clock (for measuring durations) — ms from an arbitrary point
    inline std::uint64_t now_ms() noexcept
    {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(
            duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

    // UNIX time in ms (UTC) — use for persisted timestamps
    inline std::uint64_t unix_ms() noexcept
    {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(
            duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    }
}

#endif // VIX_TIME_HPP

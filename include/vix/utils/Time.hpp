#ifndef VIX_TIME_HPP
#define VIX_TIME_HPP

#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Vix::utils
{
    inline std::string iso8601_now()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto t = system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_r(&t, &tm);
        std::ostringstream os;
        os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return os.str();
    }

    inline uint64_t now_ms()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
}

#endif
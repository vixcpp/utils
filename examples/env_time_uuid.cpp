#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Version.hpp>
#include <iostream>

using namespace Vix::utils;

int main()
{
    std::cout << "version=" << version() << "\n";
    std::cout << "build_info=" << build_info() << "\n";

    std::cout << "APP_ENV=" << env_or("APP_ENV", "dev") << "\n";
    std::cout << "APP_DEBUG=" << (env_bool("APP_DEBUG", false) ? "true" : "false") << "\n";
    std::cout << "APP_PORT=" << env_int("APP_PORT", 8080) << "\n";

    std::cout << "iso8601_now=" << iso8601_now() << "\n";
    std::cout << "rfc1123_now=" << rfc1123_now() << "\n";
    std::cout << "unix_ms=" << unix_ms() << "\n"; // epoch ms (persistable)
    std::cout << "now_ms=" << now_ms() << "\n";   // steady_clock (durations)

    std::cout << "uuid4=" << uuid4() << "\n";
    return 0;
}

#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/UUID.hpp>
#include <iostream>

using namespace Vix::utils;

int main()
{
    std::cout << "APP_ENV=" << env_or("APP_ENV", "dev") << "\n";
    std::cout << "iso8601_now=" << iso8601_now() << "\n";
    std::cout << "uuid4=" << uuid4() << "\n";
    std::cout << "now_ms=" << now_ms() << "\n";
    return 0;
}

#include <vix/utils/Validation.hpp>
#include <vix/utils/Logger.hpp>
#include <iostream>
#include <unordered_map>

using namespace Vix::utils;
using Vix::Logger;

int main()
{
    std::unordered_map<std::string, std::string> data{
        {"name", "Gaspard"},
        {"age", "18"},
        {"email", "softadastra@example.com"}};

    Schema sch{
        {"name", required("Name")},
        {"age", num_range(1, 150, "Age")},
        {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Email")}};

    auto r = validate_map(data, sch);
    auto &log = Logger::getInstance();

    if (r.is_ok())
    {
        log.log(Logger::Level::INFO, "Validation OK");
    }
    else
    {
        for (auto &[k, msg] : r.error())
        {
            log.log(Logger::Level::ERROR, "{} -> {}", k, msg);
        }
        return 1;
    }
    return 0;
}

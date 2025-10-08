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

    auto &log = Logger::getInstance();
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);

    const auto r = validate_map(data, sch);

    if (r.is_ok())
    {
        log.log(Logger::Level::INFO, "Validation OK");
        return 0;
    }

    log.log(Logger::Level::ERROR, "Validation FAILED:");
    for (const auto &kv : r.error())
    {
        log.log(Logger::Level::ERROR, " - {} -> {}", kv.first, kv.second);
    }
    return 1;
}

/**
 * @file validation_demo.cpp
 * @brief Example of using `Vix::utils::validate_map()` with `Logger`.
 *
 * Demonstrates schema-based input validation using the `Vix::utils::Validation`
 * module, along with structured logging via `Vix::Logger`.
 *
 * ### Modules demonstrated
 * - **Validation.hpp** — Declarative field validation with `Schema`
 * - **Logger.hpp** — Thread-safe logging with configurable patterns and levels
 *
 * ### Example output
 * ```
 * [2025-10-10 19:04:12.512] [info] Validation OK
 * ```
 *
 * Or, if validation fails:
 * ```
 * [2025-10-10 19:04:12.512] [error] Validation FAILED:
 * [2025-10-10 19:04:12.512] [error]  - email -> Email has invalid format
 * ```
 *
 * ### How it works
 * 1. Define input data as a `std::unordered_map<std::string, std::string>`
 * 2. Define a validation `Schema` with field rules
 * 3. Call `validate_map(data, schema)`
 * 4. Inspect the returned `Result<void, FieldErrors>`
 * 5. Log results with `Logger`
 *
 * @see Vix::utils::validate_map
 * @see Vix::utils::Schema
 * @see Vix::utils::Rule
 * @see Vix::Logger
 */

#include <vix/utils/Validation.hpp>
#include <vix/utils/Logger.hpp>
#include <iostream>
#include <unordered_map>

using namespace vix::utils;
using vix::Logger;

/**
 * @brief Demonstrates form-style validation using the `Validation` utility.
 *
 * This program validates a set of string fields (`name`, `age`, `email`)
 * against a schema that defines type, format, and range constraints.
 *
 * The results are logged via `Vix::Logger` with formatted, colored output.
 *
 * @return int Returns `0` on success, `1` on validation error.
 */
int main()
{
    // --------------------------------------------------------
    // Input data (simulating form submission)
    // --------------------------------------------------------
    std::unordered_map<std::string, std::string> data{
        {"name", "Gaspard"},
        {"age", "18"},
        {"email", "softadastra@example.com"}};

    // --------------------------------------------------------
    // Schema definition — declarative validation rules
    // --------------------------------------------------------
    Schema sch{
        {"name", required("Name")},
        {"age", num_range(1, 150, "Age")},
        {"email", match(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)", "Email")}};

    // --------------------------------------------------------
    // Logger setup (pattern & level)
    // --------------------------------------------------------
    auto &log = Logger::getInstance();
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);

    // --------------------------------------------------------
    // Validation process
    // --------------------------------------------------------
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

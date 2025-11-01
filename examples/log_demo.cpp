/**
 * @file log_demo.cpp
 * @brief Example of using the `Vix::Logger` utility with environment-based configuration.
 *
 * Demonstrates how to configure, contextualize, and use the Vix logging system,
 * including asynchronous mode, dynamic log levels via environment variables,
 * and structured contextual fields (e.g., request IDs and module metadata).
 *
 * ### Modules demonstrated
 * - **Logger.hpp** — Advanced, thread-safe logger with async and formatting support
 * - **Env.hpp** — Environment helpers for runtime configuration
 * - **UUID.hpp** — UUIDv4 generator for request correlation
 *
 * ### Example output
 * ```
 * [2025-10-10 19:45:12.891] [info] Hello from utils/log_demo
 * [2025-10-10 19:45:12.891] [debug] Debug enabled = true
 * [2025-10-10 19:45:12.892] [info] Boot args | port=8080 async=true
 * [2025-10-10 19:45:12.892] [warn] This is a warning
 * ```
 *
 * ### Supported environment variables
 * | Variable          | Type | Default | Description |
 * |-------------------|-------|----------|--------------|
 * | `VIX_LOG_ASYNC`   | bool  | `true`   | Enables async mode (non-blocking logs). |
 * | `VIX_LOG_DEBUG`   | bool  | `false`  | Enables debug-level logs. |
 * | `APP_ENV`         | str   | `"dev"`  | Application environment name. |
 * | `APP_PORT`        | int   | `8080`   | Example port number for structured logs. |
 *
 * ### Example
 * ```bash
 * export VIX_LOG_DEBUG=1
 * export APP_ENV=prod
 * ./log_demo
 * ```
 *
 * @see Vix::Logger
 * @see Vix::Logger::Context
 * @see Vix::utils::env_bool
 * @see Vix::utils::env_or
 * @see Vix::utils::uuid4
 */

#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <vix/utils/Env.hpp>

using vix::Logger;
using namespace vix::utils;

/**
 * @brief Demonstrates structured logging and environment-driven configuration.
 *
 * This example:
 *  - Initializes the global `Logger`
 *  - Reads log options from environment variables
 *  - Sets context fields (request ID, module, environment)
 *  - Logs various message levels and structured values
 *
 * @return int Always returns 0.
 */
int main()
{
    // --------------------------------------------------------
    // Logger instance setup
    // --------------------------------------------------------
    auto &log = Logger::getInstance();

    // Pattern example: [2025-10-10 19:45:12.891] [info] message
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // --------------------------------------------------------
    // Dynamic configuration from environment variables
    // --------------------------------------------------------
    const bool async_mode = env_bool("VIX_LOG_ASYNC", true);
    log.setAsync(async_mode);
    log.setLevel(env_bool("VIX_LOG_DEBUG", false)
                     ? Logger::Level::DEBUG
                     : Logger::Level::INFO);

    // --------------------------------------------------------
    // Contextual metadata (useful for distributed tracing)
    // --------------------------------------------------------
    Logger::Context cx;
    cx.request_id = uuid4();
    cx.module = "log_demo";
    cx.fields["service"] = "utils";
    cx.fields["env"] = env_or("APP_ENV", "dev");
    log.setContext(cx);

    // --------------------------------------------------------
    // Log examples
    // --------------------------------------------------------
    log.log(Logger::Level::INFO, "Hello from utils/log_demo");
    log.log(Logger::Level::DEBUG, "Debug enabled = {}", env_bool("VIX_LOG_DEBUG", false));

    // Structured key-value logging
    log.logf(Logger::Level::INFO, "Boot args", "port", env_int("APP_PORT", 8080), "async", async_mode);

    log.log(Logger::Level::WARN, "This is a warning");

    // Uncomment for error test
    // log.throwError("Demo error: {}", "something went wrong");

    return 0;
}

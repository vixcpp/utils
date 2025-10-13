/**
 * @file Logger.cpp
 * @brief Minimalist, colorized logger for the Vix CLI — no timestamps, no noise.
 *
 * ## Overview
 * This implementation provides a clean, developer-friendly console logger for
 * command-line tools and framework messages. It is designed for clarity and
 * simplicity in terminal output, without timestamps, thread IDs, or extra prefixes.
 *
 * Example output:
 * ```
 * [vix][info] Project built successfully
 * [vix][warn] Missing CMakeLists.txt in this directory
 * [vix][error] Failed to configure CMake (code 1)
 * ```
 *
 * ## Key Characteristics
 * - **Console-only** logging (no file rotation).
 * - **Color-coded levels**: info, warn, error, critical.
 * - **No timestamps or metadata** — clean for CLI readability.
 * - Thread-safe and singleton-based (process-wide).
 * - Compatible with the full `Logger` interface (`log`, `logModule`, `logf`, etc.).
 *
 * ## Pattern
 * Uses the spdlog pattern:
 * ```
 * [vix][%^%l%$] %v
 * ```
 * where:
 *  - `[vix]` is a static tag for all Vix outputs.
 *  - `%l` is the log level (INFO/WARN/ERROR/...).
 *  - `%^` and `%$` apply color formatting.
 *  - `%v` is the user message.
 *
 * ## Thread Context
 * The thread-local `Context` (request_id, module, and custom fields) is
 * preserved and accessible but not automatically shown unless used with `logf()`.
 *
 * @note This variant is tailored for the Vix CLI and developer utilities.
 *       For server or application use, a richer logger (with timestamps or files)
 *       should be configured instead.
 */

#include "vix/utils/Logger.hpp"
#include <spdlog/pattern_formatter.h>

namespace Vix
{
    // ----------------------------------------------------------------------
    // Thread-local context instance
    // ----------------------------------------------------------------------
    thread_local Logger::Context Logger::tls_ctx_{};

    // ----------------------------------------------------------------------
    // Constructor — initialize minimal, colorized CLI logger
    // ----------------------------------------------------------------------
    Logger::Logger() : spd_(nullptr), mutex_()
    {
        try
        {
            // 1) Colored console sink only (no file sink)
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);

            // 2) Create the main logger instance
            spd_ = std::make_shared<spdlog::logger>("vix", console_sink);
            spd_->set_level(spdlog::level::info);
            spd_->flush_on(spdlog::level::warn);

            // 3) Apply simple colorized CLI pattern
            // Example:
            //   [vix][info] Starting build...
            //   [vix][warn] Missing optional dependency
            //   [vix][error] Failed to initialize database
            spd_->set_pattern("[vix][%^%l%$] %v");

            // 4) Set as the global default logger (optional)
            spdlog::set_default_logger(spd_);
        }
        catch (const spdlog::spdlog_ex &ex)
        {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    // ----------------------------------------------------------------------
    // setPattern — allows runtime override of log pattern
    // ----------------------------------------------------------------------
    void Logger::setPattern(const std::string &pattern)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (spd_)
            spd_->set_pattern(pattern);
    }

    // ----------------------------------------------------------------------
    // setAsync — intentionally disabled for CLI usage
    // ----------------------------------------------------------------------
    void Logger::setAsync(bool enable)
    {
        // CLI logging is synchronous for immediate feedback.
        (void)enable;
    }

    // ----------------------------------------------------------------------
    // Context management (thread-local)
    // ----------------------------------------------------------------------
    void Logger::setContext(Context ctx)
    {
        tls_ctx_ = std::move(ctx);
    }

    void Logger::clearContext()
    {
        tls_ctx_ = Logger::Context{};
    }

    Logger::Context Logger::getContext() const
    {
        return tls_ctx_;
    }
}

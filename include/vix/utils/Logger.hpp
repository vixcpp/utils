#ifndef VIX_LOGGER_HPP
#define VIX_LOGGER_HPP

/**
 * @file Logger.hpp
 * @brief Thin, opinionated wrapper around spdlog with context and KV logging.
 *
 * @details
 * `Vix::Logger` centralizes application logging and exposes:
 *  - A simple `Level` enum compatible with `spdlog::level`.
 *  - Format-string logging (`log()`), plus `logModule()` and structured `logf()`
 *    (key/value pairs).
 *  - A thread-local **Context** (request id, module, free-form fields) that is
 *    appended automatically to each `logf()` call.
 *  - Convenience error helpers `throwError(...)` that also log before throwing.
 *  - Runtime configuration for **pattern** and **async/sync** mode.
 *
 * Typical usage
 * ```cpp
 * auto &log = Vix::Logger::getInstance();
 * log.setLevel(Vix::Logger::Level::INFO);
 * log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
 * log.setAsync(true);
 *
 * Vix::Logger::Context ctx; ctx.request_id = "r-123"; ctx.module = "auth";
 * log.setContext(ctx);
 *
 * log.log(Vix::Logger::Level::INFO, "User {} logged in", user);
 * log.logf(Vix::Logger::Level::INFO, "Login ok", "user", user.c_str(), "latency_ms", 12);
 * ```
 */

#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <utility>
#include <stdexcept>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>

#if defined(SPDLOG_FMT_EXTERNAL)
#include <fmt/format.h>
#else
#include <spdlog/fmt/bundled/format.h>
#endif

namespace Vix
{
    /**
     * @class Logger
     * @brief Process-wide logger facade with thread-local context.
     *
     * Thread-safety: all configuration and logging calls lock an internal
     * mutex around the underlying `spdlog::logger`.
     */
    class Logger
    {
    public:
        /** @brief Logging severity levels. */
        enum class Level
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            CRITICAL
        };

        /**
         * @brief Retrieve the process-wide singleton instance.
         */
        static Logger &getInstance()
        {
            static Logger instance;
            return instance;
        }

        /**
         * @brief Set the minimum severity level for output.
         */
        void setLevel(Level level)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (spd_)
                spd_->set_level(toSpdLevel(level));
        }

        /**
         * @brief Log a formatted message at the given level.
         * @tparam Args Format arguments compatible with `fmt`.
         * @param level Severity.
         * @param fmtstr fmtlib format string.
         */
        template <typename... Args>
        void log(Level level, fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!spd_)
                return;
            switch (level)
            {
            case Level::TRACE:
                spd_->trace(fmtstr, std::forward<Args>(args)...);
                break;
            case Level::DEBUG:
                spd_->debug(fmtstr, std::forward<Args>(args)...);
                break;
            case Level::INFO:
                spd_->info(fmtstr, std::forward<Args>(args)...);
                break;
            case Level::WARN:
                spd_->warn(fmtstr, std::forward<Args>(args)...);
                break;
            case Level::ERROR:
                spd_->error(fmtstr, std::forward<Args>(args)...);
                break;
            case Level::CRITICAL:
                spd_->critical(fmtstr, std::forward<Args>(args)...);
                break;
            }
        }

        /**
         * @brief Log with a module tag prefix, e.g. "[auth] ...".
         */
        template <typename... Args>
        void logModule(const std::string &module, Level level,
                       fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(level, "[{}] {}", module, fmt::format(fmtstr, std::forward<Args>(args)...));
        }

        /**
         * @brief Log an error and throw a std::runtime_error (formatted).
         * @note Always `[[noreturn]]`—use for fatal configuration errors.
         */
        template <typename... Args>
        [[noreturn]] void throwError(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            auto msg = fmt::format(fmtstr, std::forward<Args>(args)...);
            log(Level::ERROR, "{}", msg);
            throw std::runtime_error(msg);
        }

        /** @overload */
        [[noreturn]] void throwError(const std::string &msg)
        {
            log(Level::ERROR, "{}", msg);
            throw std::runtime_error(msg);
        }

        /**
         * @brief Per-thread logging context automatically appended by `logf()`.
         */
        struct Context
        {
            std::string request_id;                    //!< Optional correlation ID
            std::string module;                        //!< Optional module tag
            std::map<std::string, std::string> fields; //!< Arbitrary KV metadata

            Context() : request_id(), module(), fields() {}
        };

        /**
         * @brief Configure output pattern (delegates to spdlog::set_pattern()).
         * @param pattern e.g. "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v"
         */
        void setPattern(const std::string &pattern);

        /**
         * @brief Enable or disable asynchronous logging.
         * @details When enabled, switches the underlying logger to an async mode
         *          configured in the corresponding .cpp (queue size, thread, etc.).
         */
        void setAsync(bool enable);

        /** @brief Set the thread-local logging context. */
        void setContext(Context ctx);
        /** @brief Clear the current thread-local context. */
        void clearContext();
        /** @brief Retrieve a copy of the current thread-local context. */
        Context getContext() const;

        /**
         * @brief Structured logging with key/value pairs.
         *
         * Appends `k=v` pairs to the message and includes context fields
         * (`rid`, `mod`, and custom fields). Example:
         * ```cpp
         * log.logf(Logger::Level::INFO, "HTTP request", "method", "GET", "latency_ms", 12);
         * ```
         */
        template <typename... Args>
        void logf(Level level, const std::string &msg, Args &&...kvpairs)
        {
            std::string line = msg;
            appendKV(line, std::forward<Args>(kvpairs)...);
            auto c = ctx();
            if (!c.request_id.empty())
                line += " rid=" + c.request_id;
            if (!c.module.empty())
                line += " mod=" + c.module;
            for (auto &[k, v] : c.fields)
                line += " " + k + "=" + v;
            log(level, "{}", line);
        }

    private:
        Logger();

        static spdlog::level::level_enum toSpdLevel(Level level) noexcept
        {
            switch (level)
            {
            case Level::TRACE:
                return spdlog::level::trace;
            case Level::DEBUG:
                return spdlog::level::debug;
            case Level::INFO:
                return spdlog::level::info;
            case Level::WARN:
                return spdlog::level::warn;
            case Level::ERROR:
                return spdlog::level::err;
            case Level::CRITICAL:
                return spdlog::level::critical;
            default:
                return spdlog::level::info;
            }
        }

        std::shared_ptr<spdlog::logger> spd_;
        std::mutex mutex_;

        // Thread-local context accessed without locking
        static thread_local Context tls_ctx_;
        const Context &ctx() const noexcept { return tls_ctx_; }

        // Compile-time KV appender (variadic, even number of args expected)
        static void appendKV(std::string &) {}
        template <typename V, typename... Rest>
        static void appendKV(std::string &s, const char *k, V &&v, Rest &&...rest)
        {
            s += " ";
            s += k;
            s += "=";
            s += fmt::format("{}", std::forward<V>(v));
            if constexpr (sizeof...(rest) > 0)
                appendKV(s, std::forward<Rest>(rest)...);
        }
    };
}

#endif // VIX_LOGGER_HPP

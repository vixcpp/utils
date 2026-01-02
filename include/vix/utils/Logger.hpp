#ifndef VIX_LOGGER_HPP
#define VIX_LOGGER_HPP

/**
 * @file Logger.hpp
 * @brief Thin, opinionated wrapper around spdlog with per-thread context and KV logging.
 *
 * - Default output: **console only** (no log file is created).
 * - Default level: **INFO** (override with `VIX_LOG_LEVEL=trace|debug|info|warn|error|critical`).
 * - Default pattern (runtime-like): `HH:MM:SS [level] message`
 *
 * Typical usage
 * ```cpp
 * auto &log = vix::utils::Logger::getInstance();
 *
 * // Optional: set explicit level (otherwise uses VIX_LOG_LEVEL or defaults to INFO)
 * log.setLevel(vix::utils::Logger::Level::INFO);
 *
 * // Optional: override console pattern
 * // Example: compact + colored level
 * log.setPattern("%T [%^%l%$] %v");
 *
 * // Optional: async mode
 * log.setAsync(true);
 *
 * // Per-thread context (request scoped)
 * vix::utils::Logger::Context ctx;
 * ctx.request_id = "r-123";
 * ctx.module = "auth";
 * ctx.fields["ip"] = "127.0.0.1";
 * log.setContext(ctx);
 *
 * log.info("User {} logged in", user);
 * log.logf(vix::utils::Logger::Level::INFO, "Login ok",
 *          "user", user.c_str(),
 *          "latency_ms", 12);
 * ```
 */

#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <utility>
#include <stdexcept>
#include <iostream>
#include <type_traits>
#include <string_view>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>

#include <vix/utils/ConsoleMutex.hpp>

#if defined(SPDLOG_FMT_EXTERNAL)
#include <fmt/format.h>
#else
#include <spdlog/fmt/bundled/format.h>
#endif

namespace vix::utils
{
    class Logger
    {
    public:
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;
        Logger(Logger &&) = delete;
        Logger &operator=(Logger &&) = delete;

        enum class Level
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            CRITICAL
        };

        enum class Format
        {
            KV,
            JSON,
            JSON_PRETTY
        };

        static Logger &getInstance()
        {
            static Logger instance;
            return instance;
        }

        void setLevel(Level level)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (spd_)
                spd_->set_level(toSpdLevel(level));
        }

        void setFormat(Format f);
        void setFormatFromEnv(std::string_view envName = "VIX_LOG_FORMAT");
        static Format parseFormat(std::string_view s);

        template <typename... Args>
        void trace(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::TRACE, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void debug(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::DEBUG, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::INFO, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warn(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::WARN, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::ERROR, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(Level::CRITICAL, fmtstr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void log(Level level, fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!spd_)
                return;

            vix::utils::console_wait_banner();

            std::lock_guard<std::mutex> lk(vix::utils::console_mutex());

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

        template <typename... Args>
        void logModule(const std::string &module, Level level,
                       fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(level, "[{}] {}", module, fmt::format(fmtstr, std::forward<Args>(args)...));
        }

        template <typename... Args>
        [[noreturn]] void throwError(fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            auto msg = fmt::format(fmtstr, std::forward<Args>(args)...);
            log(Level::ERROR, "{}", msg);
            throw std::runtime_error(msg);
        }

        [[noreturn]] void throwError(const std::string &msg)
        {
            log(Level::ERROR, "{}", msg);
            throw std::runtime_error(msg);
        }

        struct Context
        {
            std::string request_id;
            std::string module;
            std::map<std::string, std::string> fields;

            Context() : request_id(), module(), fields() {}
        };

        void setPattern(const std::string &pattern);
        void setAsync(bool enable);

        void setContext(Context ctx);
        void clearContext();
        Context getContext() const;

        template <typename... Args>
        void logf(Level level, const std::string &msg, Args &&...kvpairs)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!spd_)
                return;

            vix::utils::console_wait_banner();

            std::lock_guard<std::mutex> lk(vix::utils::console_mutex());

            if (format_ == Format::JSON_PRETTY)
            {
                const std::string json = buildJsonPretty(level, msg, std::forward<Args>(kvpairs)...);
                spd_->log(toSpdLevel(level), "{}", json);
                return;
            }

            if (format_ == Format::JSON)
            {
                const std::string json = buildJsonLine(level, msg, std::forward<Args>(kvpairs)...);
                spd_->log(toSpdLevel(level), "{}", json);
                return;
            }

            std::string line = msg;
            appendKV(line, std::forward<Args>(kvpairs)...);

            const auto &c = ctx();
            if (!c.request_id.empty())
                line += " rid=" + c.request_id;
            if (!c.module.empty())
                line += " mod=" + c.module;
            for (const auto &it : c.fields)
                line += " " + it.first + "=" + it.second;

            spd_->log(toSpdLevel(level), "{}", line);
        }

        void setLevelFromEnv(std::string_view envName = "VIX_LOG_LEVEL");
        static Level parseLevel(std::string_view s);
        static Level parseLevelFromEnv(std::string_view envName = "VIX_LOG_LEVEL", Level fallback = Level::WARN);

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
        Format format_ = Format::KV;

        static thread_local Context tls_ctx_;
        const Context &ctx() const noexcept { return tls_ctx_; }

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

        static void appendJsonEscaped(std::string &out, std::string_view s)
        {
            for (const char ch : s)
            {
                switch (ch)
                {
                case '\"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\b':
                    out += "\\b";
                    break;
                case '\f':
                    out += "\\f";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20)
                    {
                        out += "\\u00";
                        static constexpr char hex[] = "0123456789abcdef";
                        const unsigned char uc = static_cast<unsigned char>(ch);
                        out.push_back(hex[(uc >> 4) & 0x0F]);
                        out.push_back(hex[uc & 0x0F]);
                    }
                    else
                    {
                        out.push_back(ch);
                    }
                    break;
                }
            }
        }

        static void appendJsonKey(std::string &out, std::string_view key)
        {
            out += "\"";
            appendJsonEscaped(out, key);
            out += "\":";
        }

        static void appendJsonStringValue(std::string &out, std::string_view value)
        {
            out += "\"";
            appendJsonEscaped(out, value);
            out += "\"";
        }

        template <typename V>
        static void appendJsonValue(std::string &out, V &&v)
        {
            using T = std::remove_cv_t<std::remove_reference_t<V>>;

            if constexpr (std::is_same_v<T, bool>)
            {
                out += (v ? "true" : "false");
            }
            else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
            {
                out += fmt::format("{}", v);
            }
            else if constexpr (std::is_same_v<T, std::string> ||
                               std::is_same_v<T, std::string_view>)
            {
                appendJsonStringValue(out, std::string_view(v));
            }
            else if constexpr (std::is_same_v<T, const char *> ||
                               std::is_same_v<T, char *>)
            {
                appendJsonStringValue(out, v ? std::string_view(v) : std::string_view(""));
            }
            else
            {
                appendJsonStringValue(out, fmt::format("{}", std::forward<V>(v)));
            }
        }

        static std::string levelToString(Level level)
        {
            switch (level)
            {
            case Level::TRACE:
                return "trace";
            case Level::DEBUG:
                return "debug";
            case Level::INFO:
                return "info";
            case Level::WARN:
                return "warn";
            case Level::ERROR:
                return "error";
            case Level::CRITICAL:
                return "critical";
            default:
                return "info";
            }
        }

        template <typename... Args>
        std::string buildJsonLine(Level level, std::string_view msg, Args &&...kvpairs) const
        {
            std::string out;
            out.reserve(256);
            out += "{";

            appendJsonKey(out, "level");
            appendJsonStringValue(out, levelToString(level));
            out += ",";

            appendJsonKey(out, "msg");
            appendJsonStringValue(out, msg);

            const auto &c = ctx();
            if (!c.request_id.empty())
            {
                out += ",";
                appendJsonKey(out, "rid");
                appendJsonStringValue(out, c.request_id);
            }
            if (!c.module.empty())
            {
                out += ",";
                appendJsonKey(out, "mod");
                appendJsonStringValue(out, c.module);
            }
            for (const auto &it : c.fields)
            {
                out += ",";
                appendJsonKey(out, it.first);
                appendJsonStringValue(out, it.second);
            }

            appendJsonKV(out, std::forward<Args>(kvpairs)...);

            out += "}";
            return out;
        }

        template <typename V>
        static void appendJsonPrettyValue(std::string &out, V &&v)
        {
            using T = std::remove_cv_t<std::remove_reference_t<V>>;

            if constexpr (std::is_same_v<T, bool>)
            {
                out += (v ? "true" : "false");
            }
            else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
            {
                out += fmt::format("{}", v);
            }
            else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
            {
                out += "\"";
                appendJsonEscaped(out, std::string_view(v));
                out += "\"";
            }
            else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
            {
                out += "\"";
                appendJsonEscaped(out, v ? std::string_view(v) : std::string_view(""));
                out += "\"";
            }
            else
            {
                out += "\"";
                appendJsonEscaped(out, fmt::format("{}", std::forward<V>(v)));
                out += "\"";
            }
        }

        static void appendJsonPrettyKV(std::string &) {}

        template <typename V, typename... Rest>
        static void appendJsonPrettyKV(std::string &out, const char *k, V &&v, Rest &&...rest)
        {
            out += "  \"";
            appendJsonEscaped(out, k);
            out += "\": ";
            appendJsonPrettyValue(out, std::forward<V>(v));
            out += ",\n";

            if constexpr (sizeof...(rest) > 0)
                appendJsonPrettyKV(out, std::forward<Rest>(rest)...);
        }

        template <typename... Args>
        std::string buildJsonPretty(Level level, std::string_view msg, Args &&...kvpairs) const
        {
            std::string out;
            out.reserve(512);
            out += "{\n";

            auto add_str = [&](std::string_view k, std::string_view v)
            {
                out += "  \"";
                appendJsonEscaped(out, k);
                out += "\": \"";
                appendJsonEscaped(out, v);
                out += "\",\n";
            };

            add_str("level", levelToString(level));
            add_str("msg", msg);

            const auto &c = ctx();
            if (!c.request_id.empty())
                add_str("rid", c.request_id);
            if (!c.module.empty())
                add_str("mod", c.module);

            for (const auto &it : c.fields)
                add_str(it.first, it.second);

            appendJsonPrettyKV(out, std::forward<Args>(kvpairs)...);

            // remove trailing ",\n" if present
            if (out.size() >= 2 && out.ends_with(",\n"))
            {
                out.pop_back(); // \n
                out.pop_back(); // ,
                out += "\n";
            }

            out += "}";
            return out;
        }

        template <typename V, typename... Rest>
        static void appendJsonKV(std::string &out, const char *k, V &&v, Rest &&...rest)
        {
            out += ",";
            appendJsonKey(out, k);
            appendJsonValue(out, std::forward<V>(v));

            if constexpr (sizeof...(rest) > 0)
                appendJsonKV(out, std::forward<Rest>(rest)...);
        }
    };
}

#endif // VIX_LOGGER_HPP

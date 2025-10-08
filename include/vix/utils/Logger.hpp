#ifndef VIX_LOGGER_HPP
#define VIX_LOGGER_HPP

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

namespace Vix
{
    class Logger
    {
    public:
        enum class Level
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            CRITICAL
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

        template <typename... Args>
        void logModule(const std::string &module, Level level, fmt::format_string<Args...> fmtstr, Args &&...args)
        {
            log(level, "[{}] {}", module, fmt::vformat(fmtstr, fmt::make_format_args(std::forward<Args>(args)...)));
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

            Context()
                : request_id(), module(), fields()
            {
            }
        };

        void setPattern(const std::string &pattern);
        void setAsync(bool enable);
        void setContext(Context ctx);
        void clearContext();
        Context getContext() const;

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
    };
}

#endif

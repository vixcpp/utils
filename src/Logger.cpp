#include <vix/utils/Logger.hpp>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>

#include <cstdlib>     // std::getenv
#include <cctype>      // std::tolower
#include <string>      // std::string
#include <string_view> // std::string_view

namespace vix::utils
{
    thread_local Logger::Context Logger::tls_ctx_{};

    static std::string lower_copy(std::string_view in)
    {
        std::string out;
        out.reserve(in.size());

        for (char ch : in)
        {
            const auto uc = static_cast<unsigned char>(ch);
            out.push_back(static_cast<char>(std::tolower(uc)));
        }

        return out;
    }

    Logger::Level Logger::parseLevel(std::string_view s)
    {
        const auto v = lower_copy(s);

        if (v == "trace")
            return Level::TRACE;
        if (v == "debug")
            return Level::DEBUG;
        if (v == "info")
            return Level::INFO;
        if (v == "warn" || v == "warning")
            return Level::WARN;
        if (v == "error")
            return Level::ERROR;
        if (v == "critical" || v == "fatal")
            return Level::CRITICAL;

        return Level::WARN;
    }

    Logger::Level Logger::parseLevelFromEnv(std::string_view envName, Level fallback)
    {
        const std::string key(envName);
        const char *raw = std::getenv(key.c_str());
        if (!raw || !*raw)
            return fallback;
        return parseLevel(raw);
    }

    void Logger::setLevelFromEnv(std::string_view envName)
    {
        setLevel(parseLevelFromEnv(envName, Level::INFO));
    }

    Logger::Logger() : spd_(nullptr), mutex_()
    {
        try
        {
            // Console only (no vix.log)
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_color_mode(spdlog::color_mode::always);

            // "runtime-like" look (readable, compact):
            // 15:36:36 [info] message
            // 15:36:36 [warn] message
            //
            // %T = HH:MM:SS
            // %^%$ = level color
            // %l = level (info/warn/error)
            console_sink->set_pattern("\033[90m%T [vix]\033[0m [%^%l%$] \033[2m%v\033[0m");

            spd_ = std::make_shared<spdlog::logger>(
                "vix",
                spdlog::sinks_init_list{console_sink});
            // Default INFO (better UX), override with env VIX_LOG_LEVEL
            auto lvl = toSpdLevel(parseLevelFromEnv("VIX_LOG_LEVEL", Level::INFO));
            spd_->set_level(lvl);
            // flush on warn+ (keep it snappy)
            spd_->flush_on(spdlog::level::warn);
            spdlog::set_default_logger(spd_);
        }
        catch (const spdlog::spdlog_ex &ex)
        {
            std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        }
    }

    void Logger::setPattern(const std::string &pattern)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!spd_)
            return;

        for (auto &sink : spd_->sinks())
            sink->set_pattern(pattern);
    }

    void Logger::setAsync(bool enable)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!spd_)
            return;

        try
        {
            // Keep same sinks and levels when switching
            auto sinks = spd_->sinks();
            auto lvl = spd_->level();
            auto flush = spd_->flush_level();

            if (enable)
            {
                if (!spdlog::thread_pool())
                    spdlog::init_thread_pool(8192, 1);

                auto async_logger = std::make_shared<spdlog::async_logger>(
                    "vix",
                    sinks.begin(),
                    sinks.end(),
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::block);

                async_logger->set_level(lvl);
                async_logger->flush_on(flush);

                spd_ = async_logger;
                spdlog::set_default_logger(spd_);
                spd_->debug("Logger switched to async mode");
            }
            else
            {
                auto sync_logger = std::make_shared<spdlog::logger>(
                    "vix",
                    sinks.begin(),
                    sinks.end());

                sync_logger->set_level(lvl);
                sync_logger->flush_on(flush);

                spd_ = sync_logger;
                spdlog::set_default_logger(spd_);
                spd_->debug("Logger switched to sync mode");
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[Logger::setAsync] Failed to toggle mode: " << e.what() << std::endl;
        }
    }

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

} // namespace vix::utils

#include "vix/utils/Logger.hpp"
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>

namespace Vix
{
    thread_local Logger::Context Logger::tls_ctx_{};

    Logger::Logger()
        : spd_(nullptr), mutex_()
    {
        try
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "vix.log", 1024 * 1024 * 5, 3);
            file_sink->set_level(spdlog::level::trace);

            spd_ = std::make_shared<spdlog::logger>(
                "vixLogger", spdlog::sinks_init_list{console_sink, file_sink});
            spd_->set_level(spdlog::level::info);
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
        if (spd_)
            spd_->set_pattern(pattern);
    }

    void Logger::setAsync(bool enable)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!spd_)
            return;

        try
        {
            if (enable)
            {
                // Switch to async mode
                if (!spdlog::thread_pool())
                    spdlog::init_thread_pool(8192, 1);

                auto sinks = spd_->sinks();

                auto async_logger = std::make_shared<spdlog::async_logger>(
                    "vixLoggerAsync",
                    sinks.begin(),
                    sinks.end(),
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::block);

                async_logger->set_level(spd_->level());
                async_logger->flush_on(spd_->flush_level());
                async_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

                spd_ = async_logger;
                spdlog::set_default_logger(spd_);
                spd_->info("Logger switched to asynchronous mode");
            }
            else
            {
                // Switch back to sync mode
                auto sinks = spd_->sinks();

                auto sync_logger = std::make_shared<spdlog::logger>(
                    "vixLogger",
                    sinks.begin(),
                    sinks.end());

                sync_logger->set_level(spd_->level());
                sync_logger->flush_on(spd_->flush_level());
                sync_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

                spd_ = sync_logger;
                spdlog::set_default_logger(spd_);
                spd_->info("Logger switched to synchronous mode");
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
}

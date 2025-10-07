#include "vix/utils/Logger.hpp"
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>

namespace Vix
{
    thread_local Logger::Context Logger::tls_ctx_{};

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
                // =========================
                // Switching to asynchronous mode
                // =========================

                // Initializes a global thread pool for spdlog (one time only)
                if (!spdlog::thread_pool())
                    spdlog::init_thread_pool(8192, 1);

                auto sinks = spd_->sinks();

                // Creates an asynchronous logger with the same sinks
                auto async_logger = std::make_shared<spdlog::async_logger>(
                    "vixLoggerAsync",
                    sinks.begin(),
                    sinks.end(),
                    spdlog::thread_pool(),
                    spdlog::async_overflow_policy::block);

                // Copies the flush level and behavior
                async_logger->set_level(spd_->level());
                async_logger->flush_on(spd_->flush_level());

                async_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

                spd_ = async_logger;
                spdlog::set_default_logger(spd_);

                spd_->info("Logger switched to asynchronous mode");
            }
            else
            {
                // =========================
                // Return to synchronous mode
                // =========================

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

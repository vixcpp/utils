#include <vix/utils/Logger.hpp>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/ansicolor_sink.h>

namespace vix::utils
{
    thread_local Logger::Context Logger::tls_ctx_{};

    Logger::Logger() : spd_(nullptr), mutex_()
    {
        try
        {
            // Console sink — pour l'affichage "runtime"
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);

            // 🔴 TRÈS IMPORTANT : forcer la couleur même si stdout n'est pas un TTY
            console_sink->set_color_mode(spdlog::color_mode::always);

            // Console : pattern compact, lisible
            console_sink->set_pattern("[%^%L%$] %v"); // [I] message, [E] message, etc.

            // File sink — pour logs détaillés
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "vix.log", 1024 * 1024 * 5, 3);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

            spd_ = std::make_shared<spdlog::logger>(
                "vixLogger",
                spdlog::sinks_init_list{console_sink, file_sink});

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
        if (!spd_)
            return;

        // Applique le pattern à toutes les sinks (console + file)
        for (auto &sink : spd_->sinks())
        {
            sink->set_pattern(pattern);
        }
    }

    void Logger::setAsync(bool enable)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!spd_)
            return;

        try
        {
            auto sinks = spd_->sinks();
            auto lvl = spd_->level();
            auto flush = spd_->flush_level();

            if (enable)
            {
                if (!spdlog::thread_pool())
                    spdlog::init_thread_pool(8192, 1);

                auto async_logger = std::make_shared<spdlog::async_logger>(
                    "vixLoggerAsync",
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
                    "vixLogger",
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
            std::cerr << "[Logger::setAsync] Failed to toggle mode: "
                      << e.what() << std::endl;
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

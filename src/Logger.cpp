/**
 *
 *  @file Logger.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#include <vix/utils/Logger.hpp>

#include <vix/utils/ConsoleMutex.hpp>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cctype>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace vix::utils
{
  struct Logger::Impl
  {
    std::shared_ptr<spdlog::logger> spd;
    mutable std::mutex mutex;
    Format format{Format::KV};
  };

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

    if (v == "off" || v == "never" || v == "none" || v == "silent" || v == "0")
      return Level::Off;

    if (v == "trace")
      return Level::Trace;
    if (v == "debug")
      return Level::Debug;
    if (v == "info")
      return Level::Info;
    if (v == "warn" || v == "warning")
      return Level::Warn;
    if (v == "error")
      return Level::Error;
    if (v == "critical" || v == "fatal")
      return Level::Critical;

    return Level::Warn;
  }

  Logger::Level Logger::parseLevelFromEnv(std::string_view envName, Level fallback)
  {
    const std::string key(envName);
    const char *raw = vix::utils::vix_getenv(key.c_str());
    if (!raw || !*raw)
      return fallback;
    return parseLevel(raw);
  }

  void Logger::setLevelFromEnv(std::string_view envName)
  {
    setLevel(parseLevelFromEnv(envName, Level::Info));
  }

  int Logger::toSpdLevel(Level level) noexcept
  {
    switch (level) { case Level::Trace: return static_cast<int>(spdlog::level::trace); case Level::Debug: return static_cast<int>(spdlog::level::debug); case Level::Info: return static_cast<int>(spdlog::level::info); case Level::Warn: return static_cast<int>(spdlog::level::warn); case Level::Error: return static_cast<int>(spdlog::level::err); case Level::Critical: return static_cast<int>(spdlog::level::critical); case Level::Off: return static_cast<int>(spdlog::level::off); }
    return static_cast<int>(spdlog::level::info);
  }

  Logger::Logger() : impl_(std::make_unique<Impl>())
  {
    try
    {
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(spdlog::level::trace);
      console_sink->set_color_mode(spdlog::color_mode::always);

      // %T = HH:MM:SS
      // %^%$ = level color
      // %l = level (info/warn/error)
      console_sink->set_pattern("\033[90m%T [vix]\033[0m [%^%l%$] \033[2m%v\033[0m");

      impl_->spd = std::make_shared<spdlog::logger>(
          "vix",
          spdlog::sinks_init_list{console_sink});
      // Default INFO, override with env VIX_LOG_LEVEL
      auto lvl = static_cast<spdlog::level::level_enum>(toSpdLevel(parseLevelFromEnv("VIX_LOG_LEVEL", Level::Info)));
      impl_->spd->set_level(lvl);
      setFormatFromEnv("VIX_LOG_FORMAT");
      // flush on warn+ (keep it snappy)
      impl_->spd->flush_on(spdlog::level::warn);
      spdlog::set_default_logger(impl_->spd);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
      std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
  }

  Logger::~Logger() = default;

  void Logger::setLevel(Level level)
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->spd) impl_->spd->set_level(static_cast<spdlog::level::level_enum>(toSpdLevel(level)));
  }

  Logger::Format Logger::format() const noexcept
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->format;
  }

  bool Logger::enabled(Level level) const noexcept
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->spd && impl_->spd->should_log(
        static_cast<spdlog::level::level_enum>(toSpdLevel(level)));
  }

  Logger::Level Logger::level() const noexcept
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->spd) return Level::Off;
    switch (impl_->spd->level())
    {
    case spdlog::level::trace: return Level::Trace;
    case spdlog::level::debug: return Level::Debug;
    case spdlog::level::info: return Level::Info;
    case spdlog::level::warn: return Level::Warn;
    case spdlog::level::err: return Level::Error;
    case spdlog::level::critical: return Level::Critical;
    default: return Level::Off;
    }
  }

  void Logger::emit(Level level, std::string_view message)
  {
    const Format output_format = format();
    if (output_format == Format::JSON_PRETTY)
    {
      emitPrepared(level, buildJsonPretty(level, message));
      return;
    }
    if (output_format == Format::JSON)
    {
      emitPrepared(level, buildJsonLine(level, message));
      return;
    }
    emitPrepared(level, message);
  }

  void Logger::emitPrepared(Level level, std::string_view message)
  {
    std::shared_ptr<spdlog::logger> logger;
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      logger = impl_->spd;
    }
    if (!logger || !logger->should_log(static_cast<spdlog::level::level_enum>(toSpdLevel(level)))) return;

    if (console_sync_enabled())
    {
      vix::utils::console_wait_banner();
      std::lock_guard<std::mutex> lock(vix::utils::console_mutex());
      logger->log(static_cast<spdlog::level::level_enum>(toSpdLevel(level)), "{}", message);
      return;
    }
    logger->log(static_cast<spdlog::level::level_enum>(toSpdLevel(level)), "{}", message);
  }

  void Logger::setPattern(const std::string &pattern)
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->spd)
      return;

    for (auto &sink : impl_->spd->sinks())
      sink->set_pattern(pattern);
  }

  void Logger::setAsync(bool enable)
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->spd)
      return;

    try
    {
      auto sinks = impl_->spd->sinks();
      auto lvl = impl_->spd->level();
      auto flush = impl_->spd->flush_level();

      if (enable)
      {
        if (!spdlog::thread_pool())
          spdlog::init_thread_pool(262144, 1);

        auto async_logger = std::make_shared<spdlog::async_logger>(
            "vix",
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);

        async_logger->set_level(lvl);
        async_logger->flush_on(flush);

        impl_->spd = async_logger;
        spdlog::set_default_logger(impl_->spd);
      }
      else
      {
        auto sync_logger = std::make_shared<spdlog::logger>(
            "vix",
            sinks.begin(),
            sinks.end());

        sync_logger->set_level(lvl);
        sync_logger->flush_on(flush);

        impl_->spd = sync_logger;
        spdlog::set_default_logger(impl_->spd);
        impl_->spd->debug("Logger switched to sync mode");
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

  Logger::Format Logger::parseFormat(std::string_view s)
  {
    const auto v = lower_copy(s);
    if (v == "json")
      return Format::JSON;
    if (v == "json-pretty" || v == "pretty-json" || v == "json_pretty")
      return Format::JSON_PRETTY;
    return Format::KV;
  }

  void Logger::setFormat(Format f)
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->format = f;

    if (!impl_->spd)
      return;

    if (impl_->format == Format::JSON || impl_->format == Format::JSON_PRETTY)
    {
      for (auto &sink : impl_->spd->sinks())
        sink->set_pattern("%v");

      impl_->spd->flush_on(spdlog::level::info);
      return;
    }

    for (auto &sink : impl_->spd->sinks())
      sink->set_pattern("\033[90m%T [vix]\033[0m [%^%l%$] \033[2m%v\033[0m");

    impl_->spd->flush_on(spdlog::level::warn);
  }

  void Logger::setFormatFromEnv(std::string_view envName)
  {
    const std::string key(envName);
    const char *raw = vix::utils::vix_getenv(key.c_str());
    if (!raw || !*raw)
      return;
    setFormat(parseFormat(raw));
  }

  bool Logger::jsonColorsEnabled()
  {
    if (const char *nc = vix::utils::vix_getenv("NO_COLOR"); nc && *nc)
      return false;

    if (const char *c = vix::utils::vix_getenv("VIX_COLOR"); c && *c)
    {
      const std::string v = lower_copy(c);

      if (v == "never" || v == "0" || v == "false")
        return false;

      if (v == "always" || v == "1" || v == "true")
        return true;
    }

#ifndef _WIN32
    return ::isatty(STDOUT_FILENO) == 1;
#else
    return true;
#endif
  }

} // namespace vix::utils

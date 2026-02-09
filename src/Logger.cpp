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

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>

#include <cctype>
#include <string>
#include <string_view>
#ifndef _WIN32
#include <unistd.h>
#endif

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

  Logger::Logger() : spd_(nullptr), mutex_()
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

      spd_ = std::make_shared<spdlog::logger>(
          "vix",
          spdlog::sinks_init_list{console_sink});
      // Default INFO, override with env VIX_LOG_LEVEL
      auto lvl = toSpdLevel(parseLevelFromEnv("VIX_LOG_LEVEL", Level::Info));
      spd_->set_level(lvl);
      setFormatFromEnv("VIX_LOG_FORMAT");
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
      auto sinks = spd_->sinks();
      auto lvl = spd_->level();
      auto flush = spd_->flush_level();

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

        spd_ = async_logger;
        spdlog::set_default_logger(spd_);
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
    std::lock_guard<std::mutex> lock(mutex_);
    format_ = f;

    if (!spd_)
      return;

    if (format_ == Format::JSON || format_ == Format::JSON_PRETTY)
    {
      for (auto &sink : spd_->sinks())
        sink->set_pattern("%v");

      spd_->flush_on(spdlog::level::info);
      return;
    }

    for (auto &sink : spd_->sinks())
      sink->set_pattern("\033[90m%T [vix]\033[0m [%^%l%$] \033[2m%v\033[0m");

    spd_->flush_on(spdlog::level::warn);
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

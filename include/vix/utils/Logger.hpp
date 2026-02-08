/**
 *
 *  @file Logger.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#ifndef VIX_UTILS_LOGGER_HPP
#define VIX_UTILS_LOGGER_HPP

/**
 * @brief Thin, opinionated wrapper around spdlog with per-thread context and KV logging.
 *
 * - Default output: console only (no log file is created).
 * - Default level: INFO (override with VIX_LOG_LEVEL=trace|debug|info|warn|error|critical).
 * - Default pattern (runtime-like): HH:MM:SS [level] message
 *
 * Typical usage
 * @code{.cpp}
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
 * @endcode
 */

#if defined(_WIN32)
// Windows headers sometimes define macros like ERROR/DEBUG/min/max.
// They break enum values or common identifiers.

#if defined(ERROR)
#pragma push_macro("ERROR")
#undef ERROR
#define VIX_UTILS_RESTORE_ERROR_MACRO 1
#endif

#if defined(DEBUG)
#pragma push_macro("DEBUG")
#undef DEBUG
#define VIX_UTILS_RESTORE_DEBUG_MACRO 1
#endif

#if defined(min)
#pragma push_macro("min")
#undef min
#define VIX_UTILS_RESTORE_MIN_MACRO 1
#endif

#if defined(max)
#pragma push_macro("max")
#undef max
#define VIX_UTILS_RESTORE_MAX_MACRO 1
#endif

#endif

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <cstdlib>

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <vix/utils/ConsoleMutex.hpp>

#if defined(SPDLOG_FMT_EXTERNAL)
#include <fmt/format.h>
#else
#include <spdlog/fmt/bundled/format.h>
#endif

namespace vix::utils
{
  /**
   * @brief Central logging facility for Vix.
   *
   * Logger wraps spdlog and adds:
   * - singleton lifetime management
   * - runtime configuration via env vars
   * - per-thread context (request id, module, custom fields)
   * - key/value logging in KV or JSON (single line or pretty)
   * - optional console synchronization (to avoid interleaving with banners)
   *
   * The logger is designed to be safe in multi-threaded applications and
   * to keep output deterministic and easy to parse.
   */
  class Logger
  {
  public:
    /**
     * @brief Delete copy operations.
     */
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Delete move operations.
     */
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    /**
     * @brief Logging severity level.
     */
    enum class Level
    {
      TRACE,
      DEBUG,
      INFO,
      WARN,
      ERROR,
      CRITICAL,
      OFF
    };

    /**
     * @brief Output format for structured logging.
     */
    enum class Format
    {
      /**
       * @brief Key-value pairs appended to the message (text).
       */
      KV,

      /**
       * @brief JSON formatted as a single line.
       */
      JSON,

      /**
       * @brief Pretty JSON with indentation (optionally colored).
       */
      JSON_PRETTY
    };

    /**
     * @brief Get the global Logger instance.
     *
     * @return Singleton logger instance.
     */
    static Logger &getInstance()
    {
      static Logger instance;
      return instance;
    }

    /**
     * @brief Set the active logging level.
     *
     * @param level New logging level.
     */
    void setLevel(Level level)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (spd_)
        spd_->set_level(toSpdLevel(level));
    }

    /**
     * @brief Set the structured output format.
     *
     * @param f Desired output format.
     */
    void setFormat(Format f);

    /**
     * @brief Read the structured output format from an environment variable.
     *
     * @param envName Name of the environment variable (default: VIX_LOG_FORMAT).
     */
    void setFormatFromEnv(std::string_view envName = "VIX_LOG_FORMAT");

    /**
     * @brief Parse a format string into Format.
     *
     * @param s Format name (kv|json|json_pretty).
     * @return Parsed format.
     */
    static Format parseFormat(std::string_view s);

    /**
     * @brief Log a TRACE message.
     */
    template <typename... Args>
    void trace(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::TRACE, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a DEBUG message.
     */
    template <typename... Args>
    void debug(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::DEBUG, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log an INFO message.
     */
    template <typename... Args>
    void info(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::INFO, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a WARN message.
     */
    template <typename... Args>
    void warn(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::WARN, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log an ERROR message.
     */
    template <typename... Args>
    void error(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::ERROR, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a CRITICAL message.
     */
    template <typename... Args>
    void critical(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      log(Level::CRITICAL, fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a formatted message at the given level.
     *
     * This function checks the logger level and only formats/logs if enabled.
     *
     * @param level Log level.
     * @param fmtstr Format string.
     * @param args Format arguments.
     */
    template <typename... Args>
    void log(Level level, fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      if (level == Level::OFF)
        return;

      std::shared_ptr<spdlog::logger> spd;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        spd = spd_;
        if (!spd)
          return;
        if (!spd->should_log(toSpdLevel(level)))
          return;
      }

      if (console_sync_enabled())
      {
        vix::utils::console_wait_banner();
        std::lock_guard<std::mutex> lk(vix::utils::console_mutex());
        spd->log(toSpdLevel(level), fmtstr, std::forward<Args>(args)...);
        return;
      }

      spd->log(toSpdLevel(level), fmtstr, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a formatted message with an explicit module prefix.
     *
     * The module prefix is prepended as: [module] message
     *
     * @param module Module name.
     * @param level Log level.
     * @param fmtstr Format string.
     * @param args Format arguments.
     */
    template <typename... Args>
    void logModule(std::string_view module,
                   Level level,
                   fmt::format_string<Args...> fmtstr,
                   Args &&...args)
    {
      if (level == Level::OFF)
        return;

      std::shared_ptr<spdlog::logger> spd;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        spd = spd_;
        if (!spd)
          return;
        if (!spd->should_log(toSpdLevel(level)))
          return;
      }

      fmt::memory_buffer buf;
      fmt::format_to(std::back_inserter(buf), "[{}] ", module);
      fmt::format_to(std::back_inserter(buf), fmtstr, std::forward<Args>(args)...);

      if (console_sync_enabled())
      {
        vix::utils::console_wait_banner();
        std::lock_guard<std::mutex> lk(vix::utils::console_mutex());
        spd->log(toSpdLevel(level), "{}", fmt::to_string(buf));
        return;
      }

      spd->log(toSpdLevel(level), "{}", fmt::to_string(buf));
    }

    /**
     * @brief Format, log as ERROR, then throw std::runtime_error.
     */
    template <typename... Args>
    [[noreturn]] void throwError(fmt::format_string<Args...> fmtstr, Args &&...args)
    {
      auto msg = fmt::format(fmtstr, std::forward<Args>(args)...);
      log(Level::ERROR, "{}", msg);
      throw std::runtime_error(msg);
    }

    /**
     * @brief Log as ERROR, then throw std::runtime_error.
     */
    [[noreturn]] void throwError(const std::string &msg)
    {
      log(Level::ERROR, "{}", msg);
      throw std::runtime_error(msg);
    }

    /**
     * @brief Per-thread logging context.
     *
     * Stored in thread-local storage and appended to KV/JSON logs.
     */
    struct Context
    {
      /**
       * @brief Request identifier for correlation.
       */
      std::string request_id;

      /**
       * @brief Logical module name.
       */
      std::string module;

      /**
       * @brief Arbitrary fields appended to logs (e.g. ip, user_id).
       */
      std::unordered_map<std::string, std::string> fields;

      Context() : request_id(), module(), fields() {}
    };

    /**
     * @brief Set the spdlog pattern for console output.
     *
     * @param pattern spdlog pattern string.
     */
    void setPattern(const std::string &pattern);

    /**
     * @brief Enable or disable async logging mode.
     *
     * @param enable True to enable async mode.
     */
    void setAsync(bool enable);

    /**
     * @brief Replace the current thread context.
     *
     * @param ctx New context.
     */
    void setContext(Context ctx);

    /**
     * @brief Clear the current thread context.
     */
    void clearContext();

    /**
     * @brief Get a copy of the current thread context.
     */
    Context getContext() const;

    /**
     * @brief Log a message with key/value pairs.
     *
     * The kvpairs arguments are expected as alternating key/value pairs:
     *   logf(INFO, "msg", "k1", v1, "k2", v2, ...).
     *
     * Depending on the configured format, output is KV, JSON, or pretty JSON.
     *
     * @param level Log level.
     * @param msg Base message.
     * @param kvpairs Key/value pairs.
     */
    template <typename... Args>
    void logf(Level level, const std::string &msg, Args &&...kvpairs)
    {
      if (level == Level::OFF)
        return;

      std::lock_guard<std::mutex> lock(mutex_);
      if (!spd_ || !spd_->should_log(toSpdLevel(level)))
        return;

      if (console_sync_enabled())
      {
        vix::utils::console_wait_banner();
        std::lock_guard<std::mutex> lk(vix::utils::console_mutex());
        (void)lk;
      }

      if (format_ == Format::JSON_PRETTY)
      {
        spd_->log(toSpdLevel(level), "{}", buildJsonPretty(level, msg, std::forward<Args>(kvpairs)...));
        return;
      }
      if (format_ == Format::JSON)
      {
        spd_->log(toSpdLevel(level), "{}", buildJsonLine(level, msg, std::forward<Args>(kvpairs)...));
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

    /**
     * @brief Set the logging level from an environment variable.
     *
     * @param envName Name of the environment variable (default: VIX_LOG_LEVEL).
     */
    void setLevelFromEnv(std::string_view envName = "VIX_LOG_LEVEL");

    /**
     * @brief Parse a level string into Level.
     */
    static Level parseLevel(std::string_view s);

    /**
     * @brief Parse a level from an environment variable.
     *
     * @param envName Environment variable name.
     * @param fallback Fallback level if not set or invalid.
     */
    static Level parseLevelFromEnv(
        std::string_view envName = "VIX_LOG_LEVEL",
        Level fallback = Level::WARN);

    /**
     * @brief Whether pretty JSON output should include ANSI colors.
     */
    static bool jsonColorsEnabled();

    /**
     * @brief Return the current spdlog level mapped to Logger::Level.
     */
    Level level() const noexcept
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!spd_)
        return Level::OFF;

      const auto lvl = spd_->level();
      if (lvl == spdlog::level::trace)
        return Level::TRACE;
      if (lvl == spdlog::level::debug)
        return Level::DEBUG;
      if (lvl == spdlog::level::info)
        return Level::INFO;
      if (lvl == spdlog::level::warn)
        return Level::WARN;
      if (lvl == spdlog::level::err)
        return Level::ERROR;
      if (lvl == spdlog::level::critical)
        return Level::CRITICAL;
      return Level::OFF;
    }

    /**
     * @brief Check whether the given level is enabled.
     */
    bool enabled(Level lvl) const noexcept
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!spd_)
        return false;
      return spd_->should_log(toSpdLevel(lvl));
    }

  private:
    /**
     * @brief Private constructor (singleton).
     */
    Logger();

    /**
     * @brief Convert Logger::Level to spdlog level.
     */
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
      case Level::OFF:
        return spdlog::level::off;
      default:
        return spdlog::level::info;
      }
    }

    std::shared_ptr<spdlog::logger> spd_;
    mutable std::mutex mutex_;
    Format format_ = Format::KV;

    /**
     * @brief Thread-local context for the current thread.
     */
    static thread_local Context tls_ctx_;

    /**
     * @brief Access the thread-local context.
     */
    const Context &ctx() const noexcept { return tls_ctx_; }

    /**
     * @brief Append key/value pairs to a string in KV form.
     */
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

    /**
     * @brief Append a string to JSON with proper escaping.
     */
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

    /**
     * @brief Append a JSON key (with quotes and escaping) and a colon.
     */
    static void appendJsonKey(std::string &out, std::string_view key)
    {
      out += "\"";
      appendJsonEscaped(out, key);
      out += "\":";
    }

    /**
     * @brief Append a JSON string value (with quotes and escaping).
     */
    static void appendJsonStringValue(std::string &out, std::string_view value)
    {
      out += "\"";
      appendJsonEscaped(out, value);
      out += "\"";
    }

    /**
     * @brief Append a JSON value of various supported types.
     *
     * Numbers and booleans are emitted as JSON primitives. Other values
     * are formatted as strings.
     */
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

    /**
     * @brief Convert Level to a lowercase string (for JSON output).
     */
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
      case Level::OFF:
        return "off";
      default:
        return "info";
      }
    }

    /**
     * @brief Build a single-line JSON log entry.
     */
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

    /**
     * @brief Append a value to pretty JSON output.
     */
    template <typename V>
    static void appendJsonPrettyValue(std::string &out, V &&v, bool color)
    {
      using T = std::remove_cv_t<std::remove_reference_t<V>>;

      if constexpr (std::is_same_v<T, bool>)
      {
        const char *s = v ? "true" : "false";
        out += c_bool(s, color);
      }
      else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
      {
        out += c_num(fmt::format("{}", v), color);
      }
      else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
      {
        std::string tmp;
        tmp.reserve(std::string_view(v).size() + 2);
        tmp += "\"";
        std::string escaped;
        escaped.reserve(std::string_view(v).size() + 8);
        appendJsonEscaped(escaped, std::string_view(v));
        tmp += escaped;
        tmp += "\"";

        out += c_str(tmp, color);
      }
      else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
      {
        std::string_view sv = v ? std::string_view(v) : std::string_view("");
        std::string escaped;
        escaped.reserve(sv.size() + 8);
        appendJsonEscaped(escaped, sv);

        std::string tmp;
        tmp.reserve(escaped.size() + 2);
        tmp += "\"";
        tmp += escaped;
        tmp += "\"";

        out += c_str(tmp, color);
      }
      else
      {
        const std::string s = fmt::format("{}", std::forward<V>(v));
        std::string escaped;
        escaped.reserve(s.size() + 8);
        appendJsonEscaped(escaped, s);

        std::string tmp;
        tmp.reserve(escaped.size() + 2);
        tmp += "\"";
        tmp += escaped;
        tmp += "\"";

        out += c_str(tmp, color);
      }
    }

    /**
     * @brief Helpers for ANSI colored output (used by pretty JSON).
     */
    static std::string c_red(std::string_view s, bool on) { return ansi_wrap("\033[31m", s, on); }
    static std::string c_blue(std::string_view s, bool on) { return ansi_wrap("\033[34m", s, on); }
    static std::string c_dim(std::string_view s, bool on) { return ansi_wrap("\033[2m", s, on); }

    /**
     * @brief Colorize HTTP status codes in pretty JSON.
     */
    template <typename Int>
    static std::string c_http_status(Int code, bool on)
    {
      const int c = static_cast<int>(code);
      const std::string s = fmt::format("{}", c);

      if (!on)
        return s;

      if (c >= 200 && c < 300)
        return ansi_wrap("\033[32m", s, true);
      if (c >= 300 && c < 400)
        return ansi_wrap("\033[36m", s, true);
      if (c >= 400 && c < 500)
        return ansi_wrap("\033[33m", s, true);
      if (c >= 500 && c < 600)
        return ansi_wrap("\033[31m", s, true);
      return ansi_wrap("\033[90m", s, true);
    }

    /**
     * @brief Check if a string ends with a suffix.
     */
    static bool ends_with(std::string_view s, std::string_view suf)
    {
      return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
    }

    /**
     * @brief Append pretty JSON key/value pairs.
     */
    static void appendJsonPrettyKV(std::string &, bool) {}

    template <typename V, typename... Rest>
    static void appendJsonPrettyKV(std::string &out, bool color, const char *k, V &&v, Rest &&...rest)
    {
      out += "  ";
      out += c_key(std::string("\"") + k + "\"", color);
      out += c_punc(": ", color);

      using T = std::remove_cv_t<std::remove_reference_t<V>>;

      if constexpr (std::is_same_v<T, bool>)
      {
        const char *s = v ? "true" : "false";
        out += c_bool(s, color);
      }
      else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
      {
        if (std::string_view(k) == "status")
        {
          out += c_http_status(v, color);
        }
        else if (std::string_view(k) == "duration_ms" || ends_with(k, "_ms"))
        {
          out += c_dim(c_blue(fmt::format("{}", v), color), color);
        }
        else
        {
          out += c_num(fmt::format("{}", v), color);
        }
      }
      else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
      {
        std::string escaped;
        escaped.reserve(std::string_view(v).size() + 8);
        appendJsonEscaped(escaped, std::string_view(v));

        std::string tmp;
        tmp.reserve(escaped.size() + 2);
        tmp += "\"";
        tmp += escaped;
        tmp += "\"";

        if (std::string_view(k) == "method" || std::string_view(k) == "path")
          out += ansi_wrap("\033[36m", tmp, color);
        else
          out += c_str(tmp, color);
      }
      else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>)
      {
        std::string_view sv = v ? std::string_view(v) : std::string_view("");
        std::string escaped;
        escaped.reserve(sv.size() + 8);
        appendJsonEscaped(escaped, sv);

        std::string tmp;
        tmp.reserve(escaped.size() + 2);
        tmp += "\"";
        tmp += escaped;
        tmp += "\"";

        if (std::string_view(k) == "method" || std::string_view(k) == "path")
          out += ansi_wrap("\033[36m", tmp, color);
        else
          out += c_str(tmp, color);
      }
      else
      {
        const std::string s = fmt::format("{}", std::forward<V>(v));
        std::string escaped;
        escaped.reserve(s.size() + 8);
        appendJsonEscaped(escaped, s);

        std::string tmp;
        tmp.reserve(escaped.size() + 2);
        tmp += "\"";
        tmp += escaped;
        tmp += "\"";

        out += c_str(tmp, color);
      }

      out += c_punc(",\n", color);

      if constexpr (sizeof...(rest) > 0)
        appendJsonPrettyKV(out, color, std::forward<Rest>(rest)...);
    }

    /**
     * @brief Build a pretty JSON log entry.
     */
    template <typename... Args>
    std::string buildJsonPretty(Level level, std::string_view msg, Args &&...kvpairs) const
    {
      const bool color = Logger::jsonColorsEnabled();

      std::string out;
      out.reserve(512);
      out += c_punc("{\n", color);

      auto add_str = [&](std::string_view k, std::string_view v)
      {
        std::string kq;
        kq.reserve(k.size() + 2);
        kq += "\"";
        kq.append(k.data(), k.size());
        kq += "\"";

        std::string escaped;
        escaped.reserve(v.size() + 8);
        appendJsonEscaped(escaped, v);

        std::string vq;
        vq.reserve(escaped.size() + 2);
        vq += "\"";
        vq += escaped;
        vq += "\"";

        out += "  ";
        out += c_key(kq, color);
        out += c_punc(": ", color);
        out += c_str(vq, color);
        out += c_punc(",\n", color);
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

      appendJsonPrettyKV(out, color, std::forward<Args>(kvpairs)...);

      auto ends_with_plain = [&](const std::string &suf) -> bool
      {
        return out.size() >= suf.size() && out.compare(out.size() - suf.size(), suf.size(), suf) == 0;
      };

      if (ends_with_plain(",\n"))
      {
        out.pop_back();
        out.pop_back();
        out += "\n";
      }

      out += c_punc("}", color);
      if (color)
        out += "\033[0m";
      return out;
    }

    /**
     * @brief Append JSON key/value pairs (single-line JSON).
     */
    template <typename V, typename... Rest>
    static void appendJsonKV(std::string &out, const char *k, V &&v, Rest &&...rest)
    {
      out += ",";
      appendJsonKey(out, k);
      appendJsonValue(out, std::forward<V>(v));

      if constexpr (sizeof...(rest) > 0)
        appendJsonKV(out, std::forward<Rest>(rest)...);
    }

    /**
     * @brief Wrap a string with an ANSI color code when enabled.
     */
    static std::string ansi_wrap(std::string_view code, std::string_view s, bool on)
    {
      if (!on)
        return std::string(s);

      std::string out;
      out.reserve(code.size() + s.size() + 8);
      out += code;
      out.append(s.data(), s.size());
      out += "\033[0m";
      return out;
    }

    static std::string c_key(std::string_view s, bool on) { return ansi_wrap("\033[36m", s, on); }
    static std::string c_str(std::string_view s, bool on) { return ansi_wrap("\033[32m", s, on); }
    static std::string c_num(std::string_view s, bool on) { return ansi_wrap("\033[33m", s, on); }
    static std::string c_bool(std::string_view s, bool on) { return ansi_wrap("\033[35m", s, on); }
    static std::string c_punc(std::string_view s, bool on) { return ansi_wrap("\033[90m", s, on); }

    /**
     * @brief Whether console synchronization is enabled.
     *
     * Controlled by VIX_CONSOLE_SYNC:
     * - unset or 0/false: disabled
     * - otherwise: enabled
     */
    static bool console_sync_enabled()
    {
      if (const char *v = std::getenv("VIX_CONSOLE_SYNC"); v && *v)
        return std::string_view(v) != "0" && std::string_view(v) != "false";
      return false;
    }
  };

} // namespace vix::utils

#if defined(_WIN32)

#if defined(VIX_UTILS_RESTORE_MAX_MACRO)
#pragma pop_macro("max")
#undef VIX_UTILS_RESTORE_MAX_MACRO
#endif

#if defined(VIX_UTILS_RESTORE_MIN_MACRO)
#pragma pop_macro("min")
#undef VIX_UTILS_RESTORE_MIN_MACRO
#endif

#if defined(VIX_UTILS_RESTORE_DEBUG_MACRO)
#pragma pop_macro("DEBUG")
#undef VIX_UTILS_RESTORE_DEBUG_MACRO
#endif

#if defined(VIX_UTILS_RESTORE_ERROR_MACRO)
#pragma pop_macro("ERROR")
#undef VIX_UTILS_RESTORE_ERROR_MACRO
#endif

#endif

#endif // VIX_UTILS_LOGGER_HPP

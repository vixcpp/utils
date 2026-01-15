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
#include <unordered_map>
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
      CRITICAL,
      OFF
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
      std::unordered_map<std::string, std::string> fields;

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
      if (level == Level::OFF)
        return;

      std::lock_guard<std::mutex> lock(mutex_);
      if (!spd_ || !spd_->should_log(toSpdLevel(level)))
        return;

      if (console_sync_enabled())
      {
        vix::utils::console_wait_banner();
        std::lock_guard<std::mutex> lk(vix::utils::console_mutex());
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

    void setLevelFromEnv(std::string_view envName = "VIX_LOG_LEVEL");
    static Level parseLevel(std::string_view s);
    static Level parseLevelFromEnv(std::string_view envName = "VIX_LOG_LEVEL", Level fallback = Level::WARN);
    static bool jsonColorsEnabled();

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

    bool enabled(Level lvl) const noexcept
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!spd_)
        return false;
      return spd_->should_log(toSpdLevel(lvl));
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
      case Level::OFF:
        return spdlog::level::off;
      default:
        return spdlog::level::info;
      }
    }

    std::shared_ptr<spdlog::logger> spd_;
    mutable std::mutex mutex_;
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
      case Level::OFF:
        return "off";
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

    static std::string c_red(std::string_view s, bool on) { return ansi_wrap("\033[31m", s, on); }
    static std::string c_blue(std::string_view s, bool on) { return ansi_wrap("\033[34m", s, on); }
    static std::string c_dim(std::string_view s, bool on) { return ansi_wrap("\033[2m", s, on); }

    template <typename Int>
    static std::string c_http_status(Int code, bool on)
    {
      const int c = static_cast<int>(code);
      const std::string s = fmt::format("{}", c);

      if (!on)
        return s;

      if (c >= 200 && c < 300)
        return ansi_wrap("\033[32m", s, true); // green
      if (c >= 300 && c < 400)
        return ansi_wrap("\033[36m", s, true); // cyan
      if (c >= 400 && c < 500)
        return ansi_wrap("\033[33m", s, true); // yellow
      if (c >= 500 && c < 600)
        return ansi_wrap("\033[31m", s, true); // red
      return ansi_wrap("\033[90m", s, true);   // gray
    }

    static bool ends_with(std::string_view s, std::string_view suf)
    {
      return s.size() >= suf.size() && s.substr(s.size() - suf.size()) == suf;
    }

    template <typename V>
    static bool is_integralish_v()
    {
      using T = std::remove_cv_t<std::remove_reference_t<V>>;
      return std::is_integral_v<T> && !std::is_same_v<T, bool>;
    }

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

    template <typename V, typename... Rest>
    static void appendJsonKV(std::string &out, const char *k, V &&v, Rest &&...rest)
    {
      out += ",";
      appendJsonKey(out, k);
      appendJsonValue(out, std::forward<V>(v));

      if constexpr (sizeof...(rest) > 0)
        appendJsonKV(out, std::forward<Rest>(rest)...);
    }

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

    static std::string c_key(std::string_view s, bool on) { return ansi_wrap("\033[36m", s, on); }  // cyan
    static std::string c_str(std::string_view s, bool on) { return ansi_wrap("\033[32m", s, on); }  // green
    static std::string c_num(std::string_view s, bool on) { return ansi_wrap("\033[33m", s, on); }  // yellow
    static std::string c_bool(std::string_view s, bool on) { return ansi_wrap("\033[35m", s, on); } // magenta
    static std::string c_punc(std::string_view s, bool on) { return ansi_wrap("\033[90m", s, on); } // gray

    static bool console_sync_enabled()
    {
      if (const char *v = std::getenv("VIX_CONSOLE_SYNC"); v && *v)
        return std::string_view(v) != "0" && std::string_view(v) != "false";
      return false;
    }
  };
}

#endif // VIX_LOGGER_HPP

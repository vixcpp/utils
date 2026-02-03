/**
 *
 *  @file ServerPrettyLogs.hpp
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
#ifndef VIX_SERVER_PRETTY_LOGS_HPP
#define VIX_SERVER_PRETTY_LOGS_HPP

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <vix/utils/ConsoleMutex.hpp>

#if !defined(_WIN32)
#include <unistd.h>
#include <cstdio>
#endif

namespace vix::utils
{
  /**
   * @struct ServerReadyInfo
   * @brief Configuration and metadata used to print the runtime ready banner.
   *
   * This structure holds the information displayed by RuntimeBanner::emit_server_ready(),
   * including application identity, mode/status, HTTP and WebSocket endpoints, and
   * optional hints such as thread counts.
   */
  struct ServerReadyInfo
  {
    /// Application name displayed in the banner.
    std::string app = "vix.cpp";

    /**
     * @brief Optional version string displayed next to the identity.
     *
     * Example: "Vix.cpp v1.16.1"
     */
    std::string version;

    /**
     * @brief Startup time in milliseconds.
     *
     * When set to a value >= 0, it is displayed as "(X ms)".
     */
    int ready_ms = -1;

    /**
     * @brief Runtime mode.
     *
     * Typically "run" or "dev".
     */
    std::string mode = "";

    /**
     * @brief Runtime status label.
     *
     * Common values: "ready", "listening", "running".
     */
    std::string status = "ready";

    /**
     * @brief Path to the configuration file (optional).
     *
     * Example: "/path/to/config.json"
     */
    std::string config_path;

    /// HTTP hostname displayed in the banner.
    std::string host = "localhost";

    /// HTTP port displayed in the banner.
    int port = 8080;

    /// HTTP scheme displayed in the banner (e.g. "http" or "https").
    std::string scheme = "http";

    /// Base path for HTTP routes (e.g. "/").
    std::string base_path = "/";

    /// Whether the WebSocket row should be shown.
    bool show_ws = true;

    /// WebSocket port.
    int ws_port = 9090;

    /// WebSocket scheme (e.g. "ws" or "wss").
    std::string ws_scheme = "ws";

    /// WebSocket hostname.
    std::string ws_host = "localhost";

    /// WebSocket path (e.g. "/").
    std::string ws_path = "/";

    /// Whether "Hint: Ctrl+C ..." should be shown.
    bool show_hints = true;

    /// Current thread count (optional).
    std::size_t threads = 0;

    /// Maximum thread count (optional).
    std::size_t max_threads = 0;
  };

  /**
   * @class RuntimeBanner
   * @brief Pretty runtime banner printed to stderr when the server is ready.
   *
   * Provides:
   * - terminal capability checks (TTY, colors, animations)
   * - OSC 8 hyperlinks when supported
   * - a single entry point RuntimeBanner::emit_server_ready()
   *
   * Threading:
   * - Uses vix::utils::console_mutex() to serialize output so banner lines do not
   *   interleave across threads.
   * - Uses vix::utils::console_reset_banner(), console_mark_banner_done(), and
   *   related primitives for banner synchronization.
   */
  class RuntimeBanner final
  {
  public:
    /**
     * @brief Return true if stdout is a TTY.
     *
     * On Windows, this currently returns true.
     */
    static bool stdout_is_tty()
    {
#if defined(_WIN32)
      return true;
#else
      return ::isatty(::fileno(stdout)) == 1;
#endif
    }

    /**
     * @brief Return true if stderr is a TTY.
     *
     * On Windows, this currently returns true.
     */
    static bool stderr_is_tty()
    {
#if defined(_WIN32)
      return true;
#else
      return ::isatty(::fileno(stderr)) == 1;
#endif
    }

    /**
     * @brief Determine whether colored output is enabled.
     *
     * Rules:
     * - If NO_COLOR is set (and non-empty): colors are disabled.
     * - If VIX_COLOR is set:
     *   - "never|0|false" disables colors
     *   - "always|1|true" enables colors
     * - Otherwise: enabled by default.
     *
     * @return True if ANSI colors should be used.
     */
    static bool colors_enabled()
    {
      if (const char *no = std::getenv("NO_COLOR"); no && *no)
        return false;

      if (const char *v = std::getenv("VIX_COLOR"); v && *v)
      {
        std::string s(v);
        for (auto &c : s)
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (s == "never" || s == "0" || s == "false")
          return false;
        if (s == "always" || s == "1" || s == "true")
          return true;
      }

      return true;
    }

    /**
     * @brief Derive runtime mode from environment.
     *
     * Reads VIX_MODE and normalizes it:
     * - "dev|watch|reload" -> "dev"
     * - anything else -> "run"
     *
     * @return Mode string ("dev" or "run").
     */
    static std::string mode_from_env()
    {
      const char *v = std::getenv("VIX_MODE");
      if (!v || !*v)
        return "run";

      std::string s(v);
      for (auto &c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

      if (s == "dev" || s == "watch" || s == "reload")
        return "dev";
      return "run";
    }

    /**
     * @brief Determine if terminal hyperlinks (OSC 8) are enabled.
     *
     * Rules:
     * - Disabled if VIX_NO_HYPERLINK is set (and non-empty).
     * - Requires stderr to be a TTY.
     * - Enabled for a conservative allowlist of terminals.
     * - Typically disabled for tmux/screen.
     *
     * @return True if OSC 8 links can be emitted.
     */
    static bool hyperlinks_enabled()
    {
      if (const char *no = std::getenv("VIX_NO_HYPERLINK"); no && *no)
        return false;

      if (!stderr_is_tty())
        return false;

      if (std::getenv("VSCODE_PID"))
        return true;
      if (std::getenv("WT_SESSION"))
        return true;
      if (std::getenv("WEZTERM_EXECUTABLE"))
        return true;

      if (const char *tp = std::getenv("TERM_PROGRAM"))
      {
        std::string s(tp);
        if (s == "iTerm.app" || s == "Apple_Terminal" || s == "WezTerm" || s == "vscode")
          return true;
      }

      if (std::getenv("KITTY_WINDOW_ID"))
        return true;

      if (std::getenv("VTE_VERSION"))
        return true;

      if (const char *term = std::getenv("TERM"))
      {
        std::string t(term);
        if (t.find("screen") != std::string::npos)
          return false;
      }

      return false;
    }

    /**
     * @brief Build an OSC 8 hyperlink string if enabled.
     *
     * @param url Link target.
     * @param text Display text.
     * @param on Whether OSC 8 should be emitted.
     * @return Hyperlink-wrapped text if enabled, otherwise `text`.
     */
    static std::string osc8_link(
        const std::string &url,
        const std::string &text,
        bool on)
    {
      if (!on)
        return text;

      // OSC 8 hyperlink:
      //   ESC ] 8 ; ; URL ST  TEXT  ESC ] 8 ; ; ST
      // ST = "ESC \\" (String Terminator)
      constexpr const char *kEsc = "\033";
      constexpr const char *kSt = "\033\\";

      std::string out;
      out.reserve(url.size() + text.size() + 32);

      out += kEsc;
      out += "]8;;";
      out += url;
      out += kSt;

      out += text;

      out += kEsc;
      out += "]8;;";
      out += kSt;

      return out;
    }

    /**
     * @brief Print the runtime "ready" banner to stderr.
     *
     * This function:
     * - resets the banner state (for coordination with other threads)
     * - serializes output using vix::utils::console_mutex()
     * - prints time, identity, status, URLs, config, threads, mode/status, hints
     * - marks the banner as done and notifies waiting threads
     *
     * @param info Banner information (endpoints, labels, timing, etc.).
     */
    static void emit_server_ready(const ServerReadyInfo &info)
    {
      vix::utils::console_reset_banner();

      const bool color = colors_enabled();
      const std::string http_url = make_http_url(info);
      const std::string ws_url = (info.show_ws ? make_ws_url(info) : std::string());

      {
        std::lock_guard<std::mutex> lk(vix::utils::console_mutex());

        {
          const std::string t = format_local_time_12h();

          if (color)
            std::cerr << "\033[0m";

          std::cerr
              << (color ? gray(t, true) : t) << "  "
              << runtime_identity(info.app, info.mode, color) << "  "
              << status_pill(to_upper_copy(info.status), color);

          if (!info.version.empty())
          {
            if (color)
              std::cerr << "  " << bold(white_bright(info.version, true), true);
            else
              std::cerr << "  " << info.version;
          }

          if (info.ready_ms >= 0)
          {
            const std::string ms = " (" + std::to_string(info.ready_ms) + " ms)";
            std::cerr << (color ? subtle_info(ms, true) : ms);
          }

          if (!info.mode.empty())
            std::cerr << "  " << mode_tag(info.mode, color);

          std::cerr << "\n";
        }

        std::cerr << "\n";

        row(bullet(color), "HTTP:", http_url, false, color);

        if (info.show_ws)
          row(bullet(color), "WS:", ws_url, false, color);

        if (!info.config_path.empty())
          row(info_mark(color), "Config:", info.config_path, true, color);

        if (info.threads > 0)
        {
          std::string v = std::to_string(info.threads);
          if (info.max_threads > 0)
            v += "/" + std::to_string(info.max_threads);

          row(info_mark(color), "Threads:", v, true, color);
        }

        row(info_mark(color), "Mode:", pretty_mode(info.mode), true, color);
        row(info_mark(color), "Status:", pretty_status(info.status), true, color);

        if (info.show_hints)
          row(info_mark(color), "Hint:", "Ctrl+C to stop the server", true, color);

        std::cerr << "\n";
        std::cerr.flush();
      }

      vix::utils::console_mark_banner_done();
    }

  private:
    /**
     * @brief Label width used to align banner rows.
     */
    static constexpr std::size_t LABEL_WIDTH = 8;

    /**
     * @brief Apply a subtle accent color to informational text.
     *
     * @param s Input string.
     * @param on Whether coloring is enabled.
     * @return Styled or raw string.
     */
    static std::string subtle_info(const std::string &s, bool on)
    {
      if (!on)
        return s;
      return "\033[38;5;110m" + s + "\033[0m";
    }

    /**
     * @brief Determine whether banner animations are enabled.
     *
     * Rules:
     * - Disabled if VIX_NO_ANIM is set (and non-empty)
     * - Requires stderr to be a TTY
     * - Disabled if NO_COLOR is set (and non-empty)
     *
     * @return True if small animations may be shown.
     */
    static bool animations_enabled()
    {
      if (const char *no = std::getenv("VIX_NO_ANIM"); no && *no)
        return false;

      if (!stderr_is_tty())
        return false;

      if (const char *nc = std::getenv("NO_COLOR"); nc && *nc)
        return false;

      return true;
    }

    /**
     * @brief Compute a repeating phase value in range [0..2].
     *
     * Used to animate the "dev" tag background color.
     *
     * @return Phase 0, 1, or 2.
     */
    static int pulse_phase_0_2()
    {
      using namespace std::chrono;
      auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
      long long t = (ms / 300) % 3; // 0,1,2
      return static_cast<int>(t);
    }

    /**
     * @brief Build an animated "[dev]" tag when color and animations are enabled.
     *
     * @param color Whether color output is enabled.
     * @return Styled dev tag.
     */
    static std::string dev_tag_animated(bool color)
    {
      if (!color || !animations_enabled())
        return "[dev]";

      const int p = pulse_phase_0_2();
      const int bg = (p == 0 ? 28 : (p == 1 ? 34 : 40));
      std::string out;
      out.reserve(48);

      out += "\033[1m";
      out += "\033[48;5;";
      out += std::to_string(bg);
      out += "m";
      out += "\033[30m";
      out += " dev ";
      out += "\033[0m";

      return out;
    }

    /**
     * @brief Build a "[run]" tag.
     *
     * @param color Whether color output is enabled.
     * @return Styled run tag.
     */
    static std::string run_tag(bool color)
    {
      if (!color)
        return "[run]";

      std::string out;
      out.reserve(32);
      out += "\033[1m";
      out += "\033[48;5;238m";
      out += "\033[97m";
      out += " run ";
      out += "\033[0m";
      return out;
    }

    /**
     * @brief Select the correct mode tag for the given mode.
     *
     * @param mode Mode string (typically "dev" or "run").
     * @param color Whether color output is enabled.
     * @return Styled mode tag.
     */
    static std::string mode_tag(const std::string &mode, bool color)
    {
      if (mode == "dev")
        return dev_tag_animated(color);
      return run_tag(color);
    }

    /**
     * @brief Return true if the given mode is considered "dev".
     *
     * @param mode Mode string.
     * @return True if dev.
     */
    static bool is_dev_mode(const std::string &mode)
    {
      return mode == "dev";
    }

    /**
     * @brief Select the runtime icon based on the mode.
     *
     * Dev uses a diamond "◆", run uses a dot "●".
     *
     * @param mode Mode string.
     * @param color Whether color output is enabled.
     * @return Styled icon string.
     */
    static std::string runtime_icon(const std::string &mode, bool color)
    {
      const bool dev = is_dev_mode(mode);
      const std::string icon = dev ? "◆" : "●";
      if (!color)
        return icon;
      return wrap("\033[32m", icon, true);
    }

    /**
     * @brief Build the application identity string.
     *
     * Normalizes common variants of "Vix.cpp" to the canonical display name.
     *
     * @param app Application name.
     * @param mode Mode string.
     * @param color Whether color output is enabled.
     * @return Styled identity string.
     */
    static std::string runtime_identity(const std::string &app, const std::string &mode, bool color)
    {
      if (!color)
        return "[" + app + "]";

      std::string name = app;
      if (name == "vix.cpp" || name == "VIX.cpp" || name == "Vix.cpp")
        name = "Vix.cpp";

      const std::string icon = runtime_icon(mode, true);
      const std::string styled = bold(wrap("\033[32m", name, true), true);
      return icon + " " + styled;
    }

    /**
     * @brief Render a colored pill for the given status.
     *
     * @param status_upper Uppercase status label (e.g. "READY").
     * @param color Whether color output is enabled.
     * @return Styled pill string.
     */
    static std::string status_pill(const std::string &status_upper, bool color)
    {
      if (!color)
        return status_upper;

      int bg = status_bg_color_code(status_upper);

      std::string out;
      out.reserve(status_upper.size() + 32);

      out += "\033[1m";
      out += "\033[48;5;";
      out += std::to_string(bg);
      out += "m";
      out += "\033[30m";

      out += " ";
      out += status_upper;
      out += " ";

      out += "\033[0m";
      return out;
    }

    /**
     * @brief Choose a background color code for the status pill.
     *
     * @param status_upper Uppercase status string.
     * @return ANSI 256-color background code.
     */
    static int status_bg_color_code(const std::string &status_upper)
    {
      if (status_upper == "READY")
        return 34;

      if (status_upper == "RUNNING" || status_upper == "LISTENING")
        return 35;

      if (status_upper == "WARN" || status_upper == "WARNING")
        return 214;

      if (status_upper == "ERROR" || status_upper == "FAILED")
        return 196;

      return 34;
    }

    /**
     * @brief Emit one aligned row in the banner.
     *
     * @param icon Left icon (bullet or info mark).
     * @param label Row label (e.g. "HTTP:").
     * @param value Row value (e.g. URL).
     * @param dim_value Whether to dim the value (instead of making it a link).
     * @param color Whether color output is enabled.
     */
    static void row(
        const std::string &icon,
        const std::string &label,
        const std::string &value,
        bool dim_value,
        bool color)
    {
      const std::string lbl = pad_label(label);
      std::cerr << "  "
                << reset_style(icon, color) << " "
                << (color ? bold(white_bright(lbl, true), true) : lbl)
                << (dim_value ? dim(value, color) : link(value, color))
                << "\n";
    }

    /**
     * @brief Pad a label to a fixed width for alignment.
     *
     * @param s Label string.
     * @return Padded label string.
     */
    static std::string pad_label(const std::string &s)
    {
      if (s.size() >= LABEL_WIDTH)
        return s;
      return s + std::string(LABEL_WIDTH - s.size(), ' ');
    }

    /**
     * @brief Format the local time in 12-hour format with seconds.
     *
     * Output example: "11:07:57 PM"
     *
     * @return Local time string.
     */
    static std::string format_local_time_12h()
    {
      using clock = std::chrono::system_clock;
      const auto now = clock::now();
      const std::time_t t = clock::to_time_t(now);

      std::tm tm{};
#if defined(_WIN32)
      localtime_s(&tm, &t);
#else
      localtime_r(&t, &tm);
#endif

      int hour = tm.tm_hour;
      const bool pm = (hour >= 12);
      hour = hour % 12;
      if (hour == 0)
        hour = 12;

      std::ostringstream oss;
      oss << hour << ":"
          << std::setw(2) << std::setfill('0') << tm.tm_min << ":"
          << std::setw(2) << std::setfill('0') << tm.tm_sec << " "
          << (pm ? "PM" : "AM");
      return oss.str();
    }

    /**
     * @brief Build the HTTP URL from the provided ServerReadyInfo.
     *
     * Ensures the base path is prefixed with '/'.
     *
     * @param i ServerReadyInfo.
     * @return HTTP URL string.
     */
    static std::string make_http_url(const ServerReadyInfo &i)
    {
      std::ostringstream oss;
      oss << i.scheme << "://" << i.host << ":" << i.port;

      if (i.base_path.empty())
      {
        oss << "/";
      }
      else
      {
        if (i.base_path.front() != '/')
          oss << "/";
        oss << i.base_path;
      }
      return oss.str();
    }

    /**
     * @brief Build the WebSocket URL from the provided ServerReadyInfo.
     *
     * Ensures the path is prefixed with '/' when non-empty.
     *
     * @param i ServerReadyInfo.
     * @return WebSocket URL string.
     */
    static std::string make_ws_url(const ServerReadyInfo &i)
    {
      std::ostringstream oss;
      oss << i.ws_scheme << "://" << i.ws_host << ":" << i.ws_port;

      if (!i.ws_path.empty())
      {
        if (i.ws_path.front() != '/')
          oss << "/";
        oss << i.ws_path;
      }
      return oss.str();
    }

    /**
     * @brief Convert a string to uppercase (ASCII).
     *
     * @param s Input string.
     * @return Uppercased copy.
     */
    static std::string to_upper_copy(std::string s)
    {
      for (auto &c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      return s;
    }

    /**
     * @brief Human-friendly mode label shown in the banner.
     *
     * @param mode Mode string.
     * @return Pretty mode text.
     */
    static std::string pretty_mode(const std::string &mode)
    {
      if (mode == "dev")
        return "dev (watch/reload)";
      if (mode.empty())
        return "run";
      return mode;
    }

    /**
     * @brief Human-friendly status label shown in the banner.
     *
     * @param status Status string.
     * @return Pretty status text.
     */
    static std::string pretty_status(const std::string &status)
    {
      if (status.empty())
        return "ready";
      return status;
    }

    /**
     * @brief Build a small mode tag like "[run]" or "[dev]".
     *
     * @param mode Mode string.
     * @param color Whether color output is enabled.
     * @return Styled tag string.
     */
    static std::string mode_tag_small(const std::string &mode, bool color)
    {
      const std::string m = mode.empty() ? "run" : mode;
      if (!color)
        return "[" + m + "]";
      return cyan("[" + m + "]", true);
    }

    /**
     * @brief Apply bright-white styling.
     *
     * @param s Input string.
     * @param on Whether enabled.
     * @return Styled or raw string.
     */
    static std::string white_bright(const std::string &s, bool on) { return wrap("\033[97m", s, on); }

    /**
     * @brief Reset the terminal style before printing a fragment.
     *
     * @param s Fragment.
     * @param on Whether enabled.
     * @return Styled or raw fragment.
     */
    static std::string reset_style(const std::string &s, bool on)
    {
      if (!on)
        return s;
      return std::string("\033[0m") + s;
    }

    /**
     * @brief Wrap a string with an ANSI code and reset.
     *
     * @param code ANSI prefix code.
     * @param s Input string.
     * @param on Whether enabled.
     * @return Styled or raw string.
     */
    static std::string wrap(const char *code, const std::string &s, bool on)
    {
      if (!on)
        return s;
      return std::string(code) + s + "\033[0m";
    }

    /**
     * @brief Apply gray styling.
     */
    static std::string gray(const std::string &s, bool on) { return wrap("\033[90m", s, on); }

    /**
     * @brief Apply green styling.
     */
    static std::string green(const std::string &s, bool on) { return wrap("\033[32m", s, on); }

    /**
     * @brief Apply cyan styling.
     */
    static std::string cyan(const std::string &s, bool on) { return wrap("\033[36m", s, on); }

    /**
     * @brief Apply dim styling.
     */
    static std::string dim(const std::string &s, bool on) { return wrap("\033[2m", s, on); }

    /**
     * @brief Apply bold styling.
     */
    static std::string bold(const std::string &s, bool on) { return wrap("\033[1m", s, on); }

    /**
     * @brief A small green dot used as an OK indicator.
     */
    static std::string ok_dot(bool color)
    {
      return color ? green("●", true) : "●";
    }

    /**
     * @brief Render a bracket badge such as "[text]".
     */
    static std::string badge(const std::string &text, bool color)
    {
      if (!color)
        return "[" + text + "]";
      return bold(gray("[" + text + "]", true), true);
    }

    /**
     * @brief Render the bullet icon used for main rows.
     */
    static std::string bullet(bool color)
    {
      return color ? cyan("›", true) : std::string(">");
    }

    /**
     * @brief Render the small info mark used for secondary rows.
     */
    static std::string info_mark(bool color)
    {
      return color ? gray("i", true) : "i";
    }

    /**
     * @brief Render a URL as a clickable terminal link when supported.
     *
     * If OSC 8 hyperlinks are supported, the URL is wrapped as a link, while
     * the visible label remains the (optionally colored) URL text.
     *
     * @param url URL to display.
     * @param color Whether color output is enabled.
     * @return Styled URL string.
     */
    static std::string link(const std::string &url, bool color)
    {
      const bool hl = hyperlinks_enabled();
      const std::string label = color ? cyan(url, true) : url;
      return osc8_link(url, label, hl);
    }
  };

} // namespace vix::utils

#endif // VIX_SERVER_PRETTY_LOGS_HPP

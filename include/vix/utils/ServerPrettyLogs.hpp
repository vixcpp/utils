#pragma once

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
    struct ServerReadyInfo
    {
        // Identity
        std::string app = "vix";
        std::string version; // e.g. "Vix.cpp v1.16.1"
        int ready_ms = -1;
        // Mode / status / config
        std::string mode = "";
        std::string status = "ready"; // "ready" | "listening" | "running"
        std::string config_path;      // "/path/to/config.json"
        // HTTP
        std::string host = "localhost";
        int port = 8080;
        std::string scheme = "http";
        std::string base_path = "/";
        // WS
        bool show_ws = true;
        int ws_port = 9090;
        std::string ws_scheme = "ws";
        std::string ws_host = "localhost";
        std::string ws_path = "/";
        // Hints
        bool show_hints = true;
        std::size_t threads = 0;
        std::size_t max_threads = 0;
    };

    class RuntimeBanner final
    {
    public:
        static bool stdout_is_tty()
        {
#if defined(_WIN32)
            return true;
#else
            return ::isatty(::fileno(stdout)) == 1;
#endif
        }

        static bool stderr_is_tty()
        {
#if defined(_WIN32)
            return true;
#else
            return ::isatty(::fileno(stderr)) == 1;
#endif
        }

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

            return true; // default ON
        }

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

        static bool hyperlinks_enabled()
        {
            if (const char *no = std::getenv("VIX_NO_HYPERLINK"); no && *no)
                return false;

            if (!stderr_is_tty())
                return false;

            // safe allowlist
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

            // tmux/screen: usually OFF
            if (const char *term = std::getenv("TERM"))
            {
                std::string t(term);
                if (t.find("screen") != std::string::npos)
                    return false;
            }

            return false;
        }

        static std::string osc8_link(const std::string &url,
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
        static constexpr std::size_t LABEL_WIDTH = 8;
        static std::string subtle_info(const std::string &s, bool on)
        {
            if (!on)
                return s;
            return "\033[38;5;110m" + s + "\033[0m";
        }

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

        static int pulse_phase_0_2()
        {
            using namespace std::chrono;
            auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            long long t = (ms / 300) % 3; // 0,1,2
            return static_cast<int>(t);
        }

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

        static std::string mode_tag(const std::string &mode, bool color)
        {
            if (mode == "dev")
                return dev_tag_animated(color);
            return run_tag(color);
        }

        static bool is_dev_mode(const std::string &mode)
        {
            return mode == "dev";
        }

        static std::string runtime_icon(const std::string &mode, bool color)
        {
            const bool dev = is_dev_mode(mode);
            const std::string icon = dev ? "◆" : "●";
            if (!color)
                return icon;
            return wrap("\033[32m", icon, true);
        }

        static std::string runtime_identity(const std::string &app, const std::string &mode, bool color)
        {
            if (!color)
                return "[" + app + "]";

            const std::string icon = runtime_icon(mode, true);
            const std::string name = bold(wrap("\033[32m", "VIX", true), true);
            return icon + " " + name;
        }

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

        static int status_bg_color_code(const std::string &status_upper)
        {
            if (status_upper == "READY")
                return 34; // green

            if (status_upper == "RUNNING" || status_upper == "LISTENING")
                return 35; // green/teal

            if (status_upper == "WARN" || status_upper == "WARNING")
                return 214; // orange

            if (status_upper == "ERROR" || status_upper == "FAILED")
                return 196; // red

            return 34; // default green
        }

        static void row(const std::string &icon,
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

        static std::string pad_label(const std::string &s)
        {
            if (s.size() >= LABEL_WIDTH)
                return s;
            return s + std::string(LABEL_WIDTH - s.size(), ' ');
        }

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

        static std::string to_upper_copy(std::string s)
        {
            for (auto &c : s)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        }

        static std::string pretty_mode(const std::string &mode)
        {
            if (mode == "dev")
                return "dev (watch/reload)";
            if (mode.empty())
                return "run";
            return mode;
        }

        static std::string pretty_status(const std::string &status)
        {
            if (status.empty())
                return "ready";
            return status;
        }

        static std::string mode_tag_small(const std::string &mode, bool color)
        {
            const std::string m = mode.empty() ? "run" : mode;
            if (!color)
                return "[" + m + "]";
            return cyan("[" + m + "]", true);
        }

        static std::string white_bright(const std::string &s, bool on) { return wrap("\033[97m", s, on); }
        static std::string reset_style(const std::string &s, bool on)
        {
            if (!on)
                return s;
            return std::string("\033[0m") + s;
        }

        static std::string wrap(const char *code, const std::string &s, bool on)
        {
            if (!on)
                return s;
            return std::string(code) + s + "\033[0m";
        }

        static std::string gray(const std::string &s, bool on) { return wrap("\033[90m", s, on); }
        static std::string green(const std::string &s, bool on) { return wrap("\033[32m", s, on); }
        static std::string cyan(const std::string &s, bool on) { return wrap("\033[36m", s, on); }
        static std::string dim(const std::string &s, bool on) { return wrap("\033[2m", s, on); }
        static std::string bold(const std::string &s, bool on) { return wrap("\033[1m", s, on); }

        static std::string ok_dot(bool color)
        {
            return color ? green("●", true) : "●";
        }

        static std::string badge(const std::string &text, bool color)
        {
            if (!color)
                return "[" + text + "]";
            return bold(gray("[" + text + "]", true), true);
        }

        static std::string bullet(bool color)
        {
            return color ? cyan("›", true) : std::string(">");
        }

        static std::string info_mark(bool color)
        {
            // Tiny info mark
            return color ? gray("i", true) : "i";
        }

        static std::string link(const std::string &url, bool color)
        {
            const bool hl = hyperlinks_enabled();
            const std::string label = color ? cyan(url, true) : url;
            return osc8_link(url, label, hl);
        }
    };

} // namespace vix::utils

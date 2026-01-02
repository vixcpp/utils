#pragma once
#include <mutex>
#include <condition_variable>

namespace vix::utils
{

    inline std::mutex &console_mutex()
    {
        static std::mutex m;
        return m;
    }

    inline std::mutex &banner_mutex()
    {
        static std::mutex m;
        return m;
    }

    inline std::condition_variable &console_cv()
    {
        static std::condition_variable cv;
        return cv;
    }

    inline bool &console_banner_done()
    {
        static bool done = true;
        return done;
    }

    inline void console_wait_banner()
    {
        std::unique_lock<std::mutex> lk(banner_mutex());
        console_cv().wait(lk, []
                          { return console_banner_done(); });
    }

    inline void console_mark_banner_done()
    {
        {
            std::lock_guard<std::mutex> lk(banner_mutex());
            console_banner_done() = true;
        }
        console_cv().notify_all();
    }

    inline void console_reset_banner()
    {
        std::lock_guard<std::mutex> lk(banner_mutex());
        console_banner_done() = false;
    }

} // namespace vix::utils

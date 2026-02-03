/**
 *
 *  @file ConsoleMutex.hpp
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
#ifndef VIX_CONSOLE_MUTEX_HPP
#define VIX_CONSOLE_MUTEX_HPP

#include <mutex>
#include <condition_variable>

namespace vix::utils
{
  /**
   * @brief Global mutex used to serialize console output.
   *
   * This mutex can be used to ensure that log lines, banners,
   * or other console writes do not interleave across threads.
   *
   * @return Reference to the global console mutex.
   */
  inline std::mutex &console_mutex()
  {
    static std::mutex m;
    return m;
  }

  /**
   * @brief Mutex protecting banner-related state.
   *
   * This mutex guards access to the banner completion flag
   * and coordinates threads waiting for banner output.
   *
   * @return Reference to the banner mutex.
   */
  inline std::mutex &banner_mutex()
  {
    static std::mutex m;
    return m;
  }

  /**
   * @brief Condition variable used for banner synchronization.
   *
   * Threads may wait on this condition variable until the
   * console banner has finished rendering.
   *
   * @return Reference to the banner condition variable.
   */
  inline std::condition_variable &console_cv()
  {
    static std::condition_variable cv;
    return cv;
  }

  /**
   * @brief Flag indicating whether the console banner is done.
   *
   * When true, threads waiting for the banner may proceed.
   *
   * @return Reference to the banner completion flag.
   */
  inline bool &console_banner_done()
  {
    static bool done = true;
    return done;
  }

  /**
   * @brief Block until the console banner has completed.
   *
   * This function waits on the banner condition variable
   * until console_banner_done() becomes true.
   */
  inline void console_wait_banner()
  {
    std::unique_lock<std::mutex> lk(banner_mutex());
    console_cv().wait(lk, []
                      { return console_banner_done(); });
  }

  /**
   * @brief Mark the console banner as completed.
   *
   * Wakes all threads waiting in console_wait_banner().
   */
  inline void console_mark_banner_done()
  {
    {
      std::lock_guard<std::mutex> lk(banner_mutex());
      console_banner_done() = true;
    }
    console_cv().notify_all();
  }

  /**
   * @brief Reset the banner completion state.
   *
   * After calling this function, threads calling
   * console_wait_banner() will block until the banner
   * is marked done again.
   */
  inline void console_reset_banner()
  {
    std::lock_guard<std::mutex> lk(banner_mutex());
    console_banner_done() = false;
  }

} // namespace vix::utils

#endif // VIX_CONSOLE_MUTEX_HPP

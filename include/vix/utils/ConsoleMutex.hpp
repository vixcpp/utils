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

#include <vix/log/ConsoleSync.hpp>

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
  inline std::mutex &console_mutex() { return vix::log::console_mutex(); }

  /**
   * @brief Mutex protecting banner-related state.
   *
   * This mutex guards access to the banner completion flag
   * and coordinates threads waiting for banner output.
   *
   * @return Reference to the banner mutex.
   */
  inline std::mutex &banner_mutex() { return vix::log::banner_mutex(); }

  /**
   * @brief Condition variable used for banner synchronization.
   *
   * Threads may wait on this condition variable until the
   * console banner has finished rendering.
   *
   * @return Reference to the banner condition variable.
   */
  inline std::condition_variable &console_cv() { return vix::log::console_cv(); }

  /**
   * @brief Flag indicating whether the console banner is done.
   *
   * When true, threads waiting for the banner may proceed.
   *
   * @return Reference to the banner completion flag.
   */
  inline bool &console_banner_done() { return vix::log::console_banner_done(); }

  /**
   * @brief Block until the console banner has completed.
   *
   * This function waits on the banner condition variable
   * until console_banner_done() becomes true.
   */
  inline void console_wait_banner() { vix::log::console_wait_banner(); }

  /**
   * @brief Mark the console banner as completed.
   *
   * Wakes all threads waiting in console_wait_banner().
   */
  inline void console_mark_banner_done() { vix::log::console_mark_banner_done(); }

  /**
   * @brief Reset the banner completion state.
   *
   * After calling this function, threads calling
   * console_wait_banner() will block until the banner
   * is marked done again.
   */
  inline void console_reset_banner() { vix::log::console_reset_banner(); }

} // namespace vix::utils

#endif // VIX_CONSOLE_MUTEX_HPP

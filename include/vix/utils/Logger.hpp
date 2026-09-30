/**
 * @file Logger.hpp
 * @brief Deprecated compatibility forwarding header for Vix2 logging.
 */
#ifndef VIX_UTILS_LOGGER_HPP
#define VIX_UTILS_LOGGER_HPP

#include <vix/log/Logger.hpp>

namespace vix::utils
{
  /**
   * @deprecated Use the canonical vix::log logging API.
   *
   * This alias preserves the Vix2 Logger surface while all logger state,
   * backend selection, formatting, context, and configuration are owned by
   * the vix::log capability.
   */
  using Logger = vix::log::Logger;
}

#endif // VIX_UTILS_LOGGER_HPP

/**
 *
 *  @file NetworkError.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira. All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_UTILS_NETWORK_ERROR_HPP
#define VIX_UTILS_NETWORK_ERROR_HPP

#include <cctype>
#include <cstddef>
#include <cerrno>
#include <string_view>
#include <system_error>

namespace vix::utils
{
  inline bool contains_token_icase(std::string_view text, std::string_view token)
  {
    if (token.empty())
    {
      return true;
    }

    if (token.size() > text.size())
    {
      return false;
    }

    const auto lower_char = [](unsigned char c) -> char
    {
      return static_cast<char>(std::tolower(c));
    };

    const char first = lower_char(static_cast<unsigned char>(token.front()));
    const std::size_t last = text.size() - token.size();

    for (std::size_t i = 0; i <= last; ++i)
    {
      if (lower_char(static_cast<unsigned char>(text[i])) != first)
      {
        continue;
      }

      std::size_t j = 1;

      for (; j < token.size(); ++j)
      {
        if (lower_char(static_cast<unsigned char>(text[i + j])) !=
            lower_char(static_cast<unsigned char>(token[j])))
        {
          break;
        }
      }

      if (j == token.size())
      {
        return true;
      }
    }

    return false;
  }

  inline bool is_normal_network_disconnect_message(std::string_view message) noexcept
  {
    return contains_token_icase(message, "broken pipe") ||
           contains_token_icase(message, "connection reset") ||
           contains_token_icase(message, "connection reset by peer") ||
           contains_token_icase(message, "operation canceled") ||
           contains_token_icase(message, "operation cancelled") ||
           contains_token_icase(message, "canceled") ||
           contains_token_icase(message, "cancelled") ||
           contains_token_icase(message, "end of file") ||
           contains_token_icase(message, "eof");
  }

  inline bool is_normal_network_disconnect(const std::system_error &e) noexcept
  {
    const auto code = e.code();

    if (code == std::errc::operation_canceled ||
        code == std::errc::broken_pipe ||
        code == std::errc::connection_reset ||
        code == std::errc::connection_aborted ||
        code == std::errc::timed_out)
    {
      return true;
    }

#ifdef ECANCELED
    if (code.value() == ECANCELED)
    {
      return true;
    }
#endif

#ifdef EPIPE
    if (code.value() == EPIPE)
    {
      return true;
    }
#endif

#ifdef ECONNRESET
    if (code.value() == ECONNRESET)
    {
      return true;
    }
#endif

#ifdef ECONNABORTED
    if (code.value() == ECONNABORTED)
    {
      return true;
    }
#endif

#ifdef ETIMEDOUT
    if (code.value() == ETIMEDOUT)
    {
      return true;
    }
#endif

    return is_normal_network_disconnect_message(e.what());
  }
}

#endif

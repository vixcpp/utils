#include <vix/log/ConsoleSync.hpp>
#include <vix/log/Logger.hpp>
#include <vix/utils/ConsoleMutex.hpp>
#include <vix/utils/Logger.hpp>

#include <type_traits>

int main()
{
  static_assert(std::is_same_v<vix::utils::Logger, vix::log::Logger>);

  auto &legacy_logger = vix::utils::Logger::getInstance();
  auto &canonical_logger = vix::log::Logger::getInstance();
  if (&legacy_logger != &canonical_logger)
    return 1;

  vix::log::Logger::Context context;
  context.request_id = "utils-log-compatibility";
  canonical_logger.setContext(context);
  if (legacy_logger.getContext().request_id != context.request_id)
    return 2;
  legacy_logger.clearContext();

  if (&vix::utils::console_mutex() != &vix::log::console_mutex())
    return 3;
  if (&vix::utils::banner_mutex() != &vix::log::banner_mutex())
    return 4;
  if (&vix::utils::console_cv() != &vix::log::console_cv())
    return 5;
  if (&vix::utils::console_banner_done() != &vix::log::console_banner_done())
    return 6;

  return 0;
}

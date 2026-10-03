#include <vix/env/GetOr.hpp>
#include <vix/env/Has.hpp>
#include <vix/env/Set.hpp>
#include <vix/env/Unset.hpp>
#include <vix/utils/ServerPrettyLogs.hpp>

#include <cassert>
#include <cstdlib>
#include <string>

namespace
{
  class EnvRestore final
  {
  public:
    explicit EnvRestore(const char *key)
        : key_(key), present_(vix::env::has(key)), value_(vix::env::get_or(key)) {}

    ~EnvRestore()
    {
      if (present_)
        (void)vix::env::set(key_, value_);
      else
        (void)vix::env::unset(key_);
    }

  private:
    const char *key_;
    bool present_;
    std::string value_;
  };

  void set_value(const char *key, const char *value)
  {
    assert(!vix::env::set(key, value));
  }
} // namespace

int main()
{
  EnvRestore restore_mode("VIX_MODE");
  EnvRestore restore_color("VIX_COLOR");
  EnvRestore restore_no_color("NO_COLOR");

  vix::utils::ServerReadyInfo info;
  assert(info.app == "vix.cpp");
  assert(info.status == "ready");
  assert(info.scheme == "http");
  assert(info.show_ws);

  assert(!vix::env::unset("VIX_MODE"));
  assert(vix::utils::RuntimeBanner::mode_from_env() == "run");
  set_value("VIX_MODE", "WATCH");
  assert(vix::utils::RuntimeBanner::mode_from_env() == "dev");
  set_value("VIX_MODE", "production");
  assert(vix::utils::RuntimeBanner::mode_from_env() == "run");

  assert(!vix::env::unset("NO_COLOR"));
  set_value("VIX_COLOR", "never");
  assert(!vix::utils::RuntimeBanner::colors_enabled());
  set_value("VIX_COLOR", "always");
  assert(vix::utils::RuntimeBanner::colors_enabled());
  set_value("NO_COLOR", "1");
  assert(!vix::utils::RuntimeBanner::colors_enabled());

  void (*emit)(const vix::utils::ServerReadyInfo &) =
      &vix::utils::RuntimeBanner::emit_server_ready;
  assert(emit != nullptr);
  return EXIT_SUCCESS;
}

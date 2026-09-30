#include <cstdlib>
#include <iostream>
#include <string>

#include <vix/env/Legacy.hpp>
#include <vix/env/Set.hpp>
#include <vix/env/Unset.hpp>
#include <vix/utils/Env.hpp>

namespace
{
  constexpr const char *key = "VIX_UTILS_ENV_COMPATIBILITY_TEST";

  void assert_true(bool condition, const std::string &message)
  {
    if (!condition)
    {
      std::cerr << "Assertion failed: " << message << '\n';
      std::exit(1);
    }
  }

  void set_value(const char *value)
  {
    const auto error = vix::env::set(key, value);
    assert_true(!error, "set should succeed");
  }

  void clear_value()
  {
    const auto error = vix::env::unset(key);
    assert_true(!error, "unset should succeed");
  }

  void assert_legacy_matches_canonical()
  {
    const char *legacy_raw = vix::utils::vix_getenv(key);
    const char *canonical_raw = vix::env::legacy::getenv(key);
    assert_true((legacy_raw == nullptr) == (canonical_raw == nullptr),
                "legacy raw lookup presence should match canonical compatibility");
    if (legacy_raw != nullptr)
    {
      assert_true(std::string(legacy_raw) == std::string(canonical_raw),
                  "legacy raw lookup value should match canonical compatibility");
    }

    assert_true(vix::utils::env_or(key, "fallback") ==
                    vix::env::legacy::env_or(key, "fallback"),
                "env_or should delegate to canonical compatibility behavior");
    assert_true(vix::utils::env_bool(key, true) ==
                    vix::env::legacy::env_bool(key, true),
                "env_bool should delegate to canonical compatibility behavior");
    assert_true(vix::utils::env_int(key, 17) ==
                    vix::env::legacy::env_int(key, 17),
                "env_int should delegate to canonical compatibility behavior");
    assert_true(vix::utils::env_uint(key, 23u) ==
                    vix::env::legacy::env_uint(key, 23u),
                "env_uint should delegate to canonical compatibility behavior");
    assert_true(vix::utils::env_double(key, 1.5) ==
                    vix::env::legacy::env_double(key, 1.5),
                "env_double should delegate to canonical compatibility behavior");
  }
} // namespace

int main()
{
  clear_value();
  assert_legacy_matches_canonical();

  set_value(" TRUE ");
  assert_legacy_matches_canonical();

  set_value("invalid");
  assert_legacy_matches_canonical();

  set_value("-42");
  assert_legacy_matches_canonical();

  set_value("");
  assert_legacy_matches_canonical();

  clear_value();
  return 0;
}

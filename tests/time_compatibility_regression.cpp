#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <iostream>
#include <locale>
#include <string>
#include <string_view>
#include <type_traits>

#include <vix/utils/Time.hpp>

namespace
{
  void assert_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "Assertion failed: " << message << '\n';
      std::exit(EXIT_FAILURE);
    }
  }

  bool is_ascii_digit(char value)
  {
    return value >= '0' && value <= '9';
  }

  bool is_one_of(std::string_view value,
                 std::initializer_list<std::string_view> choices)
  {
    for (const auto choice : choices)
    {
      if (value == choice)
        return true;
    }
    return false;
  }

  void test_utc_tm_fixed_values()
  {
    const std::tm epoch = vix::utils::utc_tm(std::time_t{0});
    assert_true(epoch.tm_year == 70, "epoch UTC year should be 1970");
    assert_true(epoch.tm_mon == 0, "epoch UTC month should be January");
    assert_true(epoch.tm_mday == 1, "epoch UTC day should be one");
    assert_true(epoch.tm_hour == 0 && epoch.tm_min == 0 && epoch.tm_sec == 0,
                "epoch UTC time should be midnight");

    const std::tm y2k = vix::utils::utc_tm(std::time_t{946684800});
    assert_true(y2k.tm_year == 100, "Y2K UTC year should be 2000");
    assert_true(y2k.tm_mon == 0 && y2k.tm_mday == 1,
                "Y2K UTC date should be January 1");
    assert_true(y2k.tm_hour == 0 && y2k.tm_min == 0 && y2k.tm_sec == 0,
                "Y2K UTC time should be midnight");
  }

  void test_iso8601_shape()
  {
    const std::string value = vix::utils::iso8601_now();
    assert_true(value.size() == 20,
                "legacy ISO-8601 output should have second precision only");
    assert_true(value[4] == '-' && value[7] == '-' && value[10] == 'T' &&
                    value[13] == ':' && value[16] == ':' && value[19] == 'Z',
                "legacy ISO-8601 output should retain its separators and UTC suffix");

    for (const std::size_t index : {std::size_t{0}, std::size_t{1}, std::size_t{2},
                                    std::size_t{3}, std::size_t{5}, std::size_t{6},
                                    std::size_t{8}, std::size_t{9}, std::size_t{11},
                                    std::size_t{12}, std::size_t{14}, std::size_t{15},
                                    std::size_t{17}, std::size_t{18}})
    {
      assert_true(is_ascii_digit(value[index]),
                  "legacy ISO-8601 numeric fields should contain ASCII digits");
    }
  }

  void test_rfc1123_shape_in_classic_locale()
  {
    const std::locale previous = std::locale();
    std::locale::global(std::locale::classic());
    const std::string value = vix::utils::rfc1123_now();
    std::locale::global(previous);

    assert_true(value.size() == 29,
                "legacy RFC-1123 output should retain its IMF-fixdate shape");
    assert_true(value[3] == ',' && value[4] == ' ' && value[7] == ' ' &&
                    value[11] == ' ' && value[16] == ' ' && value[19] == ':' &&
                    value[22] == ':' && value[25] == ' ' &&
                    value.substr(26) == "GMT",
                "legacy RFC-1123 output should retain punctuation and GMT suffix");
    assert_true(is_one_of(value.substr(0, 3),
                          {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}),
                "classic-locale RFC-1123 weekday should be English abbreviated text");
    assert_true(is_one_of(value.substr(8, 3),
                          {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}),
                "classic-locale RFC-1123 month should be English abbreviated text");

    for (const std::size_t index : {std::size_t{5}, std::size_t{6}, std::size_t{12},
                                    std::size_t{13}, std::size_t{14}, std::size_t{15},
                                    std::size_t{17}, std::size_t{18}, std::size_t{20},
                                    std::size_t{21}, std::size_t{23}, std::size_t{24}})
    {
      assert_true(is_ascii_digit(value[index]),
                  "legacy RFC-1123 numeric fields should contain ASCII digits");
    }
  }

  void test_clock_contracts()
  {
    static_assert(std::is_same_v<decltype(vix::utils::now_ms()), std::uint64_t>);
    static_assert(std::is_same_v<decltype(vix::utils::unix_ms()), std::uint64_t>);

    // now_ms is a steady-clock measurement. Equal successive values are valid
    // because the legacy contract truncates to milliseconds.
    const auto steady_before = vix::utils::now_ms();
    std::uint64_t sink = 0;
    for (std::uint64_t i = 0; i < 10'000; ++i)
      sink += i;
    const auto steady_after = vix::utils::now_ms();
    assert_true(sink != 0, "the intervening operation should execute");
    assert_true(steady_after >= steady_before,
                "now_ms should be a non-decreasing monotonic millisecond counter");

    // unix_ms is checked independently against system_clock. It deliberately
    // is not compared to now_ms because their epochs are unrelated.
    const auto wall_before = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto legacy_unix = vix::utils::unix_ms();
    const auto wall_after = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    constexpr std::int64_t tolerance_ms = 5'000;
    const auto legacy_signed = static_cast<std::int64_t>(legacy_unix);
    const auto before_delta = legacy_signed - wall_before;
    const auto after_delta = legacy_signed - wall_after;
    assert_true((before_delta >= -tolerance_ms && before_delta <= tolerance_ms) ||
                    (after_delta >= -tolerance_ms && after_delta <= tolerance_ms),
                "unix_ms should reflect the system-clock Unix epoch within tolerance");
  }
} // namespace

int main()
{
  test_utc_tm_fixed_values();
  test_iso8601_shape();
  test_rfc1123_shape_in_classic_locale();
  test_clock_contracts();
  return EXIT_SUCCESS;
}

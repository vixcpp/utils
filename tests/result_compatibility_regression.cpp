#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <vix/utils/Result.hpp>
#include <vix/utils/Validation.hpp>

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

  void test_ok_err_and_access()
  {
    using Result = vix::utils::Result<int, std::string>;

    const auto ok = Result::Ok(42);
    assert_true(ok.is_ok(), "Ok result should report is_ok()");
    assert_true(!ok.is_err(), "Ok result should not report is_err()");
    assert_true(ok.value() == 42, "Ok result should retain its value");

    const auto err = Result::Err("legacy failure");
    assert_true(!err.is_ok(), "Err result should not report is_ok()");
    assert_true(err.is_err(), "Err result should report is_err()");
    assert_true(err.error() == "legacy failure",
                "Err result should retain its typed error");
  }

  void test_from_factories_copy_and_move()
  {
    using Result = vix::utils::Result<std::string, std::string>;

    const std::string value = "value";
    const std::string error = "error";
    const auto from_ok = Result::FromOk(value);
    const auto from_err = Result::FromErr(error);
    assert_true(from_ok.value() == value, "FromOk should copy a value");
    assert_true(from_err.error() == error, "FromErr should copy an error");

    auto copied = from_ok;
    assert_true(copied.is_ok() && copied.value() == value,
                "copy construction should preserve the active value");

    Result assigned = Result::Err("initial error");
    assigned = from_ok;
    assert_true(assigned.is_ok() && assigned.value() == value,
                "copy assignment should replace the active member");

    auto movable = Result::FromOk(std::string("moved value"));
    Result moved(std::move(movable));
    assert_true(moved.is_ok() && moved.value() == "moved value",
                "move construction should preserve the active value");

    assigned = Result::FromErr(std::string("moved error"));
    assert_true(assigned.is_err() && assigned.error() == "moved error",
                "move assignment should replace the active member");
  }

  void test_void_and_typed_field_errors()
  {
    using FieldErrors = vix::utils::FieldErrors;
    using Result = vix::utils::Result<void, FieldErrors>;

    const auto ok = Result::Ok();
    assert_true(ok.is_ok(), "Result<void, E>::Ok should report success");

    FieldErrors errors{{"email", "invalid format"}};
    const auto err = Result::Err(errors);
    assert_true(err.is_err(), "Result<void, E>::Err should report failure");
    assert_true(err.error().at("email") == "invalid format",
                "Result<void, E> should retain the typed error object");
  }

  void test_public_tag_symbols()
  {
    static_assert(std::is_default_constructible_v<vix::utils::OkTag>);
    static_assert(std::is_default_constructible_v<vix::utils::ErrTag>);
    static_assert(std::is_same_v<decltype(vix::utils::OkTag_v),
                                 const vix::utils::OkTag>);
    static_assert(std::is_same_v<decltype(vix::utils::ErrTag_v),
                                 const vix::utils::ErrTag>);
  }
} // namespace

int main()
{
  test_ok_err_and_access();
  test_from_factories_copy_and_move();
  test_void_and_typed_field_errors();
  test_public_tag_symbols();
  return EXIT_SUCCESS;
}

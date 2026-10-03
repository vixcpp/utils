#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>

#include <vix/utils/Validation.hpp>

namespace
{
  using vix::utils::FieldErrors;
  using vix::utils::Result;
  using vix::utils::Rule;
  using vix::utils::Schema;

  void assert_true(bool condition, const char *message)
  {
    if (!condition)
    {
      std::cerr << "Assertion failed: " << message << '\n';
      std::exit(EXIT_FAILURE);
    }
  }

  [[nodiscard]] Result<void, FieldErrors> validate(
      const std::unordered_map<std::string, std::string> &data,
      const Schema &schema)
  {
    return vix::utils::validate_map(data, schema);
  }

  void test_presence_and_unknown_field_behavior()
  {
    const Schema required_name{{"name", vix::utils::required("Name")}};

    const auto missing = validate({}, required_name);
    assert_true(missing.is_err(), "missing required field should fail");
    assert_true(missing.error().at("name") == "Name is required",
                "missing required field should retain the historical message");

    const auto empty = validate({{"name", ""}}, required_name);
    assert_true(empty.is_err(), "empty required field should fail");
    assert_true(empty.error().at("name") == "Name is required",
                "empty required field should match missing-field behavior");

    const auto whitespace = validate({{"name", " \t"}}, required_name);
    assert_true(whitespace.is_ok(),
                "whitespace-only required field should remain present historically");

    const Schema optional_length{{"nickname", vix::utils::len(3, 5, "Nickname")}};
    const auto optional_empty = validate({{"nickname", ""}}, optional_length);
    assert_true(optional_empty.is_ok(),
                "empty non-required field should skip later validation");

    const auto unknown = validate({{"unexpected", "value"}}, optional_length);
    assert_true(unknown.is_ok(), "unknown input fields should be ignored");
  }

  void test_rule_order_and_length_bounds()
  {
    Rule ordered = vix::utils::len(5, 10, "Code");
    ordered.pattern = std::regex("[A-Z]+");
    const Schema first_failure{{"code", ordered}};

    const auto failed = validate({{"code", "a"}}, first_failure);
    assert_true(failed.is_err(), "invalid code should fail validation");
    assert_true(failed.error().size() == 1,
                "only the first failing rule should produce an error per field");
    assert_true(failed.error().at("code") == "Code must be at least 5 chars",
                "length must run before regex for a field");

    const Schema lengths{{"value", vix::utils::len(3, 5, "Value")}};
    assert_true(validate({{"value", "abc"}}, lengths).is_ok(),
                "minimum length boundary should be inclusive");
    assert_true(validate({{"value", "abcde"}}, lengths).is_ok(),
                "maximum length boundary should be inclusive");
  }

  void test_numeric_behavior()
  {
    const Schema range{{"age", vix::utils::num_range(-5, 10, "Age")}};

    assert_true(validate({{"age", "-5"}}, range).is_ok(),
                "signed lower boundary should parse and be inclusive");
    assert_true(validate({{"age", "10"}}, range).is_ok(),
                "upper boundary should be inclusive");

    const auto malformed = validate({{"age", "ten"}}, range);
    assert_true(malformed.is_err() &&
                    malformed.error().at("age") == "Age must be a number",
                "malformed numeric text should use the historical numeric failure");

    const auto partial = validate({{"age", "10x"}}, range);
    assert_true(partial.is_err() &&
                    partial.error().at("age") == "Age must be a number",
                "partial numeric text should use the historical numeric failure");

    const auto overflow = validate(
        {{"age", "999999999999999999999999999999999999"}}, range);
    assert_true(overflow.is_err() &&
                    overflow.error().at("age") == "Age must be a number",
                "out-of-range numeric text should use the historical numeric failure");
  }

  void test_regex_and_multiple_field_behavior()
  {
    const Schema format{{"code", vix::utils::match("[A-Z]{2}[0-9]{2}", "Code")}};
    assert_true(validate({{"code", "AB12"}}, format).is_ok(),
                "full regex match should succeed");

    const auto mismatch = validate({{"code", "AB1"}}, format);
    assert_true(mismatch.is_err() &&
                    mismatch.error().at("code") == "Code has invalid format",
                "regex mismatch should use the historical format failure");

    const Schema multiple{
        {"name", vix::utils::required("Name")},
        {"age", vix::utils::num_range(18, 120, "Age")}};
    const auto failures = validate({{"name", ""}, {"age", "invalid"}}, multiple);
    assert_true(failures.is_err(), "multiple invalid schema fields should fail");
    assert_true(failures.error().size() == 2,
                "different fields should retain separate FieldErrors");
    assert_true(failures.error().at("name") == "Name is required",
                "FieldErrors should retain the name error");
    assert_true(failures.error().at("age") == "Age must be a number",
                "FieldErrors should retain the age error");
  }

  void test_invalid_regex_construction()
  {
    bool threw = false;
    try
    {
      [[maybe_unused]] const auto invalid = vix::utils::match("[");
    }
    catch (const std::regex_error &)
    {
      threw = true;
    }

    assert_true(threw, "invalid regex should throw while constructing a rule");
  }

  void test_result_contract()
  {
    const Schema valid_schema{{"name", vix::utils::required("Name")}};
    const auto success = validate({{"name", "Ada"}}, valid_schema);
    assert_true(success.is_ok(),
                "successful validation should return Result<void, FieldErrors>::Ok");

    const auto failure = validate({}, valid_schema);
    assert_true(failure.is_err(),
                "failed validation should return Result<void, FieldErrors>::Err");
    const FieldErrors &errors = failure.error();
    assert_true(errors.at("name") == "Name is required",
                "failed validation should expose typed FieldErrors");
  }
} // namespace

int main()
{
  test_presence_and_unknown_field_behavior();
  test_rule_order_and_length_bounds();
  test_numeric_behavior();
  test_regex_and_multiple_field_behavior();
  test_invalid_regex_construction();
  test_result_contract();
  return EXIT_SUCCESS;
}

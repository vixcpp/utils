#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include <vix/utils/String.hpp>

namespace
{
  void test_generic_string_helpers()
  {
    using vix::utils::ends_with;
    using vix::utils::join;
    using vix::utils::split;
    using vix::utils::starts_with;
    using vix::utils::to_lower;
    using vix::utils::trim;

    assert(trim(" \t\nvalue\r ") == "value");
    assert(trim("").empty());
    assert(trim(" \t\n\v\f\r").empty());
    assert(to_lower("Vix-HTTP") == "vix-http");

    assert(starts_with("vix-core", "vix"));
    assert(!starts_with("vix", "vix-core"));
    assert(ends_with("request.hpp", ".hpp"));
    assert(!ends_with("request.hpp", ".cpp"));

    assert((split(",a,,b,", ',') ==
            std::vector<std::string>{"", "a", "", "b", ""}));
    assert((split("a--b----c", "--") ==
            std::vector<std::string>{"a", "b", "", "c"}));
    assert((split("value", std::string_view{}) ==
            std::vector<std::string>{"value"}));

    assert(join({}, "::").empty());
    assert(join({"only"}, "::") == "only");
    assert(join({"a", "", "c"}, "::") == "a::::c");
  }

  void test_url_decode_contract()
  {
    using vix::utils::url_decode;

    assert(url_decode("first+last") == "first last");
    assert(url_decode("%41%62%2F%7e") == "Ab/~");
    assert(url_decode("A%00B") == std::string("A\0B", 3));
    assert(url_decode("%G1") == "%G1");
    assert(url_decode("%") == "%");
    assert(url_decode("%A") == "%A");
  }

  void test_query_string_contract()
  {
    const auto query = vix::utils::parse_query_string(
        "name=Ada+Lovelace&&escaped=%41%62&expression=a=b=c&empty=&=ignored"
        "&duplicate=first&duplicate=last&invalid=%G1&incomplete=%");

    assert(query.at("name") == "Ada Lovelace");
    assert(query.at("escaped") == "Ab");
    assert(query.at("expression") == "a=b=c");
    assert(query.contains("empty"));
    assert(query.at("empty").empty());
    assert(!query.contains(""));
    assert(query.at("duplicate") == "last");
    assert(query.at("invalid") == "%G1");
    assert(query.at("incomplete") == "%");
  }

  void test_http_oriented_compatibility_helpers()
  {
    using vix::utils::extract_boundary;
    using vix::utils::starts_with_icase;

    assert(starts_with_icase("Application/Json", "application/"));
    assert(!starts_with_icase("text/plain", "application/"));
    assert(extract_boundary("multipart/form-data; boundary=----VixBoundary") ==
           "----VixBoundary");
    assert(extract_boundary("multipart/form-data; boundary= \"quoted boundary\"; charset=utf-8") ==
           "quoted boundary");
    assert(extract_boundary("application/json").empty());
  }
} // namespace

int main()
{
  test_generic_string_helpers();
  test_url_decode_contract();
  test_query_string_contract();
  test_http_oriented_compatibility_helpers();
  return 0;
}

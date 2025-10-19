// Catch2 v3
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "navary/result.h"
#include "navary/resultcode.h"

using namespace navary;

// Simple helpers for int-coded results
constexpr ResultInt ErrInt(int code, const char* msg) {
  return ResultInt(code, msg);
}

TEST_CASE("ResultInt works with 0 as OK", "[result][int]") {
  constexpr ResultInt ok;
  static_assert(ok.ok(), "int result default ok");

  ResultInt e = ErrInt(5, "custom error");
  REQUIRE_FALSE(e.ok());
  REQUIRE(e.code() == 5);
  REQUIRE(std::string(e.message()) == "custom error");
}

TEST_CASE("ResultOrInt composes with NAV_* macros", "[statusor][int][macros]") {
  auto parse_digit = [](char c) -> ResultOrInt<int> {
    if (c >= '0' && c <= '9')
      return static_cast<int>(c - '0');
    return ResultInt(1, "not a digit");
  };

  auto add_two = [&](char a, char b) -> ResultOrInt<int> {
    int va = 0, vb = 0;
    NVR_ASSIGN_OR_RETURN(va, parse_digit(a));
    NVR_ASSIGN_OR_RETURN(vb, parse_digit(b));
    long long sum = static_cast<long long>(va) + vb;
    if (sum > 9)
      return ResultInt(2, "sum too big");
    return static_cast<int>(sum);
  };

  {
    auto s = add_two('3', '4');
    REQUIRE(s.ok());
    REQUIRE(s.value() == 7);
  }
  {
    auto s = add_two('x', '4');
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.status().code() == 1);
  }
  {
    auto s = add_two('9', '9');
    REQUIRE_FALSE(s.ok());
    REQUIRE(s.status().code() == 2);
  }
}
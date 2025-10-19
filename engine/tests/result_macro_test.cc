#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cctype>
#include <limits>
#include <string>
#include <string_view>

#include "navary/result.h"
#include "navary/resultcode.h"

using namespace navary;
using Catch::Matchers::ContainsSubstring;

static ResultRC ValidateDigits(std::string_view s) {
  if (s.empty()) return Invalid("empty");
  for (unsigned char c : s) if (!std::isdigit(c)) return Invalid("nondigit");
  return ResultRC::OK();
}

static ResultOrRC<int> ParseInt(std::string_view s) {
  NVR_RETURN_IF_ERROR(ValidateDigits(s));
  long long v = 0;
  for (unsigned char c : s) {
    v = v * 10 + (c - '0');
    if (v > std::numeric_limits<int>::max()) return Failed("overflow");
  }
  return static_cast<int>(v);
}

static ResultOrRC<int> Add(std::string_view a, std::string_view b) {
  int x = 0, y = 0;
  NVR_ASSIGN_OR_RETURN(x, ParseInt(a));
  NVR_ASSIGN_OR_RETURN(y, ParseInt(b));
  long long s = static_cast<long long>(x) + y;
  if (s > std::numeric_limits<int>::max()) return Failed("sum overflow");
  return static_cast<int>(s);
}

TEST_CASE("NAV_* macros short-circuit and propagate", "[macros]") {
  auto r1 = Add("12", "30");
  REQUIRE(r1.ok());
  REQUIRE(r1.value() == 42);

  auto r2 = Add("12x", "30");
  REQUIRE_FALSE(r2.ok());
  REQUIRE(r2.status().code() == ResultCode::kInvalidArgument);
  REQUIRE_THAT(std::string(r2.status().message()), ContainsSubstring("nondigit"));
}

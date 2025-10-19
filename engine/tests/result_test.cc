// Catch2 v3
#include "navary/result.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "navary/resultcode.h"

using namespace navary;
using Catch::Matchers::ContainsSubstring;

static_assert(ResultRC().ok(), "Default ResultRC must be OK");

TEST_CASE("ResultRC basics", "[result][enum]") {
  ResultRC r_ok;
  REQUIRE(r_ok.ok());
  REQUIRE(r_ok.code() == ResultCode::kOk);
  REQUIRE(std::string(r_ok.message()).empty());

  ResultRC r_bad(ResultCode::kInvalidArgument, "bad arg");
  REQUIRE_FALSE(r_bad.ok());
  REQUIRE(r_bad.code() == ResultCode::kInvalidArgument);
  REQUIRE(std::string(r_bad.message()) == "bad arg");

  REQUIRE(Invalid("x").code() == ResultCode::kInvalidArgument);
  REQUIRE(Failed("x").code() == ResultCode::kInternal);
  REQUIRE(Unavail("x").code() == ResultCode::kUnavailable);
  REQUIRE(NotFound("x").code() == ResultCode::kNotFound);
}

TEST_CASE("ResultOrRC value vs error", "[statusor][enum]") {
  ResultOrRC<int> a(42);
  REQUIRE(a.ok());
  REQUIRE(a.value() == 42);

  ResultOrRC<int> b(Invalid("nope"));
  REQUIRE_FALSE(b.ok());
  REQUIRE(b.status().code() == ResultCode::kInvalidArgument);
  REQUIRE_THAT(std::string(b.status().message()), ContainsSubstring("nope"));
}

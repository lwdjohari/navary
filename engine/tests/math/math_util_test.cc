#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "navary/math/math_util.h"
#include "navary/math/vec3.h"

using navary::math::Clamp;
using navary::math::DegToRad;
using navary::math::Lerp;
using navary::math::RadToDeg;
using navary::math::Sign;
using navary::math::SmoothStep;
using navary::math::Sqr;
using navary::math::Vec3;

TEST_CASE("math_util: deg/rad round-trip", "[math][angles]") {
  REQUIRE(RadToDeg(DegToRad(0.f)) == Catch::Approx(0.f));
  REQUIRE(RadToDeg(DegToRad(90.f)) == Catch::Approx(90.f));
  REQUIRE(RadToDeg(DegToRad(180.f)) == Catch::Approx(180.f));
  REQUIRE(RadToDeg(DegToRad(-45.f)) == Catch::Approx(-45.f));
}

TEST_CASE("math_util: Sqr/Clamp/Sign basics", "[math][basics]") {
  REQUIRE(Sqr(3) == 9);
  REQUIRE(Sqr(-4) == 16);

  REQUIRE(Clamp(5, 0, 10) == 5);
  REQUIRE(Clamp(-2, 0, 10) == 0);
  REQUIRE(Clamp(42, 0, 10) == 10);

  REQUIRE(Sign(5) == 1);
  REQUIRE(Sign(-7) == -1);
  REQUIRE(Sign(0) == 0);
}

TEST_CASE("math_util: Lerp with vectors for camera blend",
          "[math][lerp][vec]") {
  Vec3 a{0, 0, 0};
  Vec3 b{10, 5, -2};

  auto p0 = Lerp(a, b, 0.0f);
  auto p5 = Lerp(a, b, 0.5f);
  auto p1 = Lerp(a, b, 1.0f);

  REQUIRE(p0.x == Catch::Approx(0.f));
  REQUIRE(p0.y == Catch::Approx(0.f));
  REQUIRE(p0.z == Catch::Approx(0.f));

  REQUIRE(p5.x == Catch::Approx(5.f));
  REQUIRE(p5.y == Catch::Approx(2.5f));
  REQUIRE(p5.z == Catch::Approx(-1.f));

  REQUIRE(p1.x == Catch::Approx(10.f));
  REQUIRE(p1.y == Catch::Approx(5.f));
  REQUIRE(p1.z == Catch::Approx(-2.f));
}

TEST_CASE("math_util: SmoothStep shape and endpoint tangents",
          "[math][smoothstep]") {
  // Endpoints
  REQUIRE(SmoothStep(0.f, 1.f, 0.f) == Catch::Approx(0.f).margin(1e-6f));
  REQUIRE(SmoothStep(0.f, 1.f, 1.f) == Catch::Approx(1.f).margin(1e-6f));

  // Monotonic increase
  float prev = 0.f;
  for (int i = 0; i <= 100; ++i) {
    float x = i / 100.0f;
    float y = SmoothStep(0.f, 1.f, x);
    REQUIRE(y >= prev - 1e-6f);
    prev = y;
  }

  // Near-zero derivative at ends (finite difference)
  auto dydx = [](float x) {
    const float h = 1e-3f;
    return (SmoothStep(0.f, 1.f, x + h) - SmoothStep(0.f, 1.f, x - h)) /
           (2 * h);
  };
  REQUIRE(std::fabs(dydx(0.0f)) < 1e-2f);
  REQUIRE(std::fabs(dydx(1.0f)) < 1e-2f);
}

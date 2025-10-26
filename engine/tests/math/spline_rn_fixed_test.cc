#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/spline_fixed.h"
#include "navary/math/fixed_vec3.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

namespace {

inline float F2f(Fixed15p16 v) {
  return v.ToFloat();
}

inline float Vx(const FixedVec3& v) {
  return v.x.ToFloat();
}

inline float Vy(const FixedVec3& v) {
  return v.y.ToFloat();
}

inline float Vz(const FixedVec3& v) {
  return v.z.ToFloat();
}

inline FixedVec3 F3(float x, float y, float z) {
  return FixedVec3{Fixed15p16::FromFloat(x), Fixed15p16::FromFloat(y),
                   Fixed15p16::FromFloat(z)};
}
}  // namespace

TEST_CASE("RnFixed: endpoints, clamping, monotonicity on linear rail",
          "[spline][fixed][rn]") {
  RnSplineFixed s;
  s.AddNode(F3(0, 0, 0));
  s.AddNode(F3(10, 0, 0));
  s.Build();

  // Endpoints
  {
    auto p0 = s.GetPosition(Fixed15p16::FromInt(0));
    auto p1 = s.GetPosition(Fixed15p16::FromInt(1));
    REQUIRE_THAT(Vx(p0), WithinAbs(0.f, 1e-5f));
    REQUIRE_THAT(Vx(p1), WithinAbs(10.f, 1e-3f));
  }

  // Clamping
  {
    auto pn = s.GetPosition(Fixed15p16::FromFloat(-1.0f));
    auto pp = s.GetPosition(Fixed15p16::FromFloat(2.0f));
    REQUIRE_THAT(Vx(pn), WithinAbs(0.f, 1e-5f));
    REQUIRE_THAT(Vx(pp), WithinAbs(10.f, 1e-3f));
  }

  // Monotonic X
  {
    float last = -1e9f;
    for (int i = 0; i <= 20; ++i) {
      float t = i / 20.f;
      auto p  = s.GetPosition(Fixed15p16::FromFloat(t));
      REQUIRE(Vx(p) >= last - 1e-4f);
      last = Vx(p);
    }
  }
}

TEST_CASE(
    "RnFixed: patrol L-rail (constant-length param) - midpoints on each leg",
    "[spline][fixed][rn]") {
  // Path: (0,0,0) -> (5,0,0) -> (5,0,5)
  RnSplineFixed s;
  s.AddNode(F3(0, 0, 0));
  s.AddNode(F3(5, 0, 0));
  s.AddNode(F3(5, 0, 5));
  s.Build();

  // Total length = 10. First half of timeline covers first 5 units.
  auto p_quarter = s.GetPosition(Fixed15p16::FromFloat(0.25f));  // ~x in (2..3)
  auto p_half    = s.GetPosition(Fixed15p16::FromFloat(0.5f));  // at corner-ish
  auto p_3quarter =
      s.GetPosition(Fixed15p16::FromFloat(0.75f));  // ~z in (2..3)

  REQUIRE(Vx(p_quarter) > 1.5f);
  REQUIRE(Vx(p_quarter) < 4.9f);  // hasn’t reached end of first leg

  REQUIRE(Vx(p_half) >= 4.6f);  // around the corner vicinity
  REQUIRE(Vz(p_half) >= -0.1f);

  REQUIRE(Vx(p_3quarter) >= 4.6f);  // on second leg, x ~5
  REQUIRE(Vz(p_3quarter) > 1.5f);
  REQUIRE(Vz(p_3quarter) < 4.9f);
}

TEST_CASE("RnFixed: degenerate segments handled (duplicate nodes)",
          "[spline][fixed][rn]") {
  RnSplineFixed s;
  s.AddNode(F3(0, 0, 0));
  s.AddNode(F3(0, 0, 0));  // duplicate
  s.AddNode(F3(3, 0, 0));
  s.Build();

  // Should still progress along the non-degenerate span.
  auto p0 = s.GetPosition(Fixed15p16::FromFloat(0.0f));
  auto pm = s.GetPosition(Fixed15p16::FromFloat(0.5f));
  auto p1 = s.GetPosition(Fixed15p16::FromFloat(1.0f));

  REQUIRE_THAT(Vx(p0), WithinAbs(0.f, 1e-5f));
  REQUIRE(Vx(pm) >= 0.f);
  REQUIRE(Vx(p1) >= 2.9f);
}

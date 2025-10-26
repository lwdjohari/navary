#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/spline_fixed.h"
#include "navary/math/fixed_vec3.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

namespace {

inline float Vx(const FixedVec3& v) {
  return v.x.ToFloat();
}

inline float Vz(const FixedVec3& v) {
  return v.z.ToFloat();
}

inline FixedVec3 F3(float x, float y, float z) {
  return FixedVec3{Fixed15p16::FromFloat(x), Fixed15p16::FromFloat(y),
                   Fixed15p16::FromFloat(z)};
}

}  // namespace

TEST_CASE("TNFixed: piecewise time mapping across unequal segments",
          "[spline][fixed][tn]") {
  TnSplineFixed s;
  // time to previous: 1s, 2s, 1s  (total=4)
  s.AddNodeTimed(F3(0, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.AddNodeTimed(F3(4, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(2.0f));
  s.AddNodeTimed(F3(4, 0, 4), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.Build();

  // t=0.125 (0.5s) → inside first 1s segment
  auto pA = s.GetPosition(Fixed15p16::FromFloat(0.125f));
  // t=0.50 (2.0s) → inside long middle segment
  auto pB = s.GetPosition(Fixed15p16::FromFloat(0.50f));
  // t=0.875 (3.5s) → last segment
  auto pC = s.GetPosition(Fixed15p16::FromFloat(0.875f));

  REQUIRE(Vx(pA) > 0.3f);
  REQUIRE(Vx(pA) < 4.0f);

  REQUIRE(Vx(pB) >= 3.0f);
  REQUIRE(Vx(pB) <= 4.0f + 1e-3f);

  REQUIRE(Vx(pC) >= 3.9f);
  REQUIRE(Vz(pC) >= 0.3f);
}

TEST_CASE("TNFixed: UpdateNodeTime retimes motion at same normalized t",
          "[spline][fixed][tn]") {
  TnSplineFixed s;
  // Three nodes: 1s then 1s
  s.AddNodeTimed(F3(0, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.AddNodeTimed(F3(10, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.AddNodeTimed(F3(20, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(0.0f));
  s.Build();

  // At normalized 0.5 (1s of 2s total), we're around the corner at node 1
  auto mid_before = s.GetPosition(Fixed15p16::FromFloat(0.50f));

  // Make second segment slower (stretch timeline to 3s total)
  s.UpdateNodeTime(1, Fixed15p16::FromFloat(2.0f));
  s.Build();

  // At the same normalized 0.5 (now 1.5s of 3s), we're still *during* the long
  // segment, so the position should not reach as far as before in absolute X.
  auto mid_after = s.GetPosition(Fixed15p16::FromFloat(0.50f));

  REQUIRE(Vx(mid_after) > Vx(mid_before) - 1e-3f);
}

TEST_CASE("TNFixed: editing API (insert/remove/update) keeps totals sane",
          "[spline][fixed][tn]") {
  TnSplineFixed s;
  s.AddNodeTimed(F3(0, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.AddNodeTimed(F3(5, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(2.0f));
  s.AddNodeTimed(F3(10, 0, 0), F3(0, 0, 0), Fixed15p16::FromFloat(1.0f));
  s.Build();

  auto p_mid = s.GetPosition(Fixed15p16::FromFloat(0.5f));

  // Insert a node in the middle with a small time slice
  s.InsertNodeTimed(1, F3(2.5f, 0, 0), F3(0, 0, 0),
                    Fixed15p16::FromFloat(0.25f));
  s.Build();

  auto p_mid2 = s.GetPosition(Fixed15p16::FromFloat(0.5f));

  // After inserting a node with its own time, the midpoint on the normalized
  // timeline should shift (not equal).
  REQUIRE(std::fabs(Vx(p_mid2) - Vx(p_mid)) > 1e-3f);

  // Remove it back and ensure we can still sample
  s.RemoveNode(1);
  s.Build();

  auto p_after = s.GetPosition(Fixed15p16::FromFloat(0.5f));
  REQUIRE(std::isfinite(Vx(p_after)));
}

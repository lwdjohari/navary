#include <catch2/catch_all.hpp>
#include "navary/math/vec3.h"
#include "navary/math/spline.h"

using namespace navary::math;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-3f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

TEST_CASE("TN: Timed nodes - piecewise time mapping", "[spline][tn]") {
  TnSpline s;

  // 3 nodes → 2 segments. Times: seg0=1.0s, seg1=2.0s, total = 3.0s
  s.AddNodeTimed(Vec3{0, 0, 0}, Vec3{0, 0, 0}, /*ignored*/ 0.0f);
  s.AddNodeTimed(Vec3{4, 0, 0}, Vec3{0, 0, 0}, /*seg0*/ 1.0f);
  s.AddNodeTimed(Vec3{4, 0, 4}, Vec3{0, 0, 0}, /*seg1*/ 2.0f);
  s.Build();

  REQUIRE(V3Close(s.GetPosition(0.0f), Vec3{0, 0, 0}));
  REQUIRE(V3Close(s.GetPosition(1.0f), Vec3{4, 0, 4}));

  // Early (inside short seg0)
  Vec3 pA = s.GetPosition(0.20f);  // ~0.6s of 3.0s
  REQUIRE(pA.x > 0.5f);
  REQUIRE(std::fabs(pA.z) < 0.5f);

  // Middle (inside long seg1)
  Vec3 pB = s.GetPosition(0.50f);  // 1.5s of 3.0s
  REQUIRE(pB.x >= 3.0f);
  REQUIRE(pB.z >= 0.1f);

  // Late (near end)
  Vec3 pC = s.GetPosition(0.95f);  // 2.85s of 3.0s
  REQUIRE(pC.x >= 3.5f);
  REQUIRE(pC.z >= 2.5f);
}

TEST_CASE("TN: UpdateNodeTime compares positions at the same absolute time",
          "[spline][tn]") {
  TnSpline s;
  s.AddNodeTimed(Vec3{0, 0, 0}, Vec3{0, 0, 0}, /*ignored*/ 0.0f);
  s.AddNodeTimed(Vec3{10, 0, 0}, Vec3{0, 0, 0}, /*seg0*/ 1.0f);
  s.Build();

  const float total_before = s.TotalParam();  // 1.0
  REQUIRE(total_before > 0.f);

  // Pick an absolute time (e.g., 0.5s) and get position before retime
  const float t_abs         = 0.5f;
  const float t_norm_before = t_abs / total_before;
  Vec3 p_before             = s.GetPosition(t_norm_before);

  // Slow the segment to 3.0s, rebuild
  s.UpdateNodeTime(1, 3.0f);  // index 1 => segment 0->1
  s.Build();

  const float total_after = s.TotalParam();  // 3.0
  REQUIRE(total_after > total_before);

  // Same absolute time, new normalized coordinate
  const float t_norm_after = t_abs / total_after;
  Vec3 p_after             = s.GetPosition(t_norm_after);

  // At the same absolute time along the same straight segment,
  // positions should closely match.
  REQUIRE(V3Close(p_before, p_after, 1e-3f));
}

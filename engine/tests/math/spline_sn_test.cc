#include <catch2/catch_all.hpp>
#include "navary/math/vec3.h"
#include "navary/math/spline.h"

using namespace navary::math;

// Finite-difference tangent estimate at normalized t
static Vec3 TangentAt(const RnSpline& s, float t) {
  constexpr float h = 1e-3f;
  float t0          = std::max(0.0f, t - h);
  float t1          = std::min(1.0f, t + h);
  Vec3 p0           = s.GetPosition(t0);
  Vec3 p1           = s.GetPosition(t1);
  Vec3 v            = p1 - p0;
  float L           = v.length();
  return (L > 0.f) ? (v / L) : Vec3{0, 0, 0};
}

static float JumpMagnitudeAtKnot(const RnSpline& s, float t_knot) {
  // Measure left/right unit tangents around the knot param and return their
  // difference length
  constexpr float eps = 5e-3f;
  float tL            = std::max(0.0f, t_knot - eps);
  float tR            = std::min(1.0f, t_knot + eps);
  Vec3 vL             = TangentAt(s, tL);
  Vec3 vR             = TangentAt(s, tR);
  return (vR - vL).length();
}

TEST_CASE("SN: Reduces derivative jump at interior knots vs RN",
          "[spline][sn]") {
  // Zig-zag path to create a real interior corner
  std::vector<Vec3> pts = {{0, 0, 0}, {4, 0, 0}, {4, 0, 3}, {8, 0, 3}};

  RnSpline rn;
  for (auto& p : pts)
    rn.AddNode(p);
  rn.Build();

  SnSpline sn;
  for (auto& p : pts)
    sn.AddNode(p);
  sn.Build();

  // Knot between node1 and node2 is roughly in the middle of normalized time.
  // We don’t know exact mapping, so probe a band around 0.5 and pick the worst
  // jump.
  float worst_rn = 0.f, worst_sn = 0.f;
  for (float t = 0.4f; t <= 0.6f; t += 0.02f) {
    worst_rn = std::max(worst_rn, JumpMagnitudeAtKnot(rn, t));
    worst_sn = std::max(worst_sn, JumpMagnitudeAtKnot(sn, t));
  }

  // Smoothing should reduce the derivative discontinuity near the corner.
  REQUIRE(worst_sn <= worst_rn + 1e-3f);
  REQUIRE(worst_sn <=
          0.95f * worst_rn + 1e-3f);  // typically noticeably smaller
}

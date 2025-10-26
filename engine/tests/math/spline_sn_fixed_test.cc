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

inline float AngleBetweenXZ(const FixedVec3& a, const FixedVec3& b) {
  // Compute angle in XZ-plane between (a) and (b)
  const float ax = Vx(a), az = Vz(a);
  const float bx = Vx(b), bz = Vz(b);
  const float la = std::sqrt(ax * ax + az * az);
  const float lb = std::sqrt(bx * bx + bz * bz);
  if (la <= 1e-7f || lb <= 1e-7f)
    return 0.f;
  float c = (ax * bx + az * bz) / (la * lb);
  c       = std::clamp(c, -1.0f, 1.0f);
  return std::acos(c);
}

}  // namespace

TEST_CASE("SNFixed: Same path, smoother corner than RN",
          "[spline][fixed][sn]") {
  // L-shaped rail; measure turning angle at the knee via finite difference
  RnSplineFixed rn;
  rn.AddNode(F3(0, 0, 0));
  rn.AddNode(F3(5, 0, 0));
  rn.AddNode(F3(5, 0, 5));
  rn.Build();

  SnSplineFixed sn;
  sn.AddNode(F3(0, 0, 0));
  sn.AddNode(F3(5, 0, 0));
  sn.AddNode(F3(5, 0, 5));
  sn.Build();

  const Fixed15p16 t  = Fixed15p16::FromFloat(0.5f);  // around the corner
  const Fixed15p16 dt = Fixed15p16::FromFloat(0.05f);

  // Discrete directions around t
  auto rn_p0 = rn.GetPosition(t - dt);
  auto rn_p1 = rn.GetPosition(t + dt);
  auto rn_dir =
      FixedVec3{rn_p1.x - rn_p0.x, rn_p1.y - rn_p0.y, rn_p1.z - rn_p0.z};

  auto sn_p0 = sn.GetPosition(t - dt);
  auto sn_p1 = sn.GetPosition(t + dt);
  auto sn_dir =
      FixedVec3{sn_p1.x - sn_p0.x, sn_p1.y - sn_p0.y, sn_p1.z - sn_p0.z};

  // Compare with directions from first leg and second leg
  FixedVec3 legX{Fixed15p16::FromInt(1), Fixed15p16::Zero(),
                 Fixed15p16::Zero()};
  FixedVec3 legZ{Fixed15p16::Zero(), Fixed15p16::Zero(),
                 Fixed15p16::FromInt(1)};

  float rn_angle =
      std::min(AngleBetweenXZ(rn_dir, legX), AngleBetweenXZ(rn_dir, legZ));
  float sn_angle =
      std::min(AngleBetweenXZ(sn_dir, legX), AngleBetweenXZ(sn_dir, legZ));

  // SN should deviate less sharply (smaller angle to one of the legs).
  REQUIRE(sn_angle <= rn_angle + 1e-3f);
}

TEST_CASE("SNFixed: Endpoints preserved and rail coverage similar to RN",
          "[spline][fixed][sn]") {
  RnSplineFixed rn;
  rn.AddNode(F3(0, 0, 0));
  rn.AddNode(F3(10, 0, 0));
  rn.Build();

  SnSplineFixed sn;
  sn.AddNode(F3(0, 0, 0));
  sn.AddNode(F3(10, 0, 0));
  sn.Build();

  auto rn0 = rn.GetPosition(Fixed15p16::FromInt(0));
  auto rn1 = rn.GetPosition(Fixed15p16::FromInt(1));
  auto sn0 = sn.GetPosition(Fixed15p16::FromInt(0));
  auto sn1 = sn.GetPosition(Fixed15p16::FromInt(1));

  REQUIRE_THAT(Vx(rn0), WithinAbs(Vx(sn0), 1e-5f));
  REQUIRE_THAT(Vx(rn1), WithinAbs(Vx(sn1), 1e-4f));
}

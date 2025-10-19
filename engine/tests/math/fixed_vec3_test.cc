#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"

using navary::math::Fixed15p16;
using navary::math::FixedVec3;

static inline double dval(Fixed15p16 f) {
  return f.ToDouble();
}
static inline Fixed15p16 F(double v) {
  return Fixed15p16::FromDouble(v);
}

TEST_CASE("FixedVec3: cross product orthogonality", "[fixed][vec3][cross]") {
  FixedVec3 x{F(1), F(0), F(0)};
  FixedVec3 y{F(0), F(1), F(0)};
  auto z = FixedVec3::Cross(x, y);
  // z ~ (0,0,1)
  REQUIRE(dval(z.x) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(z.y) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(z.z) == Catch::Approx(1.0).margin(1e-6));
  // Orthogonality: dot ~ 0
  REQUIRE(dval(FixedVec3::Dot(z, x)) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(FixedVec3::Dot(z, y)) == Catch::Approx(0.0).margin(1e-6));
}

TEST_CASE("FixedVec3: normal from triangle points", "[fixed][vec3][normal]") {
  FixedVec3 p0{F(0), F(0), F(0)};
  FixedVec3 p1{F(1), F(0), F(0)};
  FixedVec3 p2{F(0), F(1), F(0)};
  auto n = FixedVec3::Cross(p1 - p0, p2 - p0);
  n.Normalize();
  REQUIRE(dval(n.x) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(n.y) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(n.z) == Catch::Approx(1.0).margin(1e-5));
}

TEST_CASE("FixedVec3: camera basis orthonormality (forward/right/up)",
          "[fixed][vec3][camera]") {
  // Forward ~ -Z, Up ~ +Y
  FixedVec3 fwd{F(0), F(0), F(-1)};
  FixedVec3 up{F(0), F(1), F(0)};
  // Right = normalize(cross(up, fwd))
  auto right = FixedVec3::Cross(up, fwd);
  right.Normalize();
  // Re-orthogonalize up = cross(fwd, right)
  up = FixedVec3::Cross(fwd, right);
  up.Normalize();

  // Orthonormal checks (within fixed tolerance)
  REQUIRE(dval(FixedVec3::Dot(right, up)) == Catch::Approx(0.0).margin(2e-4));
  REQUIRE(dval(FixedVec3::Dot(right, fwd)) == Catch::Approx(0.0).margin(2e-4));
  REQUIRE(dval(FixedVec3::Dot(up, fwd)) == Catch::Approx(0.0).margin(2e-4));

  REQUIRE(dval(right.Length()) == Catch::Approx(1.0).margin(2e-4));
  REQUIRE(dval(up.Length()) == Catch::Approx(1.0).margin(2e-4));
  REQUIRE(dval(fwd.Length()) == Catch::Approx(1.0).margin(2e-4));
}

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"
#include "navary/math/fixed_trigonometry.h"

using navary::math::Fixed15p16;
using navary::math::FixedTrig;
using navary::math::FixedVec3;

static inline Fixed15p16 F(double v) {
  return Fixed15p16::FromDouble(v);
}
static inline double dval(Fixed15p16 f) {
  return f.ToDouble();
}

static const Fixed15p16 kPi = F(3.14159265358979323846264338327950288);
static inline Fixed15p16 Deg(double deg) {
  return F(deg) * kPi / F(180.0);
}

TEST_CASE("FixedVec3: RotateAroundX/Y/Z on basis vectors (90/180deg)",
          "[fixed][vec3][rotate]") {
  // X axis
  {
    FixedVec3 v{F(0), F(1), F(0)};
    auto v90  = v.RotateAroundX(Deg(90.0));   // (0,1,0) -> (0,0,1)
    auto v180 = v.RotateAroundX(Deg(180.0));  // (0,1,0) -> (0,-1,0)

    REQUIRE(dval(v90.x) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v90.y) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v90.z) == Catch::Approx(1.0).margin(2e-3));

    REQUIRE(dval(v180.x) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v180.y) == Catch::Approx(-1.0).margin(2e-3));
    REQUIRE(dval(v180.z) == Catch::Approx(0.0).margin(2e-3));
  }

  // Y axis
  {
    FixedVec3 v{F(1), F(0), F(0)};
    auto v90  = v.RotateAroundY(Deg(90.0));   // (1,0,0) -> (0,0,-1)
    auto v180 = v.RotateAroundY(Deg(180.0));  // (1,0,0) -> (-1,0,0)

    REQUIRE(dval(v90.x) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v90.y) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v90.z) == Catch::Approx(-1.0).margin(2e-3));

    REQUIRE(dval(v180.x) == Catch::Approx(-1.0).margin(2e-3));
    REQUIRE(dval(v180.y) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v180.z) == Catch::Approx(0.0).margin(2e-3));
  }

  // Z axis
  {
    FixedVec3 v{F(1), F(0), F(0)};
    auto v90  = v.RotateAroundZ(Deg(90.0));   // (1,0,0) -> (0,1,0)
    auto v180 = v.RotateAroundZ(Deg(180.0));  // (1,0,0) -> (-1,0,0)

    REQUIRE(dval(v90.x) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v90.y) == Catch::Approx(1.0).margin(2e-3));
    REQUIRE(dval(v90.z) == Catch::Approx(0.0).margin(2e-3));

    REQUIRE(dval(v180.x) == Catch::Approx(-1.0).margin(2e-3));
    REQUIRE(dval(v180.y) == Catch::Approx(0.0).margin(2e-3));
    REQUIRE(dval(v180.z) == Catch::Approx(0.0).margin(2e-3));
  }
}

TEST_CASE("FixedVec3: rotation preserves length across many steps",
          "[fixed][vec3][rotate][stability]") {
  // Non-axis vector to expose rounding behavior a bit.
  FixedVec3 v{F(0.7), F(-0.2), F(0.68)};
  v.Normalize();
  auto len0 = v.Length();

  // Spin 360 degrees in small steps around each axis independently.
  const auto step = Deg(360.0 / 48.0);  // 7.5°
  {
    FixedVec3 p = v;
    for (int i = 0; i < 48; ++i)
      p = p.RotateAroundX(step);
    REQUIRE(dval(p.Length()) == Catch::Approx(dval(len0)).margin(2e-3));
  }
  {
    FixedVec3 p = v;
    for (int i = 0; i < 48; ++i)
      p = p.RotateAroundY(step);
    REQUIRE(dval(p.Length()) == Catch::Approx(dval(len0)).margin(2e-3));
  }
  {
    FixedVec3 p = v;
    for (int i = 0; i < 48; ++i)
      p = p.RotateAroundZ(step);
    REQUIRE(dval(p.Length()) == Catch::Approx(dval(len0)).margin(2e-3));
  }
}

TEST_CASE("FixedVec3: camera yaw then pitch on forward vector",
          "[fixed][vec3][camera]") {
  // OpenGL-style: RH, +Y up, camera forward = -Z.
  FixedVec3 fwd{F(0), F(0), F(-1)};

  // Yaw +45° about +Y, then pitch -30° about +X.
  auto yaw   = Deg(45.0);
  auto pitch = Deg(-30.0);

  auto v = fwd.RotateAroundY(yaw)      // yaw first
               .RotateAroundX(pitch);  // then pitch

  // Expected properties:
  // - After yaw +45°, x and z should have equal magnitude (|x| ≈ |z|).
  REQUIRE(std::fabs(std::fabs(dval(v.x)) - std::fabs(dval(v.z))) < 3e-2);

  // - Pitch -30° tips some component into +Y (since initial fwd is -Z).
  REQUIRE(dval(v.y) > -0.1);  // should be around +0.5 * sin(…) depending on yaw

  // - Length should be preserved.
  REQUIRE(dval(v.Length()) == Catch::Approx(1.0).margin(2e-3));
}

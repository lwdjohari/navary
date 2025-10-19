#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec2.h"
#include "navary/math/fixed_trigonometry.h"

using navary::math::Fixed15p16;
using navary::math::FixedTrig;
using navary::math::FixedVec2;

static inline double dval(Fixed15p16 f) {
  return f.ToDouble();
}
static inline Fixed15p16 F(double v) {
  return Fixed15p16::FromDouble(v);
}

TEST_CASE("FixedVec2: seek/arrive steering", "[fixed][vec2][steering]") {
  FixedVec2 pos{F(0), F(0)};
  FixedVec2 target{F(10), F(0)};
  auto max_speed   = F(6.0);
  auto slow_radius = F(3.0);
  auto dt          = Fixed15p16::FromFraction(1, 60);

  for (int i = 0; i < 120; ++i) {
    // desired = target - pos
    FixedVec2 desired = target - pos;
    auto dist         = desired.Length();
    if (!dist.IsZero()) {
      // Arrive: scale speed linearly within slow radius
      auto speed = (dist < slow_radius) ? dist : max_speed;
      desired.NormalizeTo(speed);
      // Euler step
      pos += FixedVec2::MulScalar(desired, dt);
    }
  }
  REQUIRE(dval(pos.x) == Catch::Approx(10.0).margin(0.1));
  REQUIRE(dval(pos.y) == Catch::Approx(0.0).margin(1e-3));
}

TEST_CASE("FixedVec2: rotate by angle preserves length",
          "[fixed][vec2][rotate]") {
  FixedVec2 v{F(1.0), F(0.0)};
  auto len0 = v.Length();

  // Rotate by 360 degrees in 15-degree steps; end near start
  auto step   = F(3.14159265358979323846 / 12.0);
  FixedVec2 p = v;
  for (int i = 0; i < 24; ++i) {
    p = p.Rotate(step);
    // length preserved within small fixed tolerance
    REQUIRE(dval(p.Length()) == Catch::Approx(dval(len0)).margin(2e-4));
  }
  // after full turn, back to near (1,0)
  REQUIRE(dval(p.x) == Catch::Approx(1.0).margin(5e-3));
  REQUIRE(dval(p.y) == Catch::Approx(0.0).margin(5e-3));
}

TEST_CASE("FixedVec2: orientation and perpendiculars",
          "[fixed][vec2][orientation]") {
  FixedVec2 a{F(1), F(0)};
  FixedVec2 b{F(0), F(1)};
  // a -> b is CCW => +1
  REQUIRE(a.RelativeOrientation(b) == 1);
  // CCW perpendicular of a is +Y
  auto p = a.PerpendicularCCW();
  REQUIRE(dval(p.x) == Catch::Approx(0.0).margin(1e-6));
  REQUIRE(dval(p.y) == Catch::Approx(1.0).margin(1e-6));
}

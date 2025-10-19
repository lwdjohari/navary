#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include "navary/math/fixed.h"

using Catch::Approx;
using navary::math::Fixed;
using navary::math::Fixed15p16;
static inline double dval(Fixed15p16 f) {
  return f.ToDouble();
}

// Helper: create Fixed from double with minimal noise in tests
static inline Fixed15p16 F(double v) {
  return Fixed15p16::FromDouble(v);
}

TEST_CASE("Fixed: basic construction and conversions", "[fixed][construct]") {
  auto z = Fixed15p16::Zero();
  REQUIRE(z.IsZero());

  auto a = Fixed15p16::FromInt(5);
  REQUIRE(a.ToIntTrunc() == 5);
  REQUIRE(a.ToIntFloor() == 5);
  REQUIRE(a.ToIntCeil() == 5);

  auto b = Fixed15p16::FromFloat(1.25f);
  REQUIRE(b.ToIntTrunc() == 1);
  REQUIRE(b.ToIntCeil() == 2);
  REQUIRE(b.ToIntFloor() == 1);

  auto c = Fixed15p16::FromFraction(1, 3);  // ~0.33333
  REQUIRE(c.ToIntTrunc() == 0);
  REQUIRE(c.ToIntCeil() == 1);

  // nearest ties up (away from zero)
  auto half = Fixed15p16::FromFraction(1, 2);
  REQUIRE(half.ToIntNearestTiesUp() == 1);
  auto nhalf = Fixed15p16::FromDouble(-0.5);
  REQUIRE(nhalf.ToIntNearestTiesUp() == 0);  // -0.5 + 0.5 -> 0
}

TEST_CASE("Fixed: arithmetic, Mul, MulDiv, Mod", "[fixed][arith]") {
  auto a = F(3.0);
  auto b = F(1.25);

  // a + b == 4.25
  auto s = a + b;
  REQUIRE(s.ToIntTrunc() == 4);
  REQUIRE(dval(s) == Approx(4.25).margin(1e-5));

  // fixed × int
  auto five_a = a * 5;
  REQUIRE(dval(five_a) == Approx(15.0).margin(1e-5));

  // fixed × fixed (Mul)
  auto p = Mul(a, b);
  REQUIRE(dval(p) == Approx(3.75).margin(1e-5));

  // (a * 7) / 2
  auto md = MulDiv(a, Fixed15p16::FromInt(7), Fixed15p16::FromInt(2));
  REQUIRE(dval(md) == Approx(10.5).margin(1e-5));

  // remainder with sign of divisor
  auto r1 = Mod(Fixed15p16::FromInt(7), Fixed15p16::FromInt(3));
  REQUIRE(dval(r1) == Approx(1.0).margin(1e-5));
  auto r2 = Mod(Fixed15p16::FromInt(-7), Fixed15p16::FromInt(3));
  REQUIRE(dval(r2) ==
          Approx(2.0).margin(1e-5));  // same sign as divisor (positive)
}

TEST_CASE("Fixed: sqrt matches reference within LSB tolerance",
          "[fixed][sqrt]") {
  // perfect squares
  REQUIRE(dval(Sqrt(F(0))) == Approx(0.0).margin(1e-6));
  REQUIRE(dval(Sqrt(F(1))) == Approx(1.0).margin(1e-6));
  REQUIRE(dval(Sqrt(F(4))) == Approx(2.0).margin(1e-6));
  REQUIRE(dval(Sqrt(F(9))) == Approx(3.0).margin(1e-6));

  // non-perfect squares (compare to double sqrt loosely)
  for (double v : {0.5, 2.5, 10.75, 123.456}) {
    auto fx = F(v);
    auto sr = Sqrt(fx);
    REQUIRE(dval(sr) ==
            Approx(std::sqrt(v)).margin(2e-4));  // safe tolerance for 15.16
  }

  // negative clamp to zero
  REQUIRE(dval(Sqrt(F(-1.0))) == Approx(0.0).margin(1e-6));
}

TEST_CASE("Fixed: kinematics step (s = s0 + v*t + 0.5*a*t^2)",
          "[fixed][physics]") {
  auto s0 = F(10.0);
  auto v  = F(3.0);                           // m/s
  auto a  = F(2.0);                           // m/s^2
  auto t  = Fixed15p16::FromFraction(1, 60);  // 16.666... ms

  // compute 0.5*a*t^2 via fixed
  auto half = Fixed15p16::FromFraction(1, 2);
  auto t2   = Mul(t, t);
  auto term = Mul(Mul(a, half), t2);

  auto s1 = s0 + Mul(v, t) + term;

  // reference in double
  double ref =
      10.0 + 3.0 * (1.0 / 60.0) + 0.5 * 2.0 * (1.0 / 60.0) * (1.0 / 60.0);
  REQUIRE(dval(s1) == Approx(ref).margin(2e-5));
}

TEST_CASE("Fixed: movement integration over frames (deterministic stepping)",
          "[fixed][movement]") {
  auto pos = F(0.0);
  auto vel = F(5.0);  // units/sec
  auto dt  = Fixed15p16::FromFraction(1, 60);

  // integrate 120 frames (~2 seconds)
  for (int i = 0; i < 120; ++i) {
    pos += Mul(vel, dt);
  }
  REQUIRE(dval(pos) ==
          Approx(10.0).margin(0.003));  // 15.16 quantization (~0.00244)

  // Two half-steps should be close to one full step for simple v*dt integration
  auto pos_full = F(0.0);
  auto pos_half = F(0.0);
  auto dt_full  = Fixed15p16::FromFraction(1, 60);
  auto dt_half  = Fixed15p16::FromFraction(1, 120);

  pos_full += Mul(vel, dt_full);
  pos_half += Mul(vel, dt_half);
  pos_half += Mul(vel, dt_half);
  REQUIRE(dval(pos_half) == Approx(dval(pos_full)).margin(1e-6));
}

TEST_CASE("Fixed: timer accumulator and wrap using Mod", "[fixed][timer]") {
  // Simulate a looping animation timer in [0, duration)
  auto duration = F(2.5);  // 2.5 seconds
  auto t        = F(0.0);

  // accumulate 10 seconds
  auto step = Fixed15p16::FromFraction(1, 60);
  for (int i = 0; i < 600; ++i)
    t += step;

  // wrap
  auto wrapped = Mod(t, duration);
  // Instead of forcing near 0, accept remainder near 0 or near duration
  // (wrap-around)
  double w   = dval(wrapped);
  double dur = dval(duration);
  double eps = std::min(std::fabs(w), std::fabs(dur - w));
  REQUIRE(eps <= 0.003);
}

TEST_CASE("Fixed: AABB check with player radius", "[fixed][aabb][collision]") {
  // World bounds [0, 100] x [0, 100]
  auto min = F(0.0);
  auto max = F(100.0);

  auto px = F(5.0);
  auto py = F(12.0);
  auto r  = F(1.5);

  // clamp inside world with radius
  auto cx = px;
  auto cy = py;

  if (cx < min + r)
    cx = min + r;
  if (cy < min + r)
    cy = min + r;
  if (cx > max - r)
    cx = max - r;
  if (cy > max - r)
    cy = max - r;

  REQUIRE(dval(cx) == Approx(5.0).margin(1e-5));
  REQUIRE(dval(cy) == Approx(12.0).margin(1e-5));

  // move near right edge and clamp again
  px = F(99.0);
  py = F(1.0);
  cx = px;
  cy = py;
  if (cx > max - r)
    cx = max - r;
  if (cy < min + r)
    cy = min + r;

  REQUIRE(dval(cx) == Approx(98.5).margin(1e-5));
  REQUIRE(dval(cy) == Approx(1.5).margin(1e-5));
}

TEST_CASE("Fixed: rounding modes sanity on negatives", "[fixed][rounding]") {
  auto x = F(-1.75);
  REQUIRE(x.ToIntFloor() == -2);
  REQUIRE(x.ToIntCeil() == -1);
  REQUIRE(x.ToIntTrunc() == -1);
  // ties up uses +0.5 in raw, so -1.5 -> -1, -2.5 -> -2
  auto m15 = F(-1.5);
  auto m25 = F(-2.5);
  REQUIRE(m15.ToIntNearestTiesUp() == -1);
  REQUIRE(m25.ToIntNearestTiesUp() == -2);
}

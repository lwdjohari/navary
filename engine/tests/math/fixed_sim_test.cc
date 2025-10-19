#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "navary/math/fixed.h"

using navary::math::Fixed15p16;

static inline double dval(Fixed15p16 f) {
  return f.ToDouble();
}
static inline Fixed15p16 F(double v) {
  return Fixed15p16::FromDouble(v);
}

TEST_CASE("FixedSim: s = s0 + v*t + 0.5*a*t^2 (1/60s step)",
          "[fixed][sim][kinematics]") {
  auto s0   = F(0.0);
  auto v    = F(12.0);                          // m/s
  auto a    = F(-9.8);                          // m/s^2
  auto dt   = Fixed15p16::FromFraction(1, 60);  // 1/60 s
  auto half = Fixed15p16::FromFraction(1, 2);

  // step once
  auto s1 = s0 + Mul(v, dt) + Mul(Mul(a, half), Mul(dt, dt));
  double ref =
      0.0 + 12.0 * (1.0 / 60.0) + 0.5 * (-9.8) * (1.0 / 60.0) * (1.0 / 60.0);
  REQUIRE(dval(s1) == Catch::Approx(ref).margin(2e-4));
}

TEST_CASE(
    "FixedSim: velocity integration—naive drift vs remainder accumulation",
    "[fixed][sim][movement]") {
  auto pos_naive = F(0.0);
  auto pos_accum = F(0.0);

  auto vel = F(5.0);                           // units/s
  auto dt  = Fixed15p16::FromFraction(1, 60);  // 1/60 s

  // Naive integrate 120 frames
  for (int i = 0; i < 120; ++i)
    pos_naive += Mul(vel, dt);

  // Remainder-accumulated integrate 120 frames (integer division with carry)
  int64_t v_raw   = vel.Raw();
  int64_t pos_raw = 0;
  int64_t rem     = 0;
  for (int i = 0; i < 120; ++i) {
    rem += v_raw;
    int64_t step_raw = rem / 60;
    rem -= step_raw * 60;
    pos_raw += step_raw;
  }
  pos_accum = Fixed15p16::FromRaw(static_cast<int32_t>(pos_raw));

  // Ground truth 5 * 120/60 = 10
  REQUIRE(dval(pos_accum) == Catch::Approx(10.0).margin(1e-6));
  // Naive shows small quantization error (~0.00244)
  REQUIRE(dval(pos_naive) == Catch::Approx(10.0).margin(0.003));
}

TEST_CASE("FixedSim: looping timer with Mod and remainder accumulation",
          "[fixed][sim][timer]") {
  auto duration = F(2.5);
  auto dt       = Fixed15p16::FromFraction(1, 60);  // 1/60 s
  // Accumulate 10 seconds
  auto t_naive = F(0.0);
  for (int i = 0; i < 600; ++i)
    t_naive += dt;
  auto wrapped_naive = Mod(t_naive, duration);
  // Accept near 0 (or near duration) due to quantization
  double w   = dval(wrapped_naive);
  double d   = dval(duration);
  double eps = std::min(std::fabs(w), std::fabs(d - w));
  REQUIRE(eps <= 0.003);

  // Remainder-accumulated
  int64_t one_raw = Fixed15p16::FromInt(1).Raw();
  int64_t t_raw = 0, rem = 0;
  for (int i = 0; i < 600; ++i) {
    rem += one_raw;
    int64_t inc = rem / 60;
    rem -= inc * 60;
    t_raw += inc;
  }
  int64_t dur_raw = Fixed15p16::FromDouble(2.5).Raw();
  int64_t wr      = t_raw % dur_raw;
  if (wr < 0)
    wr += dur_raw;
  auto wrapped2 = Fixed15p16::FromRaw(static_cast<int32_t>(wr));

  REQUIRE(std::fabs(dval(wrapped2)) <= 1e-6);
}

TEST_CASE("FixedSim: per-tick budget splitter (deterministic)",
          "[fixed][sim][budget]") {
  // Distribute 100.0 resource over 37 ticks deterministically.
  auto total = F(100.0);
  int ticks  = 37;

  int64_t total_raw = total.Raw();
  int64_t rem       = 0;
  int64_t acc_raw   = 0;

  for (int i = 0; i < ticks; ++i) {
    rem += total_raw;
    int64_t slice = rem / ticks;
    rem -= slice * ticks;
    acc_raw += slice;
  }
  auto recomposed = Fixed15p16::FromRaw(static_cast<int32_t>(acc_raw));
  REQUIRE(dval(recomposed) == Catch::Approx(100.0).margin(1e-6));
}

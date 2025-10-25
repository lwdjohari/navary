// navary/math/fixed_trigonometry.cc

#include "navary/math/fixed_trigonometry.h"
#include <algorithm>

namespace navary::math {

using F = FixedTrig::F;

static inline F f_from(double d) {
  return F::FromDouble(d);
}

// Polynomial coefficients (double → Fixed) for minimax on [-π/2, π/2]
static const F kSinC1 = f_from(-1.0 / 6.0);   // -1/6
static const F kSinC2 = f_from(1.0 / 120.0);  // +1/120

static const F kCosC1 = f_from(-1.0 / 2.0);  // -1/2
static const F kCosC2 = f_from(1.0 / 24.0);  // +1/24

// 2π and π/2 constants
static const F kTwoPi  = f_from(2.0 * kPi);
static const F kHalfPi = f_from(0.5 * kPi);

F FixedTrig::WrapToPi(F a) {
  // Reduce by 2π using fixed division; keep sign via Mod on raw ticks.
  // t = a mod (2π), map to [-π, π]
  // Using raw to avoid extra quantization:
  const auto mod =
      Mod(a, kTwoPi);  // in [0, 2π) with our Mod semantics (non-negative)
  // shift to [-π, π): if mod > π, subtract 2π
  if (mod > f_from(kPi))
    return mod - kTwoPi;
  return mod;
}

void FixedTrig::SinCos(F a, F& s_out, F& c_out) {
  // 1. range reduce to [-π, π]
  F x = WrapToPi(a);

  // 2. use symmetries to map to [-π/2, π/2] where polynomials are accurate
  bool flip_sign_sin = false;
  bool flip_sign_cos = false;
  bool use_sin_sym   = false;  // sin(x) = sin(π - x) in II quadrant
  bool use_cos_sym   = false;  // cos(x) = -cos(π - x) in II/III quadrants

  if (x > kHalfPi) {
    // Quadrant II: x in (π/2, π]  -> y = π - x in [0, π/2)
    F y = f_from(kPi) - x;
    x   = y;
    // sin stays positive (no flip), cos negative
    flip_sign_cos = true;
    use_cos_sym   = true;
  } else if (x < -kHalfPi) {
    // Quadrant III: x in [-π, -π/2) -> y = -π - x ∈ [0, π/2)
    // Better: use y = -(π + x), but equivalently y = -(x + π)
    F y = -(f_from(kPi) + x);
    x   = y;
    // sin negative, cos negative
    flip_sign_sin = true;
    flip_sign_cos = true;
  }
  // Now x ∈ [-π/2, π/2]

  // 3. Evaluate polynomials via Estrin (faster on fixed too)
  // sin(x) ≈ x + x^3 (-1/6 + x^2*(1/120))
  // cos(x) ≈ 1 + x^2 (-1/2 + x^2*(1/24))
  const F x2 = Mul(x, x);

  // sin
  F sin_poly = kSinC2;
  sin_poly   = Mul(sin_poly, x2) + kSinC1;
  sin_poly   = Mul(sin_poly, x2) + F::FromInt(1);
  F s        = Mul(sin_poly, x);  // x * (1 + x^2(-1/6 + x^2/120))

  // cos
  F cos_poly = kCosC2;
  cos_poly   = Mul(cos_poly, x2) + kCosC1;
  F c        = F::FromInt(1) + Mul(cos_poly, x2);

  // 4. restore signs from quadrant handling
  if (flip_sign_sin)
    s = F::FromInt(-1) * s;
  if (flip_sign_cos)
    c = F::FromInt(-1) * c;

  s_out = s;
  c_out = c;
}

}  // namespace navary::math

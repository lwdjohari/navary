// navary/math/fixed_trigonometry.h

#pragma once
#include "navary/math/fixed.h"
#include "navary/math/math_util.h"  // kPi

namespace navary::math {

/**
 * Deterministic fixed-point trig for gameplay/sim:
 * - Angle in radians (OpenGL-style), Fixed15p16 domain.
 * - Clean-room polynomial approx with symmetry + range reduction.
 * - Typical abs error ~1e-4..5e-4 for 15.16 (sufficient for sim/AI).
 */
class FixedTrig {
 public:
  using F = Fixed15p16;

  /** Compute sin(a), cos(a) together (more cache-friendly). */
  static void SinCos(F a, F& s_out, F& c_out);

  /** Convenience single-function wrappers. */
  static F Sin(F a) {
    F s, c;
    SinCos(a, s, c);
    return s;
  }
  static F Cos(F a) {
    F s, c;
    SinCos(a, s, c);
    return c;
  }

 private:
  static F WrapToPi(F a);  // Map to [-pi, pi]
};

}  // namespace navary::math

#pragma once

#include "navary/math/fixed.h"  // Fixed15p16, Mul, Sqrt
#include "navary/math/fixed_trigonometry.h"
namespace navary::math {

/**
 * @brief 2D fixed-point vector (15.16 by default).
 * Deterministic math for simulation/gameplay.
 */
class FixedVec2 {
 public:
  using F = Fixed15p16;

  F x{F::Zero()};
  F y{F::Zero()};

  FixedVec2() = default;
  constexpr FixedVec2(F x_, F y_) : x(x_), y(y_) {}

  // Equality / inequality
  friend constexpr bool operator==(const FixedVec2& a, const FixedVec2& b) {
    return a.x == b.x && a.y == b.y;
  }

  friend constexpr bool operator!=(const FixedVec2& a, const FixedVec2& b) {
    return !(a == b);
  }

  // Add / sub / negation
  friend constexpr FixedVec2 operator+(const FixedVec2& a, const FixedVec2& b) {
    return FixedVec2{a.x + b.x, a.y + b.y};
  }

  friend constexpr FixedVec2 operator-(const FixedVec2& a, const FixedVec2& b) {
    return FixedVec2{a.x - b.x, a.y - b.y};
  }

  constexpr FixedVec2 operator-() const {
    return FixedVec2{-x, -y};
  }

  FixedVec2& operator+=(const FixedVec2& v) {
    x += v.x;
    y += v.y;
    return *this;
  }

  FixedVec2& operator-=(const FixedVec2& v) {
    x -= v.x;
    y -= v.y;
    return *this;
  }

  // Scalar by int (exact)
  friend constexpr FixedVec2 operator*(const FixedVec2& v, int s) {
    return FixedVec2{v.x * s, v.y * s};
  }

  friend constexpr FixedVec2 operator*(int s, const FixedVec2& v) {
    return v * s;
  }

  friend constexpr FixedVec2 operator/(const FixedVec2& v, int s) {
    // caller must ensure s != 0
    return FixedVec2{v.x / s, v.y / s};
  }

  // Scalar by fixed (deterministic)
  static FixedVec2 MulScalar(const FixedVec2& v, F s) {
    return FixedVec2{Mul(v.x, s), Mul(v.y, s)};
  }

  // Dot and 2D cross (z-component of 3D cross)
  static F Dot(const FixedVec2& a, const FixedVec2& b) {
    return Mul(a.x, b.x) + Mul(a.y, b.y);
  }

  static F CrossZ(const FixedVec2& a, const FixedVec2& b) {
    return Mul(a.x, b.y) - Mul(a.y, b.x);
  }

  // Length and normalization
  F Length() const {
    return Sqrt(SquaredLength());
  }

  F SquaredLength() const {
    return Mul(x, x) + Mul(y, y);
  }

  // Normalize to length ~1 (no-op for zero vector)
  void Normalize() {
    F l = Length();
    if (!l.IsZero()) {
      x = x / l;
      y = y / l;
    }
  }

  // Normalize to specific length n (no-op for zero vector)
  void NormalizeTo(F n) {
    F l = Length();
    if (!l.IsZero()) {
      x = MulDiv(x, n, l);
      y = MulDiv(y, n, l);
    }
  }

  // Compare length to a target length (avoids sqrt)
  // return -1 (shorter), 0 (equal), +1 (longer)
  int CompareLength(F cmp_len) const {
    const F d2   = SquaredLength();
    const F cmp2 = Mul(cmp_len, cmp_len);
    if (d2 < cmp2)
      return -1;
    if (d2 > cmp2)
      return +1;
    return 0;
  }

  // Compare length to another vector's length (avoids sqrt)
  int CompareLength(const FixedVec2& other) const {
    const F d2 = SquaredLength();
    const F o2 = other.SquaredLength();
    if (d2 < o2)
      return -1;
    if (d2 > o2)
      return +1;
    return 0;
  }

  // Orientation in 2D: sign of cross (CCW positive)
  // Returns -1 (right/clockwise), 0 (collinear), +1 (left/counterclockwise)
  int RelativeOrientation(const FixedVec2& v) const {
    F z = CrossZ(*this, v);
    if (z > F::Zero())
      return 1;
    if (z < F::Zero())
      return -1;
    return 0;
  }

  // Perpendicular (CCW)
  FixedVec2 PerpendicularCCW() const {
    return FixedVec2{y, F::FromInt(-1) * x};
  }

  // Perpendicular (CW)
  FixedVec2 PerpendicularCW() const {
    return FixedVec2{F::FromInt(-1) * y, x};
  }

  // Rotate by precomputed sin/cos (anticlockwise): v' = [x c - y s, x s + y c]
  // Use a deterministic approximator to produce s,c as Fixed elsewhere.
  FixedVec2 RotateBySinCos(F s, F c) const {
    return FixedVec2{(Mul(x, c) - Mul(y, s)), (Mul(x, s) + Mul(y, c))};
  }

  // Rotate by angle (radians, OpenGL-style), anticlockwise.
  // Internally uses deterministic fixed trig (no float).
  FixedVec2 Rotate(F angle) const {
    F s, c;
    FixedTrig::SinCos(angle, s, c);
    return RotateBySinCos(s, c);
  }

  // Utility
  bool IsZero() const {
    return x.IsZero() && y.IsZero();
  }
};

}  // namespace navary::math

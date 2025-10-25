// navary/math/fixed_vec3.h

#pragma once

#include "navary/math/fixed.h"  // Fixed15p16, Mul, Sqrt
#include "navary/math/fixed_trigonometry.h"

namespace navary::math {

/**
 * @brief 3D fixed-point vector (15.16 by default).
 */
class FixedVec3 {
 public:
  using F = Fixed15p16;

  F x{F::Zero()};
  F y{F::Zero()};
  F z{F::Zero()};

  FixedVec3() = default;
  constexpr FixedVec3(F x_, F y_, F z_) : x(x_), y(y_), z(z_) {}

  // Rotate around principal axes (angle in radians, anticlockwise looking
  // along the positive axis; OpenGL-style right-handed).
  FixedVec3 RotateAroundX(F angle) const;
  FixedVec3 RotateAroundY(F angle) const;
  FixedVec3 RotateAroundZ(F angle) const;

  // Rotate by Euler angles in one call.
  // Order: Yaw (Y), then Pitch (X), then Roll (Z).
  // This matches a common camera pipeline in RH OpenGL.
  // Usage: v_rot = v.RotateEuler(yaw, pitch, roll);
  FixedVec3 RotateEuler(F yaw, F pitch, F roll) const;

  // Equality / inequality
  friend constexpr bool operator==(const FixedVec3& a, const FixedVec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
  }

  friend constexpr bool operator!=(const FixedVec3& a, const FixedVec3& b) {
    return !(a == b);
  }

  // Add / sub / negation
  friend constexpr FixedVec3 operator+(const FixedVec3& a, const FixedVec3& b) {
    return FixedVec3{a.x + b.x, a.y + b.y, a.z + b.z};
  }

  friend constexpr FixedVec3 operator-(const FixedVec3& a, const FixedVec3& b) {
    return FixedVec3{a.x - b.x, a.y - b.y, a.z - b.z};
  }

  constexpr FixedVec3 operator-() const {
    return FixedVec3{-x, -y, -z};
  }

  FixedVec3& operator+=(const FixedVec3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }

  FixedVec3& operator-=(const FixedVec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }

  // Scalar by int (exact)
  friend constexpr FixedVec3 operator*(const FixedVec3& v, int s) {
    return FixedVec3{v.x * s, v.y * s, v.z * s};
  }

  friend constexpr FixedVec3 operator*(int s, const FixedVec3& v) {
    return v * s;
  }
  friend constexpr FixedVec3 operator/(const FixedVec3& v, int s) {
    return FixedVec3{v.x / s, v.y / s, v.z / s};
  }

  // Scalar by fixed (deterministic)
  static FixedVec3 MulScalar(const FixedVec3& v, F s) {
    return FixedVec3{Mul(v.x, s), Mul(v.y, s), Mul(v.z, s)};
  }

  // Dot / Cross
  static F Dot(const FixedVec3& a, const FixedVec3& b) {
    return Mul(a.x, b.x) + Mul(a.y, b.y) + Mul(a.z, b.z);
  }

  static FixedVec3 Cross(const FixedVec3& a, const FixedVec3& b) {
    // (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x)
    F cx = Mul(a.y, b.z) - Mul(a.z, b.y);
    F cy = Mul(a.z, b.x) - Mul(a.x, b.z);
    F cz = Mul(a.x, b.y) - Mul(a.y, b.x);
    return FixedVec3{cx, cy, cz};
  }

  // Length and normalization
  F Length() const {
    return Sqrt(SquaredLength());
  }
  F SquaredLength() const {
    return Mul(x, x) + Mul(y, y) + Mul(z, z);
  }

  void Normalize() {
    F l = Length();
    if (!l.IsZero()) {
      x = x / l;
      y = y / l;
      z = z / l;
    }
  }

  void NormalizeTo(F n) {
    F l = Length();
    if (!l.IsZero()) {
      x = MulDiv(x, n, l);
      y = MulDiv(y, n, l);
      z = MulDiv(z, n, l);
    }
  }

  // Utility
  bool IsZero() const {
    return x.IsZero() && y.IsZero() && z.IsZero();
  }
};

}  // namespace navary::math

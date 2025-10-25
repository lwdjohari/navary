// navary/math/quat.h

#pragma once
// Navary Engine - Math
// Clean-room unit quaternion (float) for rotations (graphics).

#include "navary/math/vec3.h"
#include "navary/math/vec4.h"

namespace navary::math {

class Mat4;  // forward

class Quat {
 public:
  // Stored as (x, y, z, w) with |q|=1 for rotations.
  float x{0}, y{0}, z{0}, w{1};

  Quat() = default;
  Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

  static Quat Identity() {
    return Quat{0, 0, 0, 1};
  }

  // Construct from axis (normalized) and angle in radians.
  static Quat FromAxisAngle(const Vec3& axis, float angle_radians);

  // Construct from Euler XYZ (pitch=x, yaw=y, roll=z), radians.
  static Quat FromEulerXYZ(const Vec3& euler_xyz);

  // Basic ops.
  Quat operator*(const Quat& rhs) const;  // Hamilton product
  Quat& operator*=(const Quat& rhs);

  // Scale (for lerp-like ops).
  Quat operator*(float s) const {
    return Quat{x * s, y * s, z * s, w * s};
  }
  Quat operator+(const Quat& r) const {
    return Quat{x + r.x, y + r.y, z + r.z, w + r.w};
  }
  Quat operator-(const Quat& r) const {
    return Quat{x - r.x, y - r.y, z - r.z, w - r.w};
  }

  // Unit ops.
  float Dot(const Quat& r) const {
    return x * r.x + y * r.y + z * r.z + w * r.w;
  }
  
  void Normalize();

  Quat Normalized() const {
    Quat q = *this;
    q.Normalize();
    return q;
  }

  Quat Inverse() const {
    return Quat{-x, -y, -z, w};
  }  // valid if unit

  // Rotate a vector (q * v * q^-1).
  Vec3 Rotate(const Vec3& v) const;

  // Convert to matrix (column-major).
  Mat4 ToMat4() const;

  // Spherical interpolation (shortest arc). ratio in [0,1].
  static Quat Slerp(const Quat& a, const Quat& b, float ratio);

  // Normalized linear interpolation (fast, good for small angles).
  static Quat Nlerp(const Quat& a, const Quat& b, float ratio);

  static inline Quat QMul(const Quat& a, const Quat& b) { return a * b; }

};

}  // namespace navary::math

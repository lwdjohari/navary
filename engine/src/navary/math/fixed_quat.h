#pragma once
// Navary Engine - Fixed Math
// Unit quaternion (Fixed15p16) for lockstep rotation math.

#include <cmath>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"
#include "navary/math/fixed_trigonometry.h"
#include "navary/math/fixed_mat4.h"

namespace navary::math {

/** Convenience alias. */
using F = Fixed15p16;

/**
 * @brief Fixed-point quaternion (x,y,z,w) with |q|=1 for rotations.
 * Stored as Fixed15p16 to ensure deterministic simulation.
 */
class FixedQuat {
 public:
  F x{F::Zero()}, y{F::Zero()}, z{F::Zero()}, w{F::FromInt(1)};

  FixedQuat() = default;
  FixedQuat(F x_, F y_, F z_, F w_) : x(x_), y(y_), z(z_), w(w_) {}

  static FixedQuat Identity() {
    return FixedQuat{F::Zero(), F::Zero(), F::Zero(), F::FromInt(1)};
  }

  /** Construct from axis (need not be unit; will be normalized) and angle
   * (radians). */
  static FixedQuat FromAxisAngle(const FixedVec3& axis, F angle) {
    // s = sin(theta/2), c = cos(theta/2)
    F half = angle / F::FromInt(2);
    F s, c;
    FixedTrig::SinCos(half, s, c);

    // normalize axis
    FixedVec3 n = axis;
    n.Normalize();

    return FixedQuat{Mul(n.x, s), Mul(n.y, s), Mul(n.z, s), c};
  }

  /** Euler XYZ (pitch=x, yaw=y, roll=z), radians; column-vector convention:
      apply X, then Y, then Z  ⇒ q = qz * qy * qx */
  static FixedQuat FromEulerXYZ(const FixedVec3& e) {
    F hx = e.x / F::FromInt(2);
    F hy = e.y / F::FromInt(2);
    F hz = e.z / F::FromInt(2);

    F sx, cx;
    FixedTrig::SinCos(hx, sx, cx);
    F sy, cy;
    FixedTrig::SinCos(hy, sy, cy);
    F sz, cz;
    FixedTrig::SinCos(hz, sz, cz);

    FixedQuat qx{sx, F::Zero(), F::Zero(), cx};
    FixedQuat qy{F::Zero(), sy, F::Zero(), cy};
    FixedQuat qz{F::Zero(), F::Zero(), sz, cz};

    return qz * (qy * qx);
  }

  // Hamilton product
  FixedQuat operator*(const FixedQuat& r) const {
    return FixedQuat{// x
                     w * r.x + x * r.w + y * r.z - z * r.y,
                     // y
                     w * r.y - x * r.z + y * r.w + z * r.x,
                     // z
                     w * r.z + x * r.y - y * r.x + z * r.w,
                     // w
                     w * r.w - x * r.x - y * r.y - z * r.z};
  }

  FixedQuat& operator*=(const FixedQuat& r) {
    *this = (*this) * r;
    return *this;
  }

  void Normalize() {
    // len = sqrt(x^2 + y^2 + z^2 + w^2)
    F len2 = Mul(x, x) + Mul(y, y) + Mul(z, z) + Mul(w, w);

    if (len2.IsZero()) {
      x = y = z = F::Zero();
      w         = F::FromInt(1);
      return;
    }

    F len = Sqrt(len2);
    x     = x / len;
    y     = y / len;
    z     = z / len;
    w     = w / len;
  }

  FixedQuat Normalized() const {
    FixedQuat q = *this;
    q.Normalize();

    return q;
  }

  FixedQuat Inverse() const {
    return FixedQuat{-x, -y, -z, w};
  }  // assumes unit

  /** Rotate a vector: v' = q * v * q^-1 (optimized form). */
  FixedVec3 Rotate(const FixedVec3& v) const {
    FixedVec3 qv{x, y, z};
    FixedVec3 t = FixedVec3::Cross(qv, v);
    t           = FixedVec3::MulScalar(t, F::FromInt(2));
    FixedVec3 r = v + FixedVec3::MulScalar(t, w) + FixedVec3::Cross(qv, t);

    return r;
  }

  /** Convert to FixedMat4 rotation (no translation). */
  FixedMat4 ToMat4() const {
    // Precompute products
    F xx = Mul(x, x), yy = Mul(y, y), zz = Mul(z, z);
    F xy = Mul(x, y), xz = Mul(x, z), yz = Mul(y, z);
    F wx = Mul(w, x), wy = Mul(w, y), wz = Mul(w, z);

    FixedMat4 r = FixedMat4::Identity();
    // column 0
    r.set(0, 0, F::FromInt(1) - F::FromInt(2) * (yy + zz));
    r.set(0, 1, F::FromInt(2) * (xy + wz));
    r.set(0, 2, F::FromInt(2) * (xz - wy));

    // column 1
    r.set(1, 0, F::FromInt(2) * (xy - wz));
    r.set(1, 1, F::FromInt(1) - F::FromInt(2) * (xx + zz));
    r.set(1, 2, F::FromInt(2) * (yz + wx));

    // column 2
    r.set(2, 0, F::FromInt(2) * (xz + wy));
    r.set(2, 1, F::FromInt(2) * (yz - wx));
    r.set(2, 2, F::FromInt(1) - F::FromInt(2) * (xx + yy));

    return r;
  }

  // Simple nlerp (shortest-arc when needed).
  static FixedQuat Nlerp(const FixedQuat& a, const FixedQuat& b, F t) {
    // dot
    F dot = Mul(a.x, b.x) + Mul(a.y, b.y) + Mul(a.z, b.z) + Mul(a.w, b.w);
    FixedQuat bb = (dot.Raw() < 0) ? FixedQuat{-b.x, -b.y, -b.z, -b.w} : b;

    FixedQuat r{a.x + Mul(t, (bb.x - a.x)), a.y + Mul(t, (bb.y - a.y)),
                a.z + Mul(t, (bb.z - a.z)), a.w + Mul(t, (bb.w - a.w))};
    r.Normalize();

    return r;
  }
};

}  // namespace navary::math

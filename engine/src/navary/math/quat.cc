// navary/math/quat.cc

#include "navary/math/quat.h"

#include <cmath>
#include "navary/math/mat4.h"

namespace navary::math {

Quat Quat::FromAxisAngle(const Vec3& axis, float angle) {
  const float half = 0.5f * angle;
  const float s    = std::sin(half);
  const float c    = std::cos(half);
  const Vec3 n     = axis.normalized();
  return Quat{n.x * s, n.y * s, n.z * s, c};
}

Quat Quat::FromEulerXYZ(const Vec3& e) {
  const float hx = 0.5f * e.x, hy = 0.5f * e.y, hz = 0.5f * e.z;
  const float cx = std::cos(hx), sx = std::sin(hx);
  const float cy = std::cos(hy), sy = std::sin(hy);
  const float cz = std::cos(hz), sz = std::sin(hz);

  const Quat qx{ sx, 0.f, 0.f, cx };
  const Quat qy{ 0.f, sy, 0.f, cy };
  const Quat qz{ 0.f, 0.f, sz, cz };

  // Column-vectors: apply X, then Y, then Z  ⇒  R = Rz * Ry * Rx  ⇒  q = qz * qy * qx
  return qz * (qy * qx);
}

Quat Quat::operator*(const Quat& r) const {
  // Hamilton product.
  return Quat{w * r.x + x * r.w + y * r.z - z * r.y,
              w * r.y - x * r.z + y * r.w + z * r.x,
              w * r.z + x * r.y - y * r.x + z * r.w,
              w * r.w - x * r.x - y * r.y - z * r.z};
}

Quat& Quat::operator*=(const Quat& r) {
  *this = (*this) * r;
  return *this;
}

void Quat::Normalize() {
  const float len2 = x * x + y * y + z * z + w * w;
  if (len2 <= 0.0f) {
    x = y = z = 0.0f;
    w         = 1.0f;
    return;
  }
  const float inv = 1.0f / std::sqrt(len2);
  x *= inv;
  y *= inv;
  z *= inv;
  w *= inv;
}

Vec3 Quat::Rotate(const Vec3& v) const {
  // Optimized q*v*q^-1 (assuming unit).
  const Vec3 qv{x, y, z};
  const Vec3 t = 2.0f * qv.cross(v);
  return v + w * t + qv.cross(t);
}

Mat4 Quat::ToMat4() const {
  const float xx = x * x, yy = y * y, zz = z * z;
  const float xy = x * y, xz = x * z, yz = y * z;
  const float wx = w * x, wy = w * y, wz = w * z;

  Mat4 r = Mat4::Identity();
  // column 0
  r.set(0, 0, 1.f - 2.f * (yy + zz));
  r.set(0, 1, 2.f * (xy + wz));
  r.set(0, 2, 2.f * (xz - wy));
  // column 1
  r.set(1, 0, 2.f * (xy - wz));
  r.set(1, 1, 1.f - 2.f * (xx + zz));
  r.set(1, 2, 2.f * (yz + wx));
  // column 2
  r.set(2, 0, 2.f * (xz + wy));
  r.set(2, 1, 2.f * (yz - wx));
  r.set(2, 2, 1.f - 2.f * (xx + yy));
  return r;
}

Quat Quat::Slerp(const Quat& a, const Quat& b, float t) {
  // Shortest path.
  float cos_omega = a.Dot(b);
  Quat b1         = b;
  if (cos_omega < 0.0f) {
    cos_omega = -cos_omega;
    b1        = Quat{-b.x, -b.y, -b.z, -b.w};
  }

  if (cos_omega > 0.9995f) {
    // Nearly colinear: fall back to nlerp and normalize.
    return Nlerp(a, b1, t);
  }

  const float omega     = std::acos(cos_omega);
  const float sin_omega = std::sin(omega);
  const float s0        = std::sin((1.0f - t) * omega) / sin_omega;
  const float s1        = std::sin(t * omega) / sin_omega;
  Quat out              = a * s0 + b1 * s1;
  out.Normalize();
  return out;
}


Quat Quat::Nlerp(const Quat& a, const Quat& b, float t) {
  // Prefer not to flip when dot is ~0 to avoid choosing the -90° arc in the 180° case.
  // Use a tiny negative tolerance to prevent accidental flips from FP noise.
  float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;

  const float hemi_tol = 1e-6f;  // tiny safety margin
  Quat bb = (dot < -hemi_tol) ? Quat{-b.x, -b.y, -b.z, -b.w} : b;

  Quat r{
    a.x + t * (bb.x - a.x),
    a.y + t * (bb.y - a.y),
    a.z + t * (bb.z - a.z),
    a.w + t * (bb.w - a.w)
  };
  // normalize
  const float len = std::sqrt(r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w);
  if (len > 0.f) { r.x/=len; r.y/=len; r.z/=len; r.w/=len; }
  return r;
}


}  // namespace navary::math

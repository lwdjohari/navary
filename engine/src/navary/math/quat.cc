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
  // R = Rx * Ry * Rz (XYZ intrinsic). Adjust if your engine prefers a different
  // order.
  const float cx = std::cos(e.x * 0.5f), sx = std::sin(e.x * 0.5f);
  const float cy = std::cos(e.y * 0.5f), sy = std::sin(e.y * 0.5f);
  const float cz = std::cos(e.z * 0.5f), sz = std::sin(e.z * 0.5f);

  // q = qx * qy * qz
  Quat qx{sx, 0, 0, cx};
  Quat qy{0, sy, 0, cy};
  Quat qz{0, 0, sz, cz};
  return (qx * qy * qz).Normalized();
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
  const float xx = x * x * 2.0f;
  const float yy = y * y * 2.0f;
  const float zz = z * z * 2.0f;
  const float xy = x * y * 2.0f;
  const float xz = x * z * 2.0f;
  const float yz = y * z * 2.0f;
  const float wx = w * x * 2.0f;
  const float wy = w * y * 2.0f;
  const float wz = w * z * 2.0f;

  Mat4 m = Mat4::Identity();
  // Column-major fill, rotation only.
  m(0, 0) = 1.0f - (yy + zz);
  m(1, 0) = xy + wz;
  m(2, 0) = xz - wy;
  m(0, 1) = xy - wz;
  m(1, 1) = 1.0f - (xx + zz);
  m(2, 1) = yz + wx;
  m(0, 2) = xz + wy;
  m(1, 2) = yz - wx;
  m(2, 2) = 1.0f - (xx + yy);
  return m;
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
  Quat b1 = b;
  if (a.Dot(b) < 0.0f) {
    b1 = Quat{-b.x, -b.y, -b.z, -b.w};
  }
  Quat out = a * (1.0f - t) + b1 * t;
  out.Normalize();
  return out;
}

}  // namespace navary::math

#include "navary/math/mat4.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace navary::math {

Mat4 Mat4::Identity() {
  Mat4 r{};
  r(0, 0) = 1;
  r(1, 1) = 1;
  r(2, 2) = 1;
  r(3, 3) = 1;
  return r;
}

Mat4 Mat4::Zero() {
  Mat4 r{};
  std::fill(r.m_.begin(), r.m_.end(), 0.0f);
  return r;
}

Mat4 Mat4::operator*(const Mat4& rhs) const {
  Mat4 out{};
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      float sum = 0.0f;
      sum += get(0, r) * rhs.get(c, 0);
      sum += get(1, r) * rhs.get(c, 1);
      sum += get(2, r) * rhs.get(c, 2);
      sum += get(3, r) * rhs.get(c, 3);
      out.set(c, r, sum);
    }
  }
  return out;
}

Mat4& Mat4::operator*=(const Mat4& rhs) {
  *this = (*this) * rhs;
  return *this;
}

bool Mat4::operator==(const Mat4& rhs) const {
  for (int i = 0; i < 16; ++i) {
    if (m_[i] != rhs.m_[i])
      return false;
  }
  return true;
}

Vec4 Mat4::Transform(const Vec4& v) const {
  // result = M * v  (column-major)
  return Vec4{
      get(0, 0) * v.x + get(1, 0) * v.y + get(2, 0) * v.z + get(3, 0) * v.w,
      get(0, 1) * v.x + get(1, 1) * v.y + get(2, 1) * v.z + get(3, 1) * v.w,
      get(0, 2) * v.x + get(1, 2) * v.y + get(2, 2) * v.z + get(3, 2) * v.w,
      get(0, 3) * v.x + get(1, 3) * v.y + get(2, 3) * v.z + get(3, 3) * v.w};
}

Vec3 Mat4::TransformPoint(const Vec3& p) const {
  const Vec4 v = Transform(Vec4{p.x, p.y, p.z, 1.0f});
  return Vec3{v.x / v.w, v.y / v.w, v.z / v.w};
}

Vec3 Mat4::TransformVector(const Vec3& v3) const {
  const Vec4 v = Transform(Vec4{v3.x, v3.y, v3.z, 0.0f});
  return Vec3{v.x, v.y, v.z};
}

Mat4 Mat4::Translation(const Vec3& t) {
  Mat4 r = Identity();
  r.set(3, 0, t.x);
  r.set(3, 1, t.y);
  r.set(3, 2, t.z);
  return r;
}

Mat4 Mat4::Scaling(const Vec3& s) {
  Mat4 r = Identity();
  r.set(0, 0, s.x);
  r.set(1, 1, s.y);
  r.set(2, 2, s.z);
  return r;
}

Mat4 Mat4::RotationX(float angle) {
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  Mat4 r        = Identity();
  r.set(1, 1, c);
  r.set(2, 1, s);
  r.set(1, 2, -s);
  r.set(2, 2, c);
  return r;
}

Mat4 Mat4::RotationY(float angle) {
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  Mat4 r        = Identity();
  r.set(0, 0, c);
  r.set(2, 0, -s);
  r.set(0, 2, s);
  r.set(2, 2, c);
  return r;
}

Mat4 Mat4::RotationZ(float angle) {
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  Mat4 r        = Identity();
  r.set(0, 0, c);
  r.set(1, 0, s);
  r.set(0, 1, -s);
  r.set(1, 1, c);
  return r;
}

Mat4 Mat4::PerspectiveRH(float fovy, float aspect, float z_near, float z_far) {
  // OpenGL clip: z in [-1, 1], right-handed, -Z forward.
  const float f = 1.0f / std::tan(fovy * 0.5f);
  Mat4 r        = Zero();
  r.set(0, 0, f / aspect);
  r.set(1, 1, f);
  r.set(2, 2, (z_far + z_near) / (z_near - z_far));
  r.set(3, 2, (2.0f * z_far * z_near) / (z_near - z_far));
  r.set(2, 3, -1.0f);
  return r;
}

Mat4 Mat4::OrthoRH(float left, float right, float bottom, float top,
                   float z_near, float z_far) {
  Mat4 r = Identity();
  r.set(0, 0, 2.0f / (right - left));
  r.set(1, 1, 2.0f / (top - bottom));
  r.set(2, 2, -2.0f / (z_far - z_near));
  r.set(3, 0, -(right + left) / (right - left));
  r.set(3, 1, -(top + bottom) / (top - bottom));
  r.set(3, 2, -(z_far + z_near) / (z_far - z_near));
  return r;
}

Mat4 Mat4::TRS(const Vec3& t, const Vec3& rot_xyz, const Vec3& s) {
  Mat4 m = Translation(t);
  m *= RotationZ(rot_xyz.z);
  m *= RotationY(rot_xyz.y);
  m *= RotationX(rot_xyz.x);
  m *= Scaling(s);
  return m;
}

Mat4 Mat4::Transpose(const Mat4& a) {
  Mat4 r{};
  for (int c = 0; c < 4; ++c)
    for (int r_i = 0; r_i < 4; ++r_i)
      r.set(r_i, c, a.get(c, r_i));
  return r;
}

// General 4x4 inverse via Gauss-Jordan (not the fastest, but robust).
Mat4 Mat4::Inverse(const Mat4& m) {
  Mat4 inv = Identity();
  Mat4 a   = m;

  for (int i = 0; i < 4; ++i) {
    // Find pivot.
    int pivot  = i;
    float maxv = std::fabs(a.get(i, i));
    for (int r = i + 1; r < 4; ++r) {
      const float v = std::fabs(a.get(i, r));
      if (v > maxv) {
        maxv  = v;
        pivot = r;
      }
    }
    // Swap rows in both a and inv.
    if (pivot != i) {
      for (int c = 0; c < 4; ++c) {
        std::swap(a.m_[c * 4 + i], a.m_[c * 4 + pivot]);
        std::swap(inv.m_[c * 4 + i], inv.m_[c * 4 + pivot]);
      }
    }
    // Scale row to make diagonal 1.
    const float diag     = a.get(i, i);
    const float inv_diag = 1.0f / diag;
    for (int c = 0; c < 4; ++c) {
      a.set(c, i, a.get(c, i) * inv_diag);
      inv.set(c, i, inv.get(c, i) * inv_diag);
    }
    // Eliminate other rows.
    for (int r = 0; r < 4; ++r) {
      if (r == i)
        continue;
      const float f = a.get(i, r);
      if (f == 0.0f)
        continue;
      for (int c = 0; c < 4; ++c) {
        a.set(c, r, a.get(c, r) - f * a.get(c, i));
        inv.set(c, r, inv.get(c, r) - f * inv.get(c, i));
      }
    }
  }
  return inv;
}

// Inverse for affine matrices [ R t; 0 1 ], where R is 3x3.
Mat4 Mat4::InverseAffine(const Mat4& m) {
  // Extract R and t.
  const Vec3 r0{m.get(0, 0), m.get(0, 1), m.get(0, 2)};
  const Vec3 r1{m.get(1, 0), m.get(1, 1), m.get(1, 2)};
  const Vec3 r2{m.get(2, 0), m.get(2, 1), m.get(2, 2)};
  const Vec3 t{m.get(3, 0), m.get(3, 1), m.get(3, 2)};

  // Inverse(R) = transpose(R) if R is orthonormal. If scaled, this assumes
  // uniform or that users won’t call this—otherwise, use general Inverse().
  Mat4 inv_r = Identity();
  inv_r.set(0, 0, r0.x);
  inv_r.set(1, 0, r0.y);
  inv_r.set(2, 0, r0.z);
  inv_r.set(0, 1, r1.x);
  inv_r.set(1, 1, r1.y);
  inv_r.set(2, 1, r1.z);
  inv_r.set(0, 2, r2.x);
  inv_r.set(1, 2, r2.y);
  inv_r.set(2, 2, r2.z);

  // inv translation = -invR * t
  const Vec3 t_inv = Vec3{
      -(inv_r.get(0, 0) * t.x + inv_r.get(1, 0) * t.y + inv_r.get(2, 0) * t.z),
      -(inv_r.get(0, 1) * t.x + inv_r.get(1, 1) * t.y + inv_r.get(2, 1) * t.z),
      -(inv_r.get(0, 2) * t.x + inv_r.get(1, 2) * t.y + inv_r.get(2, 2) * t.z)};

  Mat4 out = inv_r;
  out.set(3, 0, t_inv.x);
  out.set(3, 1, t_inv.y);
  out.set(3, 2, t_inv.z);
  out.set(3, 3, 1.0f);
  return out;
}

Vec3 Mat4::Right() const {
  return Vec3{get(0, 0), get(0, 1), get(0, 2)};
}
Vec3 Mat4::Up() const {
  return Vec3{get(1, 0), get(1, 1), get(1, 2)};
}
Vec3 Mat4::Forward() const {
  return Vec3{get(2, 0), get(2, 1), get(2, 2)};
}
Vec3 Mat4::Translation() const {
  return Vec3{get(3, 0), get(3, 1), get(3, 2)};
}

}  // namespace navary::math

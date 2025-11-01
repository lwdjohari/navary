#pragma once
// Navary Engine - Fixed Math
// 4x4 matrix (column-major, OpenGL-style) in Fixed15p16 for lockstep
// simulation.

#include <array>
#include <algorithm>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"
#include "navary/math/fixed_trigonometry.h"

namespace navary::math {

using F = Fixed15p16;

// /** Deterministic fixed abs (kept local to avoid header cycles). */
// inline F FAbs(F v) {
//   return (v.Raw() < 0) ? F::FromRaw(static_cast<F::RepType>(-v.Raw())) : v;
// }

/**
 * @brief Column-major 4x4 fixed-point matrix (like GLSL / OpenGL).
 * Indices are (column, row). So m(0,0) is first column, first row.
 */
class FixedMat4 {
 public:
  FixedMat4() = default;

  // Construct from 16 scalars (column-major; a00=m(0,0), a10=m(1,0), ...).
  FixedMat4(F a00, F a01, F a02, F a03, F a10, F a11, F a12, F a13, F a20,
            F a21, F a22, F a23, F a30, F a31, F a32, F a33) {
    set(0, 0, a00);
    set(0, 1, a01);
    set(0, 2, a02);
    set(0, 3, a03);
    set(1, 0, a10);
    set(1, 1, a11);
    set(1, 2, a12);
    set(1, 3, a13);
    set(2, 0, a20);
    set(2, 1, a21);
    set(2, 2, a22);
    set(2, 3, a23);
    set(3, 0, a30);
    set(3, 1, a31);
    set(3, 2, a32);
    set(3, 3, a33);
  }

  // Identity / Zero
  static FixedMat4 Identity() {
    FixedMat4 r;
    r(0, 0) = F::FromInt(1);
    r(1, 1) = F::FromInt(1);
    r(2, 2) = F::FromInt(1);
    r(3, 3) = F::FromInt(1);
    return r;
  }

  static FixedMat4 Zero() {
    FixedMat4 r;  // default zero-initialized due to F default?
    std::fill(r.m_.begin(), r.m_.end(), F::Zero());
    return r;
  }

  // Access
  F& operator()(int col, int row) {
    return m_[col * 4 + row];
  }
  const F& operator()(int col, int row) const {
    return m_[col * 4 + row];
  }

  void set(int col, int row, F v) {
    m_[col * 4 + row] = v;
  }
  F get(int col, int row) const {
    return m_[col * 4 + row];
  }

  const F* data() const {
    return m_.data();
  }
  F* data() {
    return m_.data();
  }

  // Matrix multiply (column-major): out = this * rhs
  FixedMat4 operator*(const FixedMat4& rhs) const {
    FixedMat4 out = Zero();
    for (int c = 0; c < 4; ++c) {
      for (int r = 0; r < 4; ++r) {
        // sum_k this(k,r) * rhs(c,k)
        F sum = F::Zero();
        sum += Mul(get(0, r), rhs.get(c, 0));
        sum += Mul(get(1, r), rhs.get(c, 1));
        sum += Mul(get(2, r), rhs.get(c, 2));
        sum += Mul(get(3, r), rhs.get(c, 3));
        out.set(c, r, sum);
      }
    }
    return out;
  }

  FixedMat4& operator*=(const FixedMat4& rhs) {
    *this = (*this) * rhs;
    return *this;
  }

  bool operator==(const FixedMat4& rhs) const {
    for (int i = 0; i < 16; ++i)
      if (m_[i] != rhs.m_[i])
        return false;
    return true;
  }
  bool operator!=(const FixedMat4& rhs) const {
    return !(*this == rhs);
  }

  // Transform (column-vector convention)
  FixedVec3 TransformPoint(const FixedVec3& p) const {
    // v = M * [x y z 1]^T ; simulation uses affine only (no perspective)
    const F one = F::FromInt(1);
    F x = get(0, 0) * p.x + get(1, 0) * p.y + get(2, 0) * p.z + get(3, 0) * one;
    F y = get(0, 1) * p.x + get(1, 1) * p.y + get(2, 1) * p.z + get(3, 1) * one;
    F z = get(0, 2) * p.x + get(1, 2) * p.y + get(2, 2) * p.z + get(3, 2) * one;

    // Do NOT divide by w for fixed-lockstep (we don’t use projection here).
    return FixedVec3{x, y, z};
  }

  FixedVec3 TransformVector(const FixedVec3& v3) const {
    // w=0 so translation ignored
    F x = get(0, 0) * v3.x + get(1, 0) * v3.y + get(2, 0) * v3.z;
    F y = get(0, 1) * v3.x + get(1, 1) * v3.y + get(2, 1) * v3.z;
    F z = get(0, 2) * v3.x + get(1, 2) * v3.y + get(2, 2) * v3.z;
    return FixedVec3{x, y, z};
  }

  // Builders (angles in radians, fixed)
  static FixedMat4 Translation(const FixedVec3& t) {
    FixedMat4 r = Identity();
    r.set(3, 0, t.x);
    r.set(3, 1, t.y);
    r.set(3, 2, t.z);
    return r;
  }

  static FixedMat4 Scaling(const FixedVec3& s) {
    FixedMat4 r = Zero();
    r.set(0, 0, s.x);
    r.set(1, 1, s.y);
    r.set(2, 2, s.z);
    r.set(3, 3, F::FromInt(1));
    return r;
  }

  static FixedMat4 RotationX(F angle) {
    F s, c;
    FixedTrig::SinCos(angle, s, c);
    FixedMat4 r = Identity();
    r.set(1, 1, c);
    r.set(2, 1, s);
    r.set(1, 2, -s);
    r.set(2, 2, c);
    return r;
  }

  static FixedMat4 RotationY(F angle) {
    F s, c;
    FixedTrig::SinCos(angle, s, c);
    FixedMat4 r = Identity();
    r.set(0, 0, c);
    r.set(2, 0, -s);
    r.set(0, 2, s);
    r.set(2, 2, c);
    return r;
  }

  static FixedMat4 RotationZ(F angle) {
    F s, c;
    FixedTrig::SinCos(angle, s, c);
    FixedMat4 r = Identity();
    r.set(0, 0, c);
    r.set(1, 0, s);
    r.set(0, 1, -s);
    r.set(1, 1, c);
    return r;
  }

  // PerspectiveRH/OrthoRH (kept for parity – use float for rendering normally).
  static FixedMat4 PerspectiveRH(F fovy, F aspect, F z_near, F z_far) {
    // f = 1/tan(fovy/2)
    F half = fovy / Fixed15p16::FromInt(2);
    F s, c;
    FixedTrig::SinCos(half, s, c);
    F f = c / s;

    FixedMat4 r = Zero();
    r.set(0, 0, f / aspect);
    r.set(1, 1, f);
    r.set(2, 2, (z_far + z_near) / (z_near - z_far));
    r.set(3, 2, (Fixed15p16::FromInt(2) * z_far * z_near) / (z_near - z_far));
    r.set(2, 3, Fixed15p16::FromInt(-1));
    return r;
  }

  static FixedMat4 OrthoRH(F left, F right, F bottom, F top, F z_near,
                           F z_far) {
    FixedMat4 r = Identity();
    r.set(0, 0, Fixed15p16::FromInt(2) / (right - left));
    r.set(1, 1, Fixed15p16::FromInt(2) / (top - bottom));
    r.set(2, 2, (Fixed15p16::FromInt(-2)) / (z_far - z_near));
    r.set(3, 0, -(right + left) / (right - left));
    r.set(3, 1, -(top + bottom) / (top - bottom));
    r.set(3, 2, -(z_far + z_near) / (z_far - z_near));
    return r;
  }

  // TRS = T * Rz * Ry * Rx * S   (column-vector convention)
  static FixedMat4 TRS(const FixedVec3& t, const FixedVec3& rot_xyz,
                       const FixedVec3& s) {
    return Translation(t) * RotationZ(rot_xyz.z) * RotationY(rot_xyz.y) *
           RotationX(rot_xyz.x) * Scaling(s);
  }

  // Utilities
  static FixedMat4 Transpose(const FixedMat4& a) {
    FixedMat4 r = Zero();
    for (int c = 0; c < 4; ++c)
      for (int rr = 0; rr < 4; ++rr)
        r.set(rr, c, a.get(c, rr));
    return r;
  }

  // General 4x4 inverse via Gauss-Jordan (fixed, deterministic).
  static FixedMat4 Inverse(const FixedMat4& m) {
    FixedMat4 a   = m;
    FixedMat4 inv = Identity();

    for (int i = 0; i < 4; ++i) {
      // pivot: pick row with max |a(i, row)|
      int pivot = i;
      F maxv    = FAbs(a.get(i, i));
      for (int r = i + 1; r < 4; ++r) {
        F v = FAbs(a.get(i, r));
        if (v > maxv) {
          maxv  = v;
          pivot = r;
        }
      }
      // swap rows
      if (pivot != i) {
        for (int c = 0; c < 4; ++c) {
          std::swap(a.m_[c * 4 + i], a.m_[c * 4 + pivot]);
          std::swap(inv.m_[c * 4 + i], inv.m_[c * 4 + pivot]);
        }
      }
      // scale row to make diagonal 1
      F diag = a.get(i, i);
      // If singular (diag == 0), leave as best-effort (no throw in lockstep
      // code)
      if (diag.IsZero())
        return Identity();
      for (int c = 0; c < 4; ++c) {
        a.set(c, i, a.get(c, i) / diag);
        inv.set(c, i, inv.get(c, i) / diag);
      }
      // eliminate others
      for (int r = 0; r < 4; ++r)
        if (r != i) {
          F f = a.get(i, r);
          if (!f.IsZero()) {
            for (int c = 0; c < 4; ++c) {
              a.set(c, r, a.get(c, r) - f * a.get(c, i));
              inv.set(c, r, inv.get(c, r) - f * inv.get(c, i));
            }
          }
        }
    }
    return inv;
  }

  // Inverse for affine (rigid or uniform scale) [ R t; 0 1 ]
  static FixedMat4 InverseAffine(const FixedMat4& m) {
    // Extract R columns and t
    const F r00 = m.get(0, 0), r01 = m.get(0, 1), r02 = m.get(0, 2);
    const F r10 = m.get(1, 0), r11 = m.get(1, 1), r12 = m.get(1, 2);
    const F r20 = m.get(2, 0), r21 = m.get(2, 1), r22 = m.get(2, 2);
    const FixedVec3 t{m.get(3, 0), m.get(3, 1), m.get(3, 2)};

    // invR = R^T (put ROWS of R as COLUMNS of invR)
    FixedMat4 inv = FixedMat4::Identity();
    inv.set(0, 0, r00);
    inv.set(0, 1, r10);
    inv.set(0, 2, r20);
    inv.set(1, 0, r01);
    inv.set(1, 1, r11);
    inv.set(1, 2, r21);
    inv.set(2, 0, r02);
    inv.set(2, 1, r12);
    inv.set(2, 2, r22);

    // t_inv = -invR * t
    const F tx =
        -(inv.get(0, 0) * t.x + inv.get(1, 0) * t.y + inv.get(2, 0) * t.z);
    const F ty =
        -(inv.get(0, 1) * t.x + inv.get(1, 1) * t.y + inv.get(2, 1) * t.z);
    const F tz =
        -(inv.get(0, 2) * t.x + inv.get(1, 2) * t.y + inv.get(2, 2) * t.z);

    inv.set(3, 0, tx);
    inv.set(3, 1, ty);
    inv.set(3, 2, tz);
    inv.set(3, 3, F::FromInt(1));
    return inv;
  }

  // Extract basis/translation (assuming affine)
  FixedVec3 Right() const {
    return FixedVec3{get(0, 0), get(0, 1), get(0, 2)};
  }

  FixedVec3 Up() const {
    return FixedVec3{get(1, 0), get(1, 1), get(1, 2)};
  }

  FixedVec3 Forward() const {
    return FixedVec3{get(2, 0), get(2, 1), get(2, 2)};
  }

  FixedVec3 Translation() const {
    return FixedVec3{get(3, 0), get(3, 1), get(3, 2)};
  }

 private:
  std::array<F, 16> m_{};
};

}  // namespace navary::math

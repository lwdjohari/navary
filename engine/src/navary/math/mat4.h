#pragma once
// Navary Engine - Math
// 4x4 matrix (column-major, OpenGL-style) for graphics transforms.
// Use this for rendering/camera; keep fixed-point types for simulation.

#include <array>
#include <cstddef>
#include <cstdint>

#include "navary/math/vec3.h"
#include "navary/math/vec4.h"

namespace navary::math {

// Column-major 4x4 float matrix.
// Layout matches GLSL: the `data()` pointer can be passed directly to OpenGL.
// Indices are (column, row). So m(0,0) is first column, first row.
class Mat4 {
 public:
  // Uninitialized by default (for speed).
  Mat4() = default;

  // Construct from 16 scalars (column-major order).
  // Args a00..a33 are in (column,row) order: a00=m(0,0), a10=m(1,0), etc.
  Mat4(float a00, float a01, float a02, float a03, float a10, float a11,
       float a12, float a13, float a20, float a21, float a22, float a23,
       float a30, float a31, float a32, float a33) {
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

  // Identity / Zero.
  static Mat4 Identity();
  static Mat4 Zero();

  // Basic access.
  float& operator()(int col, int row) {
    return m_[col * 4 + row];
  }
  const float& operator()(int col, int row) const {
    return m_[col * 4 + row];
  }

  void set(int col, int row, float v) {
    m_[col * 4 + row] = v;
  }
  float get(int col, int row) const {
    return m_[col * 4 + row];
  }

  const float* data() const {
    return m_.data();
  }
  float* data() {
    return m_.data();
  }

  // Matrix ops.
  Mat4 operator*(const Mat4& rhs) const;  // matrix multiply
  Mat4& operator*=(const Mat4& rhs);

  bool operator==(const Mat4& rhs) const;
  bool operator!=(const Mat4& rhs) const {
    return !(*this == rhs);
  }

  // Transform points/vectors (column vector convention).
  // TransformPoint uses w=1; TransformVector uses w=0.
  Vec3 TransformPoint(const Vec3& p) const;
  Vec3 TransformVector(const Vec3& v) const;
  Vec4 Transform(const Vec4& v) const;

  // Builders (OpenGL-style).
  static Mat4 Translation(const Vec3& t);
  static Mat4 Scaling(const Vec3& s);
  static Mat4 RotationX(float radians);
  static Mat4 RotationY(float radians);
  static Mat4 RotationZ(float radians);

  // Perspective (right-handed, OpenGL clip space: z ∈ [-1,1], -Z forward).
  // fovy in radians.
  static Mat4 PerspectiveRH(float fovy, float aspect, float z_near,
                            float z_far);

  // Orthographic (right-handed, OpenGL clip space).
  static Mat4 OrthoRH(float left, float right, float bottom, float top,
                      float z_near, float z_far);

  // Compose TRS in this order: T * Rz * Ry * Rx * S (common camera/object use).
  static Mat4 TRS(const Vec3& t, const Vec3& rot_xyz_radians, const Vec3& s);

  // Useful utilities.
  static Mat4 Transpose(const Mat4& m);
  static Mat4 Inverse(const Mat4& m);  // general 4x4 inverse
  static Mat4 InverseAffine(
      const Mat4& m);  // faster when last row is [0 0 0 1]

  // Extract basis/translation (assuming affine).
  Vec3 Right() const;        // +X axis (column 0)
  Vec3 Up() const;           // +Y axis (column 1)
  Vec3 Forward() const;      // +Z axis (column 2)
  Vec3 Translation() const;  // column 3 (xyz)

 private:
  std::array<float, 16> m_{};  // column-major, 4*4

  // Friend for quaternion to fill a rotation.
  friend class Quat;
};

}  // namespace navary::math

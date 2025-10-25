// navary/math/vec3.h

#pragma once
#include <cmath>
#include <cstdio>

namespace navary::math {

/**
 * @brief Simple 3-component float vector for math, physics, and graphics.
 * No templates, no dependencies.
 */
class Vec3 {
 public:
  float x{0.f}, y{0.f}, z{0.f};

  Vec3() = default;
  constexpr Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

  // --- Arithmetic operators ---
  constexpr Vec3 operator+(const Vec3& v) const {
    return {x + v.x, y + v.y, z + v.z};
  }
  constexpr Vec3 operator-(const Vec3& v) const {
    return {x - v.x, y - v.y, z - v.z};
  }
  constexpr Vec3 operator*(float s) const {
    return {x * s, y * s, z * s};
  }
  constexpr Vec3 operator/(float s) const {
    return {x / s, y / s, z / s};
  }

  friend constexpr Vec3 operator*(float s, const Vec3& v) noexcept {
    return {v.x * s, v.y * s, v.z * s};
  }

  Vec3& operator+=(const Vec3& v);
  Vec3& operator-=(const Vec3& v);
  Vec3& operator*=(float s);
  Vec3& operator/=(float s);

  constexpr bool operator==(const Vec3& v) const {
    return x == v.x && y == v.y && z == v.z;
  }
  constexpr bool operator!=(const Vec3& v) const {
    return !(*this == v);
  }

  // --- Math helpers ---
  float length() const;
  float length_sq() const;
  Vec3 normalized() const;
  void normalize();
  float dot(const Vec3& v) const;
  Vec3 cross(const Vec3& v) const;

  static Vec3 Zero();
  static Vec3 One();

  void print() const;
};
}  // namespace navary::math
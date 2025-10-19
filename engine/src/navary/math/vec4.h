#pragma once
#include <cmath>
#include <cstdio>

#include "navary/math/vec3.h"

namespace navary::math {

/**
 * @brief Simple 4-component float vector for homogeneous coordinates or color.
 */
class Vec4 {
 public:
  float x{0.f}, y{0.f}, z{0.f}, w{0.f};

  Vec4() = default;

  constexpr Vec4(float _x, float _y, float _z, float _w)
      : x(_x), y(_y), z(_z), w(_w) {}

  constexpr explicit Vec4(const Vec3& v, float _w = 1.f)
      : x(v.x), y(v.y), z(v.z), w(_w) {}

  // --- Arithmetic operators ---
  constexpr Vec4 operator+(const Vec4& v) const {
    return {x + v.x, y + v.y, z + v.z, w + v.w};
  }

  constexpr Vec4 operator-(const Vec4& v) const {
    return {x - v.x, y - v.y, z - v.z, w - v.w};
  }

  constexpr Vec4 operator*(float s) const {
    return {x * s, y * s, z * s, w * s};
  }

  constexpr Vec4 operator/(float s) const {
    return {x / s, y / s, z / s, w / s};
  }

  friend constexpr Vec4 operator*(float s, const Vec4& v) noexcept {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
  }

  Vec4& operator+=(const Vec4& v);
  Vec4& operator-=(const Vec4& v);
  Vec4& operator*=(float s);
  Vec4& operator/=(float s);

  constexpr bool operator==(const Vec4& v) const {
    return x == v.x && y == v.y && z == v.z && w == v.w;
  }

  constexpr bool operator!=(const Vec4& v) const {
    return !(*this == v);
  }

  // --- Math helpers ---
  float length() const;
  float length_sq() const;
  Vec4 normalized() const;
  void normalize();
  float dot(const Vec4& v) const;

  static Vec4 Zero();
  static Vec4 One();

  void print() const;
};
}  // namespace navary::math
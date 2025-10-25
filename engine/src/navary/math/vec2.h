// navary/math/vec2.h

#pragma once
#include <cmath>
#include <cstdio>

namespace navary::math {

/**
 * @brief 2D float vector for positions, directions, and math ops.
 * Provides arithmetic, normalization, and dot/cross helpers.
 */
class Vec2 {
 public:
  float x{0.f};
  float y{0.f};

  Vec2() = default;
  constexpr Vec2(float _x, float _y) : x(_x), y(_y) {}

  // --- Arithmetic ---
  constexpr Vec2 operator+(const Vec2& v) const {
    return {x + v.x, y + v.y};
  }

  constexpr Vec2 operator-(const Vec2& v) const {
    return {x - v.x, y - v.y};
  }

  constexpr Vec2 operator*(float s) const {
    return {x * s, y * s};
  }

  constexpr Vec2 operator/(float s) const {
    return {x / s, y / s};
  }

  friend constexpr Vec2 operator*(float s, const Vec2& v) noexcept {
    return {v.x * s, v.y * s};
  }

  Vec2& operator+=(const Vec2& v);
  Vec2& operator-=(const Vec2& v);
  Vec2& operator*=(float s);
  Vec2& operator/=(float s);

  constexpr bool operator==(const Vec2& v) const {
    return x == v.x && y == v.y;
  }
  constexpr bool operator!=(const Vec2& v) const {
    return !(*this == v);
  }

  // --- Math helpers ---
  float length() const;
  float length_sq() const;
  Vec2 normalized() const;
  void normalize();
  float dot(const Vec2& v) const;
  float cross(const Vec2& v) const;  // scalar 2D cross (determinant)

  static Vec2 Zero();
  static Vec2 One();

  void print() const;
};

}  // namespace navary::math

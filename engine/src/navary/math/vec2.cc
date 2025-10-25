// navary/math/vec2.cc

#include "navary/math/vec2.h"
#include <cmath>
#include <cstdio>

namespace navary::math {

Vec2& Vec2::operator+=(const Vec2& v) {
  x += v.x;
  y += v.y;
  return *this;
}

Vec2& Vec2::operator-=(const Vec2& v) {
  x -= v.x;
  y -= v.y;
  return *this;
}

Vec2& Vec2::operator*=(float s) {
  x *= s;
  y *= s;
  return *this;
}

Vec2& Vec2::operator/=(float s) {
  x /= s;
  y /= s;
  return *this;
}

float Vec2::length_sq() const {
  return x * x + y * y;
}
float Vec2::length() const {
  return std::sqrt(length_sq());
}

Vec2 Vec2::normalized() const {
  float len = length();
  return (len > 0.f) ? (*this / len) : Vec2{0.f, 0.f};
}

void Vec2::normalize() {
  float len = length();
  if (len > 0.f) {
    x /= len;
    y /= len;
  }
}

float Vec2::dot(const Vec2& v) const {
  return x * v.x + y * v.y;
}

// cross product in 2D (returns scalar z-component)
float Vec2::cross(const Vec2& v) const {
  return x * v.y - y * v.x;
}

Vec2 Vec2::Zero() {
  return {0.f, 0.f};
}

Vec2 Vec2::One() {
  return {1.f, 1.f};
}

void Vec2::print() const {
  std::printf("Vec2(%.3f, %.3f)\n", x, y);
}

}  // namespace navary::math

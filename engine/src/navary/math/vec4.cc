#include "navary/math/vec4.h"
#include <cmath>
#include <cstdio>

namespace navary::math {
    
// ---------------------- Vec4 ----------------------
Vec4& Vec4::operator+=(const Vec4& v) {
  x += v.x;
  y += v.y;
  z += v.z;
  w += v.w;
  return *this;
}

Vec4& Vec4::operator-=(const Vec4& v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
  w -= v.w;
  return *this;
}

Vec4& Vec4::operator*=(float s) {
  x *= s;
  y *= s;
  z *= s;
  w *= s;
  return *this;
}

Vec4& Vec4::operator/=(float s) {
  x /= s;
  y /= s;
  z /= s;
  w /= s;
  return *this;
}

float Vec4::length_sq() const {
  return x * x + y * y + z * z + w * w;
}

float Vec4::length() const {
  return std::sqrt(length_sq());
}

Vec4 Vec4::normalized() const {
  float len = length();
  return (len > 0.f) ? (*this / len) : Vec4{0.f, 0.f, 0.f, 0.f};
}

void Vec4::normalize() {
  float len = length();
  if (len > 0.f) {
    x /= len;
    y /= len;
    z /= len;
    w /= len;
  }
}

float Vec4::dot(const Vec4& v) const {
  return x * v.x + y * v.y + z * v.z + w * v.w;
}

Vec4 Vec4::Zero() {
  return {0.f, 0.f, 0.f, 0.f};
}
Vec4 Vec4::One() {
  return {1.f, 1.f, 1.f, 1.f};
}

void Vec4::print() const {
  std::printf("Vec4(%.3f, %.3f, %.3f, %.3f)\n", x, y, z, w);
}

}  // namespace navary::math
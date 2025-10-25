// navary/math/vec3.cc

#include "navary/math/vec3.h"
#include <cmath>
#include <cstdio>

namespace navary::math {

// ---------------------- Vec3 ----------------------
Vec3& Vec3::operator+=(const Vec3& v) {
  x += v.x;
  y += v.y;
  z += v.z;
  return *this;
}

Vec3& Vec3::operator-=(const Vec3& v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
  return *this;
}

Vec3& Vec3::operator*=(float s) {
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

Vec3& Vec3::operator/=(float s) {
  x /= s;
  y /= s;
  z /= s;
  return *this;
}

float Vec3::length_sq() const {
  return x * x + y * y + z * z;
}

float Vec3::length() const {
  return std::sqrt(length_sq());
}

Vec3 Vec3::normalized() const {
  float len = length();
  return (len > 0.f) ? (*this / len) : Vec3{0.f, 0.f, 0.f};
}

void Vec3::normalize() {
  float len = length();
  if (len > 0.f) {
    x /= len;
    y /= len;
    z /= len;
  }
}

float Vec3::dot(const Vec3& v) const {
  return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::cross(const Vec3& v) const {
  return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
}

Vec3 Vec3::Zero() {
  return {0.f, 0.f, 0.f};
}

Vec3 Vec3::One() {
  return {1.f, 1.f, 1.f};
}

void Vec3::print() const {
  std::printf("Vec3(%.3f, %.3f, %.3f)\n", x, y, z);
}
}  // namespace navary::math
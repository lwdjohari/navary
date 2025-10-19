#pragma once
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace navary::math {

// Angles
constexpr float kPi = 3.14159265358979323846f;

constexpr float DegToRad(float deg) {
  return deg * (kPi / 180.0f);
}
constexpr float RadToDeg(float rad) {
  return rad * (180.0f / kPi);
}

// Square
template <typename T>
constexpr T Sqr(T x) {
  return x * x;
}

// Clamp
template <typename T>
constexpr T Clamp(T v, T lo, T hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Lerp
template <typename T, typename U>
constexpr T Lerp(const T& a, const T& b, U t) {
  return a + (b - a) * static_cast<float>(t);
}

// SmoothStep (Hermite)
template <typename T>
T SmoothStep(T edge0, T edge1, T x) {
  const T t = Clamp<T>((x - edge0) / (edge1 - edge0), static_cast<T>(0),
                       static_cast<T>(1));
  return t * t * (static_cast<T>(3) - static_cast<T>(2) * t);
}

// Sign
template <typename T>
constexpr T Sign(const T v) {
  return (v > T(0)) ? T(1) : (v < T(0)) ? T(-1) : T(0);
}

}  // namespace navary::math

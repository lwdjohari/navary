// navary/math/size2.h

#pragma once
#include <cstdint>

namespace navary::math {

/**
 * @brief Lightweight width/height container (no math).
 */
struct Size2 {
  float w{0.f};
  float h{0.f};
  constexpr Size2() = default;
  constexpr Size2(float _w, float _h) : w(_w), h(_h) {}
  friend constexpr bool operator==(const Size2& a, const Size2& b) {
    return a.w == b.w && a.h == b.h;
  }
};

}  // namespace navary::math
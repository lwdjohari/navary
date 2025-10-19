#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include "navary/math/vec4.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

TEST_CASE("Vec4: color operations (RGBA)", "[vec4][color][game]") {
  Vec4 c{0.2f, 0.4f, 0.8f, 1.0f};  // base color
  Vec4 tint{1.1f, 0.9f, 0.7f, 1.0f};

  // simple modulate and additive pulse
  Vec4 mod = Vec4{c.x * tint.x, c.y * tint.y, c.z * tint.z, c.w * tint.w};
  Vec4 add = c + Vec4{0.1f, 0.0f, 0.0f, 0.0f};
  REQUIRE(mod.x > c.x - 1e-6f);
  REQUIRE(add.x > c.x);

  // alpha pre-multiply check
  Vec4 a{0.5f, 0.25f, 0.75f, 0.5f};
  Vec4 premul{a.x * a.w, a.y * a.w, a.z * a.w, a.w};
  REQUIRE_THAT(premul.x, WithinAbs(0.25f, 1e-6f));
}

TEST_CASE("Vec4: vector length and normalization", "[vec4][math]") {
  Vec4 v{2, 2, 1, 1};
  REQUIRE_THAT(v.length_sq(), WithinAbs(10.f, 1e-6f));
  auto n = v.normalized();
  REQUIRE_THAT(n.length(), WithinAbs(1.f, 1e-6f));

  // division safety
  Vec4 z = Vec4::Zero();
  REQUIRE(z.normalized() == Vec4::Zero());
}

TEST_CASE("Vec4: homogeneous point from Vec3", "[vec4][homogeneous][game]") {
  Vec3 p3{3.f, 4.f, 5.f};
  Vec4 p4(p3, 1.f);
  REQUIRE_THAT(p4.w, WithinAbs(1.f, 1e-6f));
  REQUIRE_THAT(p4.x, WithinAbs(3.f, 1e-6f));
  REQUIRE_THAT(p4.y, WithinAbs(4.f, 1e-6f));
  REQUIRE_THAT(p4.z, WithinAbs(5.f, 1e-6f));
}

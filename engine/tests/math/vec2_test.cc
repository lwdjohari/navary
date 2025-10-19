#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include "navary/math/vec2.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

TEST_CASE("Vec2: player movement and steering", "[vec2][movement][game]") {
  // WASD-like input -> velocity accumulation
  Vec2 pos{10.f, 5.f};
  Vec2 v = Vec2::Zero();

  // press W then D
  v += Vec2{0.f, -1.f};
  v += Vec2{1.f, 0.f};

  // normalize to avoid diagonal speed boost
  v.normalize();
  REQUIRE_THAT(v.length(), WithinAbs(1.f, 1e-6f));
  pos += v * 5.f;  // move 5 units
  REQUIRE_THAT(pos.x, WithinAbs(10.f + 5.f / std::sqrt(2.f), 1e-4f));
  REQUIRE_THAT(pos.y, WithinAbs(5.f - 5.f / std::sqrt(2.f), 1e-4f));
}

TEST_CASE("Vec2: facing and signed angle via cross/dot",
          "[vec2][facing][game]") {
  Vec2 fwd{1.f, 0.f};     // facing right
  Vec2 target{0.f, 1.f};  // up

  float dot    = fwd.dot(target);
  float crossz = fwd.cross(target);  // +1 => target is to the left (CCW turn)
  REQUIRE_THAT(dot, WithinAbs(0.f, 1e-6f));
  REQUIRE_THAT(crossz, WithinAbs(1.f, 1e-6f));
  // Left turn needed
  REQUIRE(crossz > 0.f);
}

TEST_CASE("Vec2: normalize edge cases", "[vec2][edge]") {
  Vec2 z = Vec2::Zero();
  // normalized() must not produce NaNs or crash
  auto n = z.normalized();
  REQUIRE(n == Vec2::Zero());

  // large numbers remain stable
  Vec2 big{10000.f, 0.f};
  auto u = big.normalized();
  REQUIRE_THAT(u.x, WithinAbs(1.f, 1e-6f));
  REQUIRE_THAT(u.y, WithinAbs(0.f, 1e-6f));
}

TEST_CASE("Vec2: arithmetic identities", "[vec2][math]") {
  Vec2 a{2.f, -3.f};
  Vec2 b{-5.f, 7.f};
  REQUIRE(a + b == Vec2{-3.f, 4.f});
  REQUIRE(a - b == Vec2{7.f, -10.f});
  REQUIRE(a * 2.f == Vec2{4.f, -6.f});
  REQUIRE(b / 2.f == Vec2{-2.5f, 3.5f});

  a += b;  // {-3,4}
  a -= Vec2{-3.f, 4.f};
  REQUIRE(a == Vec2{0.f, 0.f});
}

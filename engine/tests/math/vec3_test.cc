#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include "navary/math/vec3.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

TEST_CASE("Vec3: surface normals and reflect-like math",
          "[vec3][lighting][game]") {
  // Triangle normal (right-handed, CCW)
  Vec3 a{0.f, 0.f, 0.f};
  Vec3 b{1.f, 0.f, 0.f};
  Vec3 c{0.f, 1.f, 0.f};
  Vec3 ab = b - a;
  Vec3 ac = c - a;
  Vec3 n  = ab.cross(ac).normalized();
  REQUIRE_THAT(n.x, WithinAbs(0.f, 1e-6f));
  REQUIRE_THAT(n.y, WithinAbs(0.f, 1e-6f));
  REQUIRE_THAT(n.z, WithinAbs(1.f, 1e-6f));

  // simple lambert term
  Vec3 lightDir = Vec3{0.f, 0.f, -1.f}.normalized();
  float ndotl =
      std::max(0.f, n.dot(-1.f * lightDir));  // light coming from +Z towards -Z
  REQUIRE_THAT(ndotl, WithinAbs(1.f, 1e-6f));
}

TEST_CASE("Vec3: camera forward/right/up orthonormality check",
          "[vec3][camera][game]") {
  Vec3 forward = Vec3{0.1f, 0.2f, -1.f}.normalized();
  Vec3 worldUp{0.f, 1.f, 0.f};
  Vec3 right = worldUp.cross(forward).normalized();
  Vec3 up    = forward.cross(right);

  REQUIRE_THAT(forward.length(), WithinAbs(1.f, 1e-5f));
  REQUIRE_THAT(right.length(), WithinAbs(1.f, 1e-5f));
  REQUIRE_THAT(up.length(), WithinAbs(1.f, 1e-5f));

  // pairwise orthogonal
  REQUIRE_THAT(forward.dot(right), WithinAbs(0.f, 1e-4f));
  REQUIRE_THAT(forward.dot(up), WithinAbs(0.f, 1e-4f));
  REQUIRE_THAT(right.dot(up), WithinAbs(0.f, 1e-4f));
}

TEST_CASE("Vec3: accumulate velocity, clamp speed", "[vec3][movement][game]") {
  Vec3 vel = Vec3::Zero();
  vel += Vec3{1.f, 0.f, 0.f};
  vel += Vec3{0.f, 1.f, 1.f};

  const float maxSpeed = 2.0f;
  if (vel.length() > maxSpeed) {
    vel = vel.normalized() * maxSpeed;
  }
  REQUIRE(vel.length() <= maxSpeed + 1e-6f);

  // integration step
  Vec3 pos{0.f, 0.f, 0.f};
  pos += vel * 0.016f;  // 16 ms frame
  REQUIRE(pos != Vec3::Zero());
}

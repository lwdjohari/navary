#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/quat.h"
#include "navary/math/mat4.h"
#include "navary/math/vec3.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

TEST_CASE("Quat: identity and axis-angle rotation", "[quat]") {
  Quat I = Quat::Identity();
  Vec3 v{1, 0, 0};
  REQUIRE(V3Close(I.Rotate(v), v));

  // 90-deg about +Z: +X -> +Y
  Quat q = Quat::FromAxisAngle(Vec3{0, 0, 1}, 3.1415926535f * 0.5f);
  Vec3 r = q.Rotate({1, 0, 0});
  REQUIRE_THAT(r.x, WithinAbs(0.f, 1e-5f));
  REQUIRE_THAT(r.y, WithinAbs(1.f, 1e-5f));
  REQUIRE_THAT(r.z, WithinAbs(0.f, 1e-5f));
}

TEST_CASE("Quat: compose rotations via Hamilton product", "[quat]") {
  // Rotate +X -> +Y (90deg Z), then +Y -> +Z (90deg X negative? we’ll use +Y
  // axis 90deg)
  Quat qz = Quat::FromAxisAngle({0, 0, 1}, 3.1415926535f * 0.5f);
  Quat qy = Quat::FromAxisAngle({0, 1, 0}, 3.1415926535f * 0.5f);
  Quat q  = qy * qz;  // apply qz first, then qy

  Vec3 v{1, 0, 0};
  Vec3 r = q.Rotate(v);
  // Expect ~ +Y
  REQUIRE_THAT(r.x, WithinAbs(0.f, 1e-5f));
  REQUIRE_THAT(r.y, WithinAbs(1.f, 1e-5f));
  REQUIRE_THAT(r.z, WithinAbs(0.f, 1e-5f));
}

TEST_CASE("Quat: FromEulerXYZ matches Mat4 composed rotations",
          "[quat][mat4]") {
  const Vec3 euler{0.2f, -0.3f, 0.5f};  // (x=pitch, y=yaw, z=roll)
  Quat q  = Quat::FromEulerXYZ(euler);
  Mat4 Rq = q.ToMat4();

  Mat4 Rx = Mat4::RotationX(euler.x);
  Mat4 Ry = Mat4::RotationY(euler.y);
  Mat4 Rz = Mat4::RotationZ(euler.z);
  // Match q = qz * qy * qx  (apply X then Y then Z)
  Mat4 Re = Rz * Ry * Rx;

  // Compare rotating an arbitrary vector
  Vec3 v{0.3f, -1.2f, 2.7f};
  REQUIRE(V3Close(Rq.TransformVector(v), Re.TransformVector(v), 1e-5f));
}

TEST_CASE("Quat: Slerp endpoints and halfway", "[quat]") {
  Quat a = Quat::FromAxisAngle({0, 1, 0}, 0.0f);
  Quat b = Quat::FromAxisAngle({0, 1, 0}, 3.1415926535f * 0.5f);  // 90deg Y

  // Endpoints exact
  REQUIRE(
      V3Close(Quat::Slerp(a, b, 0.0f).Rotate({1, 0, 0}), a.Rotate({1, 0, 0})));
  REQUIRE(
      V3Close(Quat::Slerp(a, b, 1.0f).Rotate({1, 0, 0}), b.Rotate({1, 0, 0})));

  // Halfway ~ 45deg about Y: +X -> (cos45, 0, -sin45)
  Vec3 mid = Quat::Slerp(a, b, 0.5f).Rotate({1, 0, 0});
  REQUIRE_THAT(mid.x, WithinAbs(0.7071067f, 1e-5f));
  REQUIRE_THAT(mid.z, WithinAbs(-0.7071067f, 1e-5f));
}

TEST_CASE("Quat: Nlerp monotonic and normalized", "[quat]") {
  Quat a = Quat::FromAxisAngle({0, 0, 1}, 0.0f);
  Quat b = Quat::FromAxisAngle({0, 0, 1}, 3.1415926535f);

  Quat h = Quat::Nlerp(a, b, 0.5f);
  // Should be unit-ish
  float len2 = h.x * h.x + h.y * h.y + h.z * h.z + h.w * h.w;
  REQUIRE_THAT(len2, WithinAbs(1.0f, 1e-5f));

  // Interpolated rotation should map +X to ~ +Y (halfway to 180deg is 90deg)
  Vec3 r = h.Rotate({1, 0, 0});
  REQUIRE_THAT(r.x, WithinAbs(0.f, 1e-4f));
  REQUIRE_THAT(r.y, WithinAbs(1.f, 1e-4f));
}

TEST_CASE("Quat: Rotate equals Mat4::ToMat4 rotation of vector",
          "[quat][mat4]") {
  Quat q = Quat::FromAxisAngle(Vec3({0.2f, 0.9f, -0.4f}).normalized(), 1.3f);
  Mat4 R = q.ToMat4();

  Vec3 v{2.0f, -0.5f, 4.2f};
  REQUIRE(V3Close(q.Rotate(v), R.TransformVector(v), 1e-5f));
}

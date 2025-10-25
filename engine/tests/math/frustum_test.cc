#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/vec3.h"
#include "navary/math/vec4.h"
#include "navary/math/mat4.h"
#include "navary/math/plane.h"
#include "navary/math/frustum.h"
#include "navary/math/aabb.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

TEST_CASE("Frustum: build from view-projection and basic point visibility",
          "[frustum]") {
  // Camera at origin, looking toward -Z (RH), OpenGL clip depth
  const float fovy   = 60.0f * 3.1415926535f / 180.0f;
  const float aspect = 16.0f / 9.0f;
  const float zn = 0.1f, zf = 1000.0f;

  Mat4 view = Mat4::Identity();  // camera at origin
  Mat4 proj = Mat4::PerspectiveRH(fovy, aspect, zn, zf);

  Frustum f = Frustum::FromViewProj(proj, view);
  REQUIRE(f.size() == 6);

  // A point straight ahead at z=-5 is inside
  REQUIRE(f.IsPointVisible({0, 0, -5}));

  // A point behind the camera is outside
  REQUIRE_FALSE(f.IsPointVisible({0, 0, +5}));

  // A point far away beyond far plane is outside
  REQUIRE_FALSE(f.IsPointVisible({0, 0, -5000}));
}

TEST_CASE("Frustum: FromClipMatrix vs FromViewProj equivalence", "[frustum]") {
  const float fovy   = 45.0f * 3.1415926535f / 180.0f;
  const float aspect = 1.3333333f;
  const float zn = 0.5f, zf = 250.0f;

  Mat4 view = Mat4::Translation({0, 0, 0}) * Mat4::RotationY(0.25f);
  Mat4 proj = Mat4::PerspectiveRH(fovy, aspect, zn, zf);
  Mat4 clip = proj * view;

  Frustum fA = Frustum::FromViewProj(proj, view);
  Frustum fB = Frustum::FromClipMatrix(clip);

  // Test a grid of points and compare classifications
  for (int z = -1; z >= -200; z -= 25) {
    for (int x = -20; x <= 20; x += 5) {
      for (int y = -20; y <= 20; y += 5) {
        Vec3 p{static_cast<float>(x), static_cast<float>(y),
               static_cast<float>(z)};
        REQUIRE(fA.IsPointVisible(p) == fB.IsPointVisible(p));
      }
    }
  }
}

TEST_CASE("Frustum: AABB & sphere visibility typical cases", "[frustum]") {
  const float fovy = 75.0f * 3.1415926535f / 180.0f;
  Mat4 proj        = Mat4::PerspectiveRH(fovy, 16.f / 10.f, 0.1f, 200.f);
  // Camera translated up and back a bit, pitched slightly downward
  Mat4 view = Mat4::RotationX(-0.1f) * Mat4::Translation({0, 2, 5});

  Frustum f = Frustum::FromViewProj(proj, view);

  // An AABB centered in front of camera
  Aabb box({-1, -1, -12}, {+1, +1, -8});  // world space
  REQUIRE(f.IsAabbVisible(box.min(), box.max()));

  // AABB behind camera
  Aabb box_back({-1, -1, +2}, {+1, +1, +4});
  REQUIRE_FALSE(f.IsAabbVisible(box_back.min(), box_back.max()));

  // Sphere near right edge: visible
  REQUIRE(f.IsSphereVisible({3, 2, -15}, 2.0f));

  // Sphere entirely behind far plane: not visible
  REQUIRE_FALSE(f.IsSphereVisible({0, 2, -500}, 5.0f));
}

TEST_CASE(
    "Frustum: Transform() of planes matches moving the frustum with a world "
    "transform",
    "[frustum]") {
  const float fovy = 60.0f * 3.1415926535f / 180.0f;
  Mat4 proj        = Mat4::PerspectiveRH(fovy, 1.0f, 0.1f, 100.0f);
  Mat4 view        = Mat4::Identity();
  Frustum f0       = Frustum::FromViewProj(proj, view);

  // Move the entire frustum by T; points should be classified consistently
  Mat4 T     = Mat4::Translation({10, -3, 7});
  Frustum f1 = f0;
  f1.Transform(T);  // planes transformed by M^{-T}

  // A point considered by f0 at p should match f1 at p' = T^{-1}*p (since
  // planes moved by T)
  Mat4 Tinv = Mat4::Inverse(T);
  for (int i = 0; i < 50; ++i) {
    Vec3 p{(float)(i % 5) - 2.f, (float)(i % 7) - 3.f, -float(1 + (i % 40))};
    // Planes moved by T  ⇒  f1(p) == f0(T^{-1} p)

    REQUIRE(f1.IsPointVisible(p) == f0.IsPointVisible(Tinv.TransformPoint(p)));
  }
}

TEST_CASE("Frustum: grazing cases (on-plane points and boxes)", "[frustum]") {
  const float fovy = 50.0f * 3.1415926535f / 180.0f;
  Mat4 proj        = Mat4::PerspectiveRH(fovy, 1.0f, 1.0f, 10.0f);
  Frustum f        = Frustum::FromViewProj(proj, Mat4::Identity());

  // Point exactly on near plane at z = -1 looking down -Z -> should count as
  // visible
  REQUIRE(f.IsPointVisible({0, 0, -1}));

  // Thin box that lies exactly straddling near plane still counts as visible
  Aabb box({-0.5f, -0.5f, -1.0f}, {+0.5f, +0.5f, -0.9999f});
  REQUIRE(f.IsAabbVisible(box.min(), box.max()));
}

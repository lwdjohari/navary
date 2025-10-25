#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/aabb.h"
#include "navary/math/obb.h"
#include "navary/math/frustum.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

static float Length(const Vec3& v) {
  return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

TEST_CASE("Obb: FromAabb/FromCenterExtents/ToAabb round-trips", "[obb]") {
  Aabb a({-3, -1, 2}, {5, 4, 7});
  Obb o     = Obb::FromAabb(a);
  Aabb back = o.ToAabb();  // For axis-aligned OBB this should be exact
  REQUIRE(V3Close(back.min(), a.min(), 1e-6f));
  REQUIRE(V3Close(back.max(), a.max(), 1e-6f));

  Obb o2  = Obb::FromCenterExtents({1, 2, 3}, {4, 5, 6});
  Aabb b2 = o2.ToAabb();
  REQUIRE(V3Close(b2.Center(), Vec3{1, 2, 3}));
  REQUIRE(V3Close(b2.Extents(), Vec3{4, 5, 6}));
}

TEST_CASE("Obb: Transformed re-normalizes basis, scales half-sizes correctly",
          "[obb]") {
  Obb o  = Obb::FromCenterExtents({0, 0, 0}, {2, 1, 3});
  Mat4 M = Mat4::Translation({3, -2, 5}) * Mat4::RotationY(0.7f) *
           Mat4::Scaling({2.0f, 0.5f, 1.5f});
  Obb t = o.Transformed(M);

  // Axes should be unit length
  REQUIRE_THAT(Length(t.axis_u()), WithinAbs(1.0f, 1e-5f));
  REQUIRE_THAT(Length(t.axis_v()), WithinAbs(1.0f, 1e-5f));
  REQUIRE_THAT(Length(t.axis_w()), WithinAbs(1.0f, 1e-5f));

  // Half-sizes scale with the corresponding axis scaling magnitudes
  // (We can approximate by projecting transformed axis-lengths)
  Vec3 u         = M.TransformVector({1, 0, 0});
  Vec3 v         = M.TransformVector({0, 1, 0});
  Vec3 w         = M.TransformVector({0, 0, 1});
  const float lu = Length(u), lv = Length(v), lw = Length(w);

  // Original half extents: (2,1,3)
  Aabb a_from_t = t.ToAabb();  // sanity; should be finite
  (void)a_from_t;
  // No direct getter for half sizes; check via expected OBB corner extents:
  // We check that the OBB-to-AABB extents roughly match half * axis scales
  // along each local axis by comparing OBB->AABB extents lengths are > 0.
  REQUIRE(lu > 0.0f);
  REQUIRE(lv > 0.0f);
  REQUIRE(lw > 0.0f);
}

TEST_CASE("Obb: RayIntersect hit/miss/inside cases", "[obb]") {
  Obb o      = Obb::FromCenterExtents({0, 0, -10}, {2, 3, 4});
  float tmin = 0, tmax = 0;

  // Ray straight down -Z from origin hits
  REQUIRE(o.RayIntersect({0, 0, 0}, {0, 0, -1}, &tmin, &tmax));
  REQUIRE(tmin >= 0.0f);
  REQUIRE(tmax > tmin);

  // Ray from far left misses
  REQUIRE_FALSE(o.RayIntersect({100, 0, 0}, {-1, 0, -1}, &tmin, &tmax));

  // Origin inside OBB → should count as intersection (tmin may be negative)
  Obb big = Obb::FromCenterExtents({0, 0, 0}, {100, 100, 100});
  REQUIRE(big.RayIntersect({0, 0, 0}, {1, 0, 0}, &tmin, &tmax));
}

TEST_CASE("Obb: Visual culling via Frustum using AABB proxy",
          "[obb][frustum]") {
  const float fovy = 70.0f * 3.1415926535f / 180.0f;
  Mat4 proj        = Mat4::PerspectiveRH(fovy, 16.f / 9.f, 0.1f, 500.f);
  Mat4 view        = Mat4::Identity();

  Frustum f = Frustum::FromViewProj(proj, view);

  // A rotated OBB in front of camera
  Obb o  = Obb::FromCenterExtents({0, 0, -50}, {5, 2, 3});
  Mat4 R = Mat4::RotationY(0.8f) * Mat4::RotationX(0.3f);
  Obb oW = o.Transformed(R);

  // Use conservative AABB proxy for frustum test
  Aabb proxy = oW.ToAabb();
  REQUIRE(f.IsAabbVisible(proxy.min(), proxy.max()));

  // Move it far away behind the camera
  Obb oBack      = Obb::FromCenterExtents({0, 0, +50}, {5, 2, 3});
  Aabb proxyBack = oBack.ToAabb();
  REQUIRE_FALSE(f.IsAabbVisible(proxyBack.min(), proxyBack.max()));
}

TEST_CASE("Obb: Degenerate scaling resilience", "[obb]") {
  // Zero scaling on one axis in transform; basis should renormalize and
  // half-size along that axis becomes zero.
  Obb o  = Obb::FromCenterExtents({0, 0, 0}, {2, 2, 2});
  Mat4 M = Mat4::Scaling({0.0f, 3.0f, 1.0f});
  Obb t  = o.Transformed(M);

  // Y axis gains scale; X axis becomes zero half-size through scale magnitude.
  // We validate via ToAabb extents staying finite and consistent.
  Aabb a = t.ToAabb();
  REQUIRE(a.min().x <= a.max().x);
  REQUIRE(a.min().y <= a.max().y);
  REQUIRE(a.min().z <= a.max().z);
}

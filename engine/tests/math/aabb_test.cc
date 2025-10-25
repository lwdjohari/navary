#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/aabb.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

TEST_CASE("Aabb: FromPoints builds correct bounds", "[aabb]") {
  Vec3 pts[] = {{1, 2, 3}, {-5, 0, 2}, {3, -4, 1}, {0, 7, -2}};
  Aabb box   = Aabb::FromPoints(pts, 4);

  REQUIRE(V3Close(box.min(), Vec3{-5, -4, -2}));
  REQUIRE(V3Close(box.max(), Vec3{3, 7, 3}));
  REQUIRE_THAT(box.Volume(), WithinAbs((8.0f * 11.0f * 5.0f), 1e-5f));
}

TEST_CASE("Aabb: Transformed matches brute-force 8-corner transform",
          "[aabb]") {
  Aabb box({-1, -2, -3}, {2, 1, 0});
  Mat4 M = Mat4::Translation({3, -1, 5}) * Mat4::RotationY(0.6f) *
           Mat4::Scaling({1.5f, 0.5f, 2.0f});

  // Fast path:
  Aabb tfast = box.Transformed(M);

  // Brute force: transform 8 corners then bounds
  Vec3 mn = box.min(), mx = box.max();
  Vec3 corners[8] = {
      {mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mn.x, mx.y, mn.z},
      {mx.x, mx.y, mn.z}, {mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z},
      {mn.x, mx.y, mx.z}, {mx.x, mx.y, mx.z},
  };
  Aabb tslow;
  tslow.SetEmpty();
  for (int i = 0; i < 8; ++i)
    tslow.Extend(M.TransformPoint(corners[i]));

  REQUIRE(V3Close(tfast.min(), tslow.min(), 1e-5f));
  REQUIRE(V3Close(tfast.max(), tslow.max(), 1e-5f));
}

TEST_CASE("Aabb: Union and Intersect typical use", "[aabb]") {
  Aabb a({-2, -1, -3}, {1, 2, 0});
  Aabb b({-1, -2, -1}, {3, 1, 2});

  Aabb u = Aabb::Union(a, b);
  REQUIRE(V3Close(u.min(), Vec3{-2, -2, -3}));
  REQUIRE(V3Close(u.max(), Vec3{3, 2, 2}));

  Aabb i = Aabb::Intersect(a, b);
  REQUIRE_FALSE(i.IsEmpty());
  REQUIRE(V3Close(i.min(), Vec3{-1, -1, -1}));
  REQUIRE(V3Close(i.max(), Vec3{1, 1, 0}));

  // Disjoint intersection -> empty
  Aabb c({10, 10, 10}, {11, 11, 11});
  Aabb id = Aabb::Intersect(a, c);
  REQUIRE(id.IsEmpty());
}

TEST_CASE("Aabb: ContainsPoint and Center/Extents", "[aabb]") {
  Aabb b({-2, -2, -2}, {2, 2, 2});
  REQUIRE(b.ContainsPoint({0, 0, 0}));
  REQUIRE(b.ContainsPoint({2, 2, 2}));
  REQUIRE_FALSE(b.ContainsPoint({2.0001f, 0, 0}));

  REQUIRE(V3Close(b.Center(), Vec3{0, 0, 0}));
  REQUIRE(V3Close(b.Extents(), Vec3{2, 2, 2}));
}

TEST_CASE("Aabb: FromTransformed convenience", "[aabb]") {
  Aabb box({0, 0, 0}, {1, 2, 3});
  Mat4 R  = Mat4::RotationX(0.25f) * Mat4::RotationZ(-0.3f);
  Aabb t0 = Aabb::FromTransformed(box, R);
  Aabb t1 = box.Transformed(R);
  REQUIRE(V3Close(t0.min(), t1.min(), 1e-6f));
  REQUIRE(V3Close(t0.max(), t1.max(), 1e-6f));
}

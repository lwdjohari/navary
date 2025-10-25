#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/aabb.h"
#include "navary/math/bounding_sphere.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

TEST_CASE("BoundingSphere: FromAabbTight matches half-diagonal of AABB",
          "[bsphere]") {
  Aabb box({-2, -1, -3}, {4, 5, 1});  // center = (1,2,-1)
  BoundingSphere s = BoundingSphere::FromAabbTight(box);

  Vec3 expected_center = (box.min() + box.max()) * 0.5f;  // (1,2,-1)
  Vec3 ext             = (box.max() - box.min()) * 0.5f;  // (3,3,2)
  float expected_r = std::sqrt(ext.x * ext.x + ext.y * ext.y + ext.z * ext.z);

  REQUIRE(V3Close(s.center(), expected_center));
  REQUIRE_THAT(s.radius(), WithinAbs(expected_r, 1e-6f));

  // Points at AABB corners should be inside or on the sphere
  Vec3 corners[8] = {
      {box.min().x, box.min().y, box.min().z},
      {box.max().x, box.min().y, box.min().z},
      {box.min().x, box.max().y, box.min().z},
      {box.max().x, box.max().y, box.min().z},
      {box.min().x, box.min().y, box.max().z},
      {box.max().x, box.min().y, box.max().z},
      {box.min().x, box.max().y, box.max().z},
      {box.max().x, box.max().y, box.max().z},
  };
  for (auto& c : corners) {
    REQUIRE(s.ContainsPoint(c));
  }
}

TEST_CASE("BoundingSphere: FromSweptAabb encloses all rotations about origin",
          "[bsphere]") {
  // Box vertices farthest from origin set radius. Use an AABB not centered at
  // origin.
  Aabb box({-2, 1, -5}, {4, 3, -1});
  BoundingSphere s = BoundingSphere::FromSweptAabb(box);

  // The sphere is centered at origin. Its radius is the length of the farthest
  // corner.
  const Vec3 cnrs[2]  = {box.min(), box.max()};
  float r_expected_sq = 0.0f;
  for (int ix = 0; ix < 2; ++ix)
    for (int iy = 0; iy < 2; ++iy)
      for (int iz = 0; iz < 2; ++iz) {
        Vec3 p{ix ? cnrs[1].x : cnrs[0].x, iy ? cnrs[1].y : cnrs[0].y,
               iz ? cnrs[1].z : cnrs[0].z};
        r_expected_sq = std::max(r_expected_sq, p.dot(p));
      }
  REQUIRE(V3Close(s.center(), Vec3{0, 0, 0}));
  REQUIRE_THAT(s.radius(), WithinAbs(std::sqrt(r_expected_sq), 1e-6f));

  // A random rotation about origin cannot push any corner beyond s.radius().
  // (We don't rotate here—this check already validates the worst-case.)
}

TEST_CASE("BoundingSphere: FromPoints mean-center + max distance radius",
          "[bsphere]") {
  Vec3 pts[]       = {{2, 0, 0}, {-2, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 3}};
  BoundingSphere s = BoundingSphere::FromPoints(pts, 5);

  // mean
  Vec3 mean{0, 0, 0.6f};
  REQUIRE(V3Close(s.center(), mean, 1e-6f));

  // radius is max distance to mean
  float r2 = 0.f;
  for (auto& p : pts) {
    Vec3 d = p - mean;
    r2     = std::max(r2, d.dot(d));
  }
  REQUIRE_THAT(s.radius(), WithinAbs(std::sqrt(r2), 1e-6f));
}

TEST_CASE(
    "BoundingSphere: Merge produces minimal enclosing sphere for two spheres",
    "[bsphere]") {
  BoundingSphere a({0, 0, 0}, 2.0f);
  BoundingSphere b({5, 0, 0}, 1.0f);

  // Disjoint, a does not contain b, b does not contain a
  BoundingSphere m = BoundingSphere::Merge(a, b);

  // New center lies along the line between them
  // radius = (d + ra + rb)/2; center offset = (new_r - ra) along AB direction
  float d     = 5.0f;                                  // |b.center - a.center|
  float new_r = 0.5f * (d + a.radius() + b.radius());  // 0.5 * (5 + 2 + 1) = 4
  float t     = (new_r - a.radius()) / d;              // (4-2)/5 = 0.4
  Vec3 expected_c = Vec3{0, 0, 0} + Vec3{1, 0, 0} * t * d;  // (2,0,0)

  REQUIRE_THAT(m.radius(), WithinAbs(new_r, 1e-6f));
  REQUIRE(V3Close(m.center(), expected_c, 1e-6f));

  // Containment edge cases:
  // If one contains the other, should return the containing sphere unchanged.
  BoundingSphere c({0, 0, 0}, 10.0f);
  BoundingSphere d_small({3, 0, 0}, 1.0f);
  BoundingSphere md = BoundingSphere::Merge(c, d_small);
  REQUIRE(V3Close(md.center(), c.center()));
  REQUIRE_THAT(md.radius(), WithinAbs(c.radius(), 1e-6f));
}

TEST_CASE("BoundingSphere: Expand increases radius only", "[bsphere]") {
  BoundingSphere s({1, 2, 3}, 2.0f);
  s.Expand(3.0f);
  REQUIRE(V3Close(s.center(), Vec3{1, 2, 3}));
  REQUIRE_THAT(s.radius(), WithinAbs(5.0f, 1e-6f));

  // Negative amount is a no-op by contract
  s.Expand(-100.0f);
  REQUIRE_THAT(s.radius(), WithinAbs(5.0f, 1e-6f));
}

TEST_CASE(
    "BoundingSphere: Transformed scales by max axis stretch and translates "
    "center",
    "[bsphere]") {
  BoundingSphere s({-2, 1, -5}, 3.0f);

  // Non-uniform scale + rotation + translation. Radius scales by max |linear
  // axis|
  Mat4 M = Mat4::Translation({10, -3, 7}) * Mat4::RotationY(0.25f) *
           Mat4::Scaling({2.0f, 0.5f, 1.5f});
  BoundingSphere t = s.Transformed(M);

  // Center transformed as a point
  Vec3 expected_c = M.TransformPoint(s.center());
  REQUIRE(V3Close(t.center(), expected_c, 1e-5f));

  // Radius scaled by max axis stretch
  float sx   = M.TransformVector({1, 0, 0}).length();
  float sy   = M.TransformVector({0, 1, 0}).length();
  float sz   = M.TransformVector({0, 0, 1}).length();
  float smax = std::max(sx, std::max(sy, sz));
  REQUIRE_THAT(t.radius(), WithinAbs(s.radius() * smax, 1e-5f));
}

TEST_CASE("BoundingSphere: RayIntersect hit/miss/inside cases", "[bsphere]") {
  BoundingSphere s({0, 0, -10}, 2.5f);

  float tmin, tmax;  // not used; RayIntersect returns only boolean
  (void)tmin;
  (void)tmax;

  // Ray from origin along -Z should hit
  REQUIRE(s.RayIntersect({0, 0, 0}, {0, 0, -1}));

  // Ray from far left aiming away should miss
  REQUIRE_FALSE(s.RayIntersect({100, 0, 0}, {1, 0, 0}));

  // Origin inside sphere -> counts as hit (ray outward still intersects)
  BoundingSphere big({0, 0, 0}, 100.f);
  REQUIRE(big.RayIntersect({0, 0, 0}, {1, 0, 0}));
}

TEST_CASE("BoundingSphere: ContainsPoint, ContainsSphere, IntersectsSphere",
          "[bsphere]") {
  BoundingSphere s({2, -1, 5}, 3.0f);

  // Points
  REQUIRE(s.ContainsPoint({2, -1, 5}));           // center
  REQUIRE(s.ContainsPoint({4.9f, -1, 5}));        // inside
  REQUIRE_FALSE(s.ContainsPoint({6.1f, -1, 5}));  // outside (radius=3)

  // Spheres
  BoundingSphere inside({2, -1, 6}, 1.0f);
  REQUIRE(s.ContainsSphere(inside));
  REQUIRE(s.IntersectsSphere(inside));

  BoundingSphere touch({5, -1, 5}, 0.0f);  // touching at x=5 (on surface)
  REQUIRE(s.IntersectsSphere(touch));
  REQUIRE_FALSE(s.ContainsSphere(
      touch));  // zero radius at boundary → not strictly contained

  BoundingSphere disjoint({20, -1, 5}, 1.0f);
  REQUIRE_FALSE(s.IntersectsSphere(disjoint));
}

TEST_CASE("BoundingSphere: IntersectsAabb via closest-point on AABB",
          "[bsphere]") {
  BoundingSphere s({0, 0, -10}, 3.0f);

  // AABB fully inside sphere
  Aabb box1({-1, -1, -11}, {1, 1, -9});
  REQUIRE(s.IntersectsAabb(box1));

  // AABB just touching sphere surface at x = -3 (tangent contact)
  Aabb box2({-3.0f, -10.0f, -13.1f}, {-3.0f, 10.0f, -9.0f});

  REQUIRE(s.IntersectsAabb(box2));

  // AABB clearly outside
  Aabb box3({10, 10, 10}, {12, 12, 12});
  REQUIRE_FALSE(s.IntersectsAabb(box3));

  // Edge case: AABB degenerate to a point outside
  Aabb box4({5, 5, 5}, {5, 5, 5});
  REQUIRE_FALSE(s.IntersectsAabb(box4));

  // Edge case: AABB degenerate to a point on the surface
  Aabb box5({0, 0, -7},
            {0, 0, -7});  // sphere center (0,0,-10), r=3 → distance=3
  REQUIRE(s.IntersectsAabb(box5));
}

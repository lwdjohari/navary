#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "navary/math/vec2.h"
#include "navary/math/vec3.h"
#include "navary/math/plane.h"      // Plane
#include "navary/math/math_util.h"  // DegToRad, etc.

using Catch::Matchers::WithinAbs;
using navary::math::DegToRad;
using navary::math::Plane;
using navary::math::PlaneSide;
using navary::math::Vec3;
using navary::math::Vec4;

TEST_CASE("Plane: camera ground raycast (y=0)", "[math][plane]") {
  // Ground plane: y = 0, normal pointing up (+Y)
  Plane ground =
      Plane::FromNormalAndPoint(Vec3{0.f, 1.f, 0.f}, Vec3{0.f, 0.f, 0.f});
  ground.Normalize();

  // Camera 1.8 m above ground, looking downward -30° pitch, forward -Z
  Vec3 cam_pos{0.f, 1.8f, 0.f};
  float pitch = DegToRad(-30.f);
  float yaw   = 0.f;

  // RH coords: +Y up, -Z forward
  Vec3 forward{
      std::cos(pitch) * std::sin(yaw),  // x
      std::sin(pitch),                  // y
      -std::cos(pitch) * std::cos(yaw)  // z
  };
  forward.normalize();

  Vec3 hit;
  bool ok = ground.IntersectRay(cam_pos, forward, &hit);

  REQUIRE(ok);
  REQUIRE_THAT(hit.y, WithinAbs(0.f, 1e-5f));
  REQUIRE(hit.z < 0.f);  // Ground hit in front of camera
  REQUIRE(hit.x == Catch::Approx(0.f).margin(1e-5f));
}

TEST_CASE("Plane: hitscan vs wall (segment intersection)",
          "[plane][segment][combat]") {
  // Wall: x = 10  => normal (1,0,0), d = -10
  Plane wall = Plane::FromNormalAndPoint(Vec3{1, 0, 0}, Vec3{10, 0, 0});
  wall.Normalize();

  // Bullet trace from left to right across the wall
  Vec3 start{0, 1, 0};
  Vec3 end{20, 1, 0};

  Vec3 hit{};
  bool ok = wall.IntersectSegment(start, end, &hit);
  REQUIRE(ok);
  REQUIRE(hit.x == Catch::Approx(10.f).margin(1e-5f));
  REQUIRE(hit.y == Catch::Approx(1.f).margin(1e-5f));
  REQUIRE(hit.z == Catch::Approx(0.f).margin(1e-5f));

  // Reverse direction also works
  ok = wall.IntersectSegment(end, start, &hit);
  REQUIRE(ok);
  REQUIRE(hit.x == Catch::Approx(10.f).margin(1e-5f));
}

TEST_CASE("Plane: construct from three points and classify",
          "[plane][classify][frustum]") {
  // Plane through a triangle on the XZ plane at y = 5, normal up.
  Vec3 p1{0, 5, 0}, p2{1, 5, 0}, p3{0, 5, 1};
  Plane pl = Plane::FromPoints(p1, p2, p3);
  pl.Normalize();

  // Points above/below plane
  Vec3 above{0, 7, 0};
  Vec3 below{0, 3, 0};
  REQUIRE(pl.ClassifyPoint(above) == PlaneSide::kFront);
  REQUIRE(pl.ClassifyPoint(below) == PlaneSide::kBack);
  REQUIRE(pl.ClassifyPoint(p1) == PlaneSide::kOn);

  // Signed distance matches y-5 for normalized plane (normal = +Y).
  REQUIRE(pl.DistanceTo(above) == Catch::Approx(2.f).margin(1e-5f));
  REQUIRE(pl.DistanceTo(below) == Catch::Approx(-2.f).margin(1e-5f));
}

TEST_CASE("Plane: parallel and near-parallel rays robustness",
          "[plane][robust]") {
  Plane floor = Plane::FromNormalAndPoint(Vec3{0, 1, 0}, Vec3{0, 0, 0});
  floor.Normalize();

  // Perfectly parallel ray (dir parallel to plane)
  Vec3 origin{0, 1, 0};
  Vec3 dir{1, 0, 0};  // parallel to floor
  Vec3 out{};
  REQUIRE_FALSE(floor.IntersectRay(origin, dir, &out));

  // Nearly parallel: tiny downward component should produce a hit
  dir = Vec3{1, -1e-4f, 0}.normalized();
  REQUIRE(floor.IntersectRay(origin, dir, &out));
  REQUIRE(out.y == Catch::Approx(0.f).margin(1e-4f));
}

TEST_CASE("Plane: non-normalized still classifies consistently after Normalize",
          "[plane][normalize]") {
  // Build an arbitrary scaled plane: 2*(y) + 2*d = 0  ⇔ y + d = 0
  Plane pl(Vec4{0, 2, 0, -10});  // equivalent to y - 5 = 0 scaled by 2
  Vec3 pt{0, 7, 0};

  float dist_unnorm = pl.DistanceTo(pt);  // depends on scale
  pl.Normalize();
  float dist_norm = pl.DistanceTo(pt);

  // After normalize, distance is true geometric distance (7 - 5 = 2)
  REQUIRE(dist_norm == Catch::Approx(2.f).margin(1e-5f));

  // Classification should agree before/after normalize
  PlaneSide side_before = (dist_unnorm > Plane::kEps)    ? PlaneSide::kFront
                          : (dist_unnorm < -Plane::kEps) ? PlaneSide::kBack
                                                         : PlaneSide::kOn;
  REQUIRE(side_before == pl.ClassifyPoint(pt));
}

TEST_CASE("Plane: ray intersection parallel and grazing cases",
          "[math][plane]") {
  Plane p = Plane::FromNormalAndPoint(Vec3{0, 1, 0}, Vec3{0, 0, 0});

  Vec3 hit;
  // Parallel ray (should fail)
  bool ok1 = p.IntersectRay(Vec3{0, 1, 0}, Vec3{1, 0, 0}, &hit);
  REQUIRE_FALSE(ok1);

  // Grazing ray (tiny pitch)
  Vec3 near_parallel_dir{0.f, -0.0001f, -1.f};
  near_parallel_dir.normalize();

  bool ok2 = p.IntersectRay(Vec3{0, 1, 0}, near_parallel_dir, &hit);
  REQUIRE(ok2);
  REQUIRE(hit.y == Catch::Approx(0.f).margin(1e-4f));
}

TEST_CASE("Plane: distance and normalize behavior", "[math][plane]") {
  Plane p = Plane::FromNormalAndPoint(
      Vec3{0, 2, 0}, Vec3{0, 1, 0});  // unnormalized normal (0,2,0)
  float dist_before = p.DistanceTo(Vec3{0, 3, 0});
  p.Normalize();
  float dist_after = p.DistanceTo(Vec3{0, 3, 0});

  // Normalization should halve the stored normal and D, doubling measured
  // distance
  REQUIRE(dist_after == Catch::Approx(dist_before / 2.f).margin(1e-5f));
}
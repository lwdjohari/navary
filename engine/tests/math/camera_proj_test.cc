#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/mat4.h"
#include "navary/math/vec3.h"
#include "navary/math/vec4.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

// -----------------------------------------------------------------------------
// Small helper: compare two vectors with tolerance
// -----------------------------------------------------------------------------
static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

// -----------------------------------------------------------------------------
// Build a classic OpenGL-style right-handed LookAt matrix.
//
//  eye    → camera world position
//  center → target position the camera looks toward
//  up     → approximate up direction (usually {0,1,0})
//
// Result maps world → view space, where camera sits at origin
// looking along -Z and +Y is up.
// -----------------------------------------------------------------------------
static Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up) {
  const Vec3 f = (center - eye).normalized();  // forward vector (to target)
  const Vec3 s = f.cross(up).normalized();     // right vector
  const Vec3 u = s.cross(f);                   // true up vector

  Mat4 m = Mat4::Identity();

  // rotation basis (columns are the camera axes)
  m(0, 0) = s.x;
  m(0, 1) = s.y;
  m(0, 2) = s.z;
  m(1, 0) = u.x;
  m(1, 1) = u.y;
  m(1, 2) = u.z;
  m(2, 0) = -f.x;
  m(2, 1) = -f.y;
  m(2, 2) = -f.z;

  // translation moves world so eye becomes origin
  m(3, 0) = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
  m(3, 1) = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
  m(3, 2) = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
  m(3, 3) = 1.0f;
  return m;
}

// -----------------------------------------------------------------------------
// Convert a world-space point to screen coordinates.
//
// width/height → viewport size in pixels
// returns {screenX, screenY, ndcZ}
//  - screen origin is top-left (0,0)
//  - NDC Z remains in [-1,1] for OpenGL
// -----------------------------------------------------------------------------
static Vec3 ProjectToScreen(const Mat4& VP, const Vec3& world, float width,
                            float height) {
  const Vec4 clip  = VP.Transform(Vec4{world.x, world.y, world.z, 1.0f});
  const float invW = 1.0f / clip.w;

  // normalize to NDC
  const float x_ndc = clip.x * invW;
  const float y_ndc = clip.y * invW;
  const float z_ndc = clip.z * invW;

  // map NDC → screen pixels
  const float sx = (x_ndc * 0.5f + 0.5f) * width;
  const float sy = (1.0f - (y_ndc * 0.5f + 0.5f)) * height;

  return Vec3{sx, sy, z_ndc};
}

// -----------------------------------------------------------------------------
// Inverse of ProjectToScreen: convert screen → world for a known ndcZ.
//
// This is the core of “click-to-world” or ray picking systems.
// -----------------------------------------------------------------------------
static Vec3 UnprojectFromScreen(const Mat4& VP, float sx, float sy, float ndcZ,
                                float width, float height) {
  // screen → NDC
  const float x_ndc = (sx / width) * 2.0f - 1.0f;
  const float y_ndc = 1.0f - (sy / height) * 2.0f;
  const Vec4 clip{x_ndc, y_ndc, ndcZ, 1.0f};

  // transform back to world
  const Mat4 inv    = Mat4::Inverse(VP);
  const Vec4 world4 = inv.Transform(clip);
  const float invW  = 1.0f / world4.w;
  return Vec3{world4.x * invW, world4.y * invW, world4.z * invW};
}

// -----------------------------------------------------------------------------
// Test 1: verify projection/unprojection at center of screen.
// -----------------------------------------------------------------------------
TEST_CASE("Camera: project/unproject center mapping", "[camera][proj]") {
  const float fovy  = 60.0f * 3.1415926535f / 180.0f;
  const float width = 1920.0f, height = 1080.0f;

  const Mat4 P  = Mat4::PerspectiveRH(fovy, width / height, 0.1f, 1000.0f);
  const Mat4 V  = LookAtRH({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
  const Mat4 VP = P * V;

  // A point straight in front should appear at screen center.
  const Vec3 world{0, 0, -5};
  const Vec3 screen = ProjectToScreen(VP, world, width, height);
  REQUIRE_THAT(screen.x, WithinAbs(width * 0.5f, 1e-4f));
  REQUIRE_THAT(screen.y, WithinAbs(height * 0.5f, 1e-4f));

  // Reverse the operation and verify we return to the same world position.
  const Vec3 world2 =
      UnprojectFromScreen(VP, screen.x, screen.y, screen.z, width, height);
  REQUIRE(V3Close(world2, world, 1e-4f));
}

// -----------------------------------------------------------------------------
// Test 2: sanity check of direction vs screen position.
// -----------------------------------------------------------------------------
TEST_CASE("Camera: right/left/up/down screen mapping sanity",
          "[camera][proj]") {
  const float width = 1280.0f, height = 720.0f;
  const Mat4 P  = Mat4::PerspectiveRH(45.0f * 3.1415926535f / 180.0f,
                                      width / height, 0.1f, 500.0f);
  const Mat4 V  = LookAtRH({0, 0, 2}, {0, 0, 0}, {0, 1, 0});
  const Mat4 VP = P * V;

  // right of origin should appear to the right
  const Vec3 centerS = ProjectToScreen(VP, {0, 0, 0}, width, height);
  const Vec3 rightS  = ProjectToScreen(VP, {1, 0, 0}, width, height);
  REQUIRE(rightS.x > centerS.x);

  // above origin should have smaller Y (top-left origin)
  const Vec3 upS = ProjectToScreen(VP, {0, 1, 0}, width, height);
  REQUIRE(upS.y < centerS.y);
}

// -----------------------------------------------------------------------------
// Test 3: build a picking ray from screen center and check
//         that it intersects the ground (y=0) near the origin.
// -----------------------------------------------------------------------------
TEST_CASE("Camera: screen-space picking ray hits ground plane",
          "[camera][picking]") {
  const float width = 1600.0f, height = 900.0f;
  const Mat4 P = Mat4::PerspectiveRH(60.0f * 3.1415926535f / 180.0f,
                                     width / height, 0.1f, 1000.0f);
  const Vec3 eye{0, 10, 10};
  const Mat4 V  = LookAtRH(eye, {0, 0, 0}, {0, 1, 0});
  const Mat4 VP = P * V;

  // convert screen center to near/far world points
  const float sx   = width * 0.5f;
  const float sy   = height * 0.5f;
  const Vec3 nearW = UnprojectFromScreen(VP, sx, sy, -1.0f, width, height);
  const Vec3 farW  = UnprojectFromScreen(VP, sx, sy, 1.0f, width, height);

  // ray = near + t * dir
  const Vec3 rayDir = (farW - nearW).normalized();
  const Vec3 rayPos = nearW;

  // solve for intersection with plane y=0
  const float denom = rayDir.y;
  REQUIRE(std::fabs(denom) > 1e-6f);  // ensure not parallel
  const float t = -rayPos.y / denom;
  REQUIRE(t > 0.0f);

  const Vec3 hit = rayPos + rayDir * t;
  // Expect roughly near world origin.
  REQUIRE_THAT(hit.y, WithinAbs(0.0f, 1e-4f));
  REQUIRE(std::fabs(hit.x) < 0.25f);
  REQUIRE(std::fabs(hit.z) < 0.25f);
}

// -----------------------------------------------------------------------------
// Test 4: project → unproject round-trip for arbitrary world point.
// Demonstrates numerical stability of matrix math.
// -----------------------------------------------------------------------------
TEST_CASE("Camera: project/unproject arbitrary point round-trip",
          "[camera][proj]") {
  const float w = 1024.0f, h = 768.0f;
  const Mat4 P =
      Mat4::PerspectiveRH(50.0f * 3.1415926535f / 180.0f, w / h, 0.5f, 500.0f);
  const Mat4 V  = LookAtRH({3, 4, 5}, {0, 1, -2}, {0, 1, 0});
  const Mat4 VP = P * V;

  const Vec3 world{2.2f, -0.7f, -3.5f};
  const Vec3 screen = ProjectToScreen(VP, world, w, h);

  // Reverse and compare.
  const Vec3 world2 =
      UnprojectFromScreen(VP, screen.x, screen.y, screen.z, w, h);
  REQUIRE(V3Close(world2, world, 1e-4f));
}

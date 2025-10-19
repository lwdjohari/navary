#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/mat4.h"
#include "navary/math/vec3.h"
#include "navary/math/vec4.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

// -----------------------------------------------------------------------------
// Helper: fuzzy compare Vec3
// -----------------------------------------------------------------------------
static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps &&
         std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}

// -----------------------------------------------------------------------------
// Right-handed LookAt (OpenGL-style): camera looks along -Z, +Y up.
// Works fine for Vulkan as long as our projection matches the expected clip space.
// -----------------------------------------------------------------------------
static Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up) {
  const Vec3 f = (center - eye).normalized();  // forward towards target
  const Vec3 s = f.cross(up).normalized();     // right
  const Vec3 u = s.cross(f);                   // recalculated up

  Mat4 m = Mat4::Identity();

  // orientation (columns are axes in column-major)
  m(0,0) = s.x;  m(0,1) = s.y;  m(0,2) = s.z;
  m(1,0) = u.x;  m(1,1) = u.y;  m(1,2) = u.z;
  m(2,0) = -f.x; m(2,1) = -f.y; m(2,2) = -f.z;

  // translation (move world so eye becomes origin)
  m(3,0) = -(s.x*eye.x + s.y*eye.y + s.z*eye.z);
  m(3,1) = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
  m(3,2) =  (f.x*eye.x + f.y*eye.y + f.z*eye.z);
  m(3,3) = 1.0f;
  return m;
}

// -----------------------------------------------------------------------------
// Vulkan perspective projection (RH) with NDC Z in [0, 1] (a.k.a. "Zero-To-One").
//
// Notes:
// - This is the D3D/Vulkan depth convention version of a GL-style RH projection.
// - Many engines also flip Y in the projection to match framebuffer origin;
//   here we keep the same top-left screen mapping in our screen helpers instead.
// -----------------------------------------------------------------------------
static Mat4 PerspectiveRH_ZO(float fovy, float aspect, float z_near, float z_far) {
  const float f = 1.0f / std::tan(fovy * 0.5f);

  // Column-major build:
  //   [ f/aspect,   0,                 0,  0 ]
  //   [        0,   f,                 0,  0 ]
  //   [        0,   0,      zf/(zn-zf), -1 ]
  //   [        0,   0,  (zn*zf)/(zn-zf),  0 ]
  //
  // This maps z = -zn → ndcZ = 0, and z = -zf → ndcZ = 1, for a RH camera with -Z forward.
  Mat4 m = Mat4::Zero();
  m(0,0) = f / aspect;
  m(1,1) = f;
  m(2,2) = z_far / (z_near - z_far);
  m(2,3) = -1.0f;
  m(3,2) = (z_near * z_far) / (z_near - z_far);
  return m;
}

// -----------------------------------------------------------------------------
// Project a world-space point to screen pixels, using Vulkan NDC Z in [0, 1].
//
// We keep "UI-style" screen coords (origin top-left): (0,0) at top-left,
// x rightwards, y downwards.
// -----------------------------------------------------------------------------
static Vec3 ProjectToScreenVK(const Mat4& VP, const Vec3& world,
                              float width, float height) {
  const Vec4 clip = VP.Transform(Vec4{world.x, world.y, world.z, 1.0f});
  const float invW = 1.0f / clip.w;

  // clip → NDC
  const float x_ndc = clip.x * invW; // [-1, 1]
  const float y_ndc = clip.y * invW; // [-1, 1]
  const float z_ndc = clip.z * invW; // [0, 1]  (Vulkan/D3D depth convention)

  // NDC → screen pixels (top-left origin)
  const float sx = (x_ndc * 0.5f + 0.5f) * width;
  const float sy = (1.0f - (y_ndc * 0.5f + 0.5f)) * height;

  return Vec3{sx, sy, z_ndc};
}

// -----------------------------------------------------------------------------
// Unproject screen pixel + NDC Z in [0, 1] back to world space.
// -----------------------------------------------------------------------------
static Vec3 UnprojectFromScreenVK(const Mat4& VP, float sx, float sy, float ndcZ,
                                  float width, float height) {
  // screen → NDC
  const float x_ndc = (sx / width) * 2.0f - 1.0f;
  const float y_ndc = 1.0f - (sy / height) * 2.0f;
  const Vec4 clip{x_ndc, y_ndc, ndcZ, 1.0f};

  // NDC → world
  const Mat4 inv = Mat4::Inverse(VP);
  const Vec4 world4 = inv.Transform(clip);
  const float invW = 1.0f / world4.w;
  return Vec3{world4.x * invW, world4.y * invW, world4.z * invW};
}

// -----------------------------------------------------------------------------
// Test 1: center mapping using Vulkan projection (Z in [0,1]).
// -----------------------------------------------------------------------------
TEST_CASE("Vulkan Camera: project/unproject center mapping (ZO depth)", "[camera][vk][proj]") {
  const float fovy = 60.0f * 3.1415926535f / 180.0f;
  const float width = 1920.0f, height = 1080.0f;

  const Mat4 P = PerspectiveRH_ZO(fovy, width/height, 0.1f, 1000.0f);
  const Mat4 V = LookAtRH(/*eye*/{0,0,0}, /*center*/{0,0,-1}, /*up*/{0,1,0});
  const Mat4 VP = P * V;

  // World straight ahead should land at screen center.
  const Vec3 world{0,0,-5};
  const Vec3 screen = ProjectToScreenVK(VP, world, width, height);
  REQUIRE_THAT(screen.x, WithinAbs(width * 0.5f, 1e-4f));
  REQUIRE_THAT(screen.y, WithinAbs(height * 0.5f, 1e-4f));

  // With ZO depth, ndcZ should be between 0 (near) and 1 (far).
  REQUIRE(screen.z >= 0.0f);
  REQUIRE(screen.z <= 1.0f);

  // Round-trip unproject at same ndcZ.
  const Vec3 world2 = UnprojectFromScreenVK(VP, screen.x, screen.y, screen.z, width, height);
  REQUIRE(V3Close(world2, world, 1e-4f));
}

// -----------------------------------------------------------------------------
// Test 2: direction vs screen position sanity under Vulkan projection.
// -----------------------------------------------------------------------------
TEST_CASE("Vulkan Camera: right/left/up/down screen mapping", "[camera][vk][proj]") {
  const float width = 1280.0f, height = 720.0f;
  const Mat4 P = PerspectiveRH_ZO(45.0f * 3.1415926535f/180.0f, width/height, 0.1f, 500.0f);
  const Mat4 V = LookAtRH({0,0,2}, {0,0,0}, {0,1,0});
  const Mat4 VP = P * V;

  const Vec3 centerS = ProjectToScreenVK(VP, {0,0,0}, width, height);
  const Vec3 rightS  = ProjectToScreenVK(VP, {1,0,0}, width, height);
  REQUIRE(rightS.x > centerS.x);  // right goes to larger X

  const Vec3 upS = ProjectToScreenVK(VP, {0,1,0}, width, height);
  REQUIRE(upS.y < centerS.y);     // up goes to smaller Y (top-left origin)
}

// -----------------------------------------------------------------------------
// Test 3: picking ray from screen center into world, intersect ground y=0.
// Uses ndcZ 0 (near) and 1 (far), per Vulkan convention.
// -----------------------------------------------------------------------------
TEST_CASE("Vulkan Camera: picking ray hits ground plane (ZO depth)", "[camera][vk][picking]") {
  const float width = 1600.0f, height = 900.0f;
  const Mat4 P = PerspectiveRH_ZO(60.0f * 3.1415926535f/180.0f, width/height, 0.1f, 1000.0f);
  const Vec3 eye{0, 10, 10};
  const Mat4 V = LookAtRH(eye, {0,0,0}, {0,1,0});
  const Mat4 VP = P * V;

  // center pixel
  const float sx = width * 0.5f;
  const float sy = height * 0.5f;

  // build ray: unproject near/far using ZO depths (0 near, 1 far)
  const Vec3 nearW = UnprojectFromScreenVK(VP, sx, sy, 0.0f, width, height);
  const Vec3 farW  = UnprojectFromScreenVK(VP, sx, sy, 1.0f, width, height);

  const Vec3 rayPos = nearW;
  const Vec3 rayDir = (farW - nearW).normalized();

  // Intersect with plane y=0: rayPos.y + t*rayDir.y = 0 → t = -rayPos.y / rayDir.y
  const float denom = rayDir.y;
  REQUIRE(std::fabs(denom) > 1e-6f);
  const float t = -rayPos.y / denom;
  REQUIRE(t > 0.0f);

  const Vec3 hit = rayPos + rayDir * t;
  REQUIRE_THAT(hit.y, WithinAbs(0.0f, 1e-4f));
  REQUIRE(std::fabs(hit.x) < 0.25f);
  REQUIRE(std::fabs(hit.z) < 0.25f);
}

// -----------------------------------------------------------------------------
// Test 4: arbitrary round-trip (project → unproject) under Vulkan projection.
// -----------------------------------------------------------------------------
TEST_CASE("Vulkan Camera: project/unproject arbitrary round-trip", "[camera][vk][proj]") {
  const float w = 1024.0f, h = 768.0f;
  const Mat4 P = PerspectiveRH_ZO(50.0f * 3.1415926535f/180.0f, w/h, 0.5f, 500.0f);
  const Mat4 V = LookAtRH({3,4,5}, {0,1,-2}, {0,1,0});
  const Mat4 VP = P * V;

  const Vec3 world{2.2f, -0.7f, -3.5f};
  const Vec3 screen = ProjectToScreenVK(VP, world, w, h);

  // Unproject using the same ndcZ that came out of projection.
  const Vec3 world2 = UnprojectFromScreenVK(VP, screen.x, screen.y, screen.z, w, h);
  REQUIRE(V3Close(world2, world, 1e-4f));

  // Also ensure Vulkan depth range is respected.
  REQUIRE(screen.z >= 0.0f);
  REQUIRE(screen.z <= 1.0f);
}

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "navary/math/mat4.h"
#include "navary/math/quat.h"
#include "navary/math/vec3.h"
#include "navary/math/vec4.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps;
}
static inline bool V4Close(const Vec4& a, const Vec4& b, float eps = 1e-5f) {
  return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
         std::fabs(a.z - b.z) <= eps && std::fabs(a.w - b.w) <= eps;
}

TEST_CASE("Mat4: identity and zero", "[mat4]") {
  Mat4 I = Mat4::Identity();
  Mat4 Z = Mat4::Zero();

  // Identity * vector == vector
  Vec4 v{1, 2, 3, 1};
  REQUIRE(V4Close(I.Transform(v), v));

  // Zero * vector == zero
  REQUIRE(V4Close(Z.Transform(v), Vec4{0, 0, 0, 0}));
}

TEST_CASE("Mat4: translation, scaling, rotation basics", "[mat4]") {
  const Vec3 p{1, 2, 3};
  // Translation
  Mat4 T = Mat4::Translation(Vec3{10, -5, 2});
  REQUIRE(V3Close(T.TransformPoint(p), Vec3{11, -3, 5}));

  // Scaling
  Mat4 S = Mat4::Scaling(Vec3{2, 3, 4});
  REQUIRE(V3Close(S.TransformPoint(p), Vec3{2, 6, 12}));

  // RotationZ 90deg about origin
  Mat4 Rz = Mat4::RotationZ(3.1415926535f * 0.5f);
  Vec3 px{1, 0, 0};
  Vec3 rx = Rz.TransformVector(px);
  // +X -> +Y
  REQUIRE_THAT(rx.x, WithinAbs(0.f, 1e-5f));
  REQUIRE_THAT(rx.y, WithinAbs(1.f, 1e-5f));
  REQUIRE_THAT(rx.z, WithinAbs(0.f, 1e-5f));

  
}

TEST_CASE("Mat4: TRS composition order matches game-style build (T*Rz*Ry*Rx*S)",
          "[mat4]") {
  const Vec3 t{5, -2, 10};
  const Vec3 rxyz{0.1f, 0.2f, 0.3f};
  const Vec3 s{2, 0.5f, 1.5f};

  Mat4 M = Mat4::TRS(t, rxyz, s);

  // Decompose expectations by applying in the same order to a point:
  Vec3 p_local{1, 2, 3};
  // Manual compose
  Vec3 p = p_local;
  // Scale
  p = Vec3{p.x * s.x, p.y * s.y, p.z * s.z};
  // Rx
  {
    Mat4 Rx = Mat4::RotationX(rxyz.x);
    p       = Rx.TransformVector(p);
  }
  // Ry
  {
    Mat4 Ry = Mat4::RotationY(rxyz.y);
    p       = Ry.TransformVector(p);
  }
  // Rz
  {
    Mat4 Rz = Mat4::RotationZ(rxyz.z);
    p       = Rz.TransformVector(p);
  }
  // Translate
  p = Vec3{p.x + t.x, p.y + t.y, p.z + t.z};

  REQUIRE(V3Close(M.TransformPoint({1, 2, 3}), p, 1e-4f));
}

TEST_CASE("Mat4: perspective RH (OpenGL) projects -Z forward", "[mat4]") {
  const float fovy = 60.0f * 3.1415926535f / 180.0f;
  Mat4 P = Mat4::PerspectiveRH(fovy, /*aspect*/ 16.f / 9.f, /*near*/ 0.1f,
                               /*far*/ 1000.f);

  // A point straight ahead at z = -1 should map to centered NDC x=y=0.
  Vec3 world{0, 0, -1};
  Vec3 clip = P.TransformPoint(world);
  REQUIRE_THAT(clip.x, WithinAbs(0.f, 1e-5f));
  REQUIRE_THAT(clip.y, WithinAbs(0.f, 1e-5f));
  // Z will be in [-1, 1] after perspective division but depends on near/far.
  REQUIRE(clip.z <= 1.0f);
  REQUIRE(clip.z >= -1.0f);
}

TEST_CASE("Mat4: ortho RH (OpenGL) preserves grid axes", "[mat4]") {
  Mat4 O = Mat4::OrthoRH(/*l*/ -10, /*r*/ 10, /*b*/ -10, /*t*/ 10, /*n*/ 0.1f,
                         /*f*/ 100.f);

  // Axis-aligned points should map linearly to NDC
  REQUIRE(V3Close(O.TransformPoint({10, 0, -0.1f}), Vec3{1, 0, -1}, 1e-6f));
  REQUIRE(V3Close(O.TransformPoint({0, 10, -0.1f}), Vec3{0, 1, -1}, 1e-6f));
  REQUIRE(V3Close(O.TransformPoint({-10, 0, -0.1f}), Vec3{-1, 0, -1}, 1e-6f));
}

TEST_CASE("Mat4: inverse (general) and inverse affine", "[mat4]") {
  Mat4 M =
      Mat4::TRS(/*t*/ {3, 4, 5}, /*r*/ {0.1f, 0.2f, -0.3f}, /*s*/ {2, 1, 0.5f});
  Mat4 Minv_general = Mat4::Inverse(M);

  // Round-trip a point
  Vec3 p{7, -2, 9};
  Vec3 world = M.TransformPoint(p);
  Vec3 local = Minv_general.TransformPoint(world);
  REQUIRE(V3Close(local, p, 1e-4f));

  // For pure rigid (no non-uniform scale), InverseAffine is faster.
  Mat4 Rigid    = Mat4::Translation({-10, 2, 3}) * Mat4::RotationY(0.7f);
  Mat4 RigidInv = Mat4::InverseAffine(Rigid);
  Vec3 pw{2, 3, 4};
  REQUIRE(
      V3Close(RigidInv.TransformPoint(Rigid.TransformPoint(pw)), pw, 1e-5f));
}

TEST_CASE("Mat4: TransformPoint vs TransformVector semantics", "[mat4]") {
  Mat4 T = Mat4::Translation({10, 0, 0});
  Vec3 p{1, 2, 3};
  Vec3 v{1, 2, 3};

  // Points are affected by translation; vectors are not.
  REQUIRE(V3Close(T.TransformPoint(p), Vec3{11, 2, 3}));
  REQUIRE(V3Close(T.TransformVector(v), Vec3{1, 2, 3}));
}

TEST_CASE("Mat4: basis getters (Right/Up/Forward) and Translation", "[mat4]") {
  Mat4 M = Mat4::TRS({5, 6, 7}, {0.0f, 0.0f, 0.0f}, {1, 1, 1});
  REQUIRE(V3Close(M.Right(), Vec3{1, 0, 0}));
  REQUIRE(V3Close(M.Up(), Vec3{0, 1, 0}));
  REQUIRE(V3Close(M.Forward(), Vec3{0, 0, 1}));
  REQUIRE(V3Close(M.Translation(), Vec3{5, 6, 7}));
}

TEST_CASE("Mat4 + Quat: consistency between Quat::ToMat4 and Mat4 rotations",
          "[mat4][quat]") {
  const float yaw = 0.35f, pitch = -0.1f, roll = 0.7f;
  Quat q  = Quat::FromEulerXYZ({pitch, yaw, roll});
  Mat4 Rq = q.ToMat4();

  Mat4 Rx = Mat4::RotationX(pitch);
  Mat4 Ry = Mat4::RotationY(yaw);
  Mat4 Rz = Mat4::RotationZ(roll);
  Mat4 Re = Rx * Ry * Rz;  // note: must match the convention in FromEulerXYZ

  // Compare rotating a few basis vectors
  REQUIRE(V3Close(Rq.TransformVector({1, 0, 0}), Re.TransformVector({1, 0, 0}),
                  1e-5f));
  REQUIRE(V3Close(Rq.TransformVector({0, 1, 0}), Re.TransformVector({0, 1, 0}),
                  1e-5f));
  REQUIRE(V3Close(Rq.TransformVector({0, 0, 1}), Re.TransformVector({0, 0, 1}),
                  1e-5f));
}

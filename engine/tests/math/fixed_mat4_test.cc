#include <catch2/catch_all.hpp>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"
#include "navary/math/fixed_trigonometry.h"
#include "navary/math/fixed_mat4.h"
#include "navary/math/fixed_quat.h"

using namespace navary::math;

using F = Fixed15p16;

// small helpers to view fixed as float
static inline float Vf(F v) { return v.ToFloat(); }
static inline float Vx(const FixedVec3& v) { return v.x.ToFloat(); }
static inline float Vy(const FixedVec3& v) { return v.y.ToFloat(); }
static inline float Vz(const FixedVec3& v) { return v.z.ToFloat(); }
static inline F Fdeg(float deg) { return F::FromDegree(deg); }
static inline FixedVec3 F3(float x, float y, float z) {
  return FixedVec3{F::FromFloat(x), F::FromFloat(y), F::FromFloat(z)};
}

// ------------------------------------------------------------
// Basic TRS behavior with column-vectors
// ------------------------------------------------------------
TEST_CASE("FixedMat4: translation, scaling, rotation basic compose", "[fixed][mat4]") {
  // TRS = T * Rz * Ry * Rx * S
  const FixedVec3 t = F3(2, 3, 4);
  const FixedVec3 s = F3(2, 3, 4);
  const FixedVec3 r = FixedVec3{Fdeg(90), F::Zero(), F::Zero()}; // we'll rotate around Z via TRS order

  // We want Rz(90). TRS applies Rx then Ry then Rz (because order is T*Rz*Ry*Rx*S).
  // So set rot_xyz = (0,0,90deg).
  const FixedVec3 rot = FixedVec3{F::Zero(), F::Zero(), Fdeg(90)};

  FixedMat4 M = FixedMat4::TRS(t, rot, s);

  // Start with +X
  FixedVec3 p = F3(1, 0, 0);
  FixedVec3 q = M.TransformPoint(p);

  // Expectation: scale(2,3,4): (2,0,0)
  // then Rz(90): (0,2,0)
  // then translate (2,3,4): (2, 5, 4)
  REQUIRE(Vx(q) == Catch::Approx(2.f).margin(0.05f));
  REQUIRE(Vy(q) == Catch::Approx(5.f).margin(0.05f));
  REQUIRE(Vz(q) == Catch::Approx(4.f).margin(0.05f));

  // Vectors ignore translation
  FixedVec3 v = F3(1,0,0);
  FixedVec3 rv = M.TransformVector(v);
  // After S then Rz(90): (0,2,0)
  REQUIRE(Vx(rv) == Catch::Approx(0.f).margin(0.05f));
  REQUIRE(Vy(rv) == Catch::Approx(2.f).margin(0.05f));
  REQUIRE(Vz(rv) == Catch::Approx(0.f).margin(0.05f));
}

// ------------------------------------------------------------
// Quaternion → matrix consistency (rotate basis & arbitrary vectors)
// ------------------------------------------------------------
TEST_CASE("FixedMat4: From quaternion consistency", "[fixed][mat4][quat]") {
  FixedQuat q = FixedQuat::FromEulerXYZ(
      FixedVec3{Fdeg(5), Fdeg(-12), Fdeg(30)});
  FixedMat4 Rm = q.ToMat4();

  FixedVec3 vx{F::FromInt(1), F::Zero(), F::Zero()};
  FixedVec3 vy{F::Zero(), F::FromInt(1), F::Zero()};
  FixedVec3 vz{F::Zero(), F::Zero(), F::FromInt(1)};
  FixedVec3 va{F::FromFloat(0.3f), F::FromFloat(-1.1f), F::FromFloat(2.2f)};

  auto qx = q.Rotate(vx);
  auto qy = q.Rotate(vy);
  auto qz = q.Rotate(vz);
  auto qa = q.Rotate(va);

  auto mx = Rm.TransformVector(vx);
  auto my = Rm.TransformVector(vy);
  auto mz = Rm.TransformVector(vz);
  auto ma = Rm.TransformVector(va);

  const float eps = 0.03f;
  REQUIRE(Vx(mx) == Catch::Approx(Vx(qx)).margin(eps));
  REQUIRE(Vy(mx) == Catch::Approx(Vy(qx)).margin(eps));
  REQUIRE(Vz(mx) == Catch::Approx(Vz(qx)).margin(eps));

  REQUIRE(Vx(my) == Catch::Approx(Vx(qy)).margin(eps));
  REQUIRE(Vy(my) == Catch::Approx(Vy(qy)).margin(eps));
  REQUIRE(Vz(my) == Catch::Approx(Vz(qy)).margin(eps));

  REQUIRE(Vx(mz) == Catch::Approx(Vx(qz)).margin(eps));
  REQUIRE(Vy(mz) == Catch::Approx(Vy(qz)).margin(eps));
  REQUIRE(Vz(mz) == Catch::Approx(Vz(qz)).margin(eps));

  REQUIRE(Vx(ma) == Catch::Approx(Vx(qa)).margin(eps));
  REQUIRE(Vy(ma) == Catch::Approx(Vy(qa)).margin(eps));
  REQUIRE(Vz(ma) == Catch::Approx(Vz(qa)).margin(eps));
}

// ------------------------------------------------------------
// InverseAffine round-trip (rigid/uniform scale)
// ------------------------------------------------------------
TEST_CASE("FixedMat4: InverseAffine round-trip", "[fixed][mat4]") {
  FixedVec3 t = F3(3, -2, 5);
  FixedVec3 rot = FixedVec3{Fdeg(10), Fdeg(20), Fdeg(-15)};
  FixedVec3 s = F3(2, 2, 2); // uniform scale is ok for InverseAffine assumption

  FixedMat4 M = FixedMat4::TRS(t, rot, s);
  FixedMat4 Minv = FixedMat4::InverseAffine(M);

  FixedVec3 pt = F3(1.5f, -0.5f, 3.25f);
  FixedVec3 world = M.TransformPoint(pt);
  FixedVec3 back  = Minv.TransformPoint(world);

  REQUIRE(Vx(back) == Catch::Approx(Vx(pt)).margin(0.02f));
  REQUIRE(Vy(back) == Catch::Approx(Vy(pt)).margin(0.02f));
  REQUIRE(Vz(back) == Catch::Approx(Vz(pt)).margin(0.02f));
}

// ------------------------------------------------------------
// Basis getters and Translation
// ------------------------------------------------------------
TEST_CASE("FixedMat4: basis getters and translation", "[fixed][mat4]") {
  FixedVec3 t = F3(5, 6, 7);
  FixedVec3 rot = FixedVec3{F::Zero(), F::Zero(), F::Zero()};
  FixedVec3 s = F3(1, 1, 1);

  FixedMat4 M = FixedMat4::TRS(t, rot, s);
  auto right = M.Right();
  auto up    = M.Up();
  auto fwd   = M.Forward();
  auto trans = M.Translation();

  REQUIRE(Vx(right) == Catch::Approx(1.f).margin(0.001f));
  REQUIRE(Vy(right) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vz(right) == Catch::Approx(0.f).margin(0.001f));

  REQUIRE(Vx(up) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vy(up) == Catch::Approx(1.f).margin(0.001f));
  REQUIRE(Vz(up) == Catch::Approx(0.f).margin(0.001f));

  REQUIRE(Vx(fwd) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vy(fwd) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vz(fwd) == Catch::Approx(1.f).margin(0.001f));

  REQUIRE(Vx(trans) == Catch::Approx(5.f).margin(0.001f));
  REQUIRE(Vy(trans) == Catch::Approx(6.f).margin(0.001f));
  REQUIRE(Vz(trans) == Catch::Approx(7.f).margin(0.001f));
}

// ------------------------------------------------------------
// OrthoRH / PerspectiveRH sanity (basic mapping properties)
// ------------------------------------------------------------
TEST_CASE("FixedMat4: OrthoRH/PerspectiveRH mapping sanity", "[fixed][mat4]") {
  FixedMat4 O = FixedMat4::OrthoRH(
      F::FromInt(-10), F::FromInt(10),
      F::FromInt(-10), F::FromInt(10),
      F::FromFloat(0.1f), F::FromInt(100));

  // axis aligned points -> NDC
  auto a = O.TransformPoint(F3(10, 0, -0.1f));
  REQUIRE(Vx(a) == Catch::Approx(1.f).margin(0.001f));
  REQUIRE(Vy(a) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vz(a) == Catch::Approx(-1.f).margin(0.01f));

  auto b = O.TransformPoint(F3(0, 10, -0.1f));
  REQUIRE(Vx(b) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vy(b) == Catch::Approx(1.f).margin(0.001f));

  FixedMat4 P = FixedMat4::PerspectiveRH(
      F::FromDegree(60), F::FromFloat(16.0f/9.0f),
      F::FromFloat(0.1f), F::FromInt(1000));

  // forward -Z maps near center (x,y ≈ 0)
  auto c = P.TransformPoint(F3(0, 0, -1));
  REQUIRE(Vx(c) == Catch::Approx(0.f).margin(0.05f));
  REQUIRE(Vy(c) == Catch::Approx(0.f).margin(0.05f));
}

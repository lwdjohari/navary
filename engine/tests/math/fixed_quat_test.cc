#include <catch2/catch_all.hpp>
#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"
#include "navary/math/fixed_quat.h"

using namespace navary::math;

using F = Fixed15p16;

static inline float Vx(const FixedVec3& v) {
  return v.x.ToFloat();
}
static inline float Vy(const FixedVec3& v) {
  return v.y.ToFloat();
}
static inline float Vz(const FixedVec3& v) {
  return v.z.ToFloat();
}
static inline FixedVec3 F3(float x, float y, float z) {
  return FixedVec3{F::FromFloat(x), F::FromFloat(y), F::FromFloat(z)};
}
static inline F Fdeg(float d) {
  return F::FromDegree(d);
}

// ------------------------------------------------------------
// Identity & Axis-angle
// ------------------------------------------------------------
TEST_CASE("FixedQuat: identity and 90deg Z axis-angle", "[fixed][quat]") {
  FixedQuat I = FixedQuat::Identity();
  FixedVec3 v = F3(1, 0, 0);
  auto r0     = I.Rotate(v);
  REQUIRE(Vx(r0) == Catch::Approx(1.f).margin(0.001f));
  REQUIRE(Vy(r0) == Catch::Approx(0.f).margin(0.001f));
  REQUIRE(Vz(r0) == Catch::Approx(0.f).margin(0.001f));

  // 90° about Z: +X -> +Y
  FixedQuat q = FixedQuat::FromAxisAngle(F3(0, 0, 1), Fdeg(90));
  auto r      = q.Rotate(F3(1, 0, 0));
  REQUIRE(Vx(r) == Catch::Approx(0.f).margin(0.02f));
  REQUIRE(Vy(r) == Catch::Approx(1.f).margin(0.02f));
  REQUIRE(Vz(r) == Catch::Approx(0.f).margin(0.02f));
}

// ------------------------------------------------------------
// Composition & EulerXYZ parity
// ------------------------------------------------------------
TEST_CASE("FixedQuat: FromEulerXYZ matches composed matrix rotation",
          "[fixed][quat][mat4]") {
  FixedVec3 e  = FixedVec3{Fdeg(10), Fdeg(-20), Fdeg(35)};
  FixedQuat q  = FixedQuat::FromEulerXYZ(e);
  FixedMat4 Rm = q.ToMat4();

  FixedVec3 v = F3(0.3f, -1.2f, 2.7f);
  auto rq     = q.Rotate(v);
  auto rm     = Rm.TransformVector(v);

  REQUIRE(Vx(rm) == Catch::Approx(Vx(rq)).margin(0.03f));
  REQUIRE(Vy(rm) == Catch::Approx(Vy(rq)).margin(0.03f));
  REQUIRE(Vz(rm) == Catch::Approx(Vz(rq)).margin(0.03f));
}

// ------------------------------------------------------------
// Nlerp endpoints & halfway sanity
// ------------------------------------------------------------
TEST_CASE("FixedQuat: Nlerp endpoints and halfway", "[fixed][quat]") {
  FixedQuat a =
      FixedQuat::FromAxisAngle(F3(0, 1, 0), F::FromInt(0));  // identity
  FixedQuat b = FixedQuat::FromAxisAngle(F3(0, 1, 0), Fdeg(90));

  // Endpoints exact
  auto v = F3(1, 0, 0);
  REQUIRE(Vx(FixedQuat::Nlerp(a, b, F::Zero()).Rotate(v)) ==
          Catch::Approx(Vx(a.Rotate(v))).margin(0.001f));
  REQUIRE(Vx(FixedQuat::Nlerp(a, b, F::FromInt(1)).Rotate(v)) ==
          Catch::Approx(Vx(b.Rotate(v))).margin(0.001f));

  // Halfway ~ 45° about Y: +X -> (cos45, 0, -sin45)
  auto mid = FixedQuat::Nlerp(a, b, F::FromFraction(1, 2)).Rotate(v);
  REQUIRE(Vx(mid) == Catch::Approx(0.707f).margin(0.02f));
  REQUIRE(Vz(mid) == Catch::Approx(-0.707f).margin(0.02f));
}

// ------------------------------------------------------------
// ToMat4 parity: basis vectors
// ------------------------------------------------------------
TEST_CASE("FixedQuat: ToMat4 rotates basis like quaternion",
          "[fixed][quat][mat4]") {
  FixedQuat q =
      FixedQuat::FromEulerXYZ(FixedVec3{Fdeg(-7), Fdeg(25), Fdeg(13)});
  FixedMat4 R = q.ToMat4();

  FixedVec3 bx{F::FromInt(1), F::Zero(), F::Zero()};
  FixedVec3 by{F::Zero(), F::FromInt(1), F::Zero()};
  FixedVec3 bz{F::Zero(), F::Zero(), F::FromInt(1)};

  auto qx = q.Rotate(bx);
  auto qy = q.Rotate(by);
  auto qz = q.Rotate(bz);

  auto mx = R.TransformVector(bx);
  auto my = R.TransformVector(by);
  auto mz = R.TransformVector(bz);

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
}

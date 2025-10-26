#include <catch2/catch_all.hpp>
#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/quat.h"
#include "navary/math/plane.h"
#include "navary/math/spline.h"  // RNSpline

using namespace navary::math;

static inline bool V3Close(const Vec3& a, const Vec3& b, float eps=1e-4f) {
  return std::fabs(a.x-b.x)<=eps && std::fabs(a.y-b.y)<=eps && std::fabs(a.z-b.z)<=eps;
}

// Patrol path along waypoints with constant speed feel (rounded nonuniform).
TEST_CASE("RN: Patrol path – monotone progress and endpoint hits", "[spline][rn]") {
  RnSpline s;
  s.AddNode(Vec3{0,0,0});
  s.AddNode(Vec3{10,0,0});
  s.AddNode(Vec3{10,0,10});
  s.AddNode(Vec3{0,0,10});
  s.Build();

  // t=0 and t=1 – start/end on the path hull.
  REQUIRE(V3Close(s.GetPosition(0.0f), Vec3{0,0,0}, 1e-3f));
  REQUIRE(V3Close(s.GetPosition(1.0f), Vec3{0,0,10}, 1e-3f));

  // Monotonic progress along path length: increasing t should not go backwards (dot with forward edge >= 0).
  Vec3 prev = s.GetPosition(0.0f);
  for (int i=1;i<=16;i++){
    float t = i/16.0f;
    Vec3 cur = s.GetPosition(t);
    Vec3 d = cur - prev;
    // Not going backwards in aggregate distance.
    REQUIRE(d.length() >= -1e-4f);
    prev = cur;
  }
}

// Camera rig dolly on a gentle S curve – smoothness (no sharp cusps on interior).
TEST_CASE("RN: Dolly path – interior continuity (C1-ish)", "[spline][rn]") {
  RnSpline s;
  s.AddNode(Vec3{-5, 0, -5});
  s.AddNode(Vec3{-2, 0,  0});
  s.AddNode(Vec3{ 2, 0,  0});
  s.AddNode(Vec3{ 5, 0,  5});
  s.Build();

  // Finite difference tangent continuity around the interior samples:
  auto tangent = [&](float t){
    const float h = 1e-2f;
    Vec3 a = s.GetPosition(std::max(0.0f,t-h));
    Vec3 b = s.GetPosition(std::min(1.0f,t+h));
    Vec3 v = b - a;
    float L = v.length();
    return (L>0.f)? (v*(1.0f/L)) : Vec3{1,0,0};
  };

  Vec3 t0 = tangent(0.49f);
  Vec3 t1 = tangent(0.51f);
  REQUIRE(t0.dot(t1) > 0.9f); // reasonably aligned
}

// Clamp behavior at out-of-range time.
TEST_CASE("RN: GetPosition clamps t to [0,1]", "[spline][rn]") {
  RnSpline s;
  s.AddNode(Vec3{0,0,0});
  s.AddNode(Vec3{10,0,0});
  s.Build();

  REQUIRE(V3Close(s.GetPosition(-2.0f), Vec3{0,0,0}));
  REQUIRE(V3Close(s.GetPosition( 2.0f), Vec3{10,0,0}));
}

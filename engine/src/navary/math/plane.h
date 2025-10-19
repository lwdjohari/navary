#pragma once
#include <cfloat>
#include <cmath>

#include "navary/math/vec4.h"
#include "navary/math/vec3.h"  // Vec3, Vec4
#include "navary/math/vec2.h"  // if you keep Vec2/Size2 in a separate header

namespace navary::math {

/** Which side of the plane a point lies on. */
enum class PlaneSide { kFront, kBack, kOn };

/**
 * @brief Plane: Ax + By + Cz + D = 0, with n = (A,B,C), d = D.
 *
 * Normal does not need to be unit-length, but many ops (e.g., distance)
 * assume it is normalized for “true” geometric distance. Call Normalize()
 * after construction if you need unit-length normals.
 */
class Plane {
 public:
  Plane();                             // n = (0,0,1), d = 0
  explicit Plane(const Vec4& coeffs);  // (A,B,C,D)
  Plane(const Vec3& normal, float d);  // n, d
  static Plane FromPoints(const Vec3& p1, const Vec3& p2, const Vec3& p3);
  static Plane FromNormalAndPoint(const Vec3& normal, const Vec3& point);

  /** Normalize the plane so that |n| = 1. No-op if n is near zero. */
  void Normalize();

  /** Signed distance: n·p + d. If plane is normalized, this is true distance.
   */
  float DistanceTo(const Vec3& p) const;

  /** True if point is strictly on the back side (with tolerance). */
  bool IsBackSide(const Vec3& p, float eps = kEps) const;

  /** Classify point location w.r.t. plane, with tolerance. */
  PlaneSide ClassifyPoint(const Vec3& p, float eps = kEps) const;

  /**
   * Intersect an infinite ray R(t) = origin + t * dir, t >= 0.
   * Returns false if parallel or pointing away. If true, writes intersection.
   */
  bool IntersectRay(const Vec3& origin, const Vec3& dir, Vec3* out) const;

  /**
   * Intersect a line segment [a, b]. Returns false if no hit within the
   * segment. If true, writes intersection on the segment.
   */
  bool IntersectSegment(const Vec3& a, const Vec3& b, Vec3* out) const;

  const Vec3& normal() const {
    return n_;
  }
  float d() const {
    return d_;
  }

  void set_normal(const Vec3& n) {
    n_ = n;
  }
  void set_d(float d) {
    d_ = d;
  }

  /** Small tolerance used for classification. */
  static constexpr float kEps = 1e-3f;

 private:
  Vec3 n_{0.f, 0.f, 1.f};  // plane normal
  float d_{0.f};           // plane D
};

}  // namespace navary::math

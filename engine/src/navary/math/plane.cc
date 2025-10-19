#include "navary/math/plane.h"
#include <algorithm>

namespace navary::math {

Plane::Plane() = default;

Plane::Plane(const Vec4& coeffs)
    : n_{coeffs.x, coeffs.y, coeffs.z}, d_{coeffs.w} {}

Plane::Plane(const Vec3& normal, float d) : n_(normal), d_(d) {} 

Plane Plane::FromPoints(const Vec3& p1, const Vec3& p2, const Vec3& p3) {
  Vec3 d1 = p2 - p1;
  Vec3 d2 = p3 - p1;
  Vec3 n  = d2.cross(d1);  // <= flip order to get upward normal
  float d = -n.dot(p1);
  Plane pl(n, d);
  return pl;
}

Plane Plane::FromNormalAndPoint(const Vec3& normal, const Vec3& point) {
  Plane pl(normal, -normal.dot(point));
  return pl;
}

void Plane::Normalize() {
  float len = n_.length();
  if (len <= FLT_EPSILON)
    return;
  float inv = 1.0f / len;
  n_ *= inv;
  d_ *= inv;
}

float Plane::DistanceTo(const Vec3& p) const {
  return n_.dot(p) + d_;
}

bool Plane::IsBackSide(const Vec3& p, float eps) const {
  return DistanceTo(p) < -eps;
}

PlaneSide Plane::ClassifyPoint(const Vec3& p, float eps) const {
  const float dist = DistanceTo(p);
  if (dist > eps)
    return PlaneSide::kFront;
  if (dist < -eps)
    return PlaneSide::kBack;
  return PlaneSide::kOn;
}

bool Plane::IntersectRay(const Vec3& origin, const Vec3& dir, Vec3* out) const {
  const float denom = n_.dot(dir);
  if (std::fabs(denom) < FLT_EPSILON)
    return false;  // parallel to plane
  const float t = -(n_.dot(origin) + d_) / denom;
  if (t < 0.0f)
    return false;  // behind origin
  if (out)
    *out = origin + dir * t;
  return true;
}

bool Plane::IntersectSegment(const Vec3& a, const Vec3& b, Vec3* out) const {
  const float da = DistanceTo(a);
  const float db = DistanceTo(b);
  // If both on same side (strictly), no intersection within segment.
  if ((da > 0.f && db > 0.f) || (da < 0.f && db < 0.f))
    return false;

  const float denom = db - da;
  if (std::fabs(denom) < FLT_EPSILON) {
    // Segment is nearly parallel or both points almost on plane.
    // Treat as no unique intersection point.
    return false;
  }

  const float t = -da / denom;  // 0..1 if inside the segment
  if (t < 0.f || t > 1.f)
    return false;
  if (out)
    *out = a + (b - a) * t;
  return true;
}

}  // namespace navary::math

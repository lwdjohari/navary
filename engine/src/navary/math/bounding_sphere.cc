// navary/math/bounding_sphere.cc

#include "navary/math/bounding_sphere.h"

#include <algorithm>
#include <cmath>

namespace navary::math {

/*static*/ BoundingSphere BoundingSphere::FromSweptAabb(const Aabb& box) {
  const Vec3 mn = box.min();
  const Vec3 mx = box.max();

  // Largest absolute extent along each axis from origin among the two box
  // corners.
  const float max_x = std::max(std::fabs(mn.x), std::fabs(mx.x));
  const float max_y = std::max(std::fabs(mn.y), std::fabs(mx.y));
  const float max_z = std::max(std::fabs(mn.z), std::fabs(mx.z));

  const float r = std::sqrt(max_x * max_x + max_y * max_y + max_z * max_z);
  return BoundingSphere(Vec3{0.f, 0.f, 0.f}, r);
}

bool BoundingSphere::RayIntersect(const Vec3& origin, const Vec3& dir) const {
  // Vector from ray origin to sphere center
  Vec3 v = center_ - origin;

  // Length of projection of v onto ray direction
  const float pc_len = dir.dot(v);

  // If the sphere center projects in front of the origin, compute the shortest
  // vector from center to ray; else, keep v as center-to-origin vector.
  if (pc_len > 0.0f) {
    v = dir * pc_len - v;  // shortest vector from center to the ray line
  }

  const float dist2 = v.dot(v);
  const float r2    = radius_ * radius_;
  return dist2 <= r2;
}

BoundingSphere BoundingSphere::FromAabbTight(const Aabb& box) {
  // Center = AABB center; radius = half of diagonal length
  const Vec3 c   = (box.min() + box.max()) * 0.5f;
  const Vec3 ext = (box.max() - box.min()) * 0.5f;
  const float r  = std::sqrt(ext.x * ext.x + ext.y * ext.y + ext.z * ext.z);
  return BoundingSphere(c, r);
}

/*static*/ BoundingSphere BoundingSphere::FromPoints(const Vec3* pts,
                                                     std::size_t count) {
  if (!pts || count == 0) {
    return BoundingSphere();  // (0,0,0), r=0
  }

  // Deterministic one-pass: center = mean of points
  Vec3 sum{0.f, 0.f, 0.f};
  for (std::size_t i = 0; i < count; ++i) {
    sum += pts[i];
  }
  const float inv_n = 1.0f / static_cast<float>(count);
  const Vec3 c      = sum * inv_n;

  // Radius = max distance to center (not minimal in general, but stable & fast)
  float r2 = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    const Vec3 d = pts[i] - c;
    r2           = std::max(r2, d.dot(d));
  }
  return BoundingSphere(c, std::sqrt(r2));
}

/*static*/ BoundingSphere BoundingSphere::Merge(const BoundingSphere& a,
                                                const BoundingSphere& b) {
  // Handle degenerate cases
  if (a.radius_ <= 0.0f)
    return b;
  if (b.radius_ <= 0.0f)
    return a;

  const Vec3 d      = b.center_ - a.center_;
  const float dist2 = d.dot(d);
  const float dist  = std::sqrt(dist2);

  // One sphere contains the other -> return the larger
  if (a.radius_ >= b.radius_ + dist) {
    return a;
  }
  if (b.radius_ >= a.radius_ + dist) {
    return b;
  }

  // General case: minimal enclosing sphere of two spheres
  // New center lies along the line between centers
  const float new_r = 0.5f * (dist + a.radius_ + b.radius_);
  Vec3 new_c        = a.center_;

  if (dist > 0.0f) {
    const float t = (new_r - a.radius_) / dist;  // in [0,1]
    new_c         = a.center_ + d * t;
  } else {
    // Same center; just pick radius to enclose both
    // (new_c already equals a.center_)
  }

  return BoundingSphere(new_c, new_r);
}

void BoundingSphere::Expand(float amount) {
  if (amount <= 0.0f)
    return;
  radius_ += amount;
  if (radius_ < 0.0f)
    radius_ = 0.0f;  // safety, though amount>0 keeps it >=0
}

/*static*/ BoundingSphere BoundingSphere::Transformed(const Mat4& m) const {
  // Center transforms as a point
  const Vec3 c = m.TransformPoint(center_);

  // Radius scales by the maximum stretch of the linear part (conservative)
  const float sx = m.TransformVector(Vec3{1.f, 0.f, 0.f}).length();
  const float sy = m.TransformVector(Vec3{0.f, 1.f, 0.f}).length();
  const float sz = m.TransformVector(Vec3{0.f, 0.f, 1.f}).length();
  const float s  = std::max(sx, std::max(sy, sz));

  return BoundingSphere(c, radius_ * s);
}

/*static*/ bool BoundingSphere::ContainsPoint(const Vec3& p) const {
  const Vec3 d = p - center_;

  return d.dot(d) <= (radius_ * radius_);
}

/*static*/ bool BoundingSphere::ContainsSphere(const BoundingSphere& s) const {
  const Vec3 d     = s.center_ - center_;
  const float dist = std::sqrt(d.dot(d));
  
  // Strict containment: touching boundary is NOT contained.
  return (dist + s.radius_) < radius_;
}

/*static*/ bool BoundingSphere::IntersectsSphere(
    const BoundingSphere& s) const {
  const Vec3 d     = s.center_ - center_;
  const float rsum = radius_ + s.radius_;

  return d.dot(d) <= (rsum * rsum);
}

/*static*/ bool BoundingSphere::IntersectsAabb(const Aabb& box) const {
  // Clamp center to AABB to get closest point on the box
  const Vec3 mn = box.min();
  const Vec3 mx = box.max();
  Vec3 q{std::min(std::max(center_.x, mn.x), mx.x),
         std::min(std::max(center_.y, mn.y), mx.y),
         std::min(std::max(center_.z, mn.z), mx.z)};
  const Vec3 d = q - center_;

  return d.dot(d) <= (radius_ * radius_);
}

}  // namespace navary::math

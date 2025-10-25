// navary/math/obb.h

/**
 * @file obb.h
 * @brief Oriented bounding box = center + basis[0..2] * half_sizes.
 *
 * ```cpp
 * Obb o1 = Obb::FromCenterExtents({0,0,0}, {1,2,3});
 * Aabb bounds = o1.ToAabb();
 * ```
 */

#pragma once

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/aabb.h"

namespace navary::math {

/* Oriented bounding box = center + basis[0..2] * half_sizes */
class Obb {
 public:
  Obb() = default;
  Obb(const Vec3& center, const Vec3& u, const Vec3& v, const Vec3& w,
      const Vec3& half_sizes);

  static Obb FromAabb(const Aabb& aabb);

  /** Affine transform: center as point, axes as vectors; renormalize axes. */
  Obb Transformed(const Mat4& M) const;

  /**
   * @brief Ray vs OBB (slab test). Conservative. dir should be unit length.
   * @return true if intersects. Writes t-interval when provided.
   */
  bool RayIntersect(const Vec3& origin, const Vec3& dir, float* tmin_out,
                    float* tmax_out) const;

  const Vec3& center() const {
    return center_;
  }

  const Vec3& half_sizes() const {
    return half_;
  }

  const Vec3& axis_u() const {
    return basis_[0];
  }

  const Vec3& axis_v() const {
    return basis_[1];
  }

  const Vec3& axis_w() const {
    return basis_[2];
  }

  /** Create from center and extents aligned to world axes (axis = identity). */
  static Obb FromCenterExtents(const Vec3& center, const Vec3& half_sizes);

  /** Convert to tight AABB in world space. */
  Aabb ToAabb() const;

 private:
  static float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  Vec3 center_{0, 0, 0};
  Vec3 half_{0, 0, 0};
  Vec3 basis_[3] = {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}};
};

}  // namespace navary::math

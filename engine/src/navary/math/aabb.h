// navary/math/aabb.h

/**
 * @file aabb.h
 * @brief Axis-aligned bounding box in world space.
 *
 * Notes:
 *  - Empty AABB uses (+INF, -INF) convention; Extend() handles it naturally.
 *  - Transformed() produces the tightest AABB that contains the affine
 * transform of this box (Graphics Gems approach), respecting
 * column-major/column-vector matrices.
 *
 * ```cpp
 * Aabb scene_bounds = Aabb::FromPoints(vertices.data(), vertices.size());
 * Aabb moved_box    = Aabb::FromTransformed(scene_bounds, world_matrix);
 * Aabb combined     = Aabb::Union(box1, box2);
 * Aabb clipped      = Aabb::Intersect(box1, box2);
 * ```
 */

#pragma once

#include <limits>
#include "navary/math/vec3.h"
#include "navary/math/mat4.h"

namespace navary::math {

/* Axis-aligned bounding box in world space. */
class Aabb {
 public:
  Aabb();
  Aabb(const Vec3& mn, const Vec3& mx);

  const Vec3& min() const {
    return min_;
  }

  const Vec3& max() const {
    return max_;
  }

  void SetEmpty();
  bool IsEmpty() const;

  void Extend(const Vec3& p);
  void Extend(const Aabb& b);

  Vec3 Center() const;
  Vec3 Extents() const;
  float Volume() const;

  /** Tight AABB around M * this (affine only). */
  Aabb Transformed(const Mat4& M) const;

  bool ContainsPoint(const Vec3& p) const;

  /** Create an AABB enclosing a set of points. */
  static Aabb FromPoints(const Vec3* pts, std::size_t count);

  /** Create an AABB enclosing another AABB after applying a transform. */
  static Aabb FromTransformed(const Aabb& box, const Mat4& transform);

  /** Merge two AABBs. */
  static Aabb Union(const Aabb& a, const Aabb& b);

  /** Intersection (returns empty if disjoint). */
  static Aabb Intersect(const Aabb& a, const Aabb& b);

 private:
  Vec3 min_;
  Vec3 max_;
};

}  // namespace navary::math

// navary/math/bounding_sphere.h

#pragma once

#include "navary/math/vec3.h"
#include "navary/math/aabb.h"

namespace navary::math {

/**
 * @brief Bounding sphere in 3D.
 *
 * - Center and radius, no heap allocations, trivially copyable.
 * - Right-handed conventions (consistent with library).
 * - No exceptions / RTTI.
 */
class BoundingSphere {
 public:
  /** Default: center=(0,0,0), radius=0. */
  constexpr BoundingSphere() = default;

  /** Construct from center and radius (radius >= 0 expected). */
  constexpr BoundingSphere(const Vec3& center, float radius)
      : center_(center), radius_(radius) {}

  /** Center getter. */
  constexpr const Vec3& center() const {
    return center_;
  }

  /** Radius getter. */
  constexpr float radius() const {
    return radius_;
  }

  /**
   * @brief Construct a bounding sphere that encompasses an AABB
   * swept through all possible rotations around the origin.
   *
   * Equivalent to a sphere centered at the origin with radius equal to the
   * Euclidean norm of the largest absolute corner coordinate among the box
   * corners. Useful for conservative culling of freely rotating meshes.
   *
   * Precondition: the AABB is defined in a space where rotations are around
   * the origin (typically model space).
   */
  static BoundingSphere FromSweptAabb(const Aabb& box);

  /**
   * @brief Ray/sphere intersection (conservative, counts origin-inside as hit).
   *
   * Ray is defined as R(t) = origin + t*dir, with t >= 0.
   * Direction should be normalized for correct distances; normalization is
   * not performed here for performance and determinism.
   *
   * @return true if the ray intersects the sphere (including starting inside).
   */
  bool RayIntersect(const Vec3& origin, const Vec3& dir) const;

  /**
   * @brief Tight sphere of an AABB (center at AABB center, radius = half
   * diagonal). Deterministic and exact for AABB.
   */
  static BoundingSphere FromAabbTight(const Aabb& box);

  /**
   * @brief Sphere that encloses a set of points.
   * Deterministic one-pass: center = mean; radius = max distance to center.
   * (Not minimal in general but stable, fast, and reproducible.)
   */
  static BoundingSphere FromPoints(const Vec3* pts, std::size_t count);

  /**
   * @brief Minimal enclosing sphere of two spheres (exact).
   * If one contains the other, returns the larger. Otherwise builds the
   * unique sphere covering both.
   */
  static BoundingSphere Merge(const BoundingSphere& a, const BoundingSphere& b);

  /**
   * @brief Expand radius by non-negative amount. Center unchanged.
   * Radius will not go below zero (no-op for negative input).
   */
  void Expand(float amount);

  /**
   * @brief Transform sphere by an affine matrix.
   * Center is transformed as a point. Radius is scaled conservatively by
   * the maximum length of the linear-part basis vectors (handles non-uniform
   * scale and shear safely).
   */
  BoundingSphere Transformed(const Mat4& m) const;

  /**
   * @brief Containment / intersection tests.
   */
  bool ContainsPoint(const Vec3& p) const;
  bool ContainsSphere(const BoundingSphere& s) const;
  bool IntersectsSphere(const BoundingSphere& s) const;

  /**
   * @brief Sphere vs AABB intersection (conservative, exact).
   * Uses closest-point-on-AABB distance test.
   */
  bool IntersectsAabb(const Aabb& box) const;

 private:
  Vec3 center_{0.f, 0.f, 0.f};
  float radius_{0.f};
};

}  // namespace navary::math

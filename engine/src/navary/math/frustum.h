// navary/math/frustum.h

/**
 * @file frustum.h
 * @brief Frustum = small set of planes used for visibility tests.
 *
 * Conventions:
 *  - Right-handed coordinates, column-major Mat4, column-vector math.
 *  - Points transform as x' = M * x. Planes transform via M^{-T}.
 */

#pragma once

#include <array>
#include <cstddef>

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"
#include "navary/math/plane.h"

namespace navary::math {

class Frustum {
 public:
  static constexpr std::size_t kMaxPlanes = 10;

  Frustum() = default;

  void Clear() {
    count_ = 0;
  }
  std::size_t size() const {
    return count_;
  }

  // Clamped to kMaxPlanes.
  void SetNumPlanes(std::size_t n) {
    count_ = (n > kMaxPlanes) ? kMaxPlanes : n;
  }

  // Returns false if already full.
  bool AddPlane(const Plane& p);

  Plane& operator[](std::size_t i) {
    return planes_[i];
  }
  const Plane& operator[](std::size_t i) const {
    return planes_[i];
  }

  /**
   * @brief Transform all planes by matrix M that transforms points: x' = M * x.
   * Internally uses plane' = (M^{-T}) * plane.
   */
  void Transform(const Mat4& M);

  /** @return true if point is on or in front of all planes. */
  bool IsPointVisible(const Vec3& p) const;

  /** Sphere test (conservative): returns false only if entirely behind a plane.
   */
  bool IsSphereVisible(const Vec3& center, float radius) const;

  /**
   * @brief AABB test (conservative).
   * @param aabb_min World-space min corner.
   * @param aabb_max World-space max corner.
   */
  bool IsAabbVisible(const Vec3& aabb_min, const Vec3& aabb_max) const;

  /**
   * @brief Build a view frustum by extracting planes from a clip matrix.
   *
   * Let clip_from_world = proj * view (column-vectors). This extracts the
   * 6 canonical planes (Left, Right, Bottom, Top, Near, Far) with inward
   * normals. Planes are normalized.
   */
  static Frustum FromClipMatrix(const Mat4& clip_from_world);

  /**
   * @brief Convenience: From projection and view matrices.
   * clip_from_world = proj * view
   */
  static Frustum FromViewProj(const Mat4& proj, const Mat4& view);

  /**
   * @brief Convenience: From camera params (PerspectiveRH) and a view matrix.
   */
  static Frustum FromCameraPerspectiveRH(float fovy, float aspect, float z_near,
                                         float z_far, const Mat4& view);

  /**
   * @brief Extract the 6 planes from a clip matrix into an array.
   * Order: 0=L,1=R,2=B,3=T,4=N,5=F.
   */
  static void ExtractPlanes(const Mat4& clip_from_world, Plane out6[6]);

 private:
  static Plane TransformPlaneInvTranspose(const Plane& p, const Mat4& M);

  std::array<Plane, kMaxPlanes> planes_{};
  std::size_t count_ = 0;
};

}  // namespace navary::math

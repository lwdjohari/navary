// navary/math/frustum.cc

#include "navary/math/frustum.h"

#include <cmath>

namespace navary::math {
namespace {
// Return row r as (x,y,z,w) from column-major Mat4 with column-vectors.
inline Vec4 Row4(const Mat4& M, int r) {
  return Vec4{M.get(0, r), M.get(1, r), M.get(2, r), M.get(3, r)};
}

// Build plane from raw coeffs (A,B,C,D) and normalize.
inline Plane MakeNormalizedPlane(float A, float B, float C, float D) {
  Plane p(Vec3{A, B, C}, D);
  p.Normalize();
  return p;
}
}  // namespace

bool Frustum::AddPlane(const Plane& p) {
  if (count_ >= kMaxPlanes)
    return false;
  planes_[count_++] = p;
  return true;
}

void Frustum::Transform(const Mat4& M) {
  for (std::size_t i = 0; i < count_; ++i)
    planes_[i] = TransformPlaneInvTranspose(planes_[i], M);
}

bool Frustum::IsPointVisible(const Vec3& p) const {
  for (std::size_t i = 0; i < count_; ++i)
    if (planes_[i].DistanceTo(p) < 0.0f)
      return false;
  return true;
}

bool Frustum::IsSphereVisible(const Vec3& center, float radius) const {
  for (std::size_t i = 0; i < count_; ++i)
    if (planes_[i].DistanceTo(center) < -radius)
      return false;
  return true;
}

bool Frustum::IsAabbVisible(const Vec3& aabb_min, const Vec3& aabb_max) const {
  for (std::size_t i = 0; i < count_; ++i) {
    const Vec3 n   = planes_[i].normal();
    const float px = (n.x >= 0.0f) ? aabb_max.x : aabb_min.x;
    const float py = (n.y >= 0.0f) ? aabb_max.y : aabb_min.y;
    const float pz = (n.z >= 0.0f) ? aabb_max.z : aabb_min.z;
    if (planes_[i].DistanceTo({px, py, pz}) < 0.0f)
      return false;
  }
  return true;
}

// plane' = (M^{-T}) * plane, treated as 4-vector [nx, ny, nz, d]^T (column).
Plane Frustum::TransformPlaneInvTranspose(const Plane& p, const Mat4& M) {
  const Mat4 Minv  = Mat4::Inverse(M);
  const Mat4 MinvT = Mat4::Transpose(Minv);

  const Vec3 n_in  = p.normal();
  const float d_in = p.d();

  const float nx = MinvT.get(0, 0) * n_in.x + MinvT.get(1, 0) * n_in.y +
                   MinvT.get(2, 0) * n_in.z + MinvT.get(3, 0) * d_in;
  const float ny = MinvT.get(0, 1) * n_in.x + MinvT.get(1, 1) * n_in.y +
                   MinvT.get(2, 1) * n_in.z + MinvT.get(3, 1) * d_in;
  const float nz = MinvT.get(0, 2) * n_in.x + MinvT.get(1, 2) * n_in.y +
                   MinvT.get(2, 2) * n_in.z + MinvT.get(3, 2) * d_in;
  const float nd = MinvT.get(0, 3) * n_in.x + MinvT.get(1, 3) * n_in.y +
                   MinvT.get(2, 3) * n_in.z + MinvT.get(3, 3) * d_in;

  Plane out;
  out.set_normal({nx, ny, nz});
  out.set_d(nd);
  out.Normalize();  // keep distances meaningful
  return out;
}

/*static*/ void Frustum::ExtractPlanes(const Mat4& clip_from_world,
                                       Plane out6[6]) {
  // Using the standard extraction for column-major, column-vector math:
  // planes are formed from linear combos of the matrix rows:
  // L: r3 + r0, R: r3 - r0, B: r3 + r1, T: r3 - r1, N: r3 + r2, F: r3 - r2
  const Vec4 r0 = Row4(clip_from_world, 0);
  const Vec4 r1 = Row4(clip_from_world, 1);
  const Vec4 r2 = Row4(clip_from_world, 2);
  const Vec4 r3 = Row4(clip_from_world, 3);

  // Left  : r3 + r0
  out6[0] =
      MakeNormalizedPlane(r3.x + r0.x, r3.y + r0.y, r3.z + r0.z, r3.w + r0.w);

  // Right : r3 - r0
  out6[1] =
      MakeNormalizedPlane(r3.x - r0.x, r3.y - r0.y, r3.z - r0.z, r3.w - r0.w);

  // Bottom: r3 + r1
  out6[2] =
      MakeNormalizedPlane(r3.x + r1.x, r3.y + r1.y, r3.z + r1.z, r3.w + r1.w);

  // Top   : r3 - r1
  out6[3] =
      MakeNormalizedPlane(r3.x - r1.x, r3.y - r1.y, r3.z - r1.z, r3.w - r1.w);

  // Near  : r3 + r2   (OpenGL clip z ∈ [-1, +1])
  out6[4] =
      MakeNormalizedPlane(r3.x + r2.x, r3.y + r2.y, r3.z + r2.z, r3.w + r2.w);

  // Far   : r3 - r2
  out6[5] =
      MakeNormalizedPlane(r3.x - r2.x, r3.y - r2.y, r3.z - r2.z, r3.w - r2.w);
}

/*static*/ Frustum Frustum::FromClipMatrix(const Mat4& clip_from_world) {
  Plane planes[6];
  ExtractPlanes(clip_from_world, planes);

  Frustum f;
  f.SetNumPlanes(6);
  for (int i = 0; i < 6; ++i)
    f[i] = planes[i];

  return f;
}

/*static*/ Frustum Frustum::FromViewProj(const Mat4& proj, const Mat4& view) {
  // clip_from_world = proj * view  (column-vectors)
  Mat4 clip = proj * view;

  return FromClipMatrix(clip);
}

/*static*/ Frustum Frustum::FromCameraPerspectiveRH(float fovy, float aspect,
                                                    float z_near, float z_far,
                                                    const Mat4& view) {
  Mat4 proj = Mat4::PerspectiveRH(fovy, aspect, z_near, z_far);

  return FromViewProj(proj, view);
}

}  // namespace navary::math

// navary/math/obb.cc

#include "navary/math/obb.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace navary::math {

Obb::Obb(const Vec3& center, const Vec3& u, const Vec3& v, const Vec3& w,
         const Vec3& half_sizes)
    : center_(center), half_(half_sizes) {
  basis_[0] = u;
  basis_[1] = v;
  basis_[2] = w;
}

Obb Obb::FromAabb(const Aabb& aabb) {
  Obb o;
  if (aabb.IsEmpty()) {
    o.center_   = {0, 0, 0};
    o.half_     = {0, 0, 0};
    o.basis_[0] = {1, 0, 0};
    o.basis_[1] = {0, 1, 0};
    o.basis_[2] = {0, 0, 1};
    return o;
  }

  o.center_   = aabb.Center();
  o.half_     = aabb.Extents();
  o.basis_[0] = {1, 0, 0};
  o.basis_[1] = {0, 1, 0};
  o.basis_[2] = {0, 0, 1};

  return o;
}

Obb Obb::Transformed(const Mat4& M) const {
  Obb o;
  o.center_ = M.TransformPoint(center_);

  Vec3 u = M.TransformVector(basis_[0]);
  Vec3 v = M.TransformVector(basis_[1]);
  Vec3 w = M.TransformVector(basis_[2]);

  const auto len = [](const Vec3& a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
  };

  const float lu = len(u);
  const float lv = len(v);
  const float lw = len(w);

  o.half_ = {half_.x * lu, half_.y * lv, half_.z * lw};

  if (lu > 0.0f) {
    u.x /= lu;
    u.y /= lu;
    u.z /= lu;
  } else {
    u = {1, 0, 0};
  }

  if (lv > 0.0f) {
    v.x /= lv;
    v.y /= lv;
    v.z /= lv;
  } else {
    v = {0, 1, 0};
  }

  if (lw > 0.0f) {
    w.x /= lw;
    w.y /= lw;
    w.z /= lw;
  } else {
    w = {0, 0, 1};
  }

  o.basis_[0] = u;
  o.basis_[1] = v;
  o.basis_[2] = w;

  return o;
}

bool Obb::RayIntersect(const Vec3& origin, const Vec3& dir, float* tmin_out,
                       float* tmax_out) const {
  const Vec3 p{center_.x - origin.x, center_.y - origin.y,
               center_.z - origin.z};

  const float e0 = Dot(basis_[0], p);
  const float e1 = Dot(basis_[1], p);
  const float e2 = Dot(basis_[2], p);

  const float f0 = Dot(basis_[0], dir);
  const float f1 = Dot(basis_[1], dir);
  const float f2 = Dot(basis_[2], dir);

  float tmin = -std::numeric_limits<float>::infinity();
  float tmax = std::numeric_limits<float>::infinity();

  const auto update = [&](float e, float f, float h) -> bool {
    if (std::fabs(f) < 1e-10f) {
      return std::fabs(e) <= h;  // parallel; must lie within slab
    }

    const float invf = 1.0f / f;
    float t1         = (e + h) * invf;
    float t2         = (e - h) * invf;

    if (t1 > t2)
      std::swap(t1, t2);

    if (t1 > tmin)
      tmin = t1;

    if (t2 < tmax)
      tmax = t2;

    if (tmin > tmax || tmax < 0.0f)
      return false;

    return true;
  };

  if (!update(e0, f0, half_.x))
    return false;

  if (!update(e1, f1, half_.y))
    return false;

  if (!update(e2, f2, half_.z))
    return false;

  if (tmin_out)
    *tmin_out = tmin;

  if (tmax_out)
    *tmax_out = tmax;

  return true;
}

/*static*/ Obb Obb::FromCenterExtents(const Vec3& center,
                                      const Vec3& half_sizes) {
  return Obb(center, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, half_sizes);
}

/*static*/ Aabb Obb::ToAabb() const {
  // compute all 8 corners and extend
  const Vec3 axes[3] = {basis_[0] * half_.x, basis_[1] * half_.y,
                        basis_[2] * half_.z};
  Aabb box;
  box.SetEmpty();

  for (int sx = -1; sx <= 1; sx += 2)
    for (int sy = -1; sy <= 1; sy += 2)
      for (int sz = -1; sz <= 1; sz += 2) {
        Vec3 corner = center_ + axes[0] * sx + axes[1] * sy + axes[2] * sz;
        box.Extend(corner);
      }

  return box;
}

}  // namespace navary::math

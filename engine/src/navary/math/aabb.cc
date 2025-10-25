// navary/math/aabb.cc

#include "navary/math/aabb.h"

#include <algorithm>

namespace navary::math {

Aabb::Aabb() {
  SetEmpty();
}

Aabb::Aabb(const Vec3& mn, const Vec3& mx) : min_(mn), max_(mx) {}

void Aabb::SetEmpty() {
  const float inf = std::numeric_limits<float>::infinity();
  min_            = {+inf, +inf, +inf};
  max_            = {-inf, -inf, -inf};
}

bool Aabb::IsEmpty() const {
  return (min_.x > max_.x) || (min_.y > max_.y) || (min_.z > max_.z);
}

void Aabb::Extend(const Vec3& p) {
  min_.x = std::min(min_.x, p.x);
  min_.y = std::min(min_.y, p.y);
  min_.z = std::min(min_.z, p.z);
  max_.x = std::max(max_.x, p.x);
  max_.y = std::max(max_.y, p.y);
  max_.z = std::max(max_.z, p.z);
}

void Aabb::Extend(const Aabb& b) {
  Extend(b.min_);
  Extend(b.max_);
}

Vec3 Aabb::Center() const {
  return {(min_.x + max_.x) * 0.5f, (min_.y + max_.y) * 0.5f,
          (min_.z + max_.z) * 0.5f};
}

Vec3 Aabb::Extents() const {
  return {(max_.x - min_.x) * 0.5f, (max_.y - min_.y) * 0.5f,
          (max_.z - min_.z) * 0.5f};
}

float Aabb::Volume() const {
  if (IsEmpty())
    return 0.0f;
  const float sx = std::max(0.0f, max_.x - min_.x);
  const float sy = std::max(0.0f, max_.y - min_.y);
  const float sz = std::max(0.0f, max_.z - min_.z);

  return sx * sy * sz;
}

Aabb Aabb::Transformed(const Mat4& M) const {
  // Column-major & column-vectors: row r, col c is get(c, r).
  Aabb out;
  out.SetEmpty();

  // Per-axis accumulate translation + min/max contributions from each column.
  for (int row = 0; row < 3; ++row) {
    float t    = M.get(3, row);  // translation component
    float rmin = t, rmax = t;

    // X column
    {
      const float m = M.get(0, row);
      const float a = m * min_.x;
      const float b = m * max_.x;

      if (a < b) {
        rmin += a;
        rmax += b;
      } else {
        rmin += b;
        rmax += a;
      }
    }

    // Y column
    {
      const float m = M.get(1, row);
      const float a = m * min_.y;
      const float b = m * max_.y;
      if (a < b) {
        rmin += a;
        rmax += b;
      } else {
        rmin += b;
        rmax += a;
      }
    }

    // Z column
    {
      const float m = M.get(2, row);
      const float a = m * min_.z;
      const float b = m * max_.z;

      if (a < b) {
        rmin += a;
        rmax += b;
      } else {
        rmin += b;
        rmax += a;
      }
    }

    if (row == 0) {
      out.min_.x = rmin;
      out.max_.x = rmax;
    }

    if (row == 1) {
      out.min_.y = rmin;
      out.max_.y = rmax;
    }

    if (row == 2) {
      out.min_.z = rmin;
      out.max_.z = rmax;
    }
  }

  return out;
}

/*static*/ bool Aabb::ContainsPoint(const Vec3& p) const {
  return (p.x >= min_.x && p.x <= max_.x) && (p.y >= min_.y && p.y <= max_.y) &&
         (p.z >= min_.z && p.z <= max_.z);
}

/*static*/ Aabb Aabb::FromPoints(const Vec3* pts, std::size_t count) {
  Aabb box;
  box.SetEmpty();
  for (std::size_t i = 0; i < count; ++i)
    box.Extend(pts[i]);
  return box;
}

/*static*/ Aabb Aabb::FromTransformed(const Aabb& box, const Mat4& transform) {
  return box.Transformed(transform);
}

/*static*/ Aabb Aabb::Union(const Aabb& a, const Aabb& b) {
  Aabb r = a;
  r.Extend(b);
  return r;
}

/*static*/ Aabb Aabb::Intersect(const Aabb& a, const Aabb& b) {
  const Vec3 mn{std::max(a.min().x, b.min().x), std::max(a.min().y, b.min().y),
                std::max(a.min().z, b.min().z)};

  const Vec3 mx{std::min(a.max().x, b.max().x), std::min(a.max().y, b.max().y),
                std::min(a.max().z, b.max().z)};

  if (mn.x <= mx.x && mn.y <= mx.y && mn.z <= mx.z) {
    return Aabb(mn, mx);
  }
  
  // Disjoint ⇒ return canonical empty
  Aabb empty;
  empty.SetEmpty();
  return empty;
}

}  // namespace navary::math

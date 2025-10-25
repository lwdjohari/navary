// navary/math/rect.h

#include "navary/math/rect.h"
#include <algorithm>
#include <cstdio>

namespace navary::math {

// --- Constructors ---
Rect::Rect() = default;

Rect::Rect(float l, float t, float r, float b)
    : left(l), top(t), right(r), bottom(b) {}

Rect::Rect(const Vec2& pos)
    : left(pos.x), top(pos.y), right(pos.x), bottom(pos.y) {}

Rect::Rect(const Size2& sz) : left(0.f), top(0.f), right(sz.w), bottom(sz.h) {}

Rect::Rect(const Vec2& upper_left, const Vec2& bottom_right)
    : left(upper_left.x),
      top(upper_left.y),
      right(bottom_right.x),
      bottom(bottom_right.y) {}

Rect::Rect(const Vec2& pos, const Size2& sz)
    : left(pos.x), top(pos.y), right(pos.x + sz.w), bottom(pos.y + sz.h) {}

Rect Rect::FromLTRB(float l, float t, float r, float b) {
  return {l, t, r, b};
}

Rect Rect::FromPosSize(const Vec2& pos, const Size2& sz) {
  return {pos, sz};
}

// --- Geometry queries ---
float Rect::width() const {
  return right - left;
}

float Rect::height() const {
  return bottom - top;
}

bool Rect::empty() const {
  return width() <= 0.f || height() <= 0.f;
}

Size2 Rect::size() const {
  return {width(), height()};
}

Vec2 Rect::top_left() const {
  return {left, top};
}

Vec2 Rect::top_right() const {
  return {right, top};
}

Vec2 Rect::bottom_left() const {
  return {left, bottom};
}

Vec2 Rect::bottom_right() const {
  return {right, bottom};
}

Vec2 Rect::center() const {
  return {(left + right) * 0.5f, (top + bottom) * 0.5f};
}

bool Rect::contains(float x, float y) const {
  return x >= left && x <= right && y >= top && y <= bottom;
}

bool Rect::contains(const Vec2& p) const {
  return contains(p.x, p.y);
}

bool Rect::contains(const Rect& r) const {
  return r.left >= left && r.right <= right && r.top >= top &&
         r.bottom <= bottom;
}

bool Rect::intersects(const Rect& o) const {
  // Empty (zero-width or zero-height) rects do not intersect anything.
  if (empty() || o.empty())
    return false;

  // Require strictly positive overlap on both axes (touching edges ≠
  // intersecting).
  const float l = std::max(left, o.left);
  const float r = std::min(right, o.right);
  const float t = std::max(top, o.top);
  const float b = std::min(bottom, o.bottom);
  return (r > l) && (b > t);
}

Rect Rect::intersection(const Rect& o) const {
  if (!intersects(o))
    return {};
  return {std::max(left, o.left), std::max(top, o.top),
          std::min(right, o.right), std::min(bottom, o.bottom)};
}

Rect Rect::unite(const Rect& o) const {
  return {std::min(left, o.left), std::min(top, o.top),
          std::max(right, o.right), std::max(bottom, o.bottom)};
}

Rect Rect::clamped_to(const Rect& bounds) const {
  return intersection(bounds);
}

// --- Transformations ---
Rect Rect::translated(float dx, float dy) const {
  return {left + dx, top + dy, right + dx, bottom + dy};
}
Rect Rect::translated(const Vec2& d) const {
  return translated(d.x, d.y);
}

Rect Rect::scaled(float sx, float sy) const {
  return {left * sx, top * sy, right * sx, bottom * sy};
}

Rect Rect::inset(float dx, float dy) const {
  return {left + dx, top + dy, right - dx, bottom - dy};
}

Rect Rect::outset(float dx, float dy) const {
  return inset(-dx, -dy);
}

std::array<Vec2, 4> Rect::to_vertices_ccw() const {
  return {top_left(), top_right(), bottom_right(), bottom_left()};
}

// --- Operators ---
bool Rect::operator==(const Rect& r) const {
  return left == r.left && top == r.top && right == r.right &&
         bottom == r.bottom;
}

bool Rect::operator!=(const Rect& r) const {
  return !(*this == r);
}

Rect Rect::operator+() const {
  return *this;
}

Rect Rect::operator-() const {
  return {-right, -bottom, -left, -top};
}

Rect Rect::operator+(const Rect& r) const {
  return {left + r.left, top + r.top, right + r.right, bottom + r.bottom};
}

Rect Rect::operator-(const Rect& r) const {
  return {left - r.left, top - r.top, right - r.right, bottom - r.bottom};
}

Rect& Rect::operator+=(const Rect& r) {
  left += r.left;
  top += r.top;
  right += r.right;
  bottom += r.bottom;
  return *this;
}

Rect& Rect::operator-=(const Rect& r) {
  left -= r.left;
  top -= r.top;
  right -= r.right;
  bottom -= r.bottom;
  return *this;
}

Rect Rect::operator+(const Vec2& v) const {
  return translated(v);
}

Rect Rect::operator-(const Vec2& v) const {
  return translated(-v.x, -v.y);
}

Rect& Rect::operator+=(const Vec2& v) {
  *this = translated(v);
  return *this;
}

Rect& Rect::operator-=(const Vec2& v) {
  *this = translated(-v.x, -v.y);
  return *this;
}

Rect Rect::operator+(const Size2& s) const {
  return {left, top, right + s.w, bottom + s.h};
}

Rect Rect::operator-(const Size2& s) const {
  return {left, top, right - s.w, bottom - s.h};
}

Rect& Rect::operator+=(const Size2& s) {
  right += s.w;
  bottom += s.h;
  return *this;
}

Rect& Rect::operator-=(const Size2& s) {
  right -= s.w;
  bottom -= s.h;
  return *this;
}

Rect Rect::operator+(float k) const {
  return {left + k, top + k, right + k, bottom + k};
}

Rect Rect::operator-(float k) const {
  return {left - k, top - k, right - k, bottom - k};
}

Rect& Rect::operator+=(float k) {
  left += k;
  top += k;
  right += k;
  bottom += k;
  return *this;
}

Rect& Rect::operator-=(float k) {
  left -= k;
  top -= k;
  right -= k;
  bottom -= k;
  return *this;
}

Rect Rect::operator*(float k) const {
  return {left * k, top * k, right * k, bottom * k};
}

Rect Rect::operator/(float k) const {
  return {left / k, top / k, right / k, bottom / k};
}

Rect& Rect::operator*=(float k) {
  left *= k;
  top *= k;
  right *= k;
  bottom *= k;
  return *this;
}

Rect& Rect::operator/=(float k) {
  left /= k;
  top /= k;
  right /= k;
  bottom /= k;
  return *this;
}

void Rect::print() const {
  std::printf("Rect(l=%.2f, t=%.2f, r=%.2f, b=%.2f)\n", left, top, right,
              bottom);
}

}  // namespace navary::math

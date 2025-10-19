#pragma once
#include <array>
#include <cstdint>

#include "navary/math/size2.h"
#include "navary/math/vec2.h"

namespace navary::math {

/**
 * @brief Simple floating-point rectangle class for 2D math and rendering.
 *
 * Pure geometry (no GL/VK deps). Uses Vec2 (class) and Size2 (struct).
 * Superset of classic CRect: contains/intersects, arithmetic with
 * Rect/Vec2/Size2, inset/outset, unite, clamp, scalar ops, and vertices helper.
 */
class Rect {
 public:
  float left{0.f};
  float top{0.f};
  float right{0.f};
  float bottom{0.f};

  // --- Constructors ---
  Rect();
  Rect(float l, float t, float r, float b);
  explicit Rect(const Vec2& pos);  // degenerate: right=left, bottom=top
  explicit Rect(const Size2& sz);  // origin (0,0) + size
  Rect(const Vec2& upper_left, const Vec2& bottom_right);
  Rect(const Vec2& pos, const Size2& sz);

  static Rect FromLTRB(float l, float t, float r, float b);
  static Rect FromPosSize(const Vec2& pos, const Size2& sz);

  // --- Geometry queries ---
  float width() const;
  float height() const;
  bool empty() const;

  Size2 size() const;
  Vec2 top_left() const;
  Vec2 top_right() const;
  Vec2 bottom_left() const;
  Vec2 bottom_right() const;
  Vec2 center() const;

  bool contains(float x, float y) const;
  bool contains(const Vec2& p) const;
  bool contains(const Rect& r) const;

  bool intersects(const Rect& r) const;
  Rect intersection(const Rect& r) const;
  Rect unite(const Rect& r) const;
  Rect clamped_to(const Rect& bounds) const;

  // --- Transformations ---
  Rect translated(float dx, float dy) const;
  Rect translated(const Vec2& d) const;
  Rect scaled(float sx, float sy) const;
  Rect inset(float dx, float dy) const;
  Rect outset(float dx, float dy) const;

  std::array<Vec2, 4> to_vertices_ccw() const;

  // --- Operators ---
  bool operator==(const Rect& r) const;
  bool operator!=(const Rect& r) const;
  Rect operator+() const;
  Rect operator-() const;

  Rect operator+(const Rect& r) const;
  Rect operator-(const Rect& r) const;
  Rect& operator+=(const Rect& r);
  Rect& operator-=(const Rect& r);

  Rect operator+(const Vec2& v) const;
  Rect operator-(const Vec2& v) const;
  Rect& operator+=(const Vec2& v);
  Rect& operator-=(const Vec2& v);

  Rect operator+(const Size2& s) const;
  Rect operator-(const Size2& s) const;
  Rect& operator+=(const Size2& s);
  Rect& operator-=(const Size2& s);

  Rect operator+(float k) const;
  Rect operator-(float k) const;
  Rect& operator+=(float k);
  Rect& operator-=(float k);

  Rect operator*(float k) const;
  Rect operator/(float k) const;
  Rect& operator*=(float k);
  Rect& operator/=(float k);

  void print() const;
};

}  // namespace navary::math

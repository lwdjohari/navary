#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include "navary/math/rect.h"
#include "navary/math/vec2.h"

using namespace navary::math;
using Catch::Matchers::WithinAbs;

TEST_CASE("Rect: AABB collision and overlap region",
          "[rect][collision][game]") {
  Rect player{10, 10, 30, 30};  // 20x20
  Rect wall{25, 15, 45, 35};    // 20x20 overlapping on right

  REQUIRE(player.intersects(wall));
  Rect overlap = player.intersection(wall);
  REQUIRE_THAT(overlap.left, WithinAbs(25.f, 1e-6f));
  REQUIRE_THAT(overlap.top, WithinAbs(15.f, 1e-6f));
  REQUIRE_THAT(overlap.right, WithinAbs(30.f, 1e-6f));
  REQUIRE_THAT(overlap.bottom, WithinAbs(30.f, 1e-6f));

  // resolve by pushing player left by overlap width
  float pushX = overlap.width();
  player      = player.translated(-pushX, 0.f);
  REQUIRE_FALSE(player.intersects(wall));
}

TEST_CASE("Rect: UI layout - padding, margin, anchor", "[rect][ui][game]") {
  Rect window{0, 0, 1920, 1080};
  Size2 padding{16, 16};

  // content area inset by padding
  Rect content = window.inset(padding.w, padding.h);
  REQUIRE_THAT(content.left, WithinAbs(16.f, 1e-6f));
  REQUIRE_THAT(content.top, WithinAbs(16.f, 1e-6f));
  REQUIRE_THAT(content.right, WithinAbs(1920.f - 16.f, 1e-6f));
  REQUIRE_THAT(content.bottom, WithinAbs(1080.f - 16.f, 1e-6f));

  // place a panel at bottom-right with margin
  Size2 panelSize{300, 200};
  Size2 margin{8, 8};
  Rect panel = Rect::FromPosSize(Vec2{content.right - panelSize.w - margin.w,
                                      content.bottom - panelSize.h - margin.h},
                                 panelSize);
  REQUIRE(content.contains(panel));
  REQUIRE_THAT(panel.width(), WithinAbs(300.f, 1e-6f));
  REQUIRE_THAT(panel.height(), WithinAbs(200.f, 1e-6f));
}

TEST_CASE("Rect: camera viewport clamp and parallax layer",
          "[rect][camera][game]") {
  Rect world{0, 0, 5000, 3000};
  Size2 viewSize{1280, 720};

  // center camera around target, then clamp to world
  Vec2 target{2500, 1500};
  Rect cam = Rect::FromPosSize(
      target - Vec2{viewSize.w * 0.5f, viewSize.h * 0.5f}, viewSize);
  cam = cam.clamped_to(world);
  REQUIRE(world.contains(cam));

  // parallax: background layer moves at 50%
  Rect bg = cam.scaled(0.5f, 0.5f);
  REQUIRE_THAT(bg.width(), WithinAbs(cam.width() * 0.5f, 1e-6f));
  REQUIRE_THAT(bg.height(), WithinAbs(cam.height() * 0.5f, 1e-6f));
}

TEST_CASE("Rect: building composite bounds (unite)", "[rect][spatial][game]") {
  Rect r1{0, 0, 10, 10};
  Rect r2{20, 5, 30, 15};
  Rect r3{8, -5, 22, 8};

  Rect bounds = r1.unite(r2).unite(r3);
  REQUIRE(bounds.left == 0.f);
  REQUIRE(bounds.top == -5.f);
  REQUIRE(bounds.right == 30.f);
  REQUIRE(bounds.bottom == 15.f);
}

TEST_CASE("Rect: arithmetic and edge conditions", "[rect][math][edge]") {
  Rect a{0, 0, 100, 50};
  Rect b{10, 10, 20, 20};
  REQUIRE(a.contains(b));
  REQUIRE(a.contains(Vec2{0, 0}));
  REQUIRE_FALSE(a.contains(Vec2{-1, 0}));
  REQUIRE(a + b == Rect{10, 10, 120, 70});
  REQUIRE(a - b == Rect{-10, -10, 80, 30});

  // empty rect behavior
  Rect e{5, 5, 5, 5};
  REQUIRE(e.empty());
  REQUIRE_FALSE(e.intersects(a));
  REQUIRE(e.intersection(a).empty());

  // scalar ops + inset/outset symmetry
  Rect c = a + 5.f;
  Rect d = c - 5.f;
  REQUIRE(d == a);

  Rect g1 = a.inset(2.f, 3.f);
  Rect g2 = g1.outset(2.f, 3.f);
  REQUIRE(g2 == a);
}

TEST_CASE("Rect: vertices CCW ordering", "[rect][render][game]") {
  Rect r{0, 0, 2, 1};
  auto v = r.to_vertices_ccw();
  REQUIRE(v[0] == Vec2{0, 0});
  REQUIRE(v[1] == Vec2{2, 0});
  REQUIRE(v[2] == Vec2{2, 1});
  REQUIRE(v[3] == Vec2{0, 1});
}

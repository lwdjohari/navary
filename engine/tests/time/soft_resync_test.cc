// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/soft_resync_test.cc
// Focus: Drift correction is bounded, gentle, and monotonic.

#include <catch2/catch_test_macros.hpp>
#include "navary/core/time/soft_resync.h"

using namespace navary::core::time;

namespace {
constexpr std::uint64_t ms(std::uint64_t v) {
  return v * 1'000'000ull;
}
}  // namespace

TEST_CASE("SoftResync: gentle bias convergence", "[time][resync]") {
  SoftResync r;
  r.SetTargetWindowSec(2.0);         // correct over ~2s
  r.SetMaxSlewRateNsPerSec(ms(20));  // 20 ms/s max

  // Start with 200ms drift: wall is ahead of mono by 200ms.
  std::uint64_t mono = 0;
  std::uint64_t wall = ms(200);

  // Run 120 frames at ~16ms (simulate ~2s)
  for (int i = 0; i < 120; ++i) {
    r.Update(mono, wall);
    mono += ms(16);
    wall += ms(16);
  }

  // Bias should have reduced magnitude significantly but not overshoot.
  const auto bias = r.bias_ns();
  REQUIRE(bias > 0);
  REQUIRE(static_cast<std::uint64_t>(bias) < ms(200));

  // Adjusted time should be closer to wall than bare monotonic.
  const auto adjusted = r.AdjustedNs(mono);
  const auto err_adjusted =
      (adjusted > wall) ? (adjusted - wall) : (wall - adjusted);
  const auto err_raw = (mono > wall) ? (mono - wall) : (wall - mono);
  REQUIRE(err_adjusted < err_raw);
}

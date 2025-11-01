// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/frame_pacer_mode_test.cc
// Focus: FramePacer pacing modes (unlocked, cpu-paced, vsync-external).

#include <catch2/catch_all.hpp>
#include "navary/core/time/frame_pacer.h"
#include "navary/core/time/time_types.h"

using namespace navary::core::time;

namespace {
constexpr std::uint64_t ms(std::uint64_t v) {
  return v * 1'000'000ull;
}
}  // namespace

TEST_CASE("FramePacer: kUnlocked mode does not pace", "[time][pacer][mode]") {
  FramePacer pacer;
  pacer.SetMode(FramePacer::PacerMode::kUnlocked);
  pacer.SetTargetHz(60.0);

  const std::uint64_t t0 = 1'000'000'000ull;
  pacer.BeginFrame(t0);
  // End at t0 + 5ms (short frame) — in unlocked mode should not sleep.
  auto paced = pacer.EndFrame(t0 + ms(5));
  REQUIRE(paced == 0ull);
}

TEST_CASE("FramePacer: kVsyncExternal observes but does not pace",
          "[time][pacer][mode]") {
  FramePacer pacer;
  pacer.SetMode(FramePacer::PacerMode::kVsyncExternal);
  pacer.SetTargetHz(60.0);

  const std::uint64_t t0 = 2'000'000'000ull;
  pacer.BeginFrame(t0);
  // Even if frame is early, vsync-external should not sleep.
  auto paced = pacer.EndFrame(t0 + ms(3));
  REQUIRE(paced == 0ull);
}

TEST_CASE("FramePacer: kCpuPaced does not sleep if remainder below thresholds",
          "[time][pacer]") {
  FramePacer pacer;
  pacer.SetMode(FramePacer::PacerMode::kCpuPaced);
  pacer.SetTargetHz(60.0);
  // Ensure we avoid real sleep in test: make min_sleep larger than remainder,
  // and disable settle/yield so pacing path returns 0 quickly.
  pacer.SetMinSleepNs(ms(10));
  pacer.SetSettleWindowNs(0);
  pacer.SetYieldEnabled(false);

  const std::uint64_t period = HzToPeriodNs(60.0);  // ~16.666ms
  const std::uint64_t t0     = 3'000'000'000ull;

  // Make the frame almost the whole period so remainder < min_sleep.
  pacer.BeginFrame(t0);
  auto paced = pacer.EndFrame(t0 + (period - ms(1)));  // ~1ms short of deadline
  REQUIRE(paced == 0ull);  // remainder not paced due to thresholds
}

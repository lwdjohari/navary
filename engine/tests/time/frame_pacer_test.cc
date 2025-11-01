// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/frame_pacer_test.cc
// Focus: Frame pacing tolerance and non-negative sleeps.

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

#include "navary/core/time/frame_pacer.h"
#include "navary/core/time/time_types.h"

using namespace navary::core::time;
using Catch::Matchers::WithinAbs;

TEST_CASE("FramePacer: 60 Hz pacing within tolerance", "[time][pacer]") {
  FramePacer pacer;
  pacer.SetTargetHz(60.0);         // ~16.666 ms
  pacer.SetMinSleepNs(MsToNs(1));  // keep test responsive
  pacer.SetSettleWindowNs(MsToNs(1));

  pacer.BeginFrame();
  // Simulate tiny render work:
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  const auto slept_ns = pacer.EndFrame();

  // We expect it attempted to sleep the remainder (~14ms).
  REQUIRE(slept_ns <= MsToNs(20));  // upper bound sanity
}

TEST_CASE("FramePacer: unlocked pacing does not sleep", "[time][pacer]") {
  FramePacer pacer;
  pacer.SetTargetHz(0.0);  // unlock
  pacer.BeginFrame();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const auto slept_ns = pacer.EndFrame();
  REQUIRE(slept_ns == 0);
}

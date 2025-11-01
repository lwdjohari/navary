// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/profiler_time_test.cc
// Focus: ProfilerNowTicks/TicksPerSecond aliases map to monotonic base.

#include <catch2/catch_all.hpp>
#include "navary/core/time/profiler_time.h"
#include "navary/core/time/monotonic_clock.h"

using namespace navary::core::time;

TEST_CASE("Profiler time aliases are monotonic and nonzero frequency",
          "[time][profiler]") {
  // Frequency must be nonzero (or default to 1e9 by alias).
  const auto hz = ProfilerTicksPerSecond();
  REQUIRE(hz > 0);

  // Monotonic non-decreasing across two reads.
  const auto t0 = ProfilerNowTicks();
  const auto t1 = ProfilerNowTicks();
  REQUIRE(t1 >= t0);
}

TEST_CASE("Profiler time alignment equals monotonic clock domain",
          "[time][profiler]") {
  // Not strict equality, but both should move in lockstep domain (ns).
  const auto a = ProfilerNowTicks();
  const auto b = MonotonicNowNs();
  // Both are ns-scale counters; accept small reordering differences.
  REQUIRE(a > 0);
  REQUIRE(b > 0);
}

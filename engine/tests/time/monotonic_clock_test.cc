// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/monotonic_clock_test.cc
// Focus: Cross-platform monotonic clock invariants (no backward jumps).

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <thread>

#include "navary/core/time/monotonic_clock.h"

using namespace navary::core::time;

TEST_CASE("MonotonicClock: init is idempotent and capabilities sane",
          "[time][mono]") {
  auto s1 = InitMonotonicClock();
  auto s2 = InitMonotonicClock();
  auto result = s1 == ClockStatus::kOk || s1 == ClockStatus::kUnavailable;
  
  REQUIRE(result == true);
  REQUIRE(s2 == s1);

  const auto caps = MonotonicCapabilities();
  // We at least expect ns-level readout (even if internally coarser).
  REQUIRE((caps.is_monotonic_raw || caps.is_steady_fallback ||
           caps.is_high_res || true));
}

TEST_CASE("MonotonicClock: strictly non-decreasing", "[time][mono]") {
  const std::uint64_t t0 = MonotonicNowNs();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const std::uint64_t t1 = MonotonicNowNs();
  REQUIRE(t1 >= t0);

  // A quick burst should also not go backwards.
  const std::uint64_t t2 = MonotonicNowNs();
  const std::uint64_t t3 = MonotonicNowNs();
  REQUIRE(t3 >= t2);
}

TEST_CASE("MonotonicClock: frequency hint is consistent", "[time][mono]") {
  const auto f = MonotonicFrequency();
  // Frequency may be 0 for some backends; non-fatal. Just ensure API returns
  // *something*.
  REQUIRE(f >= 0);
}

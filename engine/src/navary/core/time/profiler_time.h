#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/profiler_time.h
// Purpose: Profiler-facing time aliases mapped to monotonic base.
// Policy: C++20, Google style, no exceptions, no RTTI, header-only.
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>
#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

// Returns profiler timestamp in ticks (nanoseconds), from the same source as
// MonotonicNowNs() to guarantee alignment with engine timing.
inline std::uint64_t ProfilerNowTicks() {
  return MonotonicNowNs();
}

// Returns ticks-per-second for profiler timestamps. Falls back to 1e9 if the
// backend does not report a specific frequency (e.g., clock_gettime).
inline std::uint64_t ProfilerTicksPerSecond() {
  const std::uint64_t f = MonotonicFrequency();
  return (f != 0) ? f : 1'000'000'000ull;
}

}  // namespace navary::core::time

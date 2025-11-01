#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/soft_resync.h
// Purpose: Gradual monotonic-to-wall clock alignment for long-running
// processes. Policy: C++20, Google style, no exceptions, no RTTI.
//
// Design Summary:
//  - Used only for long-lived servers, replays, or distributed simulations.
//  - Prevents large monotonic drift over days of uptime by gently adjusting
//    an offset ("epoch bias") over time, never jumping abruptly.
//  - Maintains deterministic tick flow: resync slope applied between frames.
//
//  Example behavior:
//   - If wall clock and monotonic diverge by +200 ms after 3 hours uptime,
//     resync layer slowly nudges offset back to zero over N seconds
//     (default 10 s window), ensuring player time remains consistent.
//
//  Math:
//     adjusted_time = monotonic_time + bias
//     bias' = bias - (bias / window) * dt
//
//  Safety:
//   - The correction never exceeds a defined slew rate (default 10 ms/s).
//   - No wall-time dependency inside hot path; monotonic base always leads.
//
// Usage:
//   SoftResync resync;
//   resync.SetTargetWindowSec(10.0);
//   resync.Update(MonotonicNowNs(), WallNowNs());
//   uint64_t adjusted = resync.AdjustedNs(MonotonicNowNs());
//
//   // The adjusted time stays close to wall-time without breaking determinism.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>
#include <cmath>
#include "navary/core/time/time_types.h"
#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

class SoftResync {
 public:
  SoftResync() = default;

  // Target correction window in seconds.
  // Default: 10 seconds (slow, imperceptible drift fix).
  void SetTargetWindowSec(double seconds);

  // Maximum slew rate (in nanoseconds per second).
  // Default: 10 ms/s (conservative).
  void SetMaxSlewRateNsPerSec(std::uint64_t ns_per_sec);

  // Reset the internal bias to 0.
  void Reset();

  // Perform a correction step using the current wall and monotonic times.
  // wall_ns:  system_clock-derived timestamp (converted to ns)
  // mono_ns:  monotonic timestamp (MonotonicNowNs)
  // Returns current bias_ns_ after update.
  std::int64_t Update(std::uint64_t mono_ns, std::uint64_t wall_ns);

  // Return bias-adjusted time from monotonic timestamp.
  std::uint64_t AdjustedNs(std::uint64_t mono_ns) const;

  // Accessors.
  std::int64_t bias_ns() const {
    return bias_ns_;
  }

  std::uint64_t target_window_ns() const {
    return target_window_ns_;
  }

  std::uint64_t max_slew_rate_ns_per_sec() const {
    return max_slew_rate_ns_per_sec_;
  }

 private:
  std::uint64_t target_window_ns_ = SecondsToNs(10.0);  // default window: 10 s
  std::uint64_t fixed_step_ns_    = MsToNs(16);  // assume ~60 Hz frame cadence
  std::uint64_t max_slew_rate_ns_per_sec_ = MsToNs(10);  // 10 ms/s
  std::int64_t bias_ns_                   = 0;  // signed correction bias
};

}  // namespace navary::core::time

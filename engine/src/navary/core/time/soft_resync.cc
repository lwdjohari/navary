// Navary Engine - Timing Subsystem
// File: navary/core/time/src/soft_resync.cc
// Purpose: Implementation of SoftResync (monotonic-to-wall drift corrector).
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include "navary/core/time/soft_resync.h"
#include <cmath>

namespace navary::core::time {


void SoftResync::SetTargetWindowSec(double seconds) {
  target_window_ns_ = SecondsToNs(seconds);
}

void SoftResync::SetMaxSlewRateNsPerSec(std::uint64_t ns_per_sec) {
  max_slew_rate_ns_per_sec_ = ns_per_sec;
}

void SoftResync::Reset() {
  bias_ns_ = 0;
}

std::int64_t SoftResync::Update(std::uint64_t mono_ns, std::uint64_t wall_ns) {
  if (target_window_ns_ == 0){
    return bias_ns_;
  }

  const std::int64_t error = static_cast<std::int64_t>(wall_ns) -
                             (static_cast<std::int64_t>(mono_ns) + bias_ns_);
  const std::int64_t dt = static_cast<std::int64_t>(fixed_step_ns_);

  // Compute desired correction step per update.
  const double alpha =
      static_cast<double>(dt) / static_cast<double>(target_window_ns_);
  std::int64_t delta =
      static_cast<std::int64_t>(alpha * static_cast<double>(error));

  // Clamp correction to maximum slew rate.
  const std::int64_t max_delta = static_cast<std::int64_t>(
      max_slew_rate_ns_per_sec_ * (static_cast<double>(dt) / 1e9));

  if (delta > max_delta)
    delta = max_delta;
  if (delta < -max_delta)
    delta = -max_delta;

  bias_ns_ += delta;

  return bias_ns_;
}

std::uint64_t SoftResync::AdjustedNs(std::uint64_t mono_ns) const {
  const std::int64_t adjusted =
      static_cast<std::int64_t>(mono_ns) + static_cast<std::int64_t>(bias_ns_);
  return adjusted > 0 ? static_cast<std::uint64_t>(adjusted) : 0ull;
}

}  // namespace navary::core::time

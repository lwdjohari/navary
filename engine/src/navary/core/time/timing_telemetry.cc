// Navary Engine - Timing Subsystem
// File: navary/core/time/src/timing_telemetry.cc
// Purpose: Implementation for TimingTelemetry (lightweight frame diagnostics).
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include "navary/core/time/timing_telemetry.h"
#include <cstring>

namespace navary::core::time {


TimingTelemetry::Stats TimingTelemetry::Snapshot() const {
  Stats copy{};
  std::memcpy(&copy, &stats_, sizeof(Stats));
  return copy;
}

std::size_t TimingTelemetry::Recent(Event* out, std::size_t count) const {
  if (count == 0 || !out)
    return 0;
  const std::uint64_t total = write_index_.load(std::memory_order_relaxed);
  const std::uint64_t start = (total > count) ? (total - count) : 0ull;
  std::size_t copied        = 0;
  for (std::uint64_t i = start; i < total && copied < count; ++i) {
    const std::size_t idx = i % kBufferSize;
    out[copied++]         = buffer_[idx];
  }
  return copied;
}

void TimingTelemetry::Reset() {
  stats_ = {};
  write_index_.store(0, std::memory_order_relaxed);
  Record(EventType::kReset);
}

}  // namespace navary::core::time

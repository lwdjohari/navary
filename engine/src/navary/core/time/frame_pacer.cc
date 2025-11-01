// Navary Engine - Timing Subsystem
// File: navary/core/time/src/frame_pacer.cc
// Purpose: Implementation of FramePacer (cross-platform FPS limiter).
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include "navary/core/time/frame_pacer.h"

#include <thread>

namespace navary::core::time {

void FramePacer::SleepNs(std::uint64_t ns) {
  if (ns == 0)
    return;
  std::this_thread::sleep_for(DurationNs(ns));
}

std::uint64_t FramePacer::SettleUntil(std::uint64_t deadline_ns,
                                      std::uint64_t budget_ns) {
  const std::uint64_t start = MonotonicNowNs();
  std::uint64_t now         = start;

  // Hint to scheduler to avoid burning a full core while settling.
  while (now < deadline_ns) {
    if ((now - start) >= budget_ns)
      break;
    std::this_thread::yield();
    now = MonotonicNowNs();
  }
  return now - start;
}

}  // namespace navary::core::time

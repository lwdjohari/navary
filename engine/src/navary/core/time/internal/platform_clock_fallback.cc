// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_fallback.cc
// Purpose: Fallback steady_clock backend for unsupported or test environments.
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include "navary/core/time/internal/platform_clock_ifacade.h"

#include "navary/core/time/time_types.h"

#include <chrono>
#include <atomic>

namespace navary::core::time {

namespace {
std::atomic<bool> g_inited{false};
}  // namespace

bool PlatformClockInit(PlatformClockInfo* info) {
  g_inited.store(true, std::memory_order_release);

  if (info) {
    info->frequency_hz  = 1'000'000'000ull;
    info->resolution_ns = 1000;  // typical steady_clock precision (1 µs)
    info->is_raw        = false;
    info->is_fallback   = true;
  }
  return true;
}

bool PlatformMonotonicNowNs(std::uint64_t* out_ns) {
  if (!out_ns)
    return false;
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  *out_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
  return true;
}

bool PlatformClockResetEpoch() {
  // steady_clock cannot reset epoch; use a wrapper in tests instead.
  return false;
}

}  // namespace navary::core::time

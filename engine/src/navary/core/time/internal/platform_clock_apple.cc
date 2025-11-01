// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_apple.cc
// Purpose: macOS / iOS high-resolution monotonic clock backend.
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#if defined(__APPLE__)

#include "navary/core/time/internal/platform_clock_ifacade.h"

#include <mach/mach_time.h>
#include <atomic>

namespace navary::core::time {

namespace {
std::atomic<bool> g_inited{false};
mach_timebase_info_data_t g_timebase{};
}  // namespace

bool PlatformClockInit(PlatformClockInfo* info) {
  if (!g_inited.load(std::memory_order_acquire)) {
    if (mach_timebase_info(&g_timebase) != KERN_SUCCESS ||
        g_timebase.denom == 0) {
      return false;
    }
    g_inited.store(true, std::memory_order_release);
  }

  if (info) {
    info->frequency_hz  = 1'000'000'000ull;  // converted to nanoseconds
    info->resolution_ns = 1;                 // ~1ns nominal precision
    info->is_raw        = true;              // mach clock is monotonic raw
    info->is_fallback   = false;
  }
  return true;
}

bool PlatformMonotonicNowNs(std::uint64_t* out_ns) {
  if (!out_ns)
    return false;
  uint64_t t = mach_absolute_time();
  if (!g_inited.load(std::memory_order_acquire)) {
    PlatformClockInit(nullptr);
  }

  // Convert mach ticks to nanoseconds.
  *out_ns = (t * g_timebase.numer) / g_timebase.denom;
  return true;
}

bool PlatformClockResetEpoch() {
  // Not supported by mach_absolute_time; would break monotonicity.
  return false;
}

}  // namespace navary::core::time

#endif  // __APPLE__

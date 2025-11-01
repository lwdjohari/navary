// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_linux.cc
// Purpose: Linux high-resolution monotonic clock backend.
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#if defined(__linux__) && !defined(__ANDROID__)

#include "navary/core/time/internal/platform_clock_ifacade.h"

#include <time.h>
#include <atomic>

namespace navary::core::time {

namespace {
std::atomic<bool> g_inited{false};
clockid_t g_clock_id = CLOCK_MONOTONIC_RAW;  // prefer RAW to avoid NTP slew
}  // namespace

bool PlatformClockInit(PlatformClockInfo* info) {
  if (!g_inited.load(std::memory_order_acquire)) {
    // Verify that CLOCK_MONOTONIC_RAW is supported; fallback otherwise.
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
      g_clock_id = CLOCK_MONOTONIC;
    }
    g_inited.store(true, std::memory_order_release);
  }

  if (info) {
    info->frequency_hz  = 1'000'000'000ull;  // nominal 1GHz (1ns units)
    info->resolution_ns = 1;                 // nanosecond API
    info->is_raw        = (g_clock_id == CLOCK_MONOTONIC_RAW);
    info->is_fallback   = false;
  }
  return true;
}

bool PlatformMonotonicNowNs(std::uint64_t* out_ns) {
  if (!out_ns)
    return false;
  timespec ts{};
  if (clock_gettime(g_clock_id, &ts) != 0) {
    return false;
  }
  *out_ns = static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ull +
            static_cast<std::uint64_t>(ts.tv_nsec);
  return true;
}

bool PlatformClockResetEpoch() {
  // Not supported; would break monotonicity.
  return false;
}

}  // namespace navary::core::time

#endif  // defined(__linux__) && !defined(__ANDROID__)

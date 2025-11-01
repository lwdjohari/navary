// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_android.cc
// Purpose: Android high-resolution monotonic clock backend (Bionic).
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#if defined(__ANDROID__)

#include "navary/core/time/internal/platform_clock_ifacade.h"

#include <time.h>
#include <atomic>

namespace navary::core::time {

namespace {
std::atomic<bool> g_inited{false};
clockid_t g_clock_id = CLOCK_MONOTONIC_RAW;
}  // namespace

bool PlatformClockInit(PlatformClockInfo* info) {
  if (!g_inited.load(std::memory_order_acquire)) {
    // Some older Android kernels may not support MONOTONIC_RAW.
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
      g_clock_id = CLOCK_MONOTONIC;
    }
    g_inited.store(true, std::memory_order_release);
  }

  if (info) {
    info->frequency_hz  = 1'000'000'000ull;
    info->resolution_ns = 1;
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
  // Android/Bionic does not allow resetting monotonic epoch.
  return false;
}

}  // namespace navary::core::time

#endif  // __ANDROID__

// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_win.cc
// Purpose: Windows high-resolution monotonic clock backend
// (QueryPerformanceCounter). 
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#if defined(_WIN32)

#include "navary/core/time/internal/platform_clock_ifacade.h"

#include <windows.h>

namespace navary::core::time {

namespace {
LARGE_INTEGER g_freq = {0};
bool g_inited        = false;
}  // namespace

bool PlatformClockInit(PlatformClockInfo* info) {
  if (!g_inited) {
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0) {
      return false;
    }
    g_freq   = freq;
    g_inited = true;
  }

  if (info) {
    info->frequency_hz  = static_cast<std::uint64_t>(g_freq.QuadPart);
    info->resolution_ns = static_cast<std::uint32_t>(
        (1'000'000'000ull / static_cast<std::uint64_t>(g_freq.QuadPart)));
    info->is_raw =
        true;  // QPC is effectively raw and monotonic on modern Windows.
    info->is_fallback = false;
  }
  return true;
}

bool PlatformMonotonicNowNs(std::uint64_t* out_ns) {
  if (!g_inited) {
    PlatformClockInit(nullptr);
  }

  LARGE_INTEGER counter;
  if (!QueryPerformanceCounter(&counter)) {
    return false;
  }

  const double seconds = static_cast<double>(counter.QuadPart) /
                         static_cast<double>(g_freq.QuadPart);
  *out_ns = static_cast<std::uint64_t>(seconds * 1'000'000'000.0);
  return true;
}

bool PlatformClockResetEpoch() {
  // QPC cannot be reset without losing monotonicity.
  return false;
}

}  // namespace navary::core::time

#endif  // _WIN32

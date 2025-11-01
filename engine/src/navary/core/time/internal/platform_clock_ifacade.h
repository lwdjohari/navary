#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/src/internal/platform_clock_ifacade.h
// Purpose: Internal per-OS monotonic clock backend interface facade.
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>

namespace navary::core::time {

// Describes the selected platform timer backend and its properties.
struct PlatformClockInfo {
  std::uint64_t frequency_hz =
      0;  // 0 if not applicable (e.g., clock_gettime 1e9)
  std::uint32_t resolution_ns =
      0;                     // nominal smallest step (<=100ns = high-res)
  bool is_raw      = false;  // true if using RAW (Linux/Android)
  bool is_fallback = false;  // true if using std::chrono::steady_clock
};

// Initialize the platform clock backend and fill |info|.
// Returns true on success. Must be safe to call multiple times.
bool PlatformClockInit(PlatformClockInfo* info);

// Read current monotonic time, in nanoseconds since an unspecified epoch.
// Returns true on success. On failure, caller may fallback (e.g.,
// steady_clock).
bool PlatformMonotonicNowNs(std::uint64_t* out_ns);

// Reset the epoch to zero (if supported by the backend). Optional.
// Returns true if supported and performed; false if unsupported.
bool PlatformClockResetEpoch();

}  // namespace navary::core::time

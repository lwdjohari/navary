#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/monotonic_clock.h
// Purpose: Cross-platform, high-resolution monotonic time API.
// Policy: C++20, Google style, no exceptions, no RTTI.
// Notes: Implementation is in per-OS backends; this header exposes a stable
// API.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>
#include <atomic>
#include <mutex>

#include "navary/core/time/time_types.h"
// Internal each implements PlatformMonotonicNowNs/Init/Freq
#include "navary/core/time/internal/platform_clock_ifacade.h"

namespace navary::core::time {

// Lightweight status enum (no exceptions)
enum class ClockStatus : std::uint8_t {
  kOk = 0,
  kUnavailable,         // platform API missing or failed
  kFailedPrecondition,  // Init not called or calibration invalid
};

// Controls whether MonotonicNowNs is allowed to fall back to steady_clock.
enum class ClockPolicy : std::uint8_t {
  kAllowSteadyFallback = 0,  // Robust but non-deterministic
  kStrict,                   // Never fall back; fail early and return 0
};

// Compile-time options are set in config.h; we forward-declare minimal query
// here.
struct ClockCaps {
  bool is_monotonic_raw;    // true if using CLOCK_MONOTONIC_RAW (Linux/Android)
  bool is_high_res;         // ~100 ns or better granularity
  bool is_steady_fallback;  // true if using std::chrono::steady_clock fallback
};

// Initialize platform timer backend.
// Safe to call multiple times; cheap after first success.
// Returns kOk on success. In ClockPolicy::kStrict, this function performs
// a live probe read and RETURNS kUnavailable if the platform backend cannot
// produce a timestamp; initialization is only marked complete on success.
ClockStatus InitMonotonicClock();

// Configure & query fallback policy (default: kAllowSteadyFallback).
void SetMonotonicClockPolicy(ClockPolicy policy);

// Get fallback policy.
ClockPolicy GetMonotonicClockPolicy();

// Returns current monotonic timestamp in **nanoseconds** since an unspecified
// epoch. No-fail, thread-safe, reentrant. On failure paths, falls back to
// steady_clock if enabled.
std::uint64_t MonotonicNowNs();

// Frequency hint in Hz (ticks per second of underlying counter).
// For QPC this is QueryPerformanceFrequency; for mach it is derived from
// timebase; for clock_gettime it's nominally 1e9. Returns 0 if
// unknown/non-applicable.
std::uint64_t MonotonicFrequency();

// Returns capabilities of current backend (informational, not required for
// correctness).
ClockCaps MonotonicCapabilities();

// Resets internal epoch (optional). Does not affect monotonicity; useful for
// tests. Returns kOk if backend supports fast epoch reset; otherwise
// kUnavailable.
ClockStatus ResetMonotonicEpoch();

}  // namespace navary::core::time

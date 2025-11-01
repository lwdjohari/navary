#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/time_types.h
// Purpose: Centralized chrono aliases, units, and small helpers.
// Policy: C++20, Google style, no exceptions, no RTTI, header-only,
// cross-platform.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace navary::core::time {

//------------------------------------------------------------------------------
// Units & constants (integer, constexpr)
//------------------------------------------------------------------------------
inline constexpr std::uint64_t kNanosPerSecond  = 1'000'000'000ull;
inline constexpr std::uint64_t kNanosPerMilli   = 1'000'000ull;
inline constexpr std::uint64_t kNanosPerMicro   = 1'000ull;
inline constexpr std::uint64_t kMicrosPerSecond = 1'000'000ull;
inline constexpr std::uint64_t kMillisPerSecond = 1'000ull;

//------------------------------------------------------------------------------
// Clock aliases (monotonic is provided by MonotonicClock; these are std aliases
// for generic code and tools/tests).
//------------------------------------------------------------------------------
using SteadyClock = std::chrono::steady_clock;  // monotonic by standard
using SystemClock = std::chrono::system_clock;  // wall time (can jump)
using TimePoint   = SteadyClock::time_point;

//------------------------------------------------------------------------------
// Duration aliases (strong, readable)
//------------------------------------------------------------------------------
using DurationNs = std::chrono::nanoseconds;
using DurationUs = std::chrono::microseconds;
using DurationMs = std::chrono::milliseconds;
using DurationSec =
    std::chrono::duration<double>;  // double seconds for human IO

//------------------------------------------------------------------------------
// ID/counter types
//------------------------------------------------------------------------------
using FrameIndex = std::uint64_t;  // monotonically increasing frame/tick index

//------------------------------------------------------------------------------
// Frequency helpers (no exceptions)
//------------------------------------------------------------------------------
constexpr std::uint64_t HzToPeriodNs(double hz) {
  // Defensive: avoid div-by-zero; return 0 to indicate "unbounded / disabled".
  return (hz > 0.0) ? static_cast<std::uint64_t>(kNanosPerSecond / hz) : 0ull;
}

constexpr double PeriodNsToHz(std::uint64_t period_ns) {
  return (period_ns > 0u) ? static_cast<double>(kNanosPerSecond) /
                                static_cast<double>(period_ns)
                          : 0.0;
}

//------------------------------------------------------------------------------
// Narrow conversions for user-facing math (explicit, best-effort).
// These are constexpr, side-effect free, and do not throw.
//------------------------------------------------------------------------------
constexpr double NsToSeconds(std::uint64_t ns) {
  return static_cast<double>(ns) / static_cast<double>(kNanosPerSecond);
}

constexpr std::uint64_t SecondsToNs(double seconds) {
  return (seconds > 0.0) ? static_cast<std::uint64_t>(
                               seconds * static_cast<double>(kNanosPerSecond))
                         : 0ull;
}

constexpr std::uint64_t MsToNs(std::uint64_t ms) {
  return ms * kNanosPerMilli;
}
constexpr std::uint64_t UsToNs(std::uint64_t us) {
  return us * kNanosPerMicro;
}
constexpr std::uint64_t NsToMs(std::uint64_t ns) {
  return ns / kNanosPerMilli;
}
constexpr std::uint64_t NsToUs(std::uint64_t ns) {
  return ns / kNanosPerMicro;
}

//------------------------------------------------------------------------------
// Small clamp utility for durations expressed as ns (no exceptions)
//------------------------------------------------------------------------------
template <typename T>
constexpr T Clamp(const T& v, const T& lo, const T& hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

}  // namespace navary::core::time

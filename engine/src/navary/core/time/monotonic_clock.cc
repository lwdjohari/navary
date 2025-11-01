// Navary Engine - Timing Subsystem
// File: navary/core/time/src/monotonic_clock.cc
// Purpose: Cross-platform dispatch for MonotonicClock backend.
// Policy: C++20, Google style, no exceptions, no RTTI.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

namespace {
// Static state (thread-safe init-once)
std::atomic<bool> g_initialized{false};
std::atomic<ClockStatus> g_status{ClockStatus::kFailedPrecondition};
std::atomic<std::uint64_t> g_freq{0};
ClockCaps g_caps{};
std::mutex g_init_lock;
std::atomic<ClockPolicy> g_policy{ClockPolicy::kAllowSteadyFallback};
}  // namespace

ClockStatus InitMonotonicClock() {
  // Fast path: already initialized successfully.
  if (g_initialized.load(std::memory_order_acquire)) {
    return g_status.load(std::memory_order_acquire);
  }

  // Attempt initialization; only one thread proceeds.
  bool expected = false;
  if (!g_initialized.compare_exchange_strong(expected, true)) {
    // Another thread is (or was) initializing. Return current status.
    return g_status.load(std::memory_order_acquire);
  }

  // We won the race; if we fail, we must roll back g_initialized to false.
  auto rollback_initialized = [&] {
    g_initialized.store(false, std::memory_order_release);
  };

  std::scoped_lock lk(g_init_lock);

  PlatformClockInfo info{};
  if (!PlatformClockInit(&info)) {
    g_status.store(ClockStatus::kUnavailable, std::memory_order_release);
    g_freq.store(0, std::memory_order_release);
    g_caps = {};
    rollback_initialized();
    return g_status.load(std::memory_order_acquire);
  }

  // Record capabilities from the chosen backend.
  g_freq.store(info.frequency_hz, std::memory_order_release);
  g_caps.is_monotonic_raw   = info.is_raw;
  g_caps.is_high_res        = info.resolution_ns <= 100;
  g_caps.is_steady_fallback = info.is_fallback;

  // STRICT POLICY: probe once and fail early if backend can't read.
  if (g_policy.load(std::memory_order_acquire) == ClockPolicy::kStrict) {
    std::uint64_t probe_ns = 0;
    if (!PlatformMonotonicNowNs(&probe_ns)) {
      g_status.store(ClockStatus::kUnavailable, std::memory_order_release);
      g_freq.store(0, std::memory_order_release);
      g_caps = {};
      rollback_initialized();
      return g_status.load(std::memory_order_acquire);
    }
  }

  // Success.
  g_status.store(ClockStatus::kOk, std::memory_order_release);
  // Keep g_initialized = true (set earlier) to prevent re-init.
  return ClockStatus::kOk;
}

void SetMonotonicClockPolicy(ClockPolicy policy) {
  g_policy.store(policy, std::memory_order_release);
}

ClockPolicy GetMonotonicClockPolicy() {
  return g_policy.load(std::memory_order_acquire);
}

std::uint64_t MonotonicNowNs() {
  // Always callable; fallback to steady_clock on failure.
  if (g_status.load(std::memory_order_acquire) ==
      ClockStatus::kFailedPrecondition) {
    InitMonotonicClock();
  }

  std::uint64_t ns = 0;
  if (!PlatformMonotonicNowNs(&ns)) {
    // fallback: steady_clock (guaranteed monotonic)
    const auto now = SteadyClock::now().time_since_epoch();
    ns             = std::chrono::duration_cast<DurationNs>(now).count();
  }
  return ns;
}

std::uint64_t MonotonicFrequency() {
  return g_freq.load(std::memory_order_acquire);
}

ClockCaps MonotonicCapabilities() {
  return g_caps;
}

ClockStatus ResetMonotonicEpoch() {
  if (!PlatformClockResetEpoch()) {
    return ClockStatus::kUnavailable;
  }
  return ClockStatus::kOk;
}

}  // namespace navary::core::time

#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/frame_pacer.h
// Purpose: Cross-platform frame pacing (FPS limiter) with sleep + yield
// strategy. Policy: C++20, Google style, no exceptions, no RTTI,
// cross-platform.
//
// Design notes:
//  - Targets a frame period (Hz -> ns). If the frame finishes early, we pace
//  the remainder.
//  - Uses coarse sleep for the bulk, then optional short yield/spin to settle
//  sub-ms remainder.
//  - Never sleeps negative durations; pacing is strictly best-effort.
//  - No exceptions; public API returns integers (ns) and booleans.
//  - Thread-safe for typical single-render-thread usage (no globals).
//
// Typical usage:
//   FramePacer pacer;
//   pacer.SetTargetHz(60.0);
//   pacer.BeginFrame();
//   ... render work ...
//   const auto slept = pacer.EndFrame();  // ns actually slept/spun to meet
//   target
//
// Implementation considerations:
//  - Mobile devices: target can be adjusted dynamically (battery saver /
//  thermal).
//  - Desktop: pair with swapchain vsync; pacer acts as CPU governor if vsync
//  off.
//  - If GPU pacing already dominates (vsync), pacer will typically do nothing.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>
#include <thread>
#include <utility>

#include "navary/core/time/time_types.h"
#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

class FramePacer {
 public:
  FramePacer() = default;

  // Pacing mode. kUnlocked = no pacing. kCpuPaced = sleep/yield pacing.
  // kVsyncExternal = assume display/GPU paces; we only observe timestamps.
  enum class PacerMode : std::uint8_t {
    kUnlocked = 0,
    kCpuPaced,
    kVsyncExternal
  };

  // Set pacing mode (default: kCpuPaced).
  void SetMode(PacerMode m) {
    mode_ = m;
  }

  // Get pacing mode
  PacerMode mode() const {
    return mode_;
  }

  // Set target refresh rate in Hz. hz <= 0 disables pacing (unlocked).
  void SetTargetHz(double hz) {
    target_period_ns_ = HzToPeriodNs(hz);
  }

  // Optional: minimum duration to hand to std::this_thread::sleep_for.
  // Rationale: many OSes have sleep jitter; sleeping very small amounts may
  // overshoot. Default: 2 ms.
  void SetMinSleepNs(std::uint64_t ns) {
    min_sleep_ns_ = ns;
  }

  // Optional: fine settle window using yield/spin after coarse sleep.
  // Default: 2000 µs (2 ms) total settle window; 0 disables.
  void SetSettleWindowNs(std::uint64_t ns) {
    settle_window_ns_ = ns;
  }

  // Optional: enable or disable a final short busy-yield loop for sub-ms
  // settle. Default: enabled.
  void SetYieldEnabled(bool enabled) {
    yield_enabled_ = enabled;
  }

  // Begin a frame. Records start timestamp (or uses provided now_ns if
  // non-zero).
  void BeginFrame(std::uint64_t now_ns = 0) {
    frame_start_ns_ = (now_ns != 0) ? now_ns : MonotonicNowNs();
  }

  // End a frame, applying pacing as needed.
  // Returns total pacing time (ns) including sleeps and yields/spins performed.
  std::uint64_t EndFrame(std::uint64_t now_ns = 0) {
    // If unlocked or vsync external, do not pace; just record end.
    if (mode_ == PacerMode::kUnlocked || mode_ == PacerMode::kVsyncExternal ||
        target_period_ns_ == 0) {
      // Unlocked; just advance last_end_ and return 0.
      last_end_ns_ = (now_ns != 0) ? now_ns : MonotonicNowNs();
      return 0ull;
    }

    const std::uint64_t end_ns = (now_ns != 0) ? now_ns : MonotonicNowNs();
    const std::uint64_t elapsed =
        (end_ns >= frame_start_ns_) ? (end_ns - frame_start_ns_) : 0ull;

    std::uint64_t paced_ns = 0ull;

    if (elapsed < target_period_ns_) {
      std::uint64_t remaining = target_period_ns_ - elapsed;

      // Coarse sleep for the bulk of the remainder (if larger than
      // min_sleep_ns_).
      if (remaining > min_sleep_ns_) {
        const std::uint64_t sleep_ns =
            remaining -
            (settle_window_ns_ < remaining ? settle_window_ns_ : 0ull);
        if (sleep_ns >= min_sleep_ns_) {
          SleepNs(sleep_ns);
          paced_ns += sleep_ns;
        }
      }

      // Fine settle using short yields/spins up to the settle window.
      if (yield_enabled_ && settle_window_ns_ > 0) {
        paced_ns +=
            SettleUntil(frame_start_ns_ + target_period_ns_, settle_window_ns_);
      }
    }

    last_end_ns_ = MonotonicNowNs();
    return paced_ns;
  }

  // Accessors
  std::uint64_t target_period_ns() const {
    return target_period_ns_;
  }
  std::uint64_t min_sleep_ns() const {
    return min_sleep_ns_;
  }
  std::uint64_t settle_window_ns() const {
    return settle_window_ns_;
  }
  bool yield_enabled() const {
    return yield_enabled_;
  }

 private:
  // Sleep helper (coarse). Best-effort, ignores failures (no exceptions).
  static void SleepNs(std::uint64_t ns);

  // Yield/spin until |deadline_ns| or |budget_ns| consumed, whichever comes
  // first. Returns the observed time spent in this settle loop.
  static std::uint64_t SettleUntil(std::uint64_t deadline_ns,
                                   std::uint64_t budget_ns);

  // State
  PacerMode mode_                 = PacerMode::kCpuPaced;
  std::uint64_t target_period_ns_ = HzToPeriodNs(60.0);  // default: 60 Hz
  std::uint64_t min_sleep_ns_     = MsToNs(2);           // conservative default
  std::uint64_t settle_window_ns_ = MsToNs(2);  // final small settle window
  bool yield_enabled_             = true;

  std::uint64_t frame_start_ns_ = MonotonicNowNs();
  std::uint64_t last_end_ns_    = frame_start_ns_;
};

}  // namespace navary::core::time

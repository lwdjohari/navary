#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/time_utils.h
// Purpose: Common helpers and utility functions for time management.
// Policy: C++20, Google style, no exceptions, no RTTI, header-only.
//
// Provides deterministic, allocation-free utilities for time smoothing,
// FPS computation, and frame budget diagnostics.
// Designed for real-time simulation and rendering systems.
//
// Author:
// Linggawasistha Djohari  [2025–Present]

#include <cstdint>
#include <array>
#include <algorithm>
#include <numeric>
#include <string>
#include <cstdio>

#include "navary/core/time/time_types.h"

namespace navary::core::time {

// Exponential moving average smoother for frame/tick deltas.
// Reduces jitter in displayed delta times or pacing estimations.
class DeltaSmoother {
 public:
  // Constructs smoother with α ∈ [0,1].
  // Lower α = smoother but slower response.
  explicit DeltaSmoother(float smoothing_factor = 0.1f)
      : smoothing_factor_(std::clamp(smoothing_factor, 0.0f, 1.0f)) {}

  // Resets the smoother to a given initial value (ns).
  void Reset(std::uint64_t initial_ns = 0) {
    smoothed_ns_ = static_cast<double>(initial_ns);
  }

  // Updates the EMA with a new sample (ns).
  // Returns the current smoothed value.
  std::uint64_t Update(std::uint64_t new_sample_ns) {
    smoothed_ns_ = (1.0 - smoothing_factor_) * smoothed_ns_ +
                   smoothing_factor_ * static_cast<double>(new_sample_ns);
    return static_cast<std::uint64_t>(smoothed_ns_);
  }

  // Returns current smoothed value (ns).
  std::uint64_t ValueNs() const {
    return static_cast<std::uint64_t>(smoothed_ns_);
  }

 private:
  float smoothing_factor_ = 0.1f;  // smoothing coefficient α
  double smoothed_ns_     = 0.0;   // accumulated average (ns)
};

// Deterministic fixed-size sliding mean over last N samples.
// No heap allocation; used for frame or latency smoothing.
template <std::size_t N>
class SlidingAverage {
 public:
  static_assert(N > 1, "SlidingAverage window must be > 1");

  // Adds a new sample (ns), overwriting oldest if full.
  void AddSample(std::uint64_t value_ns) {
    buffer_[index_] = value_ns;
    index_          = (index_ + 1) % N;
    if (count_ < N)
      ++count_;
  }

  // Returns mean of recorded samples, or 0 if empty.
  std::uint64_t AverageNs() const {
    if (count_ == 0)
      return 0ull;
    const std::uint64_t sum =
        std::accumulate(buffer_.begin(), buffer_.begin() + count_, 0ull);
    return sum / count_;
  }

  // Clears all stored samples.
  void Reset() {
    buffer_.fill(0);
    index_ = 0;
    count_ = 0;
  }

 private:
  std::array<std::uint64_t, N> buffer_{};  // circular buffer
  std::size_t index_ = 0;                  // write position
  std::size_t count_ = 0;                  // valid sample count
};

// Frame budget helper.
// Compares measured frame time against target duration.
struct FrameBudget {
  std::uint64_t target_ns;  // expected frame duration (ns)
  std::uint64_t actual_ns;  // measured frame duration (ns)

  // True if frame completed within budget.
  bool WithinBudget() const {
    return actual_ns <= target_ns;
  }

  // Returns percentage of target used (e.g. 96.2%).
  double PercentUsed() const {
    return (target_ns > 0) ? (static_cast<double>(actual_ns) /
                              static_cast<double>(target_ns)) *
                                 100.0
                           : 0.0;
  }

  // Returns how much frame exceeded its budget (ns).
  std::uint64_t OverrunNs() const {
    return (actual_ns > target_ns) ? (actual_ns - target_ns) : 0ull;
  }
};

// Converts nanoseconds per frame to frames per second (FPS).
inline double NsToFps(std::uint64_t frame_ns) {
  return (frame_ns > 0) ? static_cast<double>(kNanosPerSecond) /
                              static_cast<double>(frame_ns)
                        : 0.0;
}

// Converts frame delta to a formatted string, e.g. "16.667 ms".
inline std::string FormatFrameTimeMs(std::uint64_t frame_ns) {
  char buf[32];
  const double ms = static_cast<double>(frame_ns) / 1'000'000.0;
  std::snprintf(buf, sizeof(buf), "%.3f ms", ms);
  return std::string(buf);
}

// Returns true if frame duration exceeds tolerance factor * target.
// Used to flag stutters or performance spikes.
inline bool IsStutter(std::uint64_t frame_ns, std::uint64_t target_ns,
                      double tolerance_factor = 1.5) {
  return static_cast<double>(frame_ns) >
         static_cast<double>(target_ns) * tolerance_factor;
}

}  // namespace navary::core::time

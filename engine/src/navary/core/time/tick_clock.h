#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/tick_clock.h
// Purpose: Deterministic fixed-timestep accumulator for simulation/physics.
// Policy: C++20, Google style, no exceptions, no RTTI, cross-platform.
//
// Overview:
//   - Fixed tick size (dt) in nanoseconds.
//   - Accumulates wall-time from a monotonic source.
//   - Emits a batch of steps to run this frame + interpolation alpha [0,1).
//   - Guards against spiral-of-death via max_catchup_steps.
//   - Supports time scaling (pause/slow-mo/fast-forward).
//
// Notes:
//   - Uses MonotonicNowNs() by default; can accept externally-supplied
//     timestamps for testing or custom schedulers.
//   - All internal math uses integers (nanoseconds) for determinism.
//   - Floating point appears only in the returned interpolation alpha.
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>    

#include "navary/core/time/time_types.h"
#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

// Explicit sentinel for "use system monotonic clock now".
inline constexpr std::uint64_t kUseSystemNow = ~std::uint64_t{0};

class FixedTickClock {
 public:
  // Constructs with a target fixed tick rate in Hz (e.g., 60.0).
  // If hz <= 0, defaults to 60 Hz.
  explicit FixedTickClock(double hz = 60.0) {
    SetFixedHz(hz > 0.0 ? hz : 60.0);
  }

  // Immutable description of the work to do this frame.
  struct TickBatch {
    std::uint32_t num_steps =
        0;  // Number of fixed dt steps to execute this frame.
    std::uint64_t fixed_dt_ns = 0;     // Size of each step in nanoseconds.
    float alpha               = 0.0f;  // Interp factor in [0,1), for rendering.
    FrameIndex frame_index = 0;  // Monotonic frame counter (completed frames).
    std::uint64_t sim_time_ns =
        0;  // Simulation time as of last completed step.
    std::uint64_t wall_dt_ns =
        0;  // Unclamped wall delta observed this frame (for telemetry).
  };

  // Configure the fixed tick frequency (Hz). Values <= 0 are ignored.
  // Safe to call at runtime; will quantize to nearest integer nanoseconds
  // period.
  void SetFixedHz(double hz) {
    if (hz <= 0.0)
      return;
    fixed_dt_ns_ = HzToPeriodNs(hz);
    if (fixed_dt_ns_ == 0) {
      // Extremely high hz may quantize to 0; guard with minimum of 100 µs.
      fixed_dt_ns_ = 100'000ull;  // 0.1 ms minimum.
    }
  }

  // Limit the number of catch-up steps per frame (anti-spiral).
  // Recommended: 2..8. Default: 4.
  void SetMaxCatchUpSteps(std::uint32_t steps) {
    max_catchup_steps_ = steps;
  }

  // Scale time flow: 1.0 = real-time, 0.0 = paused, >1.0 = fast-forward.
  // Negative values are clamped to 0.
  void SetTimeScale(float s) {
    time_scale_ = (s > 0.0f) ? s : 0.0f;
  }

  // Clamp very large wall-time deltas (e.g., ~window drag, breakpoint,
  // suspend). Default: 50 ms.
  void SetStallClampNs(std::uint64_t clamp_ns) {
    stall_clamp_ns_ = clamp_ns;
  }

  // Reset internal state.
  // If now_ns == kUseSystemNow, uses MonotonicNowNs(); otherwise uses now_ns.
  // Note: 0 is a valid synthetic timestamp for deterministic tests.
  void Reset(std::uint64_t now_ns = kUseSystemNow);

  // Begin a new frame. If now_ns == kUseSystemNow, uses MonotonicNowNs();
  // otherwise uses now_ns. Returns a batch describing how many fixed steps to
  // run and the interpolation alpha.
  TickBatch BeginFrame(std::uint64_t now_ns = kUseSystemNow);

  // Call once after you've processed the batch (simulation + render).
  void EndFrame();

  // {
  //   ++frame_index_;
  // }

  // Accessors.
  std::uint64_t fixed_dt_ns() const {
    return fixed_dt_ns_;
  }

  FrameIndex frame_index() const {
    return frame_index_;
  }

  std::uint64_t sim_time_ns() const {
    return sim_time_ns_;
  }

  std::uint32_t max_catchup_steps() const {
    return max_catchup_steps_;
  }

  float time_scale() const {
    return time_scale_;
  }

  std::uint64_t stall_clamp_ns() const {
    return stall_clamp_ns_;
  }

 private:
  // State
  std::uint64_t prev_ns_     = MonotonicNowNs();
  std::uint64_t accum_ns_    = 0;
  std::uint64_t sim_time_ns_ = 0;
  FrameIndex frame_index_    = 0;

  // Config
  std::uint64_t fixed_dt_ns_       = HzToPeriodNs(60.0);  // default 60 Hz
  std::uint32_t max_catchup_steps_ = 4;                   // anti-spiral
  float time_scale_                = 1.0f;                // 1 = real-time
  std::uint64_t stall_clamp_ns_    = MsToNs(50);          // 50 ms default clamp
};

}  // namespace navary::core::time

// Navary Engine - Timing Subsystem
// File: navary/core/time/src/tick_clock.cc
// Purpose: Implementation for FixedTickClock (deterministic accumulator).
// Policy: C++20, Google style, no exceptions, no RTTI.

#include "navary/core/time/tick_clock.h"

namespace navary::core::time {

void FixedTickClock::Reset(std::uint64_t now_ns) {
  const std::uint64_t now =
      (now_ns == kUseSystemNow) ? MonotonicNowNs() : now_ns;
  prev_ns_     = now;
  accum_ns_    = 0;
  sim_time_ns_ = 0;
  frame_index_ = 0;
}

FixedTickClock::TickBatch FixedTickClock::BeginFrame(std::uint64_t now_ns) {
  const std::uint64_t now =
      (now_ns == kUseSystemNow) ? MonotonicNowNs() : now_ns;
  std::uint64_t wall_dt = (now >= prev_ns_) ? (now - prev_ns_) : 0ull;
  prev_ns_              = now;

  TickBatch b{};
  b.wall_dt_ns  = wall_dt;  // telemetry (raw wall)
  b.fixed_dt_ns = fixed_dt_ns_;
  b.frame_index = frame_index_;

  // 1) Apply time scale to wall time → effective sim delta (deterministic
  // base).
  if (time_scale_ != 1.0f) {
    const double scaled =
        static_cast<double>(wall_dt) * static_cast<double>(time_scale_);
    wall_dt = static_cast<std::uint64_t>(scaled);
  }

  // 2) Accumulate full (scaled) time — never discard.
  accum_ns_ += wall_dt;

  // 3) Convert accumulated time to whole fixed steps.
  const std::uint64_t step  = fixed_dt_ns_;
  const std::uint64_t avail = (step != 0) ? (accum_ns_ / step) : 0ull;

  // 4) Per-frame budget in STEPS:
  //    - from clamp (ns → steps), AFTER scaling so behavior matches slow/fast
  //    modes
  //    - and from max_catchup_steps_ (anti-spiral)
  std::uint32_t clamp_steps =
      (stall_clamp_ns_ == 0 || step == 0)
          ? std::numeric_limits<std::uint32_t>::max()
          : static_cast<std::uint32_t>(stall_clamp_ns_ / step);  // floor
  const std::uint32_t budget =
      std::min<std::uint32_t>(max_catchup_steps_, clamp_steps);

  const std::uint32_t run =
      static_cast<std::uint32_t>(std::min<std::uint64_t>(avail, budget));

  // 5) Consume only what we run; carry the remainder — deterministic across
  // frames.
  const std::uint64_t consume = static_cast<std::uint64_t>(run) * step;
  accum_ns_ -= consume;
  sim_time_ns_ += consume;

  b.num_steps   = run;
  b.sim_time_ns = sim_time_ns_;

  // 6) Alpha in [0,1).
  if (step > 0) {
    b.alpha = static_cast<float>(static_cast<double>(accum_ns_) /
                                 static_cast<double>(step));
    if (b.alpha >= 1.0f)
      b.alpha = std::nextafterf(1.0f, 0.0f);
  } else {
    b.alpha = 0.0f;
  }

  return b;
}

void FixedTickClock::EndFrame() {
  ++frame_index_;
}

}  // namespace navary::core::time

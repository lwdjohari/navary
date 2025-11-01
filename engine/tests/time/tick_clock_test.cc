// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/tick_clock_test.cc
// Focus: FixedTickClock deterministic accumulator and real-world behaviors.
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_floating.hpp>

#include "navary/core/time/tick_clock.h"
#include "navary/core/time/time_types.h"

using namespace navary::core::time;
using Catch::Matchers::WithinAbs;

namespace {

// Synthetic monotonic timeline for deterministic tests.
struct SimClock {
  std::uint64_t now_ns = 0;
  void advance(std::uint64_t ns) {
    now_ns += ns;
  }
};

constexpr std::uint64_t ms(std::uint64_t v) {
  return v * 1'000'000ull;
}

}  // namespace

TEST_CASE("FixedTickClock: exact step emission, stable alpha", "[time][tick]") {
  FixedTickClock clock(60.0);  // 16.666.. ms
  clock.Reset(0);

  SimClock sim;
  // First frame: no time advanced.
  auto b0 = clock.BeginFrame(sim.now_ns);
  REQUIRE(b0.num_steps == 0);
  REQUIRE(b0.alpha == 0.0f);
  clock.EndFrame();

  // Advance by exactly 16ms -> still < dt (16.666ms)
  sim.advance(ms(16));
  auto b1 = clock.BeginFrame(sim.now_ns);
  REQUIRE(b1.num_steps == 0);
  REQUIRE(b1.alpha > 0.0f);
  REQUIRE(b1.alpha < 1.0f);
  clock.EndFrame();

  // Advance by 1ms more => cross fixed_dt, produce 1 step
  sim.advance(ms(1));
  auto b2 = clock.BeginFrame(sim.now_ns);
  REQUIRE(b2.num_steps == 1);
  REQUIRE(b2.fixed_dt_ns == HzToPeriodNs(60.0));
  REQUIRE(b2.sim_time_ns == HzToPeriodNs(60.0));  // 1 step completed
  REQUIRE(b2.alpha >= 0.0f);
  REQUIRE(b2.alpha < 1.0f);
  clock.EndFrame();
}

TEST_CASE("FixedTickClock: catch-up clamp prevents spiral-of-death",
          "[time][tick]") {
  FixedTickClock clock(60.0);
  clock.SetMaxCatchUpSteps(2);     // clamp at 2
  clock.SetStallClampNs(ms(100));  // generous clamp for this test
  clock.Reset(0);

  SimClock sim;
  // Stall ~60ms (should be ~3-4 steps at 60Hz)
  sim.advance(ms(60));
  auto b = clock.BeginFrame(sim.now_ns);
  REQUIRE(b.num_steps == 2);  // clamped
  REQUIRE(b.fixed_dt_ns == HzToPeriodNs(60.0));
  // Two steps of sim time processed:
  REQUIRE(b.sim_time_ns == b.fixed_dt_ns * 2);
  // Alpha is remainder / dt:
  REQUIRE(b.alpha >= 0.0f);
  REQUIRE(b.alpha < 1.0f);
  clock.EndFrame();
}

TEST_CASE("FixedTickClock: pause and slow-motion time-scale", "[time][tick]") {
  FixedTickClock clock(30.0);  // 33.333.. ms
  clock.Reset(0);

  SimClock sim;
  // Pause
  clock.SetMaxCatchUpSteps(4);
  clock.SetStallClampNs(2 * clock.fixed_dt_ns());  // ≈ 100 ms at 30 Hz
  clock.SetTimeScale(0.0f);
  sim.advance(ms(40));  // wall time moved, but paused
  auto p = clock.BeginFrame(sim.now_ns);
  REQUIRE(p.num_steps == 0);  // paused — no steps
  REQUIRE(p.alpha == 0.0f);
  clock.EndFrame();

  // 0.5x slow motion
  clock.SetTimeScale(0.5f);
  sim.advance(ms(40));  // only 20ms effective
  auto s = clock.BeginFrame(sim.now_ns);
  REQUIRE(s.num_steps == 0);  // still < 33.3ms dt
  REQUIRE(s.alpha > 0.0f);
  REQUIRE(s.alpha < 1.0f);
  clock.EndFrame();

  // Fast-forward 2x
  clock.SetTimeScale(2.0f);
  sim.advance(
      ms(40));  // 80ms effective -> > 2 steps possible, but clamp default 4
  auto f = clock.BeginFrame(sim.now_ns);
  REQUIRE(f.num_steps >= 2);
  REQUIRE(f.num_steps <= clock.max_catchup_steps());
  clock.EndFrame();
}

TEST_CASE("FixedTickClock: stall clamp bounds massive pauses", "[time][tick]") {
  FixedTickClock clock(60.0);
  clock.SetStallClampNs(ms(50));  // clamp to 50ms
  clock.Reset(0);

  SimClock sim;
  sim.advance(ms(500));  // simulate a big pause
  auto b = clock.BeginFrame(sim.now_ns);
  // Only clamped amount is considered, so max steps <= 50ms / 16.6ms ~ 3
  REQUIRE(b.num_steps <= 3);
  clock.EndFrame();
}

TEST_CASE("FixedTickClock: networking-tick mental model", "[time][tick][net]") {
  // Real-world: lockstep networking compares frame_index and sim_time.
  FixedTickClock clock(20.0);  // 50ms per tick (e.g., server tick)
  clock.Reset(0);

  SimClock sim;
  // Advance exactly one tick:
  sim.advance(ms(50));
  auto b1 = clock.BeginFrame(sim.now_ns);
  REQUIRE(b1.num_steps == 1);
  REQUIRE(clock.frame_index() == 0);  // frame_index increments at EndFrame()
  REQUIRE(b1.sim_time_ns == HzToPeriodNs(20.0));  // one tick of sim time
  clock.EndFrame();
  REQUIRE(clock.frame_index() == 1);

  // Advance two ticks at once:
  sim.advance(ms(100));

  clock.SetMaxCatchUpSteps(4);
  clock.SetStallClampNs(2 * clock.fixed_dt_ns());

  auto b2 = clock.BeginFrame(sim.now_ns);
  REQUIRE(b2.num_steps == 2);
  clock.EndFrame();
}

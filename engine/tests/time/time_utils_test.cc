// Navary Engine - Timing Subsystem Tests
// File: tests/core/time/timing_telemetry_test.cc
// Focus: Event recording, counters, and ring buffer behavior.

#include <catch2/catch_test_macros.hpp>
#include "navary/core/time/timing_telemetry.h"

using namespace navary::core::time;

TEST_CASE("TimingTelemetry: counters and recent ring", "[time][telemetry]") {
  TimingTelemetry t;
  t.Reset();

  t.RecordDeadlineMiss(20'000'000ull, 16'666'666ull);
  t.RecordCatchUp();
  t.RecordClampDrop();
  t.RecordPacingSleep(2'000'000ull);
  t.RecordPacingYield(500'000ull);

  auto stats = t.Snapshot();
  REQUIRE(stats.total_deadline_miss >= 1);
  REQUIRE(stats.total_catchup == 1);
  REQUIRE(stats.total_clamp_drop == 1);
  REQUIRE(stats.total_pacing_sleep == 1);
  REQUIRE(stats.total_pacing_yield == 1);

  TimingTelemetry::Event recent[4]{};
  const auto copied = t.Recent(recent, 4);
  REQUIRE(copied <= 4);
}

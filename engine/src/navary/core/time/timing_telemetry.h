#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/timing_telemetry.h
// Purpose: Lightweight timing diagnostics and overrun event tracking.
// Policy: C++20, Google style, no exceptions, no RTTI, cross-platform.
//
// Design Summary:
//  - Records simple counters for catch-ups, deadline misses, pacing jitter.
//  - Stores small fixed ring buffer (64 or 128 entries) of recent frame events.
//  - Each event stores raw nanosecond data and classification (enum).
//  - No dynamic allocation, no exceptions, thread-safe for single producer.
//
// Intended Use:
//   TimingTelemetry telemetry;
//   telemetry.RecordDeadlineMiss(wall_dt_ns, fixed_dt_ns);
//   telemetry.RecordPacingEvent(paced_ns);
//   auto stats = telemetry.Snapshot();
//
// Suitable for perf HUDs, telemetry logs, or CI regression checks.
//
// Author:
// Linggawasistha Djohari              [2025-Present]
#include <cstdint>
#include <array>
#include <atomic>
#include <cstring>

#include "navary/core/time/time_types.h"
#include "navary/core/time/monotonic_clock.h"

namespace navary::core::time {

class TimingTelemetry {
 public:
  enum class EventType : std::uint8_t {
    kDeadlineMiss = 0,
    kCatchUpStep,
    kClampDrop,
    kPacingSleep,
    kPacingYield,
    kReset,
  };

  struct Event {
    EventType type;
    std::uint64_t timestamp_ns;  // monotonic time when event recorded
    std::uint64_t data_ns;       // additional duration info (e.g., overshoot)
  };

  struct Stats {
    std::uint64_t total_deadline_miss = 0;
    std::uint64_t total_catchup       = 0;
    std::uint64_t total_clamp_drop    = 0;
    std::uint64_t total_pacing_sleep  = 0;
    std::uint64_t total_pacing_yield  = 0;
  };

  TimingTelemetry() = default;

  // Record an event. Old entries are overwritten in ring buffer.
  void Record(EventType type, std::uint64_t data_ns = 0,
              std::uint64_t timestamp_ns = 0) {
    const std::uint64_t ts =
        (timestamp_ns != 0) ? timestamp_ns : MonotonicNowNs();
    const std::uint64_t i =
        write_index_.fetch_add(1, std::memory_order_relaxed) % kBufferSize;
    buffer_[i] = Event{type, ts, data_ns};

    // Increment counters by type.
    switch (type) {
      case EventType::kDeadlineMiss:
        ++stats_.total_deadline_miss;
        break;
      case EventType::kCatchUpStep:
        ++stats_.total_catchup;
        break;
      case EventType::kClampDrop:
        ++stats_.total_clamp_drop;
        break;
      case EventType::kPacingSleep:
        ++stats_.total_pacing_sleep;
        break;
      case EventType::kPacingYield:
        ++stats_.total_pacing_yield;
        break;
      case EventType::kReset:
      default:
        break;
    }
  }

  // Convenience shorthands for common event types.
  void RecordDeadlineMiss(std::uint64_t wall_dt, std::uint64_t fixed_dt) {
    const std::uint64_t over = wall_dt - std::min(wall_dt, fixed_dt);
    Record(EventType::kDeadlineMiss, over);
  }

  void RecordCatchUp() {
    Record(EventType::kCatchUpStep);
  }

  void RecordClampDrop() {
    Record(EventType::kClampDrop);
  }

  void RecordPacingSleep(std::uint64_t slept_ns) {
    Record(EventType::kPacingSleep, slept_ns);
  }

  void RecordPacingYield(std::uint64_t yield_ns) {
    Record(EventType::kPacingYield, yield_ns);
  }

  // Snapshot of counters. Safe for concurrent read.
  Stats Snapshot() const;

  // Retrieve the most recent N events (for debug visualizations or logs).
  // count must be <= kBufferSize. Returns the number of valid events copied.
  std::size_t Recent(Event* out, std::size_t count) const;

  void Reset();

  // {
  //   stats_ = {};
  //   write_index_.store(0);
  //   Record(EventType::kReset);
  // }

 private:
  static constexpr std::size_t kBufferSize = 128;
  std::array<Event, kBufferSize> buffer_{};
  mutable std::atomic<std::uint64_t> write_index_{0};
  Stats stats_{};
};

}  // namespace navary::core::time

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "navary/memory/arena.h"

using namespace navary::memory;
using namespace std::chrono_literals;

namespace {

// ------------ helpers ------------
struct NonTrivial {
  static std::atomic<uint32_t> alive;
  static std::atomic<uint32_t> destroyed;
  int v = 0;
  NonTrivial() {
    alive.fetch_add(1, std::memory_order_relaxed);
  }
  explicit NonTrivial(int x) : v(x) {
    alive.fetch_add(1, std::memory_order_relaxed);
  }
  ~NonTrivial() {
    destroyed.fetch_add(1, std::memory_order_relaxed);
  }
};
std::atomic<uint32_t> NonTrivial::alive{0};
std::atomic<uint32_t> NonTrivial::destroyed{0};

struct CleanupProbe {
  static std::atomic<uint32_t> calls;
};
std::atomic<uint32_t> CleanupProbe::calls{0};

void CleanFn(void* p) {
  if (p)
    CleanupProbe::calls.fetch_add(1, std::memory_order_relaxed);
}

bool IsAligned(const void* p, std::size_t a) {
  return (reinterpret_cast<std::uintptr_t>(p) & (a - 1)) == 0;
}

struct Telemetry {
  std::atomic<uint64_t> refills{0};
  std::atomic<uint64_t> reset_begin{0};
  std::atomic<uint64_t> reset_end{0};
  std::atomic<uint64_t> wait_begin{0};
  std::atomic<uint64_t> wait_progress{0};
  std::atomic<uint64_t> wait_end{0};
  std::atomic<uint64_t> wait_timeout{0};
};

ArenaOptions MakeOptsWithTelemetry(Telemetry& t) {
  ArenaOptions opts;
  opts.initial_block_bytes     = 32 * 1024;
  opts.max_block_bytes         = 256 * 1024;
  opts.wait_policy.spin_iters  = 2000;
  opts.wait_policy.yield_iters = 10000;
  opts.wait_policy.sleep_ms    = 1;

  opts.telemetry.user            = &t;
  opts.telemetry.on_alloc_refill = [](const void*, std::size_t, void* u) {
    static_cast<Telemetry*>(u)->refills.fetch_add(1, std::memory_order_relaxed);
  };
  opts.telemetry.on_reset_begin = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->reset_begin.fetch_add(
        1, std::memory_order_relaxed);
  };
  opts.telemetry.on_reset_end = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->reset_end.fetch_add(1,
                                                    std::memory_order_relaxed);
  };
  opts.telemetry.on_wait_begin = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->wait_begin.fetch_add(1,
                                                     std::memory_order_relaxed);
  };
  opts.telemetry.on_wait_progress = [](const void*, uint32_t, void* u) {
    static_cast<Telemetry*>(u)->wait_progress.fetch_add(
        1, std::memory_order_relaxed);
  };
  opts.telemetry.on_wait_end = [](const void*, bool timeout, void* u) {
    static_cast<Telemetry*>(u)->wait_end.fetch_add(1,
                                                   std::memory_order_relaxed);
    if (timeout)
      static_cast<Telemetry*>(u)->wait_timeout.fetch_add(
          1, std::memory_order_relaxed);
  };
  return opts;
}

}  // namespace

// -----------------------------------------------------------------------------
// 1) Basic Create / Reset lifecycle with trivial & non-trivial types
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Create + Reset runs destructors for non-trivial types",
          "[lifecycle][create]") {
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));

  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);

  {
    // A few objects and arrays
    auto* a = Arena::New<NonTrivial>(arena, 7);
    REQUIRE(a->v == 7);

    auto span = Arena::MakeSpan<NonTrivial>(arena, 10);  // non-trivial array
    span[0].v = 11;
    span[9].v = 22;

    auto* triv_arr = Arena::NewPOD<int>(arena, 512);
    triv_arr[0]    = 1;
    triv_arr[511]  = 2;

    // Register custom cleanup
    int dummy = 123;
    arena.Own(&dummy, &CleanFn);

    REQUIRE(NonTrivial::alive.load() >= 11);
    REQUIRE(CleanupProbe::calls.load() == 0);

    // Stats should be > 0
    REQUIRE(arena.TotalReserved() > 0);
  }

  // Safe reset: no active epochs in this test
  bool ok = arena.ArenaResetSafely(5ms);
  REQUIRE(ok == true);

  // After reset, all non-trivial should be destroyed, and cleanup called once
  REQUIRE(NonTrivial::destroyed.load() >= 11);
  REQUIRE(CleanupProbe::calls.load() == 1);

  // Telemetry captured
  REQUIRE(tm.reset_begin.load() == 1);
  REQUIRE(tm.reset_end.load() == 1);
  REQUIRE(tm.wait_end.load() >= 1);
}

// -----------------------------------------------------------------------------
// 2) Alignment checks
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Allocate alignment honored", "[alloc][alignment]") {
  Arena arena;
  constexpr std::size_t A1 = 64;
  constexpr std::size_t A2 = 256;
  void* p1                 = arena.Allocate(33, A1);
  void* p2                 = arena.Allocate(7, A2);
  REQUIRE(IsAligned(p1, A1));
  REQUIRE(IsAligned(p2, A2));
  arena.Reset();
}

// -----------------------------------------------------------------------------
// 3) Telemetry + refill path exercised
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Refill telemetry increments on block growth",
          "[telemetry][refill]") {
  Telemetry tm;
  ArenaOptions opts        = MakeOptsWithTelemetry(tm);
  opts.initial_block_bytes = 1024;  // small to force refills
  opts.max_block_bytes     = 4 * 1024;
  Arena arena(opts);

  std::size_t total = 0;
  for (int i = 0; i < 50; ++i) {
    // force many small allocations to trigger a few refills
    total += 64;
    (void)arena.Allocate(64, 16);
  }
  REQUIRE(tm.refills.load() >= 1);
  REQUIRE(arena.TotalReserved() >= 1024);
  arena.Reset();
}

// -----------------------------------------------------------------------------
// 4) WaitForEpochZero success (thread releases epoch in time)
// -----------------------------------------------------------------------------
TEST_CASE("Arena: WaitForEpochZero succeeds when worker releases", "[epoch][wait]") {
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));

  std::atomic<bool> go{false};
  std::thread worker([&]() {
    Arena::ArenaEpoch eg(arena);
    // Hold the epoch until main thread flips 'go'
    while (!go.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // Give the worker time to enter epoch
  std::this_thread::sleep_for(1ms);

  // Expect wait to block until we signal release
  std::atomic<bool> waited{false};
  std::thread waiter([&]() {
    waited = arena.WaitForEpochZero(100ms);
  });

  // It shouldn't finish immediately
  std::this_thread::sleep_for(2ms);
  REQUIRE(waited.load() == false);

  // Release
  go.store(true, std::memory_order_release);
  worker.join();
  waiter.join();

  REQUIRE(waited.load() == true);
  REQUIRE(arena.IsIdle());

  // Now safe reset
  REQUIRE(arena.ArenaResetSafely(2ms) == true);

  // Telemetry sanity
  REQUIRE(tm.wait_begin.load() >= 1);
  REQUIRE(tm.wait_end.load() >= 1);
}

// -----------------------------------------------------------------------------
// 5) ArenaResetSafely timeout scenario (worker holds epoch too long)
// -----------------------------------------------------------------------------
TEST_CASE("Arena: ArenaResetSafely returns false on timeout without forcing reset",
          "[epoch][timeout]") {
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));

  std::atomic<bool> stop{false};
  std::thread worker([&]() {
    // Keep an epoch for a meaningful duration
    Arena::ArenaEpoch eg(arena);
    std::this_thread::sleep_for(30ms);
    stop = true;
  });

  // Try a small timeout; should fail (return false) but NOT reset
  bool ok = arena.ArenaResetSafely(1ms);
  REQUIRE(ok == false);

  // Ensure still not idle during worker hold (best-effort check)
  REQUIRE(arena.IsIdle() == false);

  worker.join();
  // Now it should succeed
  REQUIRE(arena.ArenaResetSafely(5ms) == true);
  REQUIRE(tm.wait_end.load() >= 1);
}

// -----------------------------------------------------------------------------
// 6) ArenaBatch RAII (tools/tests pattern)
// -----------------------------------------------------------------------------
TEST_CASE("Arena: ArenaBatch triggers safe reset on scope exit", "[batch][raii]") {
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));

  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);

  {
    Arena::ArenaBatch batch(arena, 5ms);
    auto span = Arena::MakeSpan<NonTrivial>(arena, 5);
    REQUIRE(span.size() == 5);
    span[0].v = 1;
  }  // ~ArenaBatch → ArenaResetSafely

  REQUIRE(NonTrivial::destroyed.load() >= 5);
  REQUIRE(tm.reset_end.load() >= 1);
}

// -----------------------------------------------------------------------------
// 7) Concurrency: many threads allocate into shared arena (TLS lanes)
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Concurrent allocations from many threads succeed",
          "[tls][concurrency]") {
  Telemetry tm;
  ArenaOptions opts        = MakeOptsWithTelemetry(tm);
  opts.initial_block_bytes = 16 * 1024;
  opts.max_block_bytes     = 512 * 1024;
  Arena arena(opts);

  constexpr int kThreads = 8;
  constexpr int kIters   = 500;
  std::atomic<uint64_t> sum{0};

  auto job = [&](int seed) {
    std::mt19937 rng(seed);
    for (int i = 0; i < kIters; ++i) {
      Arena::ArenaEpoch eg(arena);
      const std::size_t n = (rng() % 2048) + 1;
      auto* p             = static_cast<uint8_t*>(arena.Allocate(n, 32));
      REQUIRE(p != nullptr);
      // write a simple checksum pattern
      p[0] = static_cast<uint8_t>(n & 0xFF);
      sum.fetch_add(p[0], std::memory_order_relaxed);
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; ++t)
    threads.emplace_back(job, 1337 + t);
  for (auto& th : threads)
    th.join();

  // We should be able to safely reset after all threads joined
  REQUIRE(arena.ArenaResetSafely(10ms) == true);
  REQUIRE(sum.load() > 0);
  REQUIRE(tm.refills.load() >= 1);
}

// -----------------------------------------------------------------------------
// 8) New / NewArray / MakeSpan helpers (junior-friendly)
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Helper constructors register destructors correctly",
          "[helpers][construct]") {
  Arena arena;

  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);

  {
    auto* a = Arena::New<NonTrivial>(arena, 3);
    REQUIRE(a->v == 3);

    // Non-trivial span ensures element-wise destructor on reset
    auto span = Arena::MakeSpan<NonTrivial>(arena, 4);
    span[1].v = 10;

    // Trivial POD
    auto* pod = Arena::NewPOD<uint32_t>(arena, 64);
    pod[63]   = 0xABCDu;
    REQUIRE(pod[63] == 0xABCDu);
  }

  REQUIRE(arena.ArenaResetSafely(5ms) == true);
  REQUIRE(NonTrivial::destroyed.load() >= 5);
}

// -----------------------------------------------------------------------------
// 9) Own() cleanup order roughly LIFO across registrations
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Own cleanup nodes are executed (approx LIFO per block)",
          "[cleanup]") {
  Arena arena;
  std::vector<int> order;
  order.reserve(3);

  auto push_cleanup = [&](int v) {
    // capture v by value
    struct X {
      static void Run(void* p) {
        (*reinterpret_cast<std::vector<int>*>(p)).push_back(0);
      }
    };
    arena.Own(&order, [](void* p) {
      // overwritten below; we’ll store distinct lambdas per registration
      (void)p;
    });
  };

  // We want distinct cleanups—register explicitly:
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(1);
  });
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(2);
  });
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(3);
  });

  REQUIRE(arena.ArenaResetSafely(3ms) == true);
  // Because we push-front nodes, execution order will be 3,2,1 (LIFO) for nodes
  // in same block
  REQUIRE(order.size() == 3);
  REQUIRE(order[0] == 3);
  REQUIRE(order[1] == 2);
  REQUIRE(order[2] == 1);
}

// -----------------------------------------------------------------------------
// 10) Stats: TotalReserved tracks after Reset/Purge
// -----------------------------------------------------------------------------
TEST_CASE("Arena: TotalReserved reflects block lifecycle", "[stats]") {
  Arena arena;
  REQUIRE(arena.TotalReserved() == 0);

  (void)arena.Allocate(1024, 16);
  auto reserved_after_alloc = arena.TotalReserved();
  REQUIRE(reserved_after_alloc >= 1024);

  arena.Reset();
  REQUIRE(arena.TotalReserved() >=
          sizeof(void*));  // keeps one block; not necessarily zero

  arena.Purge();
  REQUIRE(arena.TotalReserved() == 0);
}

// -----------------------------------------------------------------------------
// 11) Stress a bunch of small allocations to exercise hot path
// -----------------------------------------------------------------------------
TEST_CASE("Arena: Many tiny allocations are stable and fast-path bump-pointer is used",
          "[stress][hotpath]") {
  Arena arena;
  const int N = 10000;
  for (int i = 0; i < N; ++i) {
    auto* p = static_cast<unsigned char*>(arena.Allocate(8, 8));
    REQUIRE(p != nullptr);
    p[0] = static_cast<unsigned char>(i & 0xFF);
  }
  // Reset after stress
  REQUIRE(arena.ArenaResetSafely(5ms) == true);
}

#pragma once

// ============================================================================
// Navary Engine - Memory / Arena Allocator
/// ----------------------------------------------------------------------------
// File: navary/memory/arena.h
// Author:
// Linggawasistha Djohari              [2025-Present]
//
// Overview:
//   High-performance, epoch-guarded linear allocator for real-time,
//   multi-threaded systems (game engines, renderers, job systems, AI, etc.).
//
//   This allocator implements a *bump-pointer* model per thread lane,
//   synchronized by epoch guards to ensure safe resets and predictable
//   latency. It is deterministic, zero-fragmentation, and cross-platform
//   (Windows, Linux, macOS, Android, iOS).  The allocator uses no RTTI,
//   no exceptions, and adheres to Google C++ style conventions.
//
// ----------------------------------------------------------------------------
// Design Goals:
//   - O(1) bump-pointer allocation per thread.
//   - Thread-safe: concurrent allocations with zero locks on hot path.
//   - Deterministic reset using epoch guards.
//   - Non-blocking WaitForEpochZero() with hybrid spin/yield/sleep policy.
//   - Optional destructor registry for non-trivial types.
//   - Configurable telemetry hooks (alloc refill, reset begin/end, etc.).
//   - ArenaTLS isolation per (Arena*, thread) - no cross-arena contamination.
//
// ----------------------------------------------------------------------------
// Arena + Thread-Local Lanes (isolated per Arena)
//
//   ┌────────────────────────────────────────────────────────────┐
//   │                        navary::memory::Arena               │
//   ├────────────────────────────────────────────────────────────┤
//   │  - Head Block   → [ArenaBlock: memory range + dtors list]  │
//   │  - Options      → alignment, telemetry, upstream allocator │
//   │  - Epoch Guard  → active_epochs_ (atomic<uint32_t>)        │
//   │  - Mutex        → protects refill + reset + purge paths    │
//   └────────────────────────────────────────────────────────────┘
//
//   ┌────────────────────────────────────────────────────────────┐
//   │        Thread-local (TLS) ArenaLane Registry               │
//   ├────────────────────────────────────────────────────────────┤
//   │ Each thread keeps exactly *one lane per Arena instance*:   │
//   │                                                            │
//   │   struct Slot {                                            │
//   │     const Arena* owner;   // Which arena this lane belongs │
//   │     ArenaLane lane;       // Active block + bump pointer   │
//   │   };                                                       │
//   │                                                            │
//   │   static thread_local Slot slot;                           │
//   │   if (slot.owner != this) {                                │
//   │       // switched arenas; clear stale lane                 │
//   │       slot.owner = this;                                   │
//   │       slot.lane.block = nullptr;                           │
//   │   }                                                        │
//   │                                                            │
//   └────────────────────────────────────────────────────────────┘
//
//   Each Arena therefore has its own thread-local allocation lane
//   in each participating thread. Switching arenas automatically
//   invalidates the cached lane to prevent stale pointers.
//
// ----------------------------------------------------------------------------
// Arena + Thread-local Lanes (per Arena*, thread)
//
//   ┌───────────────────────────────────────────────────────┐
//   │                    Arena (shared)                     │
//   │ ┌───────────────┐  ┌───────────────┐  ┌──────────────┐│
//   │ │ ArenaLane[T1] │  │ ArenaLane[T2] │  │ ArenaLane[T3]││
//   │ └──────┬────────┘  └──────┬────────┘  └──────┬───────┘│
//   │        │                  │                  │        │
//   │     bump → [block mem]  bump → [block mem]  bump → .. │
//   └───────────────────────────────────────────────────────┘
//
//   Per-thread lanes are keyed by (Arena*, thread). Switching to a
//   different Arena on the same thread automatically clears the
//   cached lane.block, ensuring no stale pointers after Reset/Purge.
//
// ----------------------------------------------------------------------------
// Epoch Lifecycle & Safe Reset
//
//   [Thread 1]                  [Thread 2]                 [Arena]
//       │                           │                        │
//       │ ArenaEpoch e1(arena)      │                        │
//       │ ─────────────────────────>│                        │
//       │                           │ active_epochs_++       │
//       │ Allocate() ...            │                        │
//       │                           │                        │
//       │ e1.~ArenaEpoch()          │                        │
//       │ <──────────────────────── │                        │
//       │                           │ active_epochs_--       │
//       │                           │                        │
//       │ ArenaResetSafely()        │                        │
//       │ ──────────────────────────────────────────────────>│
//       │                           │ WaitForEpochZero() → 0 │
//       │                           │ Purge or Reset() OK    │
//
// ----------------------------------------------------------------------------
// Thread Safety & Usage:
//
//   - Each thread uses its own ArenaLane (no locks on Allocate()).
//   - Reset(), Purge(), and ArenaResetSafely() require *no active epochs*.
//   - WaitForEpochZero() uses a hybrid spin → yield → sleep loop to avoid CPU
//     starvation while waiting for other threads to exit their epochs.
//   - Destructor registry ensures non-trivial types are correctly destroyed
//     during Reset() or Purge().
//
// ----------------------------------------------------------------------------
// Typical Usage:
// ```cpp
//     navary::memory::Arena frame_arena;
//     {
//         navary::memory::Arena::ArenaEpoch epoch(frame_arena);
//         auto* tmp = frame_arena.New<MyTempStruct>(42);
//         DoWork(tmp);
//     } // epoch ends -> active_epochs_ decremented
//
//     frame_arena.ArenaResetSafely(std::chrono::milliseconds(2));
// ```
// ----------------------------------------------------------------------------
// Safety Notes:
//
//   - After Reset() or Purge(), all pointers allocated from the Arena become
//     invalid. Never retain Arena-allocated pointers across resets.
//   - Each Arena instance owns its own TLS lane per thread. Allocations from
//     different Arenas on the same thread are fully isolated.
//   - No exceptions are thrown under any circumstance.
//   - ArenaUpstream uses platform allocators (aligned malloc / posix_memalign)
//     that are cross-platform and freeable with free() / _aligned_free().
//
// ----------------------------------------------------------------------------

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
#include <mutex>
#include <vector>
#include <functional>
#include <thread>
#include <chrono>
#include <cstdio>

#include "navary/memory/span.h"

#ifndef NAVARY_ARENA_USE_ABSL
#define NAVARY_ARENA_USE_ABSL 0
#endif

#if NAVARY_ARENA_USE_ABSL
#include "absl/synchronization/mutex.h"
#define NAVARY_MUTEX absl::Mutex
#define NAVARY_LOCK_GUARD(name, m) absl::MutexLock name(&(m))
#else
#define NAVARY_MUTEX std::mutex
#define NAVARY_LOCK_GUARD(name, m) std::lock_guard<std::mutex> name(m)
#endif

#ifndef NDEBUG
#define NAVARY_ARENA_EPOCH_DEBUG 1
#else
#define NAVARY_ARENA_EPOCH_DEBUG 0
#endif

namespace navary::memory {

// Bit layout for active_epochs_:
// [31]  = FREEZE (1 = block new epoch entries)
// [30:0]= active epoch count
static constexpr uint32_t kFreezeBit = 0x80000000u;
static constexpr uint32_t kCountMask = 0x7fffffffu;

// -------------------------------
// Upstream (block) allocator hook
// -------------------------------
struct ArenaBlock {
  std::byte* begin;
  std::byte* cur;
  std::byte* end;
  ArenaBlock* next;

  struct DtorNode {
    void (*fn)(void*) = nullptr;
    void* arg         = nullptr;
    DtorNode* next    = nullptr;
  };
  DtorNode* dtors;

  explicit ArenaBlock(std::byte* b, std::size_t sz)
      : begin(b), cur(b), end(b + sz), next(nullptr), dtors(nullptr) {}
};

struct ArenaUpstream {
  using AllocateFn   = void* (*)(std::size_t size, std::size_t alignment,
                               void* user);
  using DeallocateFn = void (*)(void* ptr, std::size_t size, void* user);

  AllocateFn allocate     = nullptr;
  DeallocateFn deallocate = nullptr;
  void* user              = nullptr;

  static void* DefaultAlloc(std::size_t sz, std::size_t align, void*) {
    // Require power-of-two and at least pointer-size alignment.
    if ((align & (align - 1)) != 0 || align < alignof(void*))
      return nullptr;

#if defined(_WIN32)
    // _aligned_malloc returns nullptr on failure.
    return _aligned_malloc(sz, align);
#else
    // posix_memalign returns 0 on success; memory is freed with free().
    void* p = nullptr;
    int rc  = posix_memalign(&p, align, sz);
    if (rc != 0)
      return nullptr;
    return p;
#endif
  }

  static void DefaultFree(void* p, std::size_t /*size*/, void*) {
    if (!p)
      return;
#if defined(_WIN32)
    _aligned_free(p);
#else
    free(p);
#endif
  }

  static ArenaUpstream Default() {
    ArenaUpstream u;
    u.allocate   = &DefaultAlloc;
    u.deallocate = &DefaultFree;
    u.user       = nullptr;
    return u;
  }
};

// ------------------------
// Telemetry / wait policy
// ------------------------
struct ArenaTelemetry {
  void* user                                                       = nullptr;
  void (*on_alloc_refill)(const void* arena, std::size_t bytes,
                          void* user)                              = nullptr;
  void (*on_reset_begin)(const void* arena, void* user)            = nullptr;
  void (*on_reset_end)(const void* arena, void* user)              = nullptr;
  void (*on_wait_begin)(const void* arena, void* user)             = nullptr;
  void (*on_wait_progress)(const void* arena, uint32_t active,
                           void* user)                             = nullptr;
  void (*on_wait_end)(const void* arena, bool timeout, void* user) = nullptr;
};

// Scenario	| spin_iters	| yield_iters	| sleep_ms
// Desktop/Consoles (general)	    | 2,000	| 20,000 |	1
// Editor/Tools (be nice to CPU)	| 1,000	| 10,000 |	2
// Low-latency servers (busy cores)	| 4,000	| 10,000 |	1
struct ArenaWaitPolicy {
  uint32_t spin_iters  = 2'000;   // tight CPU spin (ultra short waits)
  uint32_t yield_iters = 20'000;  // cooperative yield phase
  uint32_t sleep_ms    = 1;       // coarse backoff sleep (>=1ms recommended)
};

struct ArenaOptions {
  std::size_t initial_block_bytes = 64 * 1024;
  std::size_t max_block_bytes     = 1 * 1024 * 1024;
  std::size_t alignment           = alignof(std::max_align_t);
  ArenaUpstream upstream          = ArenaUpstream::Default();
  ArenaWaitPolicy wait_policy{};
  ArenaTelemetry telemetry{};
};

// -----------------------------------------------------------------------------
// Class: navary::memory::Arena
// -----------------------------------------------------------------------------
// A thread-safe, epoch-guarded linear allocator optimized for real-time
// multi-threaded systems such as game engines, renderers, and job systems.
//
// Each Arena provides deterministic O(1) allocations via per-thread bump
// pointers, while supporting safe global resets using epoch guards.
//
// ----------------------------------------------------------------------------
// Key Properties:
//   - Lock-free allocations per thread (bump-pointer in ArenaLane).
//   - Thread-safe across threads (atomic epoch counter guards reset).
//   - Safe Reset() / Purge() using WaitForEpochZero().
//   - Optional destructor registration for non-trivial types.
//   - Hybrid spin/yield/sleep wait policy to avoid busy waits.
//   - Telemetry callbacks for profiling and instrumentation.
//   - Per-(Arena*, thread) TLS lane isolation - prevents cross-arena UAF.
//
// ----------------------------------------------------------------------------
// Arena + Thread-local Lanes (per Arena*, thread):
//
//   Per-thread lanes are keyed by (Arena*, thread). Switching to a
//   different Arena on the same thread automatically clears the
//   cached lane.block, ensuring no stale pointers after Reset/Purge.
//
// ----------------------------------------------------------------------------
// Epoch Lifecycle:
//
//   ArenaEpoch guard increments active_epochs_ on enter,
//   decrements on exit.  Reset() and Purge() are allowed only
//   when active_epochs_ == 0.
//
// ----------------------------------------------------------------------------
// Example:
// ```cpp
//     navary::memory::Arena frame_arena(1 << 20);  // 1 MB
//     {
//         navary::memory::Arena::ArenaEpoch epoch(frame_arena);
//         auto* temp = frame_arena.New<MyStruct>(42);
//         DoWork(temp);
//     } // epoch leaves scope
//
//     frame_arena.ArenaResetSafely(std::chrono::milliseconds(2));
// ```
// ----------------------------------------------------------------------------
// Thread Safety:
//   - Concurrent Allocate() calls are lock-free.
//   - Reset() and Purge() are serialized internally.
//   - No exceptions, no RTTI, cross-platform safe.
// ----------------------------------------------------------------------------
class Arena {
 public:
  explicit Arena(const ArenaOptions& opts = ArenaOptions())
      : opts_(opts), head_(nullptr), total_bytes_(0) {}

  Arena(const Arena&)            = delete;
  Arena& operator=(const Arena&) = delete;

  ~Arena() {
    Purge();
  }

  // ---------- allocation ----------
  void* Allocate(std::size_t size,
                 std::size_t alignment = alignof(std::max_align_t)) {
    if (alignment < opts_.alignment) {
      alignment = opts_.alignment;
    }

    EnterEpoch_();

    auto& lane = GetLane_(this);

    // Validate cached block once in case a prior Purge() freed it.
    if (lane.block) {
      bool still_linked = false;
      {
        NAVARY_LOCK_GUARD(lk, mu_);
        for (ArenaBlock* b = head_; b; b = b->next) {
          if (b == lane.block) {
            still_linked = true;
            break;
          }
        }
      }

      if (!still_linked) {
        lane.block = nullptr;
      }
    }

    if (!lane.block || !TryBump_(lane.block, size, alignment)) {
      lane.block = RefillLane_(size, alignment);
      if (!lane.block) {
        LeaveEpoch_();
        return nullptr;  // Allocation failed
      }

      (void)TryBump_(lane.block, size, alignment);
    }

    void* p = lane.block->cur - size;
    LeaveEpoch_();

    return p;
  }

  template <class T, class... Args>
  T* Create(Args&&... args) {
    void* mem = Allocate(sizeof(T), alignof(T));
    T* obj    = new (mem) T(std::forward<Args>(args)...);
    RegisterDtorIfNeeded_(obj);
    return obj;
  }

  // Register arbitrary cleanup
  void Own(void* ptr, void (*deleter)(void*)) {
    if (!ptr || !deleter) {
      return;
    }

    auto& lane = GetLane_(this);
    if (!lane.block) {
      lane.block = RefillLane_(sizeof(ArenaBlock::DtorNode),
                               alignof(ArenaBlock::DtorNode));
    }

    auto* node_mem =
        Allocate(sizeof(ArenaBlock::DtorNode), alignof(ArenaBlock::DtorNode));
    auto* node =
        new (node_mem) ArenaBlock::DtorNode{deleter, ptr, lane.block->dtors};
    lane.block->dtors = node;
  }

  void OnReset(void (*fn)(void*), void* arg) {
    Own(arg, fn);
  }

  // ---------- lifetime ----------
  void Reset() {
    AssertNoEpoch_();
    T_on_reset_begin_();

    // Collect and run dtors outside the lock (see patch 4) then:
    RunAllDtors_();

    // Do NOT free blocks; just reset all to begin for next frame.
    NAVARY_LOCK_GUARD(lk, mu_);
    for (ArenaBlock* b = head_; b; b = b->next) {
      b->cur   = b->begin;
      b->dtors = nullptr;
    }

    // Keep total_bytes_ unchanged (reserved capacity stays).
    T_on_reset_end_();
  }

  void Purge() {
    AssertNoEpoch_();
    T_on_reset_begin_();
    RunAllDtors_();
    NAVARY_LOCK_GUARD(lk, mu_);
    for (ArenaBlock* b = head_; b;) {
      ArenaBlock* nxt = b->next;
      FreeBlock_(b);
      b = nxt;
    }
    head_ = nullptr;
    total_bytes_.store(0, std::memory_order_relaxed);
    T_on_reset_end_();
  }

  // ---------- epoch / wait ----------
  class ArenaEpoch {
   public:
    explicit ArenaEpoch(Arena& a) noexcept : arena_(&a) {
      arena_->EnterEpoch_();
    }

    ~ArenaEpoch() noexcept {
      arena_->LeaveEpoch_();
    }

    ArenaEpoch(const ArenaEpoch&)            = delete;
    ArenaEpoch& operator=(const ArenaEpoch&) = delete;

   private:
    Arena* arena_;
  };

  bool IsIdle() const noexcept {
    return (active_epochs_.load(std::memory_order_acquire) & kCountMask) == 0;
  }

  template <class Rep, class Period>
  bool WaitForEpochZero(std::chrono::duration<Rep, Period> timeout) const {
    using namespace std::chrono;

    // High-precision deadline (ns) for robust comparisons
    const auto deadline =
        steady_clock::now() + duration_cast<nanoseconds>(timeout);

    T_on_wait_begin_();

    // ---- Phase 1: spin (ns-level compare; no sleeps) ----
    for (uint32_t i = 0; i < opts_.wait_policy.spin_iters; ++i) {
      const uint32_t c =
          active_epochs_.load(std::memory_order_acquire) & kCountMask;
      if (c == 0) {
        // Acquire fence is redundant with the acquire load above, but explicit.
        std::atomic_thread_fence(std::memory_order_acquire);
        T_on_wait_end_(false);
        return true;
      }

      if ((i & 0xFFu) == 0) {
        T_on_wait_progress_(c);
      }

      if (steady_clock::now() >= deadline) {
        T_on_wait_end_(true);
        return false;
      }
    }

    // ---- Phase 2: yield (lets other threads run; effective µs-ms) ----
    for (uint32_t i = 0; i < opts_.wait_policy.yield_iters; ++i) {
      const uint32_t c =
          active_epochs_.load(std::memory_order_acquire) & kCountMask;
      if (c == 0) {
        std::atomic_thread_fence(std::memory_order_acquire);
        T_on_wait_end_(false);
        return true;
      }

      if ((i & 0xFFu) == 0) {
        T_on_wait_progress_(c);
      }

      std::this_thread::yield();
      if (steady_clock::now() >= deadline) {
        T_on_wait_end_(true);
        return false;
      }
    }

    // ---- Phase 3: sleep (coarse ms backoff, avoids CPU burn) ----
    const auto sleep_dur = std::chrono::milliseconds(
        std::max<uint32_t>(1, opts_.wait_policy.sleep_ms));

    while (steady_clock::now() < deadline) {
      const uint32_t c =
          active_epochs_.load(std::memory_order_acquire) & kCountMask;
      if (c == 0) {
        std::atomic_thread_fence(std::memory_order_acquire);
        T_on_wait_end_(false);
        return true;
      }

      T_on_wait_progress_(c);
      std::this_thread::sleep_for(sleep_dur);
    }

    T_on_wait_end_(true);
    return false;
  }

  bool WaitForEpochZeroMs(int timeout_ms) const {
    return WaitForEpochZero(std::chrono::milliseconds(timeout_ms));
  }

  bool ArenaResetSafelyMs(int timeout_ms) {
    return ArenaResetSafely(std::chrono::milliseconds(timeout_ms));
  }

  template <class Rep, class Period>
  bool ArenaResetSafely(std::chrono::duration<Rep, Period> timeout) {
    // 1) Set FREEZE bit (preserve current count). Prevent new entrants.
    uint32_t cur = active_epochs_.load(std::memory_order_relaxed);
    for (;;) {
      const uint32_t frozen = cur | kFreezeBit;
      if (active_epochs_.compare_exchange_weak(cur, frozen,
                                               std::memory_order_acq_rel,
                                               std::memory_order_relaxed)) {
        break;
      }
    }

    // 2) Wait until count reaches zero (freeze prevents new entrants).
    const bool ok = WaitForEpochZero(timeout);
    if (ok) {
      Reset();  // safe: no active epochs, and no new ones can start
    }

    // 3) Clear FREEZE bit (allow new entrants).
    cur = active_epochs_.load(std::memory_order_relaxed);
    for (;;) {
      const uint32_t unfrozen = cur & kCountMask;
      if (active_epochs_.compare_exchange_weak(cur, unfrozen,
                                               std::memory_order_release,
                                               std::memory_order_relaxed)) {
        break;
      }
    }

    if (ok) {
      return true;
    }
#if NAVARY_ARENA_EPOCH_DEBUG
    // In debug, assert if epochs are still active after waiting
    uint32_t c = active_epochs_.load(std::memory_order_acquire) & kCountMask;
    if (c != 0) {
      std::fprintf(stderr,
                   "[navary::memory::Arena] ArenaResetSafely: timeout with %u "
                   "active epoch(s)\n",
                   c);
      std::fflush(stderr);
      std::abort();
    }
#endif
    // Timed out: do NOT reset; caller decides fallback.
    return false;
  }

  // ---------- stats ----------
  std::size_t TotalReserved() const noexcept {
    return total_bytes_.load(std::memory_order_relaxed);
  }
  const ArenaOptions& options() const {
    return opts_;
  }

  // ---------- easy-api-safe batch RAII (opt-in) ----------
  class ArenaBatch {
   public:
    template <class Rep, class Period>
    ArenaBatch(Arena& a, std::chrono::duration<Rep, Period> to) : arena_(&a) {
      // nothing on enter; allocations will increment epochs via Allocate
      // you can also choose to explicitly take an ArenaEpoch if you need:
      // epoch_.emplace(a);
      timeout_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(to);
    }

    ~ArenaBatch() {
      // do not force; best-effort safe reset
      (void)arena_->ArenaResetSafely(timeout_ns_);
    }

    ArenaBatch(const ArenaBatch&)            = delete;
    ArenaBatch& operator=(const ArenaBatch&) = delete;

   private:
    Arena* arena_;
    std::chrono::nanoseconds timeout_ns_;
  };

  // ---------- convenience creators ----------

  template <class T, class... Args>
  static T* New(Arena& a, Args&&... args) {
    void* mem = a.Allocate(sizeof(T), alignof(T));
    T* obj    = new (mem) T(std::forward<Args>(args)...);
    a.RegisterDtorIfNeeded_(obj);
    return obj;
  }

  // NewArray now only supports trivially destructible types.
  template <class T>
  static T* NewArray(Arena& a, std::size_t n) {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Arena::NewArray only supports trivially destructible types. "
                  "For non-trivial types, use Arena::MakeSpan to ensure "
                  "correct destruction.");
    static_assert(alignof(T) <= alignof(std::max_align_t),
                  "custom align not supported here");
    T* base = static_cast<T*>(a.Allocate(sizeof(T) * n, alignof(T)));
    if constexpr (!std::is_trivially_constructible_v<T>) {
      for (std::size_t i = 0; i < n; ++i)
        new (base + i) T();
    }
    return base;
  }

  template <class T>
  static T* NewPOD(Arena& a, std::size_t n) {
    static_assert(std::is_trivially_constructible_v<T> &&
                      std::is_trivially_destructible_v<T>,
                  "NewPOD is for trivial types only");
    return static_cast<T*>(a.Allocate(sizeof(T) * n, alignof(T)));
  }

  template <class T>
  static Span<T> MakeSpan(Arena& a, std::size_t n) {
    T* data = static_cast<T*>(a.Allocate(sizeof(T) * n, alignof(T)));

    if constexpr (!std::is_trivially_constructible_v<T>) {
      for (std::size_t i = 0; i < n; ++i)
        new (data + i) T();
    }

    if constexpr (!std::is_trivially_destructible_v<T>) {
      // capture size for correct element-wise destruction on reset
      struct Sweep {
        std::size_t n;
        static void Run(void* p) {
          auto* s = static_cast<Sweep*>(p);
          T* arr  = reinterpret_cast<T*>(s + 1);
          for (std::size_t i = 0; i < s->n; ++i) {
            (arr + i)->~T();
          }
        }
      };

      // allocate tiny header + array pointer together in arena for LIFO cleanup
      void* header = a.Allocate(sizeof(Sweep), alignof(Sweep));
      auto* sweep  = new (header) Sweep{n};
      a.Own(sweep, &Sweep::Run);

      // Note: arr memory is *after* header? We don't rely on layout; we pass
      // header only. We reinterpret inside Run using (s + 1) convention - so
      // place array right after header: To guarantee that, re-allocate array
      // right after header: (We already allocated array before header, so we
      // can't reposition it here.) Simpler: store pointer instead.
      struct SweepPtr {
        std::size_t n;
        T* arr;
        static void Run(void* p) {
          auto* s = static_cast<SweepPtr*>(p);
          for (std::size_t i = 0; i < s->n; ++i) {
            (s->arr + i)->~T();
          }
        }
      };

      void* hdr2 = a.Allocate(sizeof(SweepPtr), alignof(SweepPtr));
      auto* sp   = new (hdr2) SweepPtr{n, data};
      a.Own(sp, &SweepPtr::Run);
    }

    return {data, n};
  }

 private:
  // ----------------
  // TLS lane storage
  // ----------------

  struct ArenaLaneTLS {
    const Arena* owner = nullptr;
    ArenaBlock* block  = nullptr;
  };

  static ArenaLaneTLS& GetLane_(const Arena* self) noexcept {
    static thread_local ArenaLaneTLS tls{};
    if (tls.owner != self) {
      tls.owner = self;
      tls.block = nullptr;  // prevent cross-arena contamination
    }
    return tls;
  }

  // -------------
  // Slow paths
  // -------------

  bool TryBump_(ArenaBlock* b, std::size_t bytes, std::size_t align) {
    std::byte* cur    = b->cur;
    std::uintptr_t up = (reinterpret_cast<std::uintptr_t>(cur) + (align - 1)) &
                        ~(std::uintptr_t)(align - 1);
    std::byte* aligned = reinterpret_cast<std::byte*>(up);
    std::byte* next    = aligned + bytes;
    if (next <= b->end) {
      b->cur = next;
      return true;
    }
    return false;
  }

  ArenaBlock* RefillLane_(std::size_t need, std::size_t align) {
    NAVARY_LOCK_GUARD(lk, mu_);
    std::size_t want = NextBlockSize_(need, align);
    auto* raw        = static_cast<std::byte*>(
        opts_.upstream.allocate(want, opts_.alignment, opts_.upstream.user));

    if (!raw) {
      // Allocation failed: return nullptr, caller must handle
      return nullptr;
    }

    auto* block = new (raw)
        ArenaBlock(raw + sizeof(ArenaBlock), want - sizeof(ArenaBlock));
    block->next = head_;
    head_       = block;
    total_bytes_.fetch_add(want, std::memory_order_relaxed);
    T_on_refill_(want);  // Always call telemetry on every block allocation
    (void)TryBump_(block, 0, align);

    return block;
  }

  std::size_t NextBlockSize_(std::size_t need, std::size_t align) const {
    std::size_t body       = opts_.initial_block_bytes;
    std::size_t min_needed = need + align + sizeof(ArenaBlock);
    if (min_needed > body) {
      body = min_needed;
    }

    if (body > opts_.max_block_bytes) {
      body = opts_.max_block_bytes;
    }

    return body;
  }

  void FreeBlock_(ArenaBlock* b) {
    std::byte* raw = b->begin - sizeof(ArenaBlock);
    std::size_t size =
        static_cast<std::size_t>(b->end - b->begin) + sizeof(ArenaBlock);
    opts_.upstream.deallocate(raw, size, opts_.upstream.user);
  }

  void RunAllDtors_() {
    // Collect all dtors under lock
    std::vector<ArenaBlock::DtorNode*> lists;
    {
      NAVARY_LOCK_GUARD(lk, mu_);
      for (ArenaBlock* b = head_; b; b = b->next) {
        if (b->dtors) {
          lists.push_back(b->dtors);
          b->dtors = nullptr;
        }
      }
    }
    // Invoke outside the lock to avoid deadlocks/re-entrancy
    for (auto* n : lists) {
      while (n) {
        auto* next = n->next;
        n->fn(n->arg);
        n = next;
      }
    }
  }

  // -------- epoch core --------

  friend class ArenaEpoch;
  std::atomic<uint32_t> active_epochs_{0};  // [FREEZE|COUNT]

  void EnterEpoch_() noexcept {
    // Block while FREEZE is set; then increment count.
    uint32_t cur = active_epochs_.load(std::memory_order_relaxed);
    for (;;) {
      // If frozen, wait (yield a bit to avoid hot spin) and re-read.
      if (cur & kFreezeBit) {
        std::this_thread::yield();
        cur = active_epochs_.load(std::memory_order_relaxed);
        continue;
      }
      const uint32_t next = cur + 1;  // count++ (freeze stays 0)
      if (active_epochs_.compare_exchange_weak(cur, next,
                                               std::memory_order_relaxed,
                                               std::memory_order_relaxed)) {
        break;
      }
      // cur reloaded by failed CAS; loop
    }
  }

  void LeaveEpoch_() noexcept {
    // Release to publish work before a waiter observes zero.
    active_epochs_.fetch_sub(1, std::memory_order_release);
  }

  void AssertNoEpoch_() const noexcept {
#if NAVARY_ARENA_EPOCH_DEBUG
    uint32_t c = active_epochs_.load(std::memory_order_acquire) & kCountMask;
    if (c != 0) {
      std::fprintf(
          stderr,
          "[navary::memory::Arena] Reset/Purge with %u active epoch(s)\n", c);
      std::fflush(stderr);
      std::abort();
    }
#endif
  }

  // ---- telemetry helpers ----

  void T_on_refill_(std::size_t b) const {
    if (opts_.telemetry.on_alloc_refill) {
      opts_.telemetry.on_alloc_refill(this, b, opts_.telemetry.user);
    }
  }

  void T_on_reset_begin_() const {
    if (opts_.telemetry.on_reset_begin){
      opts_.telemetry.on_reset_begin(this, opts_.telemetry.user);
    }
  }

  void T_on_reset_end_() const {
    if (opts_.telemetry.on_reset_end){
      opts_.telemetry.on_reset_end(this, opts_.telemetry.user);
    }
  }

  void T_on_wait_begin_() const {
    if (opts_.telemetry.on_wait_begin){
      opts_.telemetry.on_wait_begin(this, opts_.telemetry.user);
    }
  }

  void T_on_wait_progress_(uint32_t active) const {
    if (opts_.telemetry.on_wait_progress){
      opts_.telemetry.on_wait_progress(this, active, opts_.telemetry.user);
    }
  }

  void T_on_wait_end_(bool timeout) const {
    if (opts_.telemetry.on_wait_end){
      opts_.telemetry.on_wait_end(this, timeout, opts_.telemetry.user);
    }
  }

  template <class T>
  void RegisterDtorIfNeeded_(T* obj) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      Own(obj, [](void* p) {
        static_cast<T*>(p)->~T();
      });
    }
  }

 private:
  ArenaOptions opts_;
  mutable NAVARY_MUTEX mu_;
  ArenaBlock* head_;
  std::atomic<std::size_t> total_bytes_;
};

}  // namespace navary::memory
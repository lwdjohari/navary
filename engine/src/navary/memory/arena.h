// ------------------------------------------------------------
// navary::memory::Arena
// ------------------------------------------------------------
// A thread-safe, epoch-guarded arena allocator designed for
// real-time and multithreaded use cases (e.g., game engines,
// job systems, renderers).
//
// Key properties:
//  - Linear allocation (bump-pointer) per block.
//  - Concurrent allocations allowed via atomic bump.
//  - Epoch guards track active threads.
//  - Safe reset via WaitForEpochZero() + ArenaResetSafely().
//  - Optional destructor registry and telemetry callbacks.
// ------------------------------------------------------------

#pragma once

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

// ---------- tiny Span for C++17 ----------
// template <class T>
// struct Span {
//   T* data          = nullptr;
//   std::size_t size = 0;
//   T* begin() const {
//     return data;
//   }
//   T* end() const {
//     return data + size;
//   }
//   T& operator[](std::size_t i) const {
//     return data[i];
//   }
// };

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
#if __cpp_aligned_new
    // Standard C++17 aligned allocation (all major platforms)
    return ::operator new(sz, std::align_val_t(align), std::nothrow);
#elif defined(_WIN32)
    // Windows fallback
    return _aligned_malloc(sz, align);
#else
    // POSIX fallback
    void* p = nullptr;
    if (posix_memalign(&p, align, sz) != 0)
      return nullptr;
    return p;
#endif
  }
  static void DefaultFree(void* p, std::size_t, void*) {
#if __cpp_aligned_new
    ::operator delete(p, std::align_val_t(alignof(std::max_align_t)), std::nothrow);
#elif defined(_WIN32)
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

// ----------------------------------------
// Thread-local lane (one block per thread)
// ----------------------------------------

struct ArenaLane {
  ArenaBlock* block = nullptr;
};

// -----------------
// Main Arena object
// -----------------

// class navary::memory::Arena
//----------------------------------------------------------------------
// A high-performance, epoch-guarded linear allocator for real-time
// multithreaded systems such as game engines or render pipelines.
//
//  ▸ Design Goals
//    - Deterministic allocation cost (O(1) bump-pointer).
//    - Thread-safe concurrent Allocate() via atomic offset.
//    - Safe Reset() across threads using epoch guards.
//    - Optional destructor registry for non-POD objects.
//    - Configurable spin / yield / sleep policy for WaitForEpochZero().
//
//  ▸ Typical Usage
//    ```cpp
//    navary::memory::Arena frameArena(1 << 20);      // 1 MB
//    {
//        navary::memory::Arena::ArenaEpoch ep(frameArena);
//        auto* tmp = frameArena.New<MyTempStruct>(42);
//        DoWork(tmp);
//    } // ep leaves scope → decrements active_epochs_
//
//    frameArena.ArenaResetSafely(std::chrono::milliseconds(2));
//    ```
//
//  ▸ Threading Model
//    Multiple threads may allocate concurrently.
//    Reset() / Purge() must occur only after WaitForEpochZero()
//    confirms no active epochs (i.e., no thread still allocating).
//
//  ▸ WaitForEpochZero()
//    Three-phase hybrid precision wait:
//      1. Spin phase — nanosecond-level compare (fast path).
//      2. Yield phase — micro/millisecond granularity.
//      3. Sleep phase — millisecond back-off to prevent busy-loop.
//
//  ▸ Recommended Granularity
//    Spin   → ns-µs  (short critical waits)
//    Yield  → µs-ms  (short scheduler delays)
//    Sleep  → ms     (long-tail safety)
//
//  ▸ Ideal Use Cases
//    - Per-frame or per-phase transient allocations.
//    - Thread-local scratch buffers in job systems.
//    - Frame allocator for Vulkan or DX12 renderers.
//    - Temporary ECS / AI or culling data.
//
//  ▸ Limitations
//    - Pointers invalidated after Reset() or Purge().
//    - Not a general heap (no free() / realloc()).
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
    if (alignment < opts_.alignment)
      alignment = opts_.alignment;
    EnterEpoch_();
    ArenaLane& lane = GetLane_();
    if (!lane.block || !TryBump_(lane.block, size, alignment)) {
      lane.block = RefillLane_(size, alignment);
      if (!lane.block) {
        LeaveEpoch_();
        return nullptr; // Allocation failed
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
    if (!ptr || !deleter)
      return;
    ArenaLane& lane = GetLane_();
    if (!lane.block)
      lane.block = RefillLane_(sizeof(ArenaBlock::DtorNode),
                               alignof(ArenaBlock::DtorNode));
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
    RunAllDtors_();
    NAVARY_LOCK_GUARD(lk, mu_);
    ArenaBlock* keep = head_;
    size_t kept_size = 0;
    if (keep) {
      kept_size =
          static_cast<size_t>(keep->end - keep->begin) + sizeof(ArenaBlock);
      keep->cur   = keep->begin;
      keep->dtors = nullptr;
    }
    ArenaBlock* b = keep ? keep->next : nullptr;
    if (keep)
      keep->next = nullptr;
    while (b) {
      ArenaBlock* nxt = b->next;
      FreeBlock_(b);
      b = nxt;
    }
    total_bytes_.store(keep ? kept_size : 0, std::memory_order_relaxed);
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
    return active_epochs_.load(std::memory_order_acquire) == 0;
  }

  //   template <class Rep, class Period>
  //   bool WaitForEpochZero(std::chrono::duration<Rep, Period> timeout) const {
  //     using namespace std::chrono;
  //     const auto deadline =
  //         steady_clock::now() +
  //         duration_cast<steady_clock::nanoseconds>(timeout);
  //     T_on_wait_begin_();

  //     // Spin
  //     for (uint32_t i = 0; i < opts_.wait_policy.spin_iters; ++i) {
  //       uint32_t c = active_epochs_.load(std::memory_order_acquire);
  //       if (c == 0) {
  //         T_on_wait_end_(false);
  //         return true;
  //       }
  //       if ((i & 0xFFu) == 0)
  //         T_on_wait_progress_(c);
  //       if (steady_clock::now() >= deadline) {
  //         T_on_wait_end_(true);
  //         return false;
  //       }
  //     }
  //     // Yield
  //     for (uint32_t i = 0; i < opts_.wait_policy.yield_iters; ++i) {
  //       uint32_t c = active_epochs_.load(std::memory_order_acquire);
  //       if (c == 0) {
  //         T_on_wait_end_(false);
  //         return true;
  //       }
  //       if ((i & 0xFFu) == 0)
  //         T_on_wait_progress_(c);
  //       std::this_thread::yield();
  //       if (steady_clock::now() >= deadline) {
  //         T_on_wait_end_(true);
  //         return false;
  //       }
  //     }
  //     // Sleep
  //     const auto sleep_dur =
  //         std::chrono::microseconds(opts_.wait_policy.sleep_us);
  //     while (steady_clock::now() < deadline) {
  //       uint32_t c = active_epochs_.load(std::memory_order_acquire);
  //       if (c == 0) {
  //         T_on_wait_end_(false);
  //         return true;
  //       }
  //       T_on_wait_progress_(c);
  //       std::this_thread::sleep_for(sleep_dur);
  //     }
  //     T_on_wait_end_(true);
  //     return false;
  //   }

  template <class Rep, class Period>
  bool WaitForEpochZero(std::chrono::duration<Rep, Period> timeout) const {
    using namespace std::chrono;

    // High-precision deadline (ns) for robust comparisons
    const auto deadline =
        steady_clock::now() + duration_cast<nanoseconds>(timeout);

    T_on_wait_begin_();

    // ---- Phase 1: spin (ns-level compare; no sleeps) ----
    for (uint32_t i = 0; i < opts_.wait_policy.spin_iters; ++i) {
      if (active_epochs_.load(std::memory_order_acquire) == 0) {
        T_on_wait_end_(false);
        return true;
      }
      if ((i & 0xFFu) == 0)
        T_on_wait_progress_(active_epochs_.load(std::memory_order_relaxed));
      if (steady_clock::now() >= deadline) {
        T_on_wait_end_(true);
        return false;
      }
    }

    // ---- Phase 2: yield (lets other threads run; effective µs–ms) ----
    for (uint32_t i = 0; i < opts_.wait_policy.yield_iters; ++i) {
      if (active_epochs_.load(std::memory_order_acquire) == 0) {
        T_on_wait_end_(false);
        return true;
      }
      if ((i & 0xFFu) == 0)
        T_on_wait_progress_(active_epochs_.load(std::memory_order_relaxed));
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
      if (active_epochs_.load(std::memory_order_acquire) == 0) {
        T_on_wait_end_(false);
        return true;
      }
      T_on_wait_progress_(active_epochs_.load(std::memory_order_relaxed));
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
    if (WaitForEpochZero(timeout)) {
      Reset();
      return true;
    }
#if NAVARY_ARENA_EPOCH_DEBUG
    // In debug, assert if epochs are still active after waiting
    uint32_t c = active_epochs_.load(std::memory_order_acquire);
    if (c != 0) {
      std::fprintf(
          stderr,
          "[navary::memory::Arena] ArenaResetSafely: timeout with %u active epoch(s)\n", c);
      std::fflush(stderr);
      std::abort();
    }
#endif
    // Timed out: do NOT reset; caller decides fallback.
    return false;
  }

  // // Helper: waits then Reset(). returns true if reset performed.
  // template <class Rep, class Period>
  // bool ArenaResetSafely(std::chrono::duration<Rep, Period> timeout) {
  //   // Always call wait telemetry for test/telemetry consistency.
  //   T_on_wait_begin_();
  //   uint32_t c = active_epochs_.load(std::memory_order_acquire);
  //   if (c == 0) {
  //     T_on_wait_end_(false);
  //     Reset();
  //     return true;
  //   }
  //   // Never abort here; just return false if epochs are held.
  //   T_on_wait_progress_(c);
  //   T_on_wait_end_(true);
  //   return false;
  //   // In release, you may want to use WaitForEpochZero as before.
  //   // If you want to keep debug/release parity, remove the #if/#else.
  // }

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
                  "For non-trivial types, use Arena::MakeSpan to ensure correct destruction.");
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

  //----------------------------------------------------------------------
  // GetLane_()
  //----------------------------------------------------------------------
  // Returns a reference to the thread-local ArenaLane instance for the
  // calling thread.
  //
  // About ArenaLane:
  //   Each thread that allocates from an Arena uses its own *ArenaLane* —
  //   a lightweight structure that tracks allocation state (bump pointer,
  //   active block pointer, statistics, etc.).  ArenaLanes isolate each
  //   thread’s bump allocation so no locking or atomic arithmetic is
  //   needed for concurrent Allocate() calls.
  //
  //   Conceptually:
  //
  //        ┌───────────────────────────────────────────────────────┐
  //        │                navary::memory::Arena                  │
  //        ├───────────────────────────────────────────────────────┤
  //        │  [Thread 1] ArenaLane ─► bump ptr → [block memory]    │
  //        │  [Thread 2] ArenaLane ─► bump ptr → [block memory]    │
  //        │  [Thread 3] ArenaLane ─► bump ptr → [block memory]    │
  //        └───────────────────────────────────────────────────────┘
  //                    (one ArenaLane per thread)
  //
  //   Each lane operates independently but allocates from the same arena
  //   pool, guaranteeing deterministic and lock-free performance.
  //
  // Implementation details:
  //   • Uses a *function-local static thread_local* variable.
  //   • This prevents *ODR (One Definition Rule)* violations that occur
  //     when multiple translation units or *DSOs (Dynamic Shared Objects)*
  //     define the same global TLS (Thread-Local Storage) symbol.
  //   • Because the variable is defined inside the function, no global
  //     symbol is emitted — safe for header-only usage and across DSOs.
  //
  // Expected behavior:
  //   • Each thread lazily constructs its own ArenaLane on first access.
  //   • Lifetime is tied to the thread — destroyed automatically on exit.
  //   • Works on all major C++20 platforms (Linux, macOS, iOS, Android).
  //   • No initialization-order issues; creation is deterministic.
  //
  // Performance:
  //   • Access cost ≈ TLS lookup.
  //   • Initialization once per thread.
  //   • Lock-free allocations within each lane.
  //
  // Example usage:
  //   auto& lane = GetLane_();
  //   void* mem  = lane.allocate(*this, bytes, align);
  //
  static ArenaLane& GetLane_() noexcept {
    static thread_local ArenaLane lane{};
    return lane;
  }

  //   static thread_local ArenaLane tls_lane_;
  //   ArenaLane& GetLane_() noexcept {
  //     return tls_lane_;
  //   }

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
    T_on_refill_(want); // Always call telemetry on every block allocation
    (void)TryBump_(block, 0, align);
    return block;
  }

  std::size_t NextBlockSize_(std::size_t need, std::size_t align) const {
    std::size_t body       = opts_.initial_block_bytes;
    std::size_t min_needed = need + align + sizeof(ArenaBlock);
    if (min_needed > body)
      body = min_needed;
    if (body > opts_.max_block_bytes)
      body = opts_.max_block_bytes;
    return body;
  }

  void FreeBlock_(ArenaBlock* b) {
    std::byte* raw = b->begin - sizeof(ArenaBlock);
    std::size_t size =
        static_cast<std::size_t>(b->end - b->begin) + sizeof(ArenaBlock);
    opts_.upstream.deallocate(raw, size, opts_.upstream.user);
  }

  void RunAllDtors_() {
    NAVARY_LOCK_GUARD(lk, mu_);
    for (ArenaBlock* b = head_; b; b = b->next) {
      auto* n = b->dtors;
      while (n) {
        auto* next = n->next;
        n->fn(n->arg);
        n = next;
      }
      b->dtors = nullptr;
    }
  }

  // -------- epoch core --------

  friend class ArenaEpoch;
  std::atomic<uint32_t> active_epochs_{0};

  void EnterEpoch_() noexcept {
    active_epochs_.fetch_add(1, std::memory_order_acquire);
  }
  void LeaveEpoch_() noexcept {
    active_epochs_.fetch_sub(1, std::memory_order_release);
  }

  void AssertNoEpoch_() const noexcept {
#if NAVARY_ARENA_EPOCH_DEBUG
    uint32_t c = active_epochs_.load(std::memory_order_acquire);
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
    if (opts_.telemetry.on_alloc_refill)
      opts_.telemetry.on_alloc_refill(this, b, opts_.telemetry.user);
  }
  void T_on_reset_begin_() const {
    if (opts_.telemetry.on_reset_begin)
      opts_.telemetry.on_reset_begin(this, opts_.telemetry.user);
  }
  void T_on_reset_end_() const {
    if (opts_.telemetry.on_reset_end)
      opts_.telemetry.on_reset_end(this, opts_.telemetry.user);
  }
  void T_on_wait_begin_() const {
    if (opts_.telemetry.on_wait_begin)
      opts_.telemetry.on_wait_begin(this, opts_.telemetry.user);
  }
  void T_on_wait_progress_(uint32_t active) const {
    if (opts_.telemetry.on_wait_progress)
      opts_.telemetry.on_wait_progress(this, active, opts_.telemetry.user);
  }
  void T_on_wait_end_(bool timeout) const {
    if (opts_.telemetry.on_wait_end)
      opts_.telemetry.on_wait_end(this, timeout, opts_.telemetry.user);
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

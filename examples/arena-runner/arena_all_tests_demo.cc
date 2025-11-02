// arena_standalone_tests.cc
// Standalone test runner
// - Runs ALL tests even if some fail
// - Prints per-test duration in seconds (ms precision)
// - Lists failed assertions per test
// - REQUIRE(...) ends current test, CHECK(...) keeps going

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>

#include "navary/memory/arena.h"

using namespace navary::memory;

// ---------- tiny harness (no exceptions) ----------
struct Failure {
  const char* file;
  int line;
  std::string expr;
  std::string msg;
};

struct TestContext {
  const char* name = nullptr;
  std::vector<Failure> failures;
  bool ended_early = false;  // set when REQUIRE fails
};

static thread_local TestContext* g_ctx = nullptr;

struct TestCase {
  const char* name;
  void (*fn)();
};

static std::vector<TestCase> g_tests;
static std::mutex g_io;

#define TEST(name_literal)                                         \
  static void test_fn_##name_literal();                            \
  struct reg_##name_literal {                                      \
    reg_##name_literal() {                                         \
      g_tests.push_back({#name_literal, &test_fn_##name_literal}); \
    }                                                              \
  } reg_inst_##name_literal;                                       \
  static void test_fn_##name_literal()

static inline double now_seconds() {
  using clock = std::chrono::steady_clock;
  return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

static inline void record_fail(const char* file, int line, const char* expr,
                               const char* msg = nullptr) {
  if (!g_ctx)
    return;
  Failure f{file, line, expr, msg ? msg : ""};
  g_ctx->failures.push_back(std::move(f));
}

#define REQUIRE(cond)                                  \
  do {                                                 \
    if (!(cond)) {                                     \
      record_fail(__FILE__, __LINE__, #cond, nullptr); \
      if (g_ctx)                                       \
        g_ctx->ended_early = true;                     \
      return;                                          \
    }                                                  \
  } while (0)

#define CHECK(cond)                                    \
  do {                                                 \
    if (!(cond)) {                                     \
      record_fail(__FILE__, __LINE__, #cond, nullptr); \
    }                                                  \
  } while (0)

#define REQUIRE_EQ(a, b) REQUIRE((a) == (b))
#define REQUIRE_NE(a, b) REQUIRE((a) != (b))

static inline void sleep_ms(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
static inline bool is_aligned(const void* p, std::size_t a) {
  return (reinterpret_cast<std::uintptr_t>(p) & (a - 1)) == 0;
}

// ---------- shared helpers ----------
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

static void CleanFn(void* p) {
  if (p)
    CleanupProbe::calls.fetch_add(1, std::memory_order_relaxed);
}

struct Telemetry {
  std::atomic<uint64_t> refills{0}, reset_begin{0}, reset_end{0};
  std::atomic<uint64_t> wait_begin{0}, wait_progress{0}, wait_end{0},
      wait_timeout{0};
};

static ArenaOptions MakeOptsWithTelemetry(Telemetry& t) {
  ArenaOptions o;
  o.initial_block_bytes       = 32 * 1024;
  o.max_block_bytes           = 256 * 1024;
  o.wait_policy.spin_iters    = 2000;
  o.wait_policy.yield_iters   = 10000;
  o.wait_policy.sleep_ms      = 1;
  o.telemetry.user            = &t;
  o.telemetry.on_alloc_refill = [](const void*, std::size_t, void* u) {
    static_cast<Telemetry*>(u)->refills.fetch_add(1, std::memory_order_relaxed);
  };
  o.telemetry.on_reset_begin = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->reset_begin.fetch_add(
        1, std::memory_order_relaxed);
  };
  o.telemetry.on_reset_end = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->reset_end.fetch_add(1,
                                                    std::memory_order_relaxed);
  };
  o.telemetry.on_wait_begin = [](const void*, void* u) {
    static_cast<Telemetry*>(u)->wait_begin.fetch_add(1,
                                                     std::memory_order_relaxed);
  };
  o.telemetry.on_wait_progress = [](const void*, uint32_t, void* u) {
    static_cast<Telemetry*>(u)->wait_progress.fetch_add(
        1, std::memory_order_relaxed);
  };
  o.telemetry.on_wait_end = [](const void*, bool timeout, void* u) {
    static_cast<Telemetry*>(u)->wait_end.fetch_add(1,
                                                   std::memory_order_relaxed);
    if (timeout)
      static_cast<Telemetry*>(u)->wait_timeout.fetch_add(
          1, std::memory_order_relaxed);
  };

  return o;
}

// simple math placeholders (alignas for SIMD-friendliness)
struct alignas(16) Mat4 {
  float m[16];
  static Mat4 Identity() {
    Mat4 r{};
    for (int i = 0; i < 16; ++i)
      r.m[i] = (i % 5 == 0) ? 1.f : 0.f;
    return r;
  }
  static Mat4 TRS(float tx, float ty, float tz) {
    Mat4 r  = Identity();
    r.m[12] = tx;
    r.m[13] = ty;
    r.m[14] = tz;
    return r;
  }
  static Mat4 Mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        float s = 0.f;
        for (int k = 0; k < 4; k++)
          s += a.m[i * 4 + k] * b.m[k * 4 + j];
        r.m[i * 4 + j] = s;
      }
    }
    return r;
  }
};

// ===================== TESTS =====================

// 1. lifecycle + dtors
TEST(Arena_Create_Reset_runs_dtors) {
  using namespace std::chrono_literals;
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));
  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);
  CleanupProbe::calls.store(0);

  {
    auto* a = Arena::New<NonTrivial>(arena, 7);
    REQUIRE(a->v == 7);
    auto span  = Arena::MakeSpan<NonTrivial>(arena, 10);
    span[0].v  = 11;
    span[9].v  = 22;
    auto* triv = Arena::NewPOD<int>(arena, 512);
    triv[0]    = 1;
    triv[511]  = 2;
    int dummy  = 123;
    arena.Own(&dummy, &CleanFn);
    CHECK(NonTrivial::alive.load() >= 11);
    CHECK(CleanupProbe::calls.load() == 0);
    CHECK(arena.TotalReserved() > 0);
  }
  REQUIRE(arena.ArenaResetSafely(std::chrono::milliseconds(5)) == true);
  CHECK(NonTrivial::destroyed.load() >= 11);
  CHECK(CleanupProbe::calls.load() == 1);
  CHECK(tm.reset_begin.load() == 1);
  CHECK(tm.reset_end.load() == 1);
  CHECK(tm.wait_end.load() >= 1);
}

// 2. alignment
TEST(Arena_Allocate_alignment) {
  Arena arena;
  void* p1 = arena.Allocate(33, 64);
  void* p2 = arena.Allocate(7, 256);
  CHECK(is_aligned(p1, 64));
  CHECK(is_aligned(p2, 256));
  arena.Reset();
}

// 3. refill telemetry
TEST(Arena_Refill_telemetry) {
  Telemetry tm;
  auto opts                = MakeOptsWithTelemetry(tm);
  opts.initial_block_bytes = 1024;
  opts.max_block_bytes     = 4 * 1024;
  Arena arena(opts);
  for (int i = 0; i < 50; i++)
    (void)arena.Allocate(64, 16);
  CHECK(tm.refills.load() >= 1);
  CHECK(arena.TotalReserved() >= 1024);
  arena.Reset();
}

// 4. WaitForEpochZero success
TEST(Arena_WaitForEpochZero_succeeds) {
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));
  std::atomic<bool> go{false};
  std::thread worker([&]() {
    Arena::ArenaEpoch eg(arena);
    while (!go.load(std::memory_order_acquire))
      std::this_thread::yield();
  });
  sleep_ms(1);
  std::atomic<bool> waited{false};
  std::thread waiter([&]() {
    waited = arena.WaitForEpochZero(std::chrono::milliseconds(100));
  });
  sleep_ms(2);
  CHECK(waited.load() == false);
  go.store(true, std::memory_order_release);
  worker.join();
  waiter.join();
  CHECK(waited.load() == true);
  CHECK(arena.IsIdle());
  CHECK(arena.ArenaResetSafely(std::chrono::milliseconds(2)) == true);
  CHECK(tm.wait_begin.load() >= 1);
  CHECK(tm.wait_end.load() >= 1);
}

// 6. ArenaBatch RAII
TEST(Arena_Batch_RAII) {
  using namespace std::chrono_literals;
  Telemetry tm;
  Arena arena(MakeOptsWithTelemetry(tm));
  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);
  {
    Arena::ArenaBatch batch(arena, 5ms);
    auto span = Arena::MakeSpan<NonTrivial>(arena, 5);
    CHECK(span.size() == 5);
    span[0].v = 1;
  }
  CHECK(NonTrivial::destroyed.load() >= 5);
  CHECK(tm.reset_end.load() >= 1);
}

// 7. concurrency hot path
TEST(Arena_Concurrency_many_threads) {
  Telemetry tm;
  auto opts                = MakeOptsWithTelemetry(tm);
  opts.initial_block_bytes = 16 * 1024;
  opts.max_block_bytes     = 512 * 1024;
  Arena arena(opts);

  constexpr int kThreads = 8, kIters = 500;
  std::atomic<uint64_t> sum{0};

  auto job = [&](int seed) {
    std::mt19937 rng(seed);
    for (int i = 0; i < kIters; i++) {
      Arena::ArenaEpoch eg(arena);
      std::size_t n = (rng() % 2048) + 1;
      auto* p       = static_cast<uint8_t*>(arena.Allocate(n, 32));
      CHECK(p != nullptr);
      if (p) {
        p[0] = static_cast<uint8_t>(n & 0xFF);
        sum.fetch_add(p[0], std::memory_order_relaxed);
      }
    }
  };

  std::vector<std::thread> ts;
  ts.reserve(kThreads);

  for (int t = 0; t < kThreads; t++)
    ts.emplace_back(job, 1337 + t);

  for (auto& th : ts)
    th.join();

  CHECK(arena.ArenaResetSafely(std::chrono::milliseconds(10)) == true);
  CHECK(sum.load() > 0);
  CHECK(tm.refills.load() >= 1);
}

// 8. helpers
TEST(Arena_Helpers_dtors) {
  using namespace std::chrono_literals;
  Arena arena;
  NonTrivial::alive.store(0);
  NonTrivial::destroyed.store(0);
  {
    auto* a = Arena::New<NonTrivial>(arena, 3);
    CHECK(a->v == 3);
    auto span = Arena::MakeSpan<NonTrivial>(arena, 4);
    span[1].v = 10;
    auto* pod = Arena::NewPOD<uint32_t>(arena, 64);
    pod[63]   = 0xABCDu;
    CHECK(pod[63] == 0xABCDu);
  }
  CHECK(arena.ArenaResetSafely(5ms) == true);
  CHECK(NonTrivial::destroyed.load() >= 5);
}

// 9. Own() cleanup order (LIFO per block)
TEST(Arena_Own_LIFO) {
  using namespace std::chrono_literals;
  Arena arena;
  std::vector<int> order;
  order.reserve(3);
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(1);
  });
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(2);
  });
  arena.Own(&order, [](void* p) {
    reinterpret_cast<std::vector<int>*>(p)->push_back(3);
  });
  CHECK(arena.ArenaResetSafely(3ms) == true);
  CHECK(order.size() == 3);
  if (order.size() == 3) {
    CHECK(order[0] == 3);
    CHECK(order[1] == 2);
    CHECK(order[2] == 1);
  }
}

// 10. stats lifecycle
TEST(Arena_TotalReserved_lifecycle) {
  Arena arena;
  CHECK(arena.TotalReserved() == 0);
  (void)arena.Allocate(1024, 16);
  auto r = arena.TotalReserved();
  CHECK(r >= 1024);
  arena.Reset();
  CHECK(arena.TotalReserved() >= sizeof(void*));
  arena.Purge();
  CHECK(arena.TotalReserved() == 0);
}

// 11. tiny alloc stress
TEST(Arena_Tiny_alloc_stable) {
  using namespace std::chrono_literals;
  Arena arena;
  for (int i = 0; i < 10000; i++) {
    auto* p = static_cast<unsigned char*>(arena.Allocate(8, 8));
    CHECK(p != nullptr);
    if (p)
      p[0] = static_cast<unsigned char>(i & 0xFF);
  }
  CHECK(arena.ArenaResetSafely(5ms) == true);
}

// complex: scene graph
struct Transform {
  Mat4 local, world;
};

template <typename T>
struct SpanT {
  T* data;
  std::size_t size;
  T* begin() const {
    return data;
  }
  T* end() const {
    return data + size;
  }
  T& operator[](std::size_t i) const {
    return data[i];
  }
  std::size_t Size() const {
    return size;
  }
};

struct SceneNode {
  std::string* name;
  Transform* xform;
  Span<SceneNode*> children;
};

static void BuildSub(Arena& a, SceneNode* parent, int depth, int fanout,
                     int& counter) {
  if (depth == 0)
    return;
  parent->children = Arena::MakeSpan<SceneNode*>(a, fanout);
  for (int i = 0; i < fanout; i++) {
    auto* n = Arena::New<SceneNode>(a);
    n->name = Arena::New<std::string>(a, "entity_" + std::to_string(counter++));
    n->xform            = Arena::New<Transform>(a);
    n->xform->local     = Mat4::TRS(float(i), float(depth), 0.f);
    n->xform->world     = Mat4::Identity();
    parent->children[i] = n;
    BuildSub(a, n, depth - 1, fanout, counter);
  }
}

static void UpdateWorld(SceneNode* n, const Mat4& p) {
  n->xform->world = Mat4::Mul(p, n->xform->local);
  for (auto* c : n->children)
    UpdateWorld(c, n->xform->world);
}

TEST(Complex_SceneGraph) {
  using namespace std::chrono_literals;
  Arena a;
  auto* root         = Arena::New<SceneNode>(a);
  root->name         = Arena::New<std::string>(a, "root");
  root->xform        = Arena::New<Transform>(a);
  root->xform->local = Mat4::Identity();
  root->xform->world = Mat4::Identity();
  int counter        = 0;
  BuildSub(a, root, 3, 5, counter);
  UpdateWorld(root, Mat4::Identity());
  CHECK(root->children.size() == 5);
  CHECK(root->children[0]->name->rfind("entity_", 0) == 0);
  CHECK(a.ArenaResetSafely(5ms) == true);
}

// complex: skeletal pose blending
struct Pose {
  Span<Mat4> bones;
};

static Pose AllocPose(Arena& a, std::size_t n) {
  auto s = Arena::MakeSpan<Mat4>(a, n);
  for (size_t i = 0; i < n; i++)
    s[i] = Mat4::Identity();
  return Pose{s};
}

static void Blend(const Pose& A, const Pose& B, float t, Pose& O) {
  CHECK(A.bones.size() == B.bones.size() && A.bones.size() == O.bones.size());
  for (size_t i = 0; i < A.bones.size(); i++)
    for (int k = 0; k < 16; k++)
      O.bones[i].m[k] = A.bones[i].m[k] * (1.f - t) + B.bones[i].m[k] * t;
}

TEST(Complex_PoseBlend) {
  using namespace std::chrono_literals;
  Arena a;
  Pose bind = AllocPose(a, 64), A = AllocPose(a, 64), B = AllocPose(a, 64),
       O = AllocPose(a, 64);
  CHECK((reinterpret_cast<std::uintptr_t>(bind.bones.data()) & 0xF) == 0);
  for (size_t i = 0; i < 64; i++) {
    A.bones[i].m[12] = float(i);
    B.bones[i].m[13] = float(i) * 2.f;
  }
  Blend(A, B, 0.25f, O);
  CHECK(std::abs(O.bones[10].m[12] - 7.5f) < 1e-5f);
  CHECK(std::abs(O.bones[10].m[13] - 5.0f) < 1e-5f);
  CHECK(a.ArenaResetSafely(5ms) == true);
}

// complex: render cmd buffer
struct DrawCmd {
  std::string* material;
  Span<float> constants;
  uint32_t meshId;
};

static void EmitDraws(Arena& a, Span<DrawCmd>& out, int n) {
  out = Arena::MakeSpan<DrawCmd>(a, n);
  for (int i = 0; i < n; i++) {
    out[i].material =
        Arena::New<std::string>(a, "mat_" + std::to_string(i % 3));
    out[i].constants = Arena::MakeSpan<float>(a, 16);
    for (int k = 0; k < 16; k++)
      out[i].constants[k] = float(i * 16 + k);
    out[i].meshId = uint32_t(i % 7);
  }
}

TEST(Complex_RenderCmdBuf) {
  using namespace std::chrono_literals;
  Arena a;
  Span<DrawCmd> buf{};
  EmitDraws(a, buf, 128);
  CHECK(buf.size() == 128);
  CHECK(*buf[5].material == "mat_2");
  CHECK(buf[5].constants[0] == 80.f);
  CHECK(a.ArenaResetSafely(2ms) == true);
}

// complex: pathfinding BFS
struct PFNode {
  uint16_t id;
  Span<PFNode*> neighbors;
};

static PFNode* MakeGrid(Arena& a, int w, int h) {
  auto nodes = Arena::MakeSpan<PFNode>(a, size_t(w * h));
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      int idx       = y * w + x;
      nodes[idx].id = uint16_t(idx);
    }
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      int idx = y * w + x, deg = 0;
      if (x > 0)
        deg++;
      if (x < w - 1)
        deg++;
      if (y > 0)
        deg++;
      if (y < h - 1)
        deg++;
      nodes[idx].neighbors = Arena::MakeSpan<PFNode*>(a, deg);
      int k                = 0;
      if (x > 0)
        nodes[idx].neighbors[k++] = &nodes[idx - 1];
      if (x < w - 1)
        nodes[idx].neighbors[k++] = &nodes[idx + 1];
      if (y > 0)
        nodes[idx].neighbors[k++] = &nodes[idx - w];
      if (y < h - 1)
        nodes[idx].neighbors[k++] = &nodes[idx + w];
    }
  return &nodes[0];
}

static int BFS(Arena& a, PFNode* s, PFNode* g, int maxN) {
  auto queue = Arena::MakeSpan<PFNode*>(a, size_t(maxN));
  auto seen  = Arena::MakeSpan<uint8_t>(a, size_t(maxN));
  for (size_t i = 0; i < seen.size(); i++)
    seen[i] = 0;
  int qb = 0, qe = 0;
  queue[qe++] = s;
  seen[s->id] = 1;
  int hops = 0, left = 1, next = 0;
  while (qb < qe) {
    PFNode* n = queue[qb++];
    if (n == g)
      return hops;
    for (auto* nb : n->neighbors)
      if (!seen[nb->id]) {
        seen[nb->id] = 1;
        queue[qe++]  = nb;
        next++;
      }
    if (--left == 0) {
      hops++;
      left = next;
      next = 0;
    }
  }
  return -1;
}

TEST(Complex_Pathfinding_BFS) {
  using namespace std::chrono_literals;
  Arena a;
  PFNode* s = MakeGrid(a, 16, 16);
  int hops  = BFS(a, s, s + (16 * 16 - 1), 16 * 16);
  CHECK(hops == 30);
  CHECK(a.ArenaResetSafely(3ms) == true);
}

// complex: net decode & copy minimal
struct NetEntityMsg {
  uint32_t id;
  float px, py, pz;
  std::string* tag;
};

struct EntityLite {
  uint32_t id;
  float px, py, pz;
  uint32_t tagHash;
};

static uint32_t Fnv1a(const char* s) {
  uint32_t h = 2166136261u;
  for (; *s; ++s) {
    h ^= uint8_t(*s);
    h *= 16777619u;
  }
  return h;
}

static Span<NetEntityMsg> FakeDecode(Arena& a, int n) {
  auto msgs = Arena::MakeSpan<NetEntityMsg>(a, n);
  for (int i = 0; i < n; i++) {
    msgs[i].id  = i + 1;
    msgs[i].px  = float(i);
    msgs[i].py  = float(i * 2);
    msgs[i].pz  = 0.f;
    msgs[i].tag = Arena::New<std::string>(a, (i % 2) == 0 ? "enemy" : "npc");
  }
  return msgs;
}

TEST(Complex_Net_decode_copy_minimal) {
  using namespace std::chrono_literals;
  Arena a;
  auto decoded = FakeDecode(a, 200);
  std::vector<EntityLite> keep;
  keep.reserve(decoded.size());
  for (auto& m : decoded) {
    keep.push_back({m.id, m.px, m.py, m.pz, Fnv1a(m.tag->c_str())});
  }
  CHECK(a.ArenaResetSafely(5ms) == true);
  CHECK(keep.size() == 200);
  CHECK(keep[0].tagHash == Fnv1a("enemy"));
  CHECK(keep[1].tagHash == Fnv1a("npc"));
}

// complex: GPU-like cleanup
struct FakeGpu {
  static std::atomic<uint32_t> freed;
};

std::atomic<uint32_t> FakeGpu::freed{0};
static void FreeGpu(void* p) {
  if (!p)
    return;
  FakeGpu::freed.fetch_add(1, std::memory_order_relaxed);
}

TEST(Complex_GPU_cleanup_Own) {
  using namespace std::chrono_literals;
  Arena a;
  FakeGpu::freed.store(0);
  for (int i = 0; i < 50; i++) {
    auto* id = Arena::New<uint32_t>(a, uint32_t(1000 + i));
    a.Own(id, &FreeGpu);
  }
  CHECK(FakeGpu::freed.load() == 0);
  CHECK(a.ArenaResetSafely(5ms) == true);
  CHECK(FakeGpu::freed.load() == 50);
}

// alignment 32/64
static bool aligned_ptr(const void* p, std::size_t a) {
  return (reinterpret_cast<std::uintptr_t>(p) & (a - 1)) == 0;
}

TEST(Complex_Alignment_32_64) {
  using namespace std::chrono_literals;
  Arena a;
  void* p32 = a.Allocate(4096, 32);
  void* p64 = a.Allocate(1024, 64);
  CHECK(aligned_ptr(p32, 32));
  CHECK(aligned_ptr(p64, 64));
  CHECK(a.ArenaResetSafely(2ms) == true);
}

// -------------------- main --------------------
int main() {
  std::cout << "\nNavary::memory::Arena Smoke Tests\n";
  std::cout << "-----------------------------------\n";
  std::cout << "Linggawasistha Djohari, 2025\n\n";

  double t0  = now_seconds();
  int passed = 0, failed = 0;

  for (const auto& tc : g_tests) {
    TestContext ctx;
    ctx.name  = tc.name;
    g_ctx     = &ctx;
    double ts = now_seconds();
    tc.fn();
    double te = now_seconds();
    g_ctx     = nullptr;

    const bool ok = ctx.failures.empty();
    {
      std::lock_guard<std::mutex> lk(g_io);
      std::cout << (ok ? "[PASS] " : "[FAIL] ") << std::left << std::setw(48)
                << tc.name << "  " << std::fixed << std::setprecision(3)
                << (te - ts) << " s\n";
      if (!ok) {
        for (const auto& f : ctx.failures) {
          std::cout << "    - " << f.file << ":" << f.line
                    << "  expr: " << f.expr;
          if (!f.msg.empty())
            std::cout << "  msg: " << f.msg;
          std::cout << "\n";
        }
        if (ctx.ended_early) {
          std::cout << "      (test ended early due to REQUIRE failure)\n";
        }
      }
    }
    ok ? ++passed : ++failed;
  }

  double t1 = now_seconds();
  std::cout
      << "\n------------------------------------------------------------\n";
  std::cout << "Total: " << (passed + failed) << ", Passed: " << passed
            << ", Failed: " << failed << "   Elapsed: " << std::fixed
            << std::setprecision(3) << (t1 - t0) << " s\n\n";
  return failed == 0 ? 0 : 1;
}

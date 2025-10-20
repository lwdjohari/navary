#include <catch2/catch_all.hpp>
#include "navary/memory/arena.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

using namespace navary::memory;
using namespace std::chrono_literals;
using Catch::Approx;

// -----------------------
// Simple math placeholders
// -----------------------
struct alignas(16) Vec4 {
  float x, y, z, w;
};
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

// ------------------------------------
// 1. Scene graph with complex objects
// ------------------------------------
struct Transform {
  Mat4 local;
  Mat4 world;
};

struct SceneNode {
  // Non-trivial name to test destructor tracking
  std::string* name;          // allocated via Arena::New<std::string>
  Transform* xform;           // single object via Arena::New<Transform>
  Span<SceneNode*> children;  // span of pointers stored in arena
};

static void BuildSubtree(Arena& arena, SceneNode* parent, int depth, int fanout,
                         int& counter) {
  if (depth == 0)
    return;
  parent->children = Arena::MakeSpan<SceneNode*>(arena, fanout);
  for (int i = 0; i < fanout; i++) {
    auto* node = Arena::New<SceneNode>(arena);
    node->name =
        Arena::New<std::string>(arena, "entity_" + std::to_string(counter++));
    node->xform         = Arena::New<Transform>(arena);
    node->xform->local  = Mat4::TRS(float(i), float(depth), 0.f);
    node->xform->world  = Mat4::Identity();
    parent->children[i] = node;
    BuildSubtree(arena, node, depth - 1, fanout, counter);
  }
}

static void UpdateWorld(SceneNode* n, const Mat4& parent) {
  n->xform->world = Mat4::Mul(parent, n->xform->local);
  for (auto* c : n->children)
    UpdateWorld(c, n->xform->world);
}

TEST_CASE("Complex: Scene graph allocation, traversal and reset",
          "[complex][scene]") {
  Arena arena;
  // Root
  auto* root         = Arena::New<SceneNode>(arena);
  root->name         = Arena::New<std::string>(arena, "root");
  root->xform        = Arena::New<Transform>(arena);
  root->xform->local = Mat4::Identity();
  root->xform->world = Mat4::Identity();

  int counter = 0;
  BuildSubtree(arena, root, /*depth*/ 3, /*fanout*/ 5,
               counter);  // ~1 + 5 + 25 = 31 nodes

  // Update world transforms (tests tree recursion + arena pointers)
  UpdateWorld(root, Mat4::Identity());

  // Spot check a couple of nodes
  REQUIRE(root->children.size() == 5);
  REQUIRE(root->children[0]->name->find("entity_") == 0);

  // Safe reset: all std::string destructors must run without leaks
  REQUIRE(arena.ArenaResetSafely(5ms));
}

// ------------------------------------------------------------
// 2. Skeletal pose blending (SIMD alignment, large arrays)
// ------------------------------------------------------------
struct Pose {
  // 64 bones * 16 floats (Mat4) = 4096 bytes; we want 16+ alignment.
  Span<Mat4> bones;
};

static Pose AllocatePose(Arena& arena, std::size_t bone_count) {
  auto span = Arena::MakeSpan<Mat4>(arena, bone_count);
  // Identity init
  for (std::size_t i = 0; i < bone_count; i++)
    span[i] = Mat4::Identity();
  return Pose{span};
}

static void Blend(const Pose& a, const Pose& b, float t, Pose& out) {
  REQUIRE(a.bones.size() == b.bones.size());
  REQUIRE(a.bones.size() == out.bones.size());
  for (std::size_t i = 0; i < a.bones.size(); i++) {
    // naive lerp on matrix elements (for test only)
    for (int k = 0; k < 16; k++) {
      out.bones[i].m[k] = a.bones[i].m[k] * (1.f - t) + b.bones[i].m[k] * t;
    }
  }
}

TEST_CASE("Complex: Skeletal pose blending with aligned buffers",
          "[complex][animation]") {
  Arena arena;

  Pose bind = AllocatePose(arena, 64);
  Pose a    = AllocatePose(arena, 64);
  Pose b    = AllocatePose(arena, 64);
  Pose out  = AllocatePose(arena, 64);

  // Ensure alignment (SIMD-friendly)
  REQUIRE((reinterpret_cast<std::uintptr_t>(bind.bones.data()) & 0xF) == 0);

  // Construct two different poses
  for (std::size_t i = 0; i < 64; i++) {
    a.bones[i].m[12] = float(i);        // translate X
    b.bones[i].m[13] = float(i) * 2.f;  // translate Y
  }

  Blend(a, b, 0.25f, out);
  REQUIRE(out.bones[10].m[12] == Approx(10.f * 0.75f));  // 7.5
  REQUIRE(out.bones[10].m[13] == Approx(20.f * 0.25f));  // 5.0

  REQUIRE(arena.ArenaResetSafely(5ms));
}

// -------------------------------------------------------------------------
// 3. Render command buffer (non-trivial members + per-frame scratch)
// -------------------------------------------------------------------------
struct DrawCmd {
  std::string* material;  // non-trivial
  Span<float> constants;  // per-draw constants
  uint32_t meshId;
};

static void EmitDraws(Arena& arena, Span<DrawCmd>& out, int count) {
  out = Arena::MakeSpan<DrawCmd>(arena, count);
  for (int i = 0; i < count; i++) {
    out[i].material =
        Arena::New<std::string>(arena, "mat_" + std::to_string(i % 3));
    out[i].constants = Arena::MakeSpan<float>(arena, 16);
    for (int k = 0; k < 16; k++)
      out[i].constants[k] = float(i * 16 + k);
    out[i].meshId = static_cast<uint32_t>(i % 7);
  }
}

TEST_CASE("Complex: Render command buffer and per-frame scratch",
          "[complex][render]") {
  Arena arena;

  Span<DrawCmd> cmdBuf{};
  EmitDraws(arena, cmdBuf, 128);

  // Validate a couple of entries
  REQUIRE(cmdBuf.size() == 128);
  REQUIRE(*cmdBuf[5].material == "mat_2");
  REQUIRE(cmdBuf[5].constants[0] == Approx(80.f));  // 5*16

  // End-of-frame cleanup
  REQUIRE(arena.ArenaResetSafely(2ms));
}

// ------------------------------------------------------------------
// 4. Pathfinding scratch: graph + BFS queue in arena scratch memory
// ------------------------------------------------------------------
struct PFNode {
  uint16_t id;
  Span<PFNode*> neighbors;
};

static PFNode* MakeGridGraph(Arena& arena, int w, int h) {
  // Store all nodes in one span
  auto nodes = Arena::MakeSpan<PFNode>(arena, std::size_t(w * h));
  // Also store pointer index for convenience
  auto ptrs = Arena::MakeSpan<PFNode*>(arena, std::size_t(w * h));
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx       = y * w + x;
      nodes[idx].id = static_cast<uint16_t>(idx);
      ptrs[idx]     = &nodes[idx];
    }
  }
  // 4-neighborhood
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int idx = y * w + x;
      int deg = 0;
      if (x > 0)
        deg++;
      if (x < w - 1)
        deg++;
      if (y > 0)
        deg++;
      if (y < h - 1)
        deg++;
      nodes[idx].neighbors = Arena::MakeSpan<PFNode*>(arena, deg);
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
  }
  // Return pointer to [0] for BFS start
  return &nodes[0];
}

static int BFS_MinHops(Arena& arena, PFNode* start, PFNode* goal,
                       int maxNodes) {
  auto queue = Arena::MakeSpan<PFNode*>(arena, std::size_t(maxNodes));
  auto seen  = Arena::MakeSpan<uint8_t>(arena, std::size_t(maxNodes));
  for (std::size_t i = 0; i < seen.size(); i++)
    seen[i] = 0;

  int qb = 0, qe = 0;
  queue[qe++]     = start;
  seen[start->id] = 1;

  int hops      = 0;
  int level_rem = 1, next_level = 0;

  while (qb < qe) {
    PFNode* n = queue[qb++];
    if (n == goal)
      return hops;
    for (auto* nb : n->neighbors) {
      if (!seen[nb->id]) {
        seen[nb->id] = 1;
        queue[qe++]  = nb;
        next_level++;
      }
    }
    if (--level_rem == 0) {
      hops++;
      level_rem  = next_level;
      next_level = 0;
    }
  }
  return -1;
}

TEST_CASE("Complex: Pathfinding BFS uses arena scratch and resets cleanly",
          "[complex][pathfinding]") {
  Arena arena;
  PFNode* start = MakeGridGraph(arena, 16, 16);
  // goal at bottom-right
  int hops = BFS_MinHops(arena, start, start + (16 * 16 - 1), 16 * 16);
  REQUIRE(hops == 30);  // Manhattan distance in 16x16 from (0,0) to (15,15)

  REQUIRE(arena.ArenaResetSafely(3ms));
}

// -------------------------------------------------------------------------
// 5. “Network decode” pattern: parse scratch → copy minimal to persistent
// -------------------------------------------------------------------------
struct NetEntityMsg {
  uint32_t id;
  float px, py, pz;
  std::string* tag;  // heavy, stays only in arena
};

// Persistent game-state (outside arena)
struct EntityLite {
  uint32_t id;
  float px, py, pz;
  // store only small tag hash; simulate “copy minimal”
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

static Span<NetEntityMsg> FakeDecode(Arena& arena, int count) {
  auto msgs = Arena::MakeSpan<NetEntityMsg>(arena, count);
  for (int i = 0; i < count; i++) {
    msgs[i].id = i + 1;
    msgs[i].px = float(i);
    msgs[i].py = float(i * 2);
    msgs[i].pz = 0.f;
    msgs[i].tag =
        Arena::New<std::string>(arena, (i % 2) == 0 ? "enemy" : "npc");
  }
  return msgs;
}

TEST_CASE("Complex: Network decode to arena, copy minimal to persistent state",
          "[complex][netcode]") {
  Arena arena;
  // Decode a burst
  auto decoded = FakeDecode(arena, 200);

  // Copy minimal, persistent data
  std::vector<EntityLite> persistent;
  persistent.reserve(decoded.size());
  for (auto& m : decoded) {
    EntityLite e;
    e.id      = m.id;
    e.px      = m.px;
    e.py      = m.py;
    e.pz      = m.pz;
    e.tagHash = Fnv1a(m.tag->c_str());
    persistent.push_back(e);
  }

  // Now we can safely reset the arena; persistent keeps minimal state
  REQUIRE(arena.ArenaResetSafely(5ms));

  // Sanity on persisted data
  REQUIRE(persistent.size() == 200);
  REQUIRE(persistent[0].tagHash == Fnv1a("enemy"));
  REQUIRE(persistent[1].tagHash == Fnv1a("npc"));
}

// -----------------------------------------------------------------------
// 6. GPU-like resource cleanup via Own() (custom teardown semantics)
// -----------------------------------------------------------------------
struct FakeGpu {
  static std::atomic<uint32_t> freedBuffers;
};
std::atomic<uint32_t> FakeGpu::freedBuffers{0};

static void FreeGpuBuf(void* p) {
  if (!p)
    return;
  // p points to a uint32_t "buffer id"
  FakeGpu::freedBuffers.fetch_add(1, std::memory_order_relaxed);
}

TEST_CASE("Complex: GPU buffer handles cleaned via Own() at reset",
          "[complex][gpu]") {
  Arena arena;
  FakeGpu::freedBuffers.store(0);

  // Allocate some “buffers”; register custom cleanup
  for (int i = 0; i < 50; i++) {
    uint32_t* bufId = Arena::New<uint32_t>(arena, uint32_t(1000 + i));
    arena.Own(bufId, &FreeGpuBuf);
  }

  REQUIRE(FakeGpu::freedBuffers.load() == 0);
  REQUIRE(arena.ArenaResetSafely(5ms));
  REQUIRE(FakeGpu::freedBuffers.load() == 50);
}

// ---------------------------------------------------------------------
// 7. Multithreaded: build transient render lists, then frame reset
// ---------------------------------------------------------------------
TEST_CASE("Complex: Multithreaded transient render lists with frame barrier",
          "[complex][mt][render]") {
  Arena arena;
  constexpr int kThreads       = 6;
  constexpr int kCmdsPerThread = 200;

  auto worker = [&](int seed) {
    std::mt19937 rng(seed);
    Arena::ArenaEpoch eg(arena);  // protect allocations region
    // Each thread emits its own mini list into shared arena
    auto myCmds = Arena::MakeSpan<DrawCmd>(arena, kCmdsPerThread);
    for (int i = 0; i < kCmdsPerThread; i++) {
      myCmds[i].material =
          Arena::New<std::string>(arena, "m" + std::to_string(rng() % 8));
      myCmds[i].constants = Arena::MakeSpan<float>(arena, 8);
      for (int k = 0; k < 8; k++)
        myCmds[i].constants[k] = float(k + i);
      myCmds[i].meshId = uint32_t(rng() % 32);
    }
    // epoch guard ends at scope exit
  };

  std::vector<std::thread> ts;
  for (int i = 0; i < kThreads; i++)
    ts.emplace_back(worker, 1337 + i);
  for (auto& t : ts)
    t.join();

  // “Frame end”: safe reset (no allocations active)
  REQUIRE(arena.ArenaResetSafely(10ms));
}

// ---------------------------------------------------------------------
// 8) Alignment for larger SIMD/UBO blocks (32/64 byte)
// ---------------------------------------------------------------------
static bool IsAlignedPtr(const void* p, std::size_t a) {
  return (reinterpret_cast<std::uintptr_t>(p) & (a - 1)) == 0;
}

TEST_CASE("Complex: 32/64-byte alignment for UBO/SSBO blobs",
          "[complex][alignment]") {
  Arena arena;
  void* p32 = arena.Allocate(4096, 32);
  void* p64 = arena.Allocate(1024, 64);
  REQUIRE(IsAlignedPtr(p32, 32));
  REQUIRE(IsAlignedPtr(p64, 64));
  REQUIRE(arena.ArenaResetSafely(2ms));
}

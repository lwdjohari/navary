// SPDX-License-Identifier: NAVARY EULA 1.0
// Author: Linggawasistha Djohari
// Bump-pointer slab-based Arena Allocator

#include <cstddef>
#include <vector>

#include "navary/macro.h"
#include "navary/memory/block.h"

NVR_INNER_NAMESPACE(memory)
using ArenaErrCode = int32_t;

namespace ARENA_ERR {
static constexpr ArenaErrCode OK                   = 0;
static constexpr ArenaErrCode ARENA_ERR_OVERRUN    = 1;
static constexpr ArenaErrCode ARENA_ERR_OOM        = 2;
static constexpr ArenaErrCode ARENA_ERR_GROW       = 3;
static constexpr ArenaErrCode ARENA_ERR_ALIGNMENT  = 4;
static constexpr ArenaErrCode ARENA_ERR_ALLOCATION = 5;

}  // namespace ARENA_ERR

// Bump-pointer slab-based Arena Allocator
class Arena {
 public:
  // Creates arena over a given memory block
  // initial_size_: how big the first block should be (in bytes).
  // alignment:  alignment for both block base and allocations.
  Arena(size_t initial_block_size = 16 * 1024,
        size_t alignment          = alignof(std::max_align_t));

  // Disallow copy
  Arena(const Arena&)            = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena();

  // Allocate `n` bytes (aligned internally). Returns nullptr on OOM.
  void* Allocate(size_t bytes, ArenaErrCode& err);

  // Returns available capacity remaining
  size_t Remaining() const;

  // Total bytes used across all blocks (incrementally tracked).
  size_t Size() const;

  // Total bytes committed across all blocks (incrementally tracked)
  size_t Capacity() const;

  // Reset for next frame
  void Reset();

 private:
  size_t alignment_;
  size_t initial_size_;        // first block size
  size_t current_block_size_;  // doubled each time, up to max
                               // Running counters
  size_t used_bytes_;          // sum of all bytes handed out
  size_t committed_bytes_;     // sum of all block capacities

  std::vector<Block*> blocks_;  // all allocated blocks, in order

  // Align an integer up to `alignment` (power of two):
  static size_t AlignUp(size_t n, size_t alignment, ArenaErrCode& err);

  // Grow by allocating a new block of at least `min_size`:
  Block* Grow(size_t min_size, ArenaErrCode& err);
};

NVR_INNER_END_NAMESPACE

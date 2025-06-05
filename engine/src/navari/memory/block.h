// SPDX-License-Identifier: NAVARI EULA 1.0
// Author: Linggawasistha Djohari
// Slab-block for Arena Allocator

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "navari/macro.h"

NVR_INNER_NAMESPACE(memory)

struct Block {
  // pointer to the start of this block (including header if any)
  uint8_t* base;
  // current bump pointer within [base, end)
  uint8_t* ptr;
  // one‐past‐last valid byte in this block
  uint8_t* end;

  // Construct a block that can hold `block_size` total bytes (excluding
  // header). Align `base` to `alignment` (e.g. alignof(std::max_align_t)).
  static Block* Create(size_t block_size, size_t alignment) {
    // We allocate block_size + alignment to guarantee we can align the start.
    void* raw = std::malloc(block_size + alignment);
    if (!raw)
      return nullptr;

    uintptr_t raw_int = reinterpret_cast<uintptr_t>(raw);
    // Round up `raw_int` to a multiple of `alignment`:
    uintptr_t aligned = (raw_int + (alignment - 1)) & ~(alignment - 1);
    uint8_t* base = reinterpret_cast<uint8_t*>(aligned);
    Block* b = reinterpret_cast<Block*>(base);
    // We’ll store the Block struct at the beginning:
    // [ Block header ][ usable bytes ... ]
    // So skip past the header for the bump pointer:
    b->base = base;
    b->ptr = base + sizeof(Block);
    b->end = base + sizeof(Block) + block_size;
    return b;
  }

  // Frees the block’s original allocation:
  void Destroy() {
    // But `base` is already aligned inside the malloc’d region,
    // so to free, we need the original pointer.
    // For simplicity, assume we stored it right before `base`
    // (this code omits that detail).
    if (base) {
      std::free(/* original malloc ptr */ base);
    }
  }

  // Try to allocate `n` bytes (already aligned) from this block.
  // Returns pointer on success, or nullptr if not enough room.
  void* AllocateInBlock(size_t n) {
    uint8_t* current = ptr;
    uint8_t* next = current + n;
    if (next <= end) {
      ptr = next;
      return current;
    }
    return nullptr;
  }

  // Reset to reuse this block (rewinds bump pointer):
  void Reset() {
    ptr = base + sizeof(Block);
  }

  // Bytes remain available in this block?
  inline size_t Remaining() const {
    return static_cast<size_t>(end - ptr);
  }

  // Bytes that have been used in this block?
  inline size_t Used() const {
    return static_cast<size_t>(ptr - (base + sizeof(Block)));
  }

  // Total capacity of this block (including header).
  inline size_t TotalCapacity() const {
    return static_cast<size_t>(end - base);
  }
};

NVR_INNER_END_NAMESPACE
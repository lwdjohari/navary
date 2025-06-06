// SPDX-License-Identifier: NAVARY EULA 1.0
// Author: Linggawasistha Djohari
// Bump-pointer slab-based Arena Allocator

#include "navary/memory/arena_allocator.h"

NVR_INNER_NAMESPACE(memory)

Arena::Arena(size_t initialBlockSize = 16 * 1024,
             size_t alignment = alignof(std::max_align_t))
                : alignment_(alignment),
                  initial_size_(initialBlockSize),
                  current_block_size_(initialBlockSize) {
  // Allocate the first block:
  Block* block = Block::Create(initialBlockSize, alignment_);
  //   assert(block && "Out of memory when creating first block");
  blocks_.push_back(block);

  // Update committed_bytes_ to include that first block’s total capacity:
  committed_bytes_ = block->TotalCapacity();
}

Arena::~Arena() {
  for (Block* b : blocks_) {
    b->Destroy();
  }
}

void Arena::Reset() {
  // Destroy all but blocks_[0]
  for (size_t i = 1; i < blocks_.size(); ++i) {
    blocks_[i]->Destroy();
  }

  blocks_.resize(1);
  blocks_[0]->Reset();

  // Reset counters to match a single block
  current_block_size_ = initial_size_;
  used_bytes_ = 0;
  committed_bytes_ = blocks_[0]->TotalCapacity();
}

void* Arena::Allocate(size_t n, ArenaErrCode& err) {
  // Round `n` up to the alignment boundary:
  size_t aligned_n = AlignUp(n, alignment_, err);

  if (err) {
    return nullptr;
  }

  // Try the current (last) block first:
  Block* last = blocks_.back();
  if (void* p = last->AllocateInBlock(aligned_n)) {
    if (!p) {
      err = ARENA_ERR::ARENA_ERR_ALLOCATION;
    }

    // increment used counter
    used_bytes_ += aligned_n;
    err = ARENA_ERR::OK;
    return p;
  }

  // Not enough room in the last block. Grow:
  Block* new_block = Grow(aligned_n, err);
  if (!new_block) {
    if (err)
      err = ARENA_ERR::ARENA_ERR_OOM;
    return nullptr;
  }
  // Now the new block is guaranteed to fit 1aligned_n` in its fresh
  // bump‐pointer:
  void* p = new_block->AllocateInBlock(aligned_n);
  //   assert(p && "New block should have enough room");

  if (!p) {
    err = ARENA_ERR::ARENA_ERR_ALLOCATION;
  }
  // increment used counter
  used_bytes_ += aligned_n;

  err = ARENA_ERR::OK;
  return p;
}

size_t Arena::Remaining() const {
  return blocks_.back()->Remaining();
}

// Total bytes used across all blocks (incrementally tracked).
size_t Arena::Size() const {
  return used_bytes_;
}

// Total bytes committed across all blocks (incrementally tracked).
size_t Arena::Capacity() const {
  return committed_bytes_;
}

// STATIC

size_t Arena::AlignUp(size_t n, size_t alignment, ArenaErrCode& err) {
  return (n + (alignment - 1)) & ~(alignment - 1);
}

// PRIVATE

Block* Arena::Grow(size_t min_size, ArenaErrCode& err) {
  // Decide the new block size: double until ≥ minSize
  size_t new_size = current_block_size_;
  while (new_size < min_size) {
    new_size <<= 1;
  }
  // (Optionally) cap newSize to some maximum, e.g. 1 MB:
  // newSize = std::min(newSize, static_cast<size_t>(1 << 20));

  Block* block = Block::Create(new_size, alignment_);
  if (!block)
    return nullptr;

  blocks_.push_back(block);
  current_block_size_ = new_size;

  // Update committed_bytes_ by this block’s total capacity
  committed_bytes_ += block->TotalCapacity();
  return block;
}

NVR_INNER_END_NAMESPACE
#pragma once

// navary/render/gpu_ring_buffer.h
// GPU ring buffer for dynamic buffer uploads.
// Purpose:
//   Provide a simple ring allocator for dynamic GPU buffer uploads.
//   Avoid per-draw allocations; keep a fixed-size buffer.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstddef>
#include <cstdint>

#include "navary/navary_status.h"
#include "navary/core/handles.h"

namespace navary::render::v1 {

struct BufferSlice {
  core::BufferHandle buffer;
  std::size_t offset;
  std::size_t size;
};

class GpuRingBuffer {
 public:
  GpuRingBuffer();
  ~GpuRingBuffer();

  NavaryRC Init(core::BufferHandle buffer, std::size_t size_bytes,
                std::uint8_t* mapped_ptr);

  NavaryResult<BufferSlice> AllocateAndWrite(const void* data,
                                               std::size_t size);

  core::BufferHandle handle() const {
    return handle_;
  }

  void ResetFrame();

 private:
  core::BufferHandle handle_;
  std::uint8_t* mapped_ptr_;
  std::size_t capacity_;
  std::size_t write_head_;
};

}  // namespace navary::render::v1

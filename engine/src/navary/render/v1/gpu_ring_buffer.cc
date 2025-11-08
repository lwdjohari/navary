// navary/render/v1/gpu_ring_buffer.cc
// Implementation of a GPU ring buffer for dynamic data uploads.
// This file is part of the Navary rendering engine.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/render/v1/gpu_ring_buffer.h"

#include <cstring>
#include "navary/navary_status.h"

namespace navary::render::v1 {

GpuRingBuffer::GpuRingBuffer()
    : handle_{0}, mapped_ptr_(nullptr), capacity_(0), write_head_(0) {}

GpuRingBuffer::~GpuRingBuffer() = default;

NavaryRC GpuRingBuffer::Init(core::BufferHandle buffer, std::size_t size_bytes,
                             std::uint8_t* mapped_ptr) {
  handle_     = buffer;
  capacity_   = size_bytes;
  mapped_ptr_ = mapped_ptr;
  write_head_ = 0;

  return NavaryRC::OK();
}

NavaryResult<BufferSlice> GpuRingBuffer::AllocateAndWrite(const void* data,
                                                            std::size_t size) {
  if (size > capacity_) {
    return NavaryResult<BufferSlice>(
        NavaryRC(NavaryStatus::kOutOfMemory,
                 "GpuRingBuffer: size larger than capacity"));
  }

  if (write_head_ + size > capacity_) {
    write_head_ = 0;  // simple wraparound; fences must ensure safety.
  }

  std::memcpy(mapped_ptr_ + write_head_, data, size);
  BufferSlice slice{handle_, write_head_, size};
  write_head_ += size;

  return NavaryResult<BufferSlice>(slice);
}

void GpuRingBuffer::ResetFrame() {
  // Optional: write_head_ = 0; if you want strict per-frame.
  write_head_ = 0;
}

}  // namespace navary::render::v1

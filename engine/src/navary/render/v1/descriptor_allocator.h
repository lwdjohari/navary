#pragma once

// navary/render/v1/descriptor_allocator.h
// Defines DescriptorAllocator interface for allocating and writing descriptor
// sets. Used by rendering system components. 
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

#include "navary/core/handles.h"
#include "navary/navary_status.h"

namespace navary::render::v1 {

class DescriptorAllocator {
 public:
  virtual ~DescriptorAllocator() = default;

  virtual NavaryResult<core::DescriptorHandle> Allocate(
      core::DescriptorSetLayoutHandle layout) = 0;

  virtual NavaryRC WriteImageSampler(core::DescriptorHandle set,
                                     std::uint32_t binding,
                                     core::TextureHandle texture) = 0;

  virtual NavaryRC WriteUniformBuffer(core::DescriptorHandle set,
                                      std::uint32_t binding,
                                      core::BufferHandle buffer,
                                      std::size_t offset,
                                      std::size_t range) = 0;
};

}  // namespace navary::render::v1

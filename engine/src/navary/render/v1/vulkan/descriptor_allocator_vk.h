#pragma once

// navary/render/v1/vulkan/descriptor_allocator_vk.h
// Vulkan implementation of DescriptorAllocator interface.
// Used by rendering system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>
#include <vulkan/vulkan.h>

#include "navary/navary_status.h"
#include "navary/core/handles.h"
#include "navary/render/v1/descriptor_allocator.h"

namespace navary::render::v1::vulkan {

// Simple mapping from handles to Vulkan resources.
// The engine owns and fills these arrays; allocator just uses them.
struct VulkanDescriptorResources {
  VkDevice device;
  VkDescriptorPool material_pool;
  const VkDescriptorSetLayout* layouts;
  std::uint32_t layouts_count;
  const VkImageView* texture_views;   // index by TextureHandle.index
  const VkSampler* texture_samplers;  // index by TextureHandle.index
  const VkBuffer* uniform_buffers;    // index by BufferHandle.index
  std::uint32_t texture_count;
  std::uint32_t buffer_count;
};

class DescriptorAllocatorVk : public DescriptorAllocator {
 public:
  DescriptorAllocatorVk();
  ~DescriptorAllocatorVk() override;

  NavaryRC Init(const VulkanDescriptorResources& resources,
                std::uint32_t max_descriptor_sets);

  NavaryResult<core::DescriptorHandle> Allocate(
      core::DescriptorSetLayoutHandle layout) override;

  NavaryRC WriteImageSampler(core::DescriptorHandle set, std::uint32_t binding,
                             core::TextureHandle texture) override;

  NavaryRC WriteUniformBuffer(core::DescriptorHandle set, std::uint32_t binding,
                              core::BufferHandle buffer, std::size_t offset,
                              std::size_t range) override;

  VkDescriptorSet GetVkSet(core::DescriptorHandle handle) const;

 private:
  struct Entry {
    VkDescriptorSet set;
    core::DescriptorSetLayoutHandle layout;
  };

  VulkanDescriptorResources resources_;
  Entry* entries_;
  std::uint32_t capacity_;
  std::uint32_t count_;
};

}  // namespace navary::render::v1::vulkan

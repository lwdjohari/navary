#pragma once

// navary/render/v1/vulkan/descriptor_resources.h
// Defines VulkanDescriptorResourceTable for managing Vulkan descriptor
// resources. Used by rendering system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>
#include <vulkan/vulkan.h>

#include "navary/navary_status.h"
#include "navary/core/handles.h"
#include "navary/render/v1/vulkan/descriptor_allocator_vk.h"

namespace navary::render::v1::vulkan {

// VulkanDescriptorResourceTable owns the arrays used by
// VulkanDescriptorResources. It provides registration functions for:
//  - descriptor set layouts -> DescriptorSetLayoutHandle
//  - textures (image view + sampler) -> TextureHandle
//  - uniform buffers -> BufferHandle
class VulkanDescriptorResourceTable {
 public:
  VulkanDescriptorResourceTable();
  ~VulkanDescriptorResourceTable();

  NavaryRC Init(VkDevice device, VkDescriptorPool material_pool,
                std::uint32_t max_layouts, std::uint32_t max_textures,
                std::uint32_t max_buffers);

  // Registers a layout. Returns a handle whose index matches the array index.
  NavaryResult<core::DescriptorSetLayoutHandle> RegisterLayout(
      VkDescriptorSetLayout layout);

  // Registers a texture view + sampler pair. Engine must have created them.
  NavaryResult<core::TextureHandle> RegisterTexture(VkImageView view,
                                                    VkSampler sampler);

  // Registers a uniform buffer. Engine must have created it.
  NavaryResult<core::BufferHandle> RegisterUniformBuffer(VkBuffer buffer);

  // Builds a VulkanDescriptorResources snapshot to pass into
  // DescriptorAllocatorVk::Init.
  VulkanDescriptorResources ToResources() const;

 private:
  VulkanDescriptorResources resources_;

  // Owned mutable arrays (non-const); resources_ points to them.
  VkDescriptorSetLayout* layouts_;
  VkImageView* texture_views_;
  VkSampler* texture_samplers_;
  VkBuffer* uniform_buffers_;

  std::uint32_t max_layouts_;
  std::uint32_t max_textures_;
  std::uint32_t max_buffers_;

  std::uint32_t layout_count_;
  std::uint32_t texture_count_;
  std::uint32_t buffer_count_;
};

}  // namespace navary::render::v1::vulkan

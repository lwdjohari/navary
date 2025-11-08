// navary/render/v1/vulkan/descriptor_resources_vk.cc
// Implementation of VulkanDescriptorResourceTable.
// This file is part of the Navary rendering engine.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/render/v1/vulkan/descriptor_resources_vk.h"

#include <cstdlib>
#include <cstring>

namespace navary::render::v1::vulkan {

VulkanDescriptorResourceTable::VulkanDescriptorResourceTable()
    : resources_{},
      layouts_(nullptr),
      texture_views_(nullptr),
      texture_samplers_(nullptr),
      uniform_buffers_(nullptr),
      max_layouts_(0),
      max_textures_(0),
      max_buffers_(0),
      layout_count_(0),
      texture_count_(0),
      buffer_count_(0) {}

VulkanDescriptorResourceTable::~VulkanDescriptorResourceTable() {
  std::free(layouts_);
  std::free(texture_views_);
  std::free(texture_samplers_);
  std::free(uniform_buffers_);
}

NavaryRC VulkanDescriptorResourceTable::Init(VkDevice device,
                                             VkDescriptorPool material_pool,
                                             std::uint32_t max_layouts,
                                             std::uint32_t max_textures,
                                             std::uint32_t max_buffers) {
  max_layouts_  = max_layouts;
  max_textures_ = max_textures;
  max_buffers_  = max_buffers;

  layouts_ = static_cast<VkDescriptorSetLayout*>(
      std::malloc(sizeof(VkDescriptorSetLayout) * max_layouts_));
  if (layouts_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "VulkanDescriptorResourceTable: layouts alloc failed");
  }

  texture_views_ = static_cast<VkImageView*>(
      std::malloc(sizeof(VkImageView) * max_textures_));
  if (texture_views_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "VulkanDescriptorResourceTable: views alloc failed");
  }

  texture_samplers_ =
      static_cast<VkSampler*>(std::malloc(sizeof(VkSampler) * max_textures_));
  if (texture_samplers_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "VulkanDescriptorResourceTable: samplers alloc failed");
  }

  uniform_buffers_ =
      static_cast<VkBuffer*>(std::malloc(sizeof(VkBuffer) * max_buffers_));
  if (uniform_buffers_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "VulkanDescriptorResourceTable: buffers alloc failed");
  }

  // Initialize resources_ pointers.
  resources_.device           = device;
  resources_.material_pool    = material_pool;
  resources_.layouts          = layouts_;
  resources_.layouts_count    = 0;
  resources_.texture_views    = texture_views_;
  resources_.texture_samplers = texture_samplers_;
  resources_.uniform_buffers  = uniform_buffers_;
  resources_.texture_count    = 0;
  resources_.buffer_count     = 0;

  layout_count_  = 0;
  texture_count_ = 0;
  buffer_count_  = 0;

  return NavaryRC::OK();
}

NavaryResult<core::DescriptorSetLayoutHandle>
VulkanDescriptorResourceTable::RegisterLayout(VkDescriptorSetLayout layout) {
  if (layout_count_ >= max_layouts_) {
    return NavaryResult<core::DescriptorSetLayoutHandle>(NavaryRC(
        NavaryStatus::kOutOfMemory, "RegisterLayout: layout array full"));
  }

  const std::uint32_t index = layout_count_++;
  layouts_[index]           = layout;
  resources_.layouts_count  = layout_count_;

  return NavaryResult<core::DescriptorSetLayoutHandle>(
      core::DescriptorSetLayoutHandle{index});
}

NavaryResult<core::TextureHandle>
VulkanDescriptorResourceTable::RegisterTexture(VkImageView view,
                                               VkSampler sampler) {
  if (texture_count_ >= max_textures_) {
    return NavaryResult<core::TextureHandle>(NavaryRC(
        NavaryStatus::kOutOfMemory, "RegisterTexture: texture array full"));
  }

  const std::uint32_t index = texture_count_++;
  texture_views_[index]     = view;
  texture_samplers_[index]  = sampler;
  resources_.texture_count  = texture_count_;

  return NavaryResult<core::TextureHandle>(core::TextureHandle{index});
}

NavaryResult<core::BufferHandle>
VulkanDescriptorResourceTable::RegisterUniformBuffer(VkBuffer buffer) {
  if (buffer_count_ >= max_buffers_) {
    return NavaryResult<core::BufferHandle>(
        NavaryRC(NavaryStatus::kOutOfMemory,
                 "RegisterUniformBuffer: buffer array full"));
  }

  const std::uint32_t index = buffer_count_++;
  uniform_buffers_[index]   = buffer;
  resources_.buffer_count   = buffer_count_;

  return NavaryResult<core::BufferHandle>(core::BufferHandle{index});
}

VulkanDescriptorResources VulkanDescriptorResourceTable::ToResources() const {
  VulkanDescriptorResources out = resources_;
  return out;
}

}  // namespace navary::render::v1::vulkan

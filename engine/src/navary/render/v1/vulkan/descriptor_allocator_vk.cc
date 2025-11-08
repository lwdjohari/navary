// navary/render/v1/vulkan/descriptor_allocator_vk.cc
// Implementation of Vulkan DescriptorAllocator.
// This file is part of the Navary rendering engine.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/render/v1/vulkan/descriptor_allocator_vk.h"

#include <cstdlib>

namespace navary::render::v1::vulkan {

DescriptorAllocatorVk::DescriptorAllocatorVk()
    : resources_{}, entries_(nullptr), capacity_(0), count_(0) {}

DescriptorAllocatorVk::~DescriptorAllocatorVk() {
  std::free(entries_);
}

NavaryRC DescriptorAllocatorVk::Init(const VulkanDescriptorResources& resources,
                                     std::uint32_t max_descriptor_sets) {
  resources_ = resources;
  capacity_  = max_descriptor_sets;
  entries_   = static_cast<Entry*>(std::malloc(sizeof(Entry) * capacity_));
  if (entries_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "DescriptorAllocatorVk: entries alloc failed");
  }

  count_ = 0;
  return NavaryRC::OK();
}

NavaryResult<core::DescriptorHandle> DescriptorAllocatorVk::Allocate(
    core::DescriptorSetLayoutHandle layout) {
  if (count_ >= capacity_) {
    return NavaryResult<core::DescriptorHandle>(
        NavaryRC(NavaryStatus::kOutOfMemory,
                 "DescriptorAllocatorVk: no free descriptor sets"));
  }

  if (layout.index >= resources_.layouts_count) {
    return NavaryResult<core::DescriptorHandle>(
        NavaryRC(NavaryStatus::kInvalidArgument,
                 "DescriptorAllocatorVk: invalid layout handle"));
  }

  VkDescriptorSetLayout vk_layout = resources_.layouts[layout.index];

  VkDescriptorSetAllocateInfo info{};
  info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool     = resources_.material_pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts        = &vk_layout;

  VkDescriptorSet vk_set = VK_NULL_HANDLE;
  VkResult res = vkAllocateDescriptorSets(resources_.device, &info, &vk_set);
  if (res != VK_SUCCESS) {
    return NavaryResult<core::DescriptorHandle>(
        NavaryRC(NavaryStatus::kInternal,
                 "DescriptorAllocatorVk: vkAllocateDescriptorSets failed"));
  }

  const std::uint32_t index = count_++;
  entries_[index].set       = vk_set;
  entries_[index].layout    = layout;

  return NavaryResult<core::DescriptorHandle>(core::DescriptorHandle{index});
}

VkDescriptorSet DescriptorAllocatorVk::GetVkSet(
    core::DescriptorHandle handle) const {
  if (handle.index >= count_) {
    return VK_NULL_HANDLE;
  }

  return entries_[handle.index].set;
}

NavaryRC DescriptorAllocatorVk::WriteImageSampler(core::DescriptorHandle handle,
                                                  std::uint32_t binding,
                                                  core::TextureHandle texture) {
  if (handle.index >= count_) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "WriteImageSampler: invalid descriptor handle");
  }

  if (texture.index >= resources_.texture_count) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "WriteImageSampler: invalid texture handle");
  }

  VkDescriptorSet vk_set = entries_[handle.index].set;

  VkDescriptorImageInfo img{};
  img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  img.imageView   = resources_.texture_views[texture.index];
  img.sampler     = resources_.texture_samplers[texture.index];

  VkWriteDescriptorSet write{};
  write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet          = vk_set;
  write.dstBinding      = binding;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo      = &img;

  vkUpdateDescriptorSets(resources_.device, 1, &write, 0, nullptr);
  return NavaryRC::OK();
}

NavaryRC DescriptorAllocatorVk::WriteUniformBuffer(
    core::DescriptorHandle handle, std::uint32_t binding,
    core::BufferHandle buffer, std::size_t offset, std::size_t range) {
  if (handle.index >= count_) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "WriteUniformBuffer: invalid descriptor handle");
  }

  if (buffer.index >= resources_.buffer_count) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "WriteUniformBuffer: invalid buffer handle");
  }

  VkDescriptorSet vk_set = entries_[handle.index].set;

  VkDescriptorBufferInfo buf{};
  buf.buffer = resources_.uniform_buffers[buffer.index];
  buf.offset = offset;
  buf.range  = range;

  VkWriteDescriptorSet write{};
  write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet          = vk_set;
  write.dstBinding      = binding;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.pBufferInfo     = &buf;

  vkUpdateDescriptorSets(resources_.device, 1, &write, 0, nullptr);

  return NavaryRC::OK();
}

}  // namespace navary::render::v1::vulkan

# Renderer usage of Texture Manager

```cpp
#include "navary/renders/v1/vulkan/descriptor_resources_vk.h"
#include "navary/render/v1/descriptor_allocator_vk.h"
#include "navary/render/v1/gpu_ring_buffer.h"
#include "navary/materials/v1/material_gpu.h"
#include "navary/materials/v1/material_registry.h"
#include "navary/textures/v1/texture_manager.h"

// Pseudocode inside your renderer init:

void InitNavaryMaterialSystem(VkDevice device,
                              VkDescriptorPool material_pool,
                              VkDescriptorSetLayout material_set_layout,
                              VkBuffer material_ubo_buffer,
                              std::uint8_t* material_ubo_mapped) {
  using namespace navary;

  VulkanDescriptorResourceTable resource_table;
  // Suppose we support up to 16 layouts, 1024 textures, 64 buffers.
  Status st = resource_table.Init(device, material_pool,
                                  /*max_layouts=*/16,
                                  /*max_textures=*/1024,
                                  /*max_buffers=*/64);
  // Check st.ok() in your engine.

  // Register layout for Set=1 (material layout).
  StatusOr<DescriptorSetLayoutHandle> layout_handle_or =
      resource_table.RegisterLayout(material_set_layout);
  DescriptorSetLayoutHandle material_set_layout_handle =
      layout_handle_or.value;

  // Register material UBO buffer.
  StatusOr<BufferHandle> material_ubo_buffer_handle_or =
      resource_table.RegisterUniformBuffer(material_ubo_buffer);
  BufferHandle material_ubo_buffer_handle = material_ubo_buffer_handle_or.value;

  // Build resources struct for allocator.
  VulkanDescriptorResources resources = resource_table.ToResources();

  // Create descriptor allocator (Vulkan).
  DescriptorAllocatorVk descriptor_alloc;
  descriptor_alloc.Init(resources, /*max_descriptor_sets=*/4096);

  // Create GPU ring buffer for MaterialUBO.
  GpuRingBuffer material_ubo_ring;
  material_ubo_ring.Init(material_ubo_buffer_handle,
                         /*size_bytes=*/1024 * 1024,  // example 1MB
                         material_ubo_mapped);

  // Context for materials.
  MaterialGpuContext mat_ctx;
  mat_ctx.descriptor_allocator = &descriptor_alloc;
  mat_ctx.material_set_layout = material_set_layout_handle;
  mat_ctx.material_ubo_ring = &material_ubo_ring;

  // Now when you create a Material in MaterialRegistry, you can call:
  //   AllocateAndWriteMaterialDescriptor(*material, &mat_ctx, &material->gpu.set1);
}

```
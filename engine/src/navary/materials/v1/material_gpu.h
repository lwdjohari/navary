#pragma once

// navary/materials/v1/material_gpu.h
// Defines GPU-related utilities for materials.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/navary_status.h"
#include "navary/materials/v1/material.h"
#include "navary/materials/v1/material_ubo.h"
#include "navary/render/v1/descriptor_allocator.h"
#include "navary/render/v1/gpu_ring_buffer.h"

namespace navary::materials::v1 {

namespace renderv1 = navary::render::v1;

struct MaterialGpuContext {
  renderv1::DescriptorAllocator* descriptor_allocator;
  DescriptorSetLayoutHandle material_set_layout;  // set=1
  renderv1::GpuRingBuffer* material_ubo_ring;
};

NavaryRC AllocateAndWriteMaterialDescriptor(const Material& material,
                                          MaterialGpuContext* ctx,
                                          DescriptorHandle* out_set1);

}  // namespace navary


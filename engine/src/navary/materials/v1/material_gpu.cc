// navary/materials/v1/material_gpu.cc
// Implementation of GPU utilities for materials.
// This file is part of the Navary engine.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/materials/v1/material_gpu.h"

namespace navary::materials::v1 {

NavaryRC AllocateAndWriteMaterialDescriptor(const Material& material,
                                            MaterialGpuContext* ctx,
                                            DescriptorHandle* out_set1) {
  if (ctx == nullptr || out_set1 == nullptr) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "AllocateAndWriteMaterialDescriptor: null arg");
  }

  NavaryResult<DescriptorHandle> set_or =
      ctx->descriptor_allocator->Allocate(ctx->material_set_layout);
  if (!set_or.status().ok()) {
    return set_or.status();
  }

  DescriptorHandle set1 = set_or.value();

  MaterialUbo ubo{};
  for (int i = 0; i < 4; ++i) {
    ubo.base_color[i] = material.params.base_color[i];
    ubo.params0[i]    = material.params.params0[i];
    ubo.params1[i]    = material.params.params1[i];
    ubo.user0[i]      = 0.0f;
    ubo.user1[i]      = 0.0f;
  }

  NavaryResult<renderv1::BufferSlice> slice_or =
      ctx->material_ubo_ring->AllocateAndWrite(&ubo, sizeof(MaterialUbo));
  if (!slice_or.status().ok()) {
    return slice_or.status();
  }

  renderv1::BufferSlice slice = slice_or.value();

  // Bind textures in fixed layout 0..7
  auto write_tex = [&](std::uint32_t binding, TextureHandle tex) -> NavaryRC {
    return ctx->descriptor_allocator->WriteImageSampler(set1, binding, tex);
  };

  NAVARY_RETURN_IF_ERROR(write_tex(0, material.textures.albedo));
  NAVARY_RETURN_IF_ERROR(write_tex(1, material.textures.normal));
  NAVARY_RETURN_IF_ERROR(write_tex(2, material.textures.orm));
  NAVARY_RETURN_IF_ERROR(write_tex(3, material.textures.emissive));
  NAVARY_RETURN_IF_ERROR(write_tex(4, material.textures.mask0));
  NAVARY_RETURN_IF_ERROR(write_tex(5, material.textures.mask1));
  NAVARY_RETURN_IF_ERROR(write_tex(6, material.textures.custom0));
  NAVARY_RETURN_IF_ERROR(write_tex(7, material.textures.custom1));

  // UBO at binding 8
  NAVARY_RETURN_IF_ERROR(ctx->descriptor_allocator->WriteUniformBuffer(
      set1, 8, slice.buffer, slice.offset, slice.size));

  *out_set1 = set1;
  return NavaryRC::OK();
}

}  // namespace navary::materials::v1

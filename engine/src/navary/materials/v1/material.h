#pragma once

// navary/materials/v1/material.h
// Defines Material struct for material instances.
// Used by material system components.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/core/handles.h"
#include "navary/materials/v1/material_types.h"

namespace navary::materials::v1 {

struct MaterialGpuData {
  DescriptorHandle set1;  // descriptor set for Set=1
};

struct Material {
  MaterialHandle handle;

  SurfaceModel surface_model;
  MaterialClass material_class;
  AlphaMode alpha_mode;

  ShaderKey shader_key;
  PipelineHandle pipeline;

  MaterialTextureSlots textures;
  MaterialParams params;

  MaterialGpuData gpu;

  bool receive_shadows;
  bool instanced;
  std::uint8_t vertex_skin;

  std::uint8_t streaming_priority;
  std::uint8_t streaming_group;
};

}  // namespace navary


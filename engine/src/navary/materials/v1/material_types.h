#pragma once

// navary/materials/v1/material_types.h
// Defines basic types and enums for material system.
// Used by multiple material system components.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/navary_status.h"
#include "navary/core/handles.h"

namespace navary::materials::v1 {

using ShaderHash                = navary::core::ShaderHash;
using TextureHandle             = navary::core::TextureHandle;
using GraphId                   = navary::core::GraphId;
using PipelineHandle            = navary::core::PipelineHandle;
using MaterialHandle            = navary::core::MaterialHandle;
using DescriptorHandle          = navary::core::DescriptorHandle;
using SurfaceModel              = navary::core::SurfaceModel;
using AlphaMode                 = navary::core::AlphaMode;
using MaterialClass             = navary::core::MaterialClass;
using ShaderKey                 = navary::core::ShaderKey;
using DescriptorSetLayoutHandle = navary::core::DescriptorSetLayoutHandle;

struct MaterialTextureSlots {
  TextureHandle albedo;
  TextureHandle normal;
  TextureHandle orm;
  TextureHandle emissive;
  TextureHandle mask0;
  TextureHandle mask1;
  TextureHandle custom0;
  TextureHandle custom1;
};

struct MaterialParams {
  float base_color[4];
  float params0[4];  // roughness, metallic, alphaCut, emissiveStrength
  float params1[4];  // e.g. rampIndex, outlineWidth, etc.
};

struct MaterialDesc {
  SurfaceModel surface_model;
  MaterialClass material_class;
  AlphaMode alpha_mode;

  GraphId graph_id;
  ShaderHash graph_hash;

  MaterialTextureSlots textures;
  MaterialParams params;

  bool receive_shadows;
  bool instanced;
  std::uint8_t vertex_skin;

  std::uint8_t streaming_priority;
  std::uint8_t streaming_group;
};

}  // namespace navary::materials::v1

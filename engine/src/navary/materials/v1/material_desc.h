#pragma once

// navary/materials/v1/material_desc.h
// Defines MaterialDesc struct for material description.
// Used by material system components.
// Author:
//   Linggawasistha Djohari  [2024-Present]

#include "navary/materials/v1/material_types.h"

namespace navary::materials::v1 {

// Fixed texture slots for materials.
// These correspond to Set 1 bindings in shaders.
// Use dummy 1x1 textures when a slot is unused.
// Keep this layout stable across pipelines and shaders.
struct MaterialTextureSlots {
  // Fixed layout for Set 1 slots.
  TextureHandle albedo;
  TextureHandle normal;
  TextureHandle orm;
  TextureHandle emissive;
  TextureHandle mask0;
  TextureHandle mask1;
  TextureHandle custom0;
  TextureHandle custom1;
  // NOTE: Use dummy 1x1 texture handles when missing.
};

// Generic material parameters.
// Specific surface models can interpret params differently.
// E.g., stylized model can use params1.x as ramp index.
// Keep this struct tightly packed.
// Align to 16 bytes for UBO compatibility.
// Total size: 32 bytes.
// Alignas(16)
// ENGINE NOTE: Alignas to ensure proper UBO alignment.
// This is important for correct data layout in GPU memory.
struct MaterialParams {
  float base_color[4];
  float params0[4];  // generic: roughness, metallic, alpha_cut,
                     // emissive_strength, etc.
  float params1[4];  // per-surface-model (e.g. rampIndex, outlineWidth).
};

// Material description struct.
// Used for defining material properties in asset files and at runtime.
struct MaterialDesc {
  SurfaceModel surface_model;
  MaterialClass material_class;
  AlphaMode alpha_mode;

  GraphId graph_id;       // Reference to .navgraph asset.
  ShaderHash graph_hash;  // Hash for ShaderKey.

  MaterialTextureSlots textures;
  MaterialParams params;

  bool receive_shadows;
  bool instanced;
  uint8_t vertex_skin;

  // Streaming hints.
  uint8_t streaming_priority;  // 0 = lowest, 255 = highest.
  uint8_t streaming_group;     // e.g., hero / squad / prop.
};

}  // namespace navary::materials::v1

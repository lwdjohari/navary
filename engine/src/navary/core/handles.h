#pragma once

// navary/core/handles.h
// Defines core handle types for various engine resources.
// Used across multiple engine components.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

namespace navary::core {

enum class SurfaceModel : std::uint8_t {
  kLitePbr  = 0,
  kStylized = 1,
  kUnlit    = 2,
};

enum class AlphaMode : std::uint8_t {
  kOpaque      = 0,
  kAlphaCutout = 1,
  kTransparent = 2,
};

enum class MaterialClass : std::uint8_t {
  kOpaquePbr   = 0,
  kAlphaCutPbr = 1,
  kTransparent = 2,
  kUnlit       = 3,
};

struct MaterialHandle {
  std::uint32_t index;
};

struct TextureHandle {
  std::uint32_t index;
};

struct DescriptorHandle {
  std::uint32_t index;
};

struct DescriptorSetLayoutHandle {
  std::uint32_t index;
};

struct PipelineHandle {
  std::uint32_t index;
};

struct BufferHandle {
  std::uint32_t index;
};

struct GraphId {
  std::uint32_t index;
};

struct ShaderHash {
  std::uint32_t value;
};

struct ShaderKey {
  SurfaceModel surface_model;
  MaterialClass material_class;
  AlphaMode alpha_mode;
  bool receive_shadows;
  bool instanced;
  std::uint8_t vertex_skin;
  ShaderHash graph_hash;

  bool operator==(const ShaderKey& other) const {
    return surface_model == other.surface_model &&
           material_class == other.material_class &&
           alpha_mode == other.alpha_mode &&
           receive_shadows == other.receive_shadows &&
           instanced == other.instanced && vertex_skin == other.vertex_skin &&
           graph_hash.value == other.graph_hash.value;
  }
};

}  // namespace navary::core

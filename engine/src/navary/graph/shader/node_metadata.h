#pragma once

// navary/graph/shader/node_metadata.h
// Defines NodeMetadata struct for shader node metadata.
// Used by shader graph processing and validation.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>
#include "navary/core/handles.h"
#include "navary/graph/shader/navgraph_binary.h"

namespace navary::graph::shader {

struct NodeMetadata {
  NodeKind kind;
  const char* name;
  const char* category;
  std::uint8_t min_inputs;
  std::uint8_t max_inputs;
  std::uint8_t min_outputs;
  std::uint8_t max_outputs;
  std::uint16_t param_count;
  std::uint8_t
      allowed_surface_models_mask;  // bit0=LitePbr, bit1=Stylized, bit2=Unlit

  std::uint16_t cost_texture_samples;
  std::uint16_t cost_normal_ops;
  std::uint16_t cost_math_ops;
  std::uint16_t cost_branches;
  std::uint16_t cost_custom_ops;
};

inline std::uint8_t SurfaceModelToMaskBit(core::SurfaceModel m) {
  switch (m) {
    case core::SurfaceModel::kLitePbr:
      return 1u << 0;
    case core::SurfaceModel::kStylized:
      return 1u << 1;
    case core::SurfaceModel::kUnlit:
      return 1u << 2;
  }
  return 0;
}

extern const NodeMetadata kNodeMetadataTable[];
extern const std::uint32_t kNodeMetadataCount;

const NodeMetadata* FindNodeMetadata(NodeKind kind);

}  // namespace navary::graph::shader
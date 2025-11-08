#pragma once

// navary/graph/shader/navgraph_binary.h
// Defines binary file format for NavGraph shader graphs.
// Used by NavGraph serialization and deserialization.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>
#include "navary/core/handles.h"

namespace navary::graph::shader {

constexpr std::uint32_t kNavGraphMagic = 0x4E415647u;  // 'NAVG'

struct NavGraphHeader {
  std::uint32_t magic;    // kNavGraphMagic
  std::uint16_t version;  // 1
  std::uint16_t reserved0;
  std::uint32_t file_size;  // total bytes
  std::uint32_t node_count;
  std::uint32_t pin_count;
  std::uint32_t param_value_count;  // num float entries
  std::uint32_t string_table_size;  // bytes
  std::uint8_t surface_model;       // SurfaceModel as uint8
  std::uint8_t reserved1[3];
};

static_assert(sizeof(NavGraphHeader) == 32, "NavGraphHeader must be 32 bytes");

// Node kinds are the serialized node types coming from the editor.
enum class NodeKind : std::uint16_t {
  kTextureSample2D = 0,
  kMultiply        = 1,
  kAdd             = 2,
  kLerp            = 3,
  kSaturate        = 4,
  kPow             = 5,
  kNormalBlend     = 6,
  kMakeSurface     = 7,
  kConstFloat      = 8,
  kConstFloat2     = 9,
  kConstFloat3     = 10,
  kConstFloat4     = 11,
  kParamFloat      = 12,
  kParamColor      = 13,
  kRampSample      = 14,
  kCustomCode      = 15,
};

enum class ValueTypeBinary : std::uint8_t {
  kFloat1 = 0,
  kFloat2 = 1,
  kFloat3 = 2,
  kFloat4 = 3,
};

enum class PinDirectionBinary : std::uint8_t {
  kInput  = 0,
  kOutput = 1,
};

// NodeRecord describes one node in the binary file.
struct NavGraphNodeRecord {
  std::uint32_t id;                // NodeId.index
  std::uint16_t kind;              // NodeKind
  std::uint16_t input_count;       // number of input pins
  std::uint16_t output_count;      // number of output pins
  std::uint16_t first_input_pin;   // index into pin table
  std::uint16_t first_output_pin;  // index into pin table
  std::uint16_t param_offset;      // index into param_value_block (in floats)
  std::uint16_t param_count;       // number of float params
};

static_assert(sizeof(NavGraphNodeRecord) == 20,
              "NavGraphNodeRecord must be 20 bytes");

// PinRecord describes a pin and its connection in the binary.
struct NavGraphPinRecord {
  std::uint32_t id;           // PinId.index
  std::uint32_t node_id;      // NodeId.index
  std::uint16_t type;         // ValueTypeBinary
  std::uint8_t direction;     // PinDirectionBinary
  std::uint8_t reserved0;     // padding
  std::uint32_t link_pin_id;  // PinId.index of connected pin, or 0xFFFFFFFF
};

static_assert(sizeof(NavGraphPinRecord) == 16,
              "NavGraphPinRecord must be 16 bytes");

}  // namespace navary
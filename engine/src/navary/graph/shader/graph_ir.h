#pragma once

// navary/graph/shader/graph_ir.h
// Defines GraphIr structure for intermediate representation of shader graphs.
// Used by shader graph compilation and processing.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>
#include "navary/core/handles.h"

namespace navary::graph::shader {

enum class ValueType : std::uint8_t {
  kFloat1 = 0,
  kFloat2,
  kFloat3,
  kFloat4,
};

// NodeId and PinId are indices into GraphIr arrays.
struct NodeId {
  std::uint32_t index;
};

// PinId represents an input or output pin on a node.
struct PinId {
  std::uint32_t index;
};

// Node operations in the shader graph.
enum class NodeOp : std::uint8_t {
  kTextureSample2D = 0,  // texture2D()
  kAdd,                  // +
  kMul,                  // *
  kLerp,                 // mix()
  kSaturate,             // clamp(0,1)
  kPow,                  // pow()
  kNormalBlend,          // blend normals
  kMakeSurface,          // construct surface output
  kConst,                // constant float(s)
  kParam,                // parameter float(s)
  kRampSample,           // sample ramp texture
  kCustomCode,           // custom shader code
};

// Pin represents an input or output on a node.
struct Pin {
  PinId id;        // PinId.index
  NodeId node;     // NodeId.index of parent node
  ValueType type;  // data type
  bool is_output;  // true = output pin, false = input pin
  PinId link;      // PinId of connected pin; link.index == UINT32_MAX -> none
};

// Node represents a shader operation with inputs, outputs, and parameters.
struct Node {
  NodeId id;
  NodeOp op;

  PinId inputs[4];
  std::uint8_t input_count;

  PinId outputs[2];
  std::uint8_t output_count;

  std::uint16_t param_offset;  // into GraphIr::param_values
  std::uint16_t param_count;   // floats
};

// GraphIr is the intermediate representation of a shader graph.
struct GraphIr {
  core::SurfaceModel surface_model;

  Node* nodes;
  std::uint32_t node_count;

  Pin* pins;
  std::uint32_t pin_count;

  float* param_values;
  std::uint32_t param_value_count;
};

}  // namespace navary
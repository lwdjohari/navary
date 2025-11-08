// navary/graph/shader/navgraph_loader.cc
// Implementation of NavGraphLoader for loading shader graphs from binary data.
// Used by shader graph processing and material system.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/graph/shader/navgraph_loader.h"

#include <cstring>
#include <new>  // std::nothrow

namespace navary::graph::shader {

NavGraphLoader::NavGraphLoader() = default;

NavaryRC NavGraphLoader::MapNodeKindToNodeOp(NodeKind kind, NodeOp* out) const {
  switch (kind) {
    case NodeKind::kTextureSample2D:
      *out = NodeOp::kTextureSample2D;
      return NavaryRC::OK();
    case NodeKind::kMultiply:
      *out = NodeOp::kMul;
      return NavaryRC::OK();
    case NodeKind::kAdd:
      *out = NodeOp::kAdd;
      return NavaryRC::OK();
    case NodeKind::kLerp:
      *out = NodeOp::kLerp;
      return NavaryRC::OK();
    case NodeKind::kSaturate:
      *out = NodeOp::kSaturate;
      return NavaryRC::OK();
    case NodeKind::kPow:
      *out = NodeOp::kPow;
      return NavaryRC::OK();
    case NodeKind::kNormalBlend:
      *out = NodeOp::kNormalBlend;
      return NavaryRC::OK();
    case NodeKind::kMakeSurface:
      *out = NodeOp::kMakeSurface;
      return NavaryRC::OK();
    case NodeKind::kConstFloat:
    case NodeKind::kConstFloat2:
    case NodeKind::kConstFloat3:
    case NodeKind::kConstFloat4:
      *out = NodeOp::kConst;
      return NavaryRC::OK();
    case NodeKind::kParamFloat:
    case NodeKind::kParamColor:
      *out = NodeOp::kParam;
      return NavaryRC::OK();
    case NodeKind::kRampSample:
      *out = NodeOp::kRampSample;
      return NavaryRC::OK();
    case NodeKind::kCustomCode:
      *out = NodeOp::kCustomCode;
      return NavaryRC::OK();
  }

  return NavaryRC(NavaryStatus::kInvalidArgument, "Unknown NodeKind");
}

NavaryRC NavGraphLoader::LoadFromMemory(const std::uint8_t* data,
                                        std::uint32_t size, GraphIr* out) {
  if (data == nullptr || out == nullptr) {
    return NavaryRC(NavaryStatus::kInvalidArgument,
                    "NavGraphLoader: null argument");
  }
  if (size < sizeof(NavGraphHeader)) {
    return NavaryRC(NavaryStatus::kParseError,
                    "NavGraphLoader: buffer too small");
  }

  const NavGraphHeader* header = reinterpret_cast<const NavGraphHeader*>(data);
  if (header->magic != kNavGraphMagic) {
    return NavaryRC(NavaryStatus::kParseError, "NavGraphLoader: bad magic");
  }
  if (header->version != 1u) {
    return NavaryRC(NavaryStatus::kParseError,
                    "NavGraphLoader: unsupported version");
  }
  if (header->file_size != size) {
    return NavaryRC(NavaryStatus::kParseError, "NavGraphLoader: size mismatch");
  }

  GraphIr ir{};
  ir.surface_model = static_cast<core::SurfaceModel>(header->surface_model);

  const std::uint8_t* cursor = data + sizeof(NavGraphHeader);
  const std::uint8_t* end    = data + size;

  const std::uint32_t node_count        = header->node_count;
  const std::uint32_t pin_count         = header->pin_count;
  const std::uint32_t param_value_count = header->param_value_count;

  const std::size_t node_bytes =
      static_cast<std::size_t>(node_count) * sizeof(NavGraphNodeRecord);
  const std::size_t pin_bytes =
      static_cast<std::size_t>(pin_count) * sizeof(NavGraphPinRecord);
  const std::size_t param_bytes =
      static_cast<std::size_t>(param_value_count) * sizeof(float);

  if (cursor + node_bytes + pin_bytes + param_bytes > end) {
    return NavaryRC(NavaryStatus::kParseError,
                    "NavGraphLoader: truncated data");
  }

  const NavGraphNodeRecord* node_records =
      reinterpret_cast<const NavGraphNodeRecord*>(cursor);
  cursor += node_bytes;

  const NavGraphPinRecord* pin_records =
      reinterpret_cast<const NavGraphPinRecord*>(cursor);
  cursor += pin_bytes;

  const float* param_values = reinterpret_cast<const float*>(cursor);
  cursor += param_bytes;

  // string table at cursor; can be ignored by runtime
  (void)cursor;

  Node* nodes   = new (std::nothrow) Node[node_count];
  Pin* pins     = new (std::nothrow) Pin[pin_count];
  float* params = new (std::nothrow) float[param_value_count];

  if (nodes == nullptr || pins == nullptr || params == nullptr) {
    delete[] nodes;
    delete[] pins;
    delete[] params;
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "NavGraphLoader: allocation failure");
  }

  std::memcpy(params, param_values, param_bytes);

  // pins
  for (std::uint32_t i = 0; i < pin_count; ++i) {
    const NavGraphPinRecord& src = pin_records[i];
    Pin& dst                     = pins[i];
    dst.id                       = PinId{src.id};
    dst.node                     = NodeId{src.node_id};
    dst.type                     = static_cast<ValueType>(src.type);
    dst.is_output                = (src.direction ==
                     static_cast<std::uint8_t>(PinDirectionBinary::kOutput));
    if (src.link_pin_id == 0xFFFFFFFFu) {
      dst.link = PinId{0xFFFFFFFFu};
    } else {
      dst.link = PinId{src.link_pin_id};
    }
  }

  // nodes
  for (std::uint32_t i = 0; i < node_count; ++i) {
    const NavGraphNodeRecord& src = node_records[i];
    Node& dst                     = nodes[i];
    dst.id                        = NodeId{src.id};

    NodeOp op;
    Status st = MapNodeKindToNodeOp(static_cast<NodeKind>(src.kind), &op);
    if (!st.ok()) {
      delete[] nodes;
      delete[] pins;
      delete[] params;
      return st;
    }
    dst.op = op;

    dst.input_count  = static_cast<std::uint8_t>(src.input_count);
    dst.output_count = static_cast<std::uint8_t>(src.output_count);

    for (std::uint32_t j = 0; j < dst.input_count && j < 4u; ++j) {
      dst.inputs[j] =
          PinId{static_cast<std::uint32_t>(src.first_input_pin + j)};
    }
    for (std::uint32_t j = 0; j < dst.output_count && j < 2u; ++j) {
      dst.outputs[j] =
          PinId{static_cast<std::uint32_t>(src.first_output_pin + j)};
    }

    dst.param_offset = src.param_offset;
    dst.param_count  = src.param_count;
  }

  ir.nodes             = nodes;
  ir.node_count        = node_count;
  ir.pins              = pins;
  ir.pin_count         = pin_count;
  ir.param_values      = params;
  ir.param_value_count = param_value_count;

  *out = ir;
  return NavaryRC::OK();
}

}  // namespace navary::graph::shader

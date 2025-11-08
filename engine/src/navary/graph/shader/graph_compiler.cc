// navary/graph/shader/graph_compiler.cc
// Implementation of GraphCompiler for compiling shader graphs to GLSL.
// Used by material system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/graph/shader/graph_compiler.h"

#include <new>      // std::nothrow
#include <sstream>  // std::ostringstream

namespace navary::graph::shader {

GraphCompiler::GraphCompiler() = default;

std::string GraphCompiler::EmitValueType(ValueType type) const {
  switch (type) {
    case ValueType::kFloat1:
      return "float";
    case ValueType::kFloat2:
      return "vec2";
    case ValueType::kFloat3:
      return "vec3";
    case ValueType::kFloat4:
      return "vec4";
  }
  return "float";
}

void GraphCompiler::EmitConstNode(const GraphIr& ir, const Node& node,
                                  std::string* out,
                                  std::string* pin_exprs) const {
  if (node.output_count == 0) {
    return;
  }
  const PinId out_pin_id        = node.outputs[0];
  const std::uint32_t pin_index = out_pin_id.index;
  const ValueType out_type      = ir.pins[pin_index].type;

  std::ostringstream expr;
  if (out_type == ValueType::kFloat1 && node.param_count >= 1u) {
    expr << ir.param_values[node.param_offset + 0];
  } else if (out_type == ValueType::kFloat2 && node.param_count >= 2u) {
    expr << "vec2(" << ir.param_values[node.param_offset + 0] << ", "
         << ir.param_values[node.param_offset + 1] << ")";
  } else if (out_type == ValueType::kFloat3 && node.param_count >= 3u) {
    expr << "vec3(" << ir.param_values[node.param_offset + 0] << ", "
         << ir.param_values[node.param_offset + 1] << ", "
         << ir.param_values[node.param_offset + 2] << ")";
  } else if (out_type == ValueType::kFloat4 && node.param_count >= 4u) {
    expr << "vec4(" << ir.param_values[node.param_offset + 0] << ", "
         << ir.param_values[node.param_offset + 1] << ", "
         << ir.param_values[node.param_offset + 2] << ", "
         << ir.param_values[node.param_offset + 3] << ")";
  } else {
    expr << "0.0";
  }
  pin_exprs[pin_index] = expr.str();
}

void GraphCompiler::EmitParamNode(const GraphIr& ir, const Node& node,
                                  std::string* out,
                                  std::string* pin_exprs) const {
  (void)out;
  if (node.output_count == 0 || node.param_count < 1u) {
    return;
  }
  const PinId out_pin_id        = node.outputs[0];
  const std::uint32_t pin_index = out_pin_id.index;
  const ValueType out_type      = ir.pins[pin_index].type;
  const float index_f           = ir.param_values[node.param_offset + 0];
  const int param_index         = static_cast<int>(index_f + 0.5f);

  std::string expr;

  // Convention:
  //  0: uBaseColor
  //  1: uParams0.x
  //  2: uParams0.y
  //  3: uParams0.z
  //  4: uParams0.w
  //  5: uParams1.x
  //  6: uParams1.y
  //  7: uParams1.z
  //  8: uParams1.w
  if (out_type == ValueType::kFloat4 && param_index == 0) {
    expr = "uBaseColor";
  } else if (out_type == ValueType::kFloat1) {
    switch (param_index) {
      case 1:
        expr = "uParams0.x";
        break;
      case 2:
        expr = "uParams0.y";
        break;
      case 3:
        expr = "uParams0.z";
        break;
      case 4:
        expr = "uParams0.w";
        break;
      case 5:
        expr = "uParams1.x";
        break;
      case 6:
        expr = "uParams1.y";
        break;
      case 7:
        expr = "uParams1.z";
        break;
      case 8:
        expr = "uParams1.w";
        break;
      default:
        expr = "0.0";
        break;
    }
  } else {
    expr = "0.0";
  }

  pin_exprs[pin_index] = expr;
}

void GraphCompiler::EmitTextureSampleNode(const GraphIr& ir, const Node& node,
                                          std::string* out,
                                          std::string* pin_exprs) const {
  if (node.output_count == 0 || node.input_count == 0 ||
      node.param_count < 1u) {
    return;
  }
  const PinId uv_pin_id        = node.inputs[0];
  const std::uint32_t uv_index = uv_pin_id.index;
  const std::string& uv_expr   = pin_exprs[uv_index];

  const PinId out_pin_id        = node.outputs[0];
  const std::uint32_t out_index = out_pin_id.index;

  const float slot_f = ir.param_values[node.param_offset + 0];
  const int slot     = static_cast<int>(slot_f + 0.5f);

  const char* tex_name = "uAlbedoTex";
  switch (slot) {
    case 0:
      tex_name = "uAlbedoTex";
      break;
    case 1:
      tex_name = "uNormalTex";
      break;
    case 2:
      tex_name = "uOrmTex";
      break;
    case 3:
      tex_name = "uEmissiveTex";
      break;
    case 4:
      tex_name = "uMask0Tex";
      break;
    case 5:
      tex_name = "uMask1Tex";
      break;
    case 6:
      tex_name = "uCustom0Tex";
      break;
    case 7:
      tex_name = "uCustom1Tex";
      break;
    default:
      break;
  }

  std::ostringstream line;
  line << "  vec4 v" << out_index << " = texture(" << tex_name << ", "
       << uv_expr << ");\n";
  *out += line.str();
  pin_exprs[out_index] = "v" + std::to_string(out_index);
}

void GraphCompiler::EmitBinaryNode(const GraphIr& ir, const Node& node,
                                   const char* op_symbol, std::string* out,
                                   std::string* pin_exprs) const {
  (void)out;
  if (node.input_count < 2u || node.output_count == 0) {
    return;
  }
  const std::uint32_t a_index   = node.inputs[0].index;
  const std::uint32_t b_index   = node.inputs[1].index;
  const std::uint32_t out_index = node.outputs[0].index;

  const std::string& a_expr = pin_exprs[a_index];
  const std::string& b_expr = pin_exprs[b_index];

  std::ostringstream expr;
  expr << "(" << a_expr << " " << op_symbol << " " << b_expr << ")";
  pin_exprs[out_index] = expr.str();
}

void GraphCompiler::EmitLerpNode(const GraphIr& ir, const Node& node,
                                 std::string* out,
                                 std::string* pin_exprs) const {
  (void)out;
  if (node.input_count < 3u || node.output_count == 0) {
    return;
  }
  const std::uint32_t a_index   = node.inputs[0].index;
  const std::uint32_t b_index   = node.inputs[1].index;
  const std::uint32_t t_index   = node.inputs[2].index;
  const std::uint32_t out_index = node.outputs[0].index;

  const std::string& a_expr = pin_exprs[a_index];
  const std::string& b_expr = pin_exprs[b_index];
  const std::string& t_expr = pin_exprs[t_index];

  std::ostringstream expr;
  expr << "mix(" << a_expr << ", " << b_expr << ", " << t_expr << ")";
  pin_exprs[out_index] = expr.str();
}

void GraphCompiler::EmitSaturateNode(const GraphIr& ir, const Node& node,
                                     std::string* out,
                                     std::string* pin_exprs) const {
  (void)out;
  if (node.input_count < 1u || node.output_count == 0u) {
    return;
  }
  const std::uint32_t in_index  = node.inputs[0].index;
  const std::uint32_t out_index = node.outputs[0].index;

  const std::string& in_expr = pin_exprs[in_index];

  std::ostringstream expr;
  expr << "clamp(" << in_expr << ", 0.0, 1.0)";
  pin_exprs[out_index] = expr.str();
}

void GraphCompiler::EmitPowNode(const GraphIr& ir, const Node& node,
                                std::string* out,
                                std::string* pin_exprs) const {
  (void)out;
  if (node.input_count < 2u || node.output_count == 0u) {
    return;
  }
  const std::uint32_t x_index   = node.inputs[0].index;
  const std::uint32_t y_index   = node.inputs[1].index;
  const std::uint32_t out_index = node.outputs[0].index;

  const std::string& x_expr = pin_exprs[x_index];
  const std::string& y_expr = pin_exprs[y_index];

  std::ostringstream expr;
  expr << "pow(" << x_expr << ", " << y_expr << ")";
  pin_exprs[out_index] = expr.str();
}

void GraphCompiler::EmitNormalBlendNode(const GraphIr& ir, const Node& node,
                                        std::string* out,
                                        std::string* pin_exprs) const {
  if (node.input_count < 2u || node.output_count == 0u) {
    return;
  }
  const std::uint32_t base_index   = node.inputs[0].index;
  const std::uint32_t detail_index = node.inputs[1].index;
  const std::uint32_t out_index    = node.outputs[0].index;

  const std::string& base_expr   = pin_exprs[base_index];
  const std::string& detail_expr = pin_exprs[detail_index];

  // Simple RNM-like normal blend.
  std::ostringstream line;
  line << "  vec3 nA" << out_index << " = " << base_expr << " * 2.0 - 1.0;\n";
  line << "  vec3 nB" << out_index << " = " << detail_expr << " * 2.0 - 1.0;\n";
  line << "  vec3 nC" << out_index << " = normalize(vec3(nA" << out_index
       << ".xy + nB" << out_index << ".xy, nA" << out_index << ".z * nB"
       << out_index << ".z));\n";
  line << "  vec3 v" << out_index << " = nC" << out_index << " * 0.5 + 0.5;\n";
  *out += line.str();
  pin_exprs[out_index] = "v" + std::to_string(out_index);
}

void GraphCompiler::EmitRampSampleNode(const GraphIr& ir, const Node& node,
                                       std::string* out,
                                       std::string* pin_exprs) const {
  if (node.input_count < 1u || node.output_count == 0u ||
      node.param_count < 2u) {
    return;
  }
  const std::uint32_t ndotl_index = node.inputs[0].index;
  const std::uint32_t out_index   = node.outputs[0].index;

  const std::string& ndotl_expr = pin_exprs[ndotl_index];

  const float slot_f = ir.param_values[node.param_offset + 0];
  const int slot     = static_cast<int>(slot_f + 0.5f);
  const float bands  = ir.param_values[node.param_offset + 1];

  const char* tex_name = "uCustom0Tex";
  if (slot == 7) {
    tex_name = "uCustom1Tex";
  }

  std::ostringstream line;
  line << "  float ndotl" << out_index << " = clamp(" << ndotl_expr
       << ", 0.0, 1.0);\n";
  line << "  float t" << out_index << " = clamp(ndotl" << out_index << " * "
       << bands << ", 0.0, 1.0);\n";
  line << "  vec3 v" << out_index << " = texture(" << tex_name << ", vec2(t"
       << out_index << ", 0.0)).rgb;\n";
  *out += line.str();
  pin_exprs[out_index] = "v" + std::to_string(out_index);
}

void GraphCompiler::EmitMakeSurfaceNode(const GraphIr& ir, const Node& node,
                                        std::string* out,
                                        std::string* pin_exprs) const {
  if (node.input_count < 4u) {
    return;
  }

  auto pin_expr = [&](std::uint32_t idx) -> const std::string& {
    static const std::string empty = "0.0";
    if (idx >= ir.pin_count)
      return empty;
    return pin_exprs[idx];
  };

  const std::uint32_t base_index   = node.inputs[0].index;
  const std::uint32_t normal_index = node.inputs[1].index;
  const std::uint32_t rough_index  = node.inputs[2].index;
  const std::uint32_t metal_index  = node.inputs[3].index;
  const std::uint32_t emissive_index =
      (node.input_count > 4u) ? node.inputs[4].index : 0u;
  const std::uint32_t emissive_strength_index =
      (node.input_count > 5u) ? node.inputs[5].index : 0u;
  const std::uint32_t alpha_index =
      (node.input_count > 6u) ? node.inputs[6].index : 0u;

  std::ostringstream code;
  code << "  surf.base_color = " << pin_expr(base_index) << ".rgb;\n";
  code << "  surf.normal_ws = normalize(" << pin_expr(normal_index)
       << ".xyz * 2.0 - 1.0);\n";
  code << "  surf.roughness = " << pin_expr(rough_index) << ".x;\n";
  code << "  surf.metallic = " << pin_expr(metal_index) << ".x;\n";
  if (node.input_count > 4u) {
    code << "  surf.emissive_color = " << pin_expr(emissive_index) << ".rgb;\n";
  } else {
    code << "  surf.emissive_color = vec3(0.0);\n";
  }
  if (node.input_count > 5u) {
    code << "  surf.emissive_strength = " << pin_expr(emissive_strength_index)
         << ".x;\n";
  } else {
    code << "  surf.emissive_strength = 0.0;\n";
  }
  if (node.input_count > 6u) {
    code << "  surf.alpha = " << pin_expr(alpha_index) << ".x;\n";
  } else {
    code << "  surf.alpha = 1.0;\n";
  }

  *out += code.str();
}

NavaryResult<std::string> GraphCompiler::GenerateUserSurfaceGlSl(
    const GraphIr& ir) const {
  // One string per pin ID for expressions (e.g. "v12", "0.5", "mix(...)").
  std::string* pin_exprs = nullptr;
  if (ir.pin_count > 0u) {
    pin_exprs = new (std::nothrow) std::string[ir.pin_count];
    if (pin_exprs == nullptr) {
      return NavaryResult<std::string>(NavaryRC(
          NavaryStatus::kOutOfMemory, "GraphCompiler: pin_exprs alloc failed"));
    }
  }

  std::string body;
  body.reserve(4096);

  // Convention: first node that is kMakeSurface finalizes surf.
  for (std::uint32_t i = 0; i < ir.node_count; ++i) {
    const Node& node = ir.nodes[i];
    switch (node.op) {
      case NodeOp::kConst:
        EmitConstNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kParam:
        EmitParamNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kTextureSample2D:
        EmitTextureSampleNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kAdd:
        EmitBinaryNode(ir, node, "+", &body, pin_exprs);
        break;
      case NodeOp::kMul:
        EmitBinaryNode(ir, node, "*", &body, pin_exprs);
        break;
      case NodeOp::kLerp:
        EmitLerpNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kSaturate:
        EmitSaturateNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kPow:
        EmitPowNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kNormalBlend:
        EmitNormalBlendNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kRampSample:
        EmitRampSampleNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kMakeSurface:
        EmitMakeSurfaceNode(ir, node, &body, pin_exprs);
        break;
      case NodeOp::kCustomCode:
        // Intentionally left as no-op; custom integration can insert hooks.
        break;
    }
  }

  delete[] pin_exprs;

  std::string src;
  src.reserve(4096);
  src +=
      "void Navary_UserSurface(in SurfaceInputs IN, inout SurfaceData surf) "
      "{\n";
  src += body;
  src += "}\n";

  return NavaryResult<std::string>(src);
}

}  // namespace navary::graph::shader

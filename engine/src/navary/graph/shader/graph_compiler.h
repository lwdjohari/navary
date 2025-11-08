#pragma once

// navary/graph/shader/graph_compiler.h
// Defines GraphCompiler class for compiling shader graphs to GLSL code.
// Used by material system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <string>

#include "navary/navary_status.h"
#include "navary/graph/shader/graph_ir.h"

namespace navary::graph::shader {

// Compiler for GraphIr to GLSL shader code.
// Generates GLSL functions for user-defined surface shaders.
// Example output function:
// ```cpp
// void Navary_UserSurface(in SurfaceInputs IN, inout SurfaceData surf)
// ```
// where SurfaceInputs and SurfaceData are predefined structs.
//
// Assumes:
// 1. Material UBO uniforms:
// - uniform MaterialUBO { vec4 uBaseColor; vec4 uParams0; vec4 uParams1; vec4
// uUser0; vec4 uUser1; };
// 2. Texture samplers:
// - sampler2D uAlbedoTex, uNormalTex, uOrmTex, uEmissiveTex, uMask0Tex,
// uMask1Tex, uCustom0Tex, uCustom1Tex;
// 3. SurfaceInputs IN has at least vec2 uv0;
// 4. SurfaceData surf has fields:
// - vec3 base_color;
// - vec3 normal_ws;
// - float roughness;
// - float metallic;
// - vec3 emissive_color;
// - float emissive_strength;
// - float alpha;
class GraphCompiler {
 public:
  GraphCompiler();

  // Emits a GLSL function:
  //   void Navary_UserSurface(in SurfaceInputs IN, inout SurfaceData surf)
  NavaryResult<std::string> GenerateUserSurfaceGlSl(const GraphIr& ir) const;

 private:
  std::string EmitValueType(ValueType type) const;

  void EmitConstNode(const GraphIr& ir, const Node& node, std::string* out,
                     std::string* pin_exprs) const;

  void EmitParamNode(const GraphIr& ir, const Node& node, std::string* out,
                     std::string* pin_exprs) const;

  void EmitTextureSampleNode(const GraphIr& ir, const Node& node,
                             std::string* out, std::string* pin_exprs) const;

  void EmitBinaryNode(const GraphIr& ir, const Node& node,
                      const char* op_symbol, std::string* out,
                      std::string* pin_exprs) const;

  void EmitLerpNode(const GraphIr& ir, const Node& node, std::string* out,
                    std::string* pin_exprs) const;

  void EmitSaturateNode(const GraphIr& ir, const Node& node, std::string* out,
                        std::string* pin_exprs) const;

  void EmitPowNode(const GraphIr& ir, const Node& node, std::string* out,
                   std::string* pin_exprs) const;

  void EmitNormalBlendNode(const GraphIr& ir, const Node& node,
                           std::string* out, std::string* pin_exprs) const;

  void EmitRampSampleNode(const GraphIr& ir, const Node& node, std::string* out,
                          std::string* pin_exprs) const;

  void EmitMakeSurfaceNode(const GraphIr& ir, const Node& node,
                           std::string* out, std::string* pin_exprs) const;
};

}  // namespace navary::graph::shader
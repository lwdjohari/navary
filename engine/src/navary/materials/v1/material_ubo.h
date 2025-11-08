#pragma once

// navary/materials/v1/material_ubo.h
// Defines MaterialUbo struct for GPU material data.
// Used by material system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

namespace navary::materials::v1 {

// Matches layout(std140) vec4 x 5 in GLSL.
struct MaterialUbo {
  float base_color[4];
  float params0[4];
  float params1[4];
  float user0[4];
  float user1[4];
};

static_assert(sizeof(MaterialUbo) == 5 * 4 * sizeof(float),
              "MaterialUbo must be 5 vec4s");

}  // namespace navary

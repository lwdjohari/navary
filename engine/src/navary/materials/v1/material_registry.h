#pragma once

// navary/materials/v1/material_registry.h
// Defines MaterialRegistry class for managing material instances.
// Used by material system components.
//
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

#include "navary/navary_status.h"
#include "navary/materials/v1/material.h"
#include "navary/materials/v1/material_types.h"

namespace navary::materials::v1 {

// Registry for Material instances.
// Manages creation, lookup, and destruction of materials.
class MaterialRegistry {
 public:
  MaterialRegistry();
  ~MaterialRegistry();

  NavaryRC Init(std::uint32_t max_materials);

  NavaryResult<MaterialHandle> CreateMaterial(const MaterialDesc& desc);

  Material* GetMaterial(MaterialHandle handle);

  const Material* GetMaterial(MaterialHandle handle) const;

  void DestroyMaterial(MaterialHandle handle);

 private:
  Material* materials_;
  std::uint32_t capacity_;
  std::uint32_t alive_count_;
  std::uint32_t* free_indices_;
  std::uint32_t free_top_;
};

}  // namespace navary::materials::v1

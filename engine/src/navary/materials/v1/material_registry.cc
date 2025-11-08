// navary/materials/v1/material_registry.cc
// Implementation of the MaterialRegistry class for managing materials.
// This file is part of the Navary engine.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/materials/v1/material_registry.h"

#include <cstdlib>
#include <cstring>

namespace navary::materials::v1 {

MaterialRegistry::MaterialRegistry()
    : materials_(nullptr),
      capacity_(0),
      alive_count_(0),
      free_indices_(nullptr),
      free_top_(0) {}

MaterialRegistry::~MaterialRegistry() {
  std::free(materials_);
  std::free(free_indices_);
}

NavaryRC MaterialRegistry::Init(std::uint32_t max_materials) {
  capacity_ = max_materials;
  materials_ =
      static_cast<Material*>(std::malloc(sizeof(Material) * capacity_));

  if (materials_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                            "MaterialRegistry: materials alloc failed");
  }

  std::memset(materials_, 0, sizeof(Material) * capacity_);

  free_indices_ = static_cast<std::uint32_t*>(
      std::malloc(sizeof(std::uint32_t) * capacity_));
  if (free_indices_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                            "MaterialRegistry: free_indices alloc failed");
  }
  
  for (std::uint32_t i = 0; i < capacity_; ++i) {
    free_indices_[i] = capacity_ - 1 - i;
  }
  
  free_top_    = capacity_;
  alive_count_ = 0;

  return NavaryRC::OK();
}

NavaryResult<MaterialHandle> MaterialRegistry::CreateMaterial(
    const MaterialDesc& desc) {
  if (free_top_ == 0) {
    return NavaryResult<MaterialHandle>(NavaryRC(
        NavaryStatus::kOutOfMemory, "MaterialRegistry: no free slots"));
  }
  
  const std::uint32_t index = free_indices_[--free_top_];
  Material* m               = &materials_[index];
  *m                        = Material{};  // zero-init

  m->handle             = MaterialHandle{index};
  m->surface_model      = desc.surface_model;
  m->material_class     = desc.material_class;
  m->alpha_mode         = desc.alpha_mode;
  m->textures           = desc.textures;
  m->params             = desc.params;
  m->receive_shadows    = desc.receive_shadows;
  m->instanced          = desc.instanced;
  m->vertex_skin        = desc.vertex_skin;
  m->streaming_priority = desc.streaming_priority;
  m->streaming_group    = desc.streaming_group;

  m->shader_key.surface_model   = desc.surface_model;
  m->shader_key.material_class  = desc.material_class;
  m->shader_key.alpha_mode      = desc.alpha_mode;
  m->shader_key.receive_shadows = desc.receive_shadows;
  m->shader_key.instanced       = desc.instanced;
  m->shader_key.vertex_skin     = desc.vertex_skin;
  m->shader_key.graph_hash      = desc.graph_hash;

  ++alive_count_;
  
  return NavaryResult<MaterialHandle>(m->handle);
}

Material* MaterialRegistry::GetMaterial(MaterialHandle handle) {
  if (handle.index >= capacity_) {
    return nullptr;
  }

  return &materials_[handle.index];
}

const Material* MaterialRegistry::GetMaterial(MaterialHandle handle) const {
  if (handle.index >= capacity_) {
    return nullptr;
  }

  return &materials_[handle.index];
}

void MaterialRegistry::DestroyMaterial(MaterialHandle handle) {
  if (handle.index >= capacity_) {
    return;
  }

  free_indices_[free_top_++] = handle.index;
  --alive_count_;
}

}  // namespace navary::materials::v1
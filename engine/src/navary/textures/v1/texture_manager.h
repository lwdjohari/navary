#pragma once

// navary/textures/v1/texture_manager.h
// Defines TextureManager class for managing texture resources.
// Used by texture system components.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

#include "navary/core/handles.h"
#include "navary/navary_status.h"

namespace navary::textures::v1 {

struct TextureInfo {
  core::TextureHandle handle;
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t mip_count;
  bool is_resident;
};

struct DummyTextures {
  core::TextureHandle white;
  core::TextureHandle black;
  core::TextureHandle flat_normal;
};

// Manages logical texture entries.
// Actual GPU upload and residency is handled by the engine.
// Caller owns TextureManager and ensures its lifetime exceeds textures.
class TextureManager {
 public:
  TextureManager();
  ~TextureManager();

  // Initializes the texture manager with a maximum number of textures.
  NavaryRC Init(std::uint32_t max_textures);

  // Allocates a logical texture entry. GPU upload is handled by engine.
  NavaryResult<core::TextureHandle> RegisterTexture(std::uint32_t width,
                                                    std::uint32_t height,
                                                    std::uint32_t mip_count);
  // Retrieves texture info by handle.
  const TextureInfo* GetTexture(core::TextureHandle handle) const;

  // Sets the residency status of a texture.
  void SetResident(core::TextureHandle handle, bool resident);

  // Accessor for dummy textures.
  DummyTextures dummy_textures() const {
    return dummy_textures_;
  }

 private:
  TextureInfo* textures_;
  std::uint32_t capacity_;
  std::uint32_t alive_count_;
  std::uint32_t* free_indices_;
  std::uint32_t free_top_;

  DummyTextures dummy_textures_;
};

}  // namespace navary::textures::v1
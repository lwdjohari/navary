// navary/textures/v1/texture_manager.cc
// Implements TextureManager class for managing texture resources.
// This file is part of the Navary texture system.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/textures/v1/texture_manager.h"

#include <cstdlib>
#include <cstring>

namespace navary::textures::v1 {

TextureManager::TextureManager()
    : textures_(nullptr),
      capacity_(0),
      alive_count_(0),
      free_indices_(nullptr),
      free_top_(0),
      dummy_textures_{} {}

TextureManager::~TextureManager() {
  std::free(textures_);
  std::free(free_indices_);
}

NavaryRC TextureManager::Init(std::uint32_t max_textures) {
  capacity_ = max_textures;
  textures_ =
      static_cast<TextureInfo*>(std::malloc(sizeof(TextureInfo) * capacity_));
  if (textures_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "TextureManager: textures alloc failed");
  }

  std::memset(textures_, 0, sizeof(TextureInfo) * capacity_);

  free_indices_ = static_cast<std::uint32_t*>(
      std::malloc(sizeof(std::uint32_t) * capacity_));
  if (free_indices_ == nullptr) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "TextureManager: free_indices alloc failed");
  }

  for (std::uint32_t i = 0; i < capacity_; ++i) {
    free_indices_[i] = capacity_ - 1 - i;
  }

  free_top_    = capacity_;
  alive_count_ = 0;

  // Reserve 3 dummy textures.
  NavaryResult<core::TextureHandle> white = RegisterTexture(1, 1, 1);
  NavaryResult<core::TextureHandle> black = RegisterTexture(1, 1, 1);
  NavaryResult<core::TextureHandle> flat  = RegisterTexture(1, 1, 1);

  if (!white.status().ok() || !black.status().ok() || !flat.status().ok()) {
    return NavaryRC(NavaryStatus::kOutOfMemory,
                    "TextureManager: dummy textures allocation failed");
  }

  dummy_textures_.white       = white.value();
  dummy_textures_.black       = black.value();
  dummy_textures_.flat_normal = flat.value();

  return NavaryRC::OK();
}

NavaryResult<core::TextureHandle> TextureManager::RegisterTexture(
    std::uint32_t width, std::uint32_t height, std::uint32_t mip_count) {
  if (free_top_ == 0) {
    return NavaryResult<core::TextureHandle>(NavaryRC(
        NavaryStatus::kOutOfMemory, "TextureManager: no free texture slots"));
  }

  const std::uint32_t index = free_indices_[--free_top_];
  TextureInfo* info         = &textures_[index];
  info->handle              = core::TextureHandle{index};
  info->width               = width;
  info->height              = height;
  info->mip_count           = mip_count;
  info->is_resident         = true;

  ++alive_count_;

  return NavaryResult<core::TextureHandle>(info->handle);
}

const TextureInfo* TextureManager::GetTexture(
    core::TextureHandle handle) const {
  if (handle.index >= capacity_) {
    return nullptr;
  }

  return &textures_[handle.index];
}

void TextureManager::SetResident(core::TextureHandle handle, bool resident) {
  if (handle.index >= capacity_) {
    return;
  }

  textures_[handle.index].is_resident = resident;
}

}  // namespace navary::textures::v1

#pragma once

// navary/textures/v1/texture_streaming.h
// Declares TextureStreamingManager for managing texture streaming and
// residency. 
// This file is part of the Navary texture system. 
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

#include "navary/core/handles.h"
#include "navary/textures/v1/texture_manager.h"

namespace navary::textures::v1 {

class TextureStreamingManager {
 public:
  explicit TextureStreamingManager(TextureManager* texture_manager);

  // Called every frame when building draw lists to mark textures as used.
  void TouchTexture(core::TextureHandle handle, std::uint8_t streaming_group,
                    std::uint8_t streaming_priority);

  // Called once per frame to update residency; trivial in this version.
  void UpdateStreaming();

 private:
  TextureManager* texture_manager_;
};

}  // namespace navary::textures::v1

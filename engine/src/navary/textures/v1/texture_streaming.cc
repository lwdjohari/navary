#include "navary/textures/v1/texture_streaming.h"

namespace navary::textures::v1 {

TextureStreamingManager::TextureStreamingManager(
    TextureManager* texture_manager)
    : texture_manager_(texture_manager) {}

void TextureStreamingManager::TouchTexture(TextureHandle handle,
                                           std::uint8_t streaming_group,
                                           std::uint8_t streaming_priority) {
  (void)streaming_group;
  (void)streaming_priority;
  // Simple implementation: keep everything resident whenever touched.
  if (texture_manager_ != nullptr) {
    texture_manager_->SetResident(handle, true);
  }
}

void TextureStreamingManager::UpdateStreaming() {
  // Trivial: no eviction yet. This is where you'd implement LRU / priorities.
}

}  // namespace navary

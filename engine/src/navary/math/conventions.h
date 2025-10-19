#pragma once

namespace navary::math {

/**
 * Graphics conventions used by the math layer.
 * OpenGL-style by default:
 * - Right-handed world (+Y up, -Z forward)
 * - Clip-space depth range: [-1, +1]
 * - Column-major matrices (when you add Mat types)
 */
struct Conventions {
  static constexpr bool kRightHanded = true;
  static constexpr bool kYUp = true;
  static constexpr bool kMinusZForward = true;   // camera looks toward -Z
  static constexpr bool kClipDepthMinusOneToOne = true; // OpenGL
  static constexpr bool kClipDepthZeroToOne = false;     // Vulkan
};

} // namespace navary::math

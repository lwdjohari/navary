// navary/graph/shader/node_metadata.cc
// Defines NodeMetadata table for shader nodes.
// Used by shader graph processing and validation.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include "navary/graph/shader/node_metadata.h"

namespace navary::graph::shader {

const NodeMetadata kNodeMetadataTable[] = {
    {
        NodeKind::kTextureSample2D,
        "Texture Sample 2D",
        "Texture",
        1, 1,  // UV in
        1, 1,  // RGBA out
        1,     // param[0] = textureSlot
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        1,  // texture samples
        0,  // normal ops
        2,  // math ops (UV transforms etc.)
        0,
        0,
    },
    {
        NodeKind::kMultiply,
        "Multiply",
        "Math",
        2, 2,
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        1,
        0,
        0,
    },
    {
        NodeKind::kAdd,
        "Add",
        "Math",
        2, 2,
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        1,
        0,
        0,
    },
    {
        NodeKind::kLerp,
        "Lerp",
        "Math",
        3, 3,  // a, b, t
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        2,
        0,
        0,
    },
    {
        NodeKind::kSaturate,
        "Saturate",
        "Math",
        1, 1,
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        1,
        0,
        0,
    },
    {
        NodeKind::kPow,
        "Pow",
        "Math",
        2, 2,
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        2,
        0,
        0,
    },
    {
        NodeKind::kNormalBlend,
        "Normal Blend",
        "Lighting",
        2, 2,  // baseN, detailN
        1, 1,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized)),
        0,
        2,
        2,
        0,
        0,
    },
    {
        NodeKind::kConstFloat,
        "Const Float",
        "Constants",
        0, 0,
        1, 1,
        1,  // 1 float
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kConstFloat2,
        "Const Float2",
        "Constants",
        0, 0,
        1, 1,
        2,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kConstFloat3,
        "Const Float3",
        "Constants",
        0, 0,
        1, 1,
        3,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kConstFloat4,
        "Const Float4",
        "Constants",
        0, 0,
        1, 1,
        4,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kParamFloat,
        "Material Param Float",
        "Material",
        0, 0,
        1, 1,
        1,  // param[0] = index
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kParamColor,
        "Material Param Color",
        "Material",
        0, 0,
        1, 1,
        1,  // param[0] = index
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kRampSample,
        "Ramp Sample",
        "Stylized",
        1, 1,  // NdotL in
        1, 1,  // RGB out
        2,     // [0] rampSlot, [1] bands
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized)),
        1,
        0,
        3,
        1,
        0,
    },
    {
        NodeKind::kMakeSurface,
        "Make Surface",
        "Output",
        4, 7,  // baseColor, normal, rough, metal, emissive, emissiveStr, alpha
        0, 0,
        0,
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        0,
    },
    {
        NodeKind::kCustomCode,
        "Custom Code",
        "Advanced",
        0, 4,
        0, 2,
        1,  // [0] = snippet index
        static_cast<std::uint8_t>(
            SurfaceModelToMaskBit(core::SurfaceModel::kLitePbr) |
            SurfaceModelToMaskBit(core::SurfaceModel::kStylized) |
            SurfaceModelToMaskBit(core::SurfaceModel::kUnlit)),
        0,
        0,
        0,
        0,
        1,  // cost_custom_ops
    },
};

const std::uint32_t kNodeMetadataCount =
    sizeof(kNodeMetadataTable) / sizeof(kNodeMetadataTable[0]);

const NodeMetadata* FindNodeMetadata(NodeKind kind) {
  for (std::uint32_t i = 0; i < kNodeMetadataCount; ++i) {
    if (kNodeMetadataTable[i].kind == kind) {
      return &kNodeMetadataTable[i];
    }
  }
  return nullptr;
}

}  // namespace navary

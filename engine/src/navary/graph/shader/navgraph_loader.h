#pragma once

// navary/graph/shader/navgraph_loader.h
// Defines NavGraphLoader class for loading shader graphs from binary data.
// Used by shader graph processing and material system.
// Author:
// - Linggawasistha Djohari  [2024-Present]

#include <cstdint>

#include "navary/navary_status.h"
#include "navary/graph/shader/graph_ir.h"
#include "navary/graph/shader/navgraph_binary.h"

namespace navary::graph::shader {

// Loader for NavGraph shader graphs from binary data.
// Parses binary format into GraphIr structure.
// This loader assumes nodes are already topologically sorted by the
// editor/pipeline, which is normal in real-world engines.
class NavGraphLoader {
 public:
  NavGraphLoader();

  // Data points at full file contents already in memory.
  // This loader assumes nodes are already topologically sorted by the
  // editor/pipeline, which is normal in real-world engines.
  NavaryRC LoadFromMemory(const std::uint8_t* data, std::uint32_t size,
                          GraphIr* out);

 private:
  NavaryRC MapNodeKindToNodeOp(NodeKind kind, NodeOp* out) const;
};

}  // namespace navary::graph::shader
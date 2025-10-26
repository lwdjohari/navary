// navary/math/spline.h

#pragma once

#include <cstddef>
#include <vector>
#include <cassert>

#include "navary/math/vec3.h"
#include "navary/math/mat4.h"

namespace navary::math {

/**
 * @brief Hermite cubic mixing matrix (column-major, column-vector convention).
 *
 * This is the standard Hermite basis H such that:
 *   p(t) = [t^3, t^2, t, 1] * H * [p0, p1, v0, v1]
 * We store H as a Mat4 in column-major layout to match the engine.
 */
Mat4 HermiteMixingMatrix();

/**
 * @brief Shape node data used by the spline solvers.
 *
 * Notes:
 * - `segment` is the param distance from this node to the *next* node.
 *   For RNSpline/SNSpline it's geometric distance; for TNSpline it's a time
 * period.
 * - `velocity` is the per-node tangent used by cubic Hermite interpolation.
 * - `rotation` is carried but not used by basic position interpolation; kept
 *   for parity with higher-level pathing features and future rotation track.
 */
struct SplineNode {
  Vec3 position{};     ///< Node position (float domain for render/tools).
  Vec3 velocity{};     ///< Tangent at node (computed by Build()).
  Vec3 rotation{};     ///< Optional payload; not used by GetPosition().
  float segment{0.f};  ///< Param distance to the next node (>= 0).
};

/**
 * @brief Rounded Nonuniform Spline (constant-speed-ish path with rounded
 * corners).
 *
 * - Uses cubic Hermite segments between nodes.
 * - Node tangents are computed from normalized direction differences (split
 * angle).
 * - Segment param is *geometric distance* between consecutive nodes.
 * - Access point with time in [0, 1] mapped over total arc length.
 *
 * No heap allocations in hot path; vector holds a small number of nodes (<= 128
 * typical).
 */
class RnSpline {
 public:
  RnSpline() = default;

  /** Clear all nodes. */
  void Clear() {
    nodes_.clear();
    total_ = 0.f;
  }

  /** Number of nodes currently in the spline. */
  int Count() const {
    return static_cast<int>(nodes_.size());
  }

  /** Read-only access to internal nodes. */
  const std::vector<SplineNode>& Nodes() const {
    return nodes_;
  }

  /**
   * @brief Add a node at the end.
   * @param p Position of the node in world or local space.
   *
   * Behavior:
   * - If not the first node, segment length of previous node is updated to
   * distance(prev, p).
   * - total_ is updated as sum of segments.
   */
  void AddNode(const Vec3& p);

  /**
   * @brief Build per-node velocities (tangents) required for Hermite
   * interpolation.
   *
   * Must be called after node edits (add/remove/insert/update). Idempotent for
   * identical node sets. For 0 or 1 nodes, this is a no-op. For 2 nodes,
   * velocities reduce to endpoint rules (start/end velocity). For >=3 nodes,
   * interior tangents split the angle.
   */
  void Build();

  /**
   * @brief Sample position along the spline.
   * @param t Normalized param in [0, 1]. Values outside are clamped.
   *
   * Mapping:
   *  - t is scaled by the cumulative segment distance (total_).
   *  - The sample is computed on the identified segment via Hermite cubic.
   */
  Vec3 GetPosition(float t) const;

  /** Total param length (sum of all segment distances). */
  float TotalParam() const {
    return total_;
  }

 protected:
  // Endpoint tangent rules (Hermite-friendly).
  Vec3 StartVelocity(int i) const;
  Vec3 EndVelocity(int i) const;

  std::vector<SplineNode> nodes_;
  float total_{0.f};
};

/**
 * @brief Smooth Nonuniform Spline.
 *
 * Inherits RnSpline, but adds a smoothing pass over tangents to reduce
 * sharp accel/decel while preserving shape (3 passes typical).
 */
class SnSpline : public RnSpline {
 public:
  void Build();

 protected:
  void SmoothOnce();
};

/**
 * @brief Timed Nonuniform Spline.
 *
 * Similar to SnSpline, but the segment param is *time per segment* instead of
 * geometric distance. This allows authoring paths where time between nodes
 * varies independently of geometry.
 *
 * API mirrors common editor operations (insert/remove/update).
 */
class TnSpline : public SnSpline {
 public:
  /** Add a node with per-segment time. Time applies to *previous* segment. */
  void AddNodeTimed(const Vec3& pos, const Vec3& rotation, float time_period);

  /** Insert node at index; time_period becomes the new segment before this
   * index. */
  void InsertNodeTimed(int index, const Vec3& pos, const Vec3& rotation,
                       float time_period);

  /** Remove node at index. */
  void RemoveNode(int index);

  /** Update node position at index. */
  void UpdateNodePos(int index, const Vec3& pos);

  /** Update segment time at index (distance to *previous* node). */
  void UpdateNodeTime(int index, float time_period);

  /** Rebuild and smooth with constraints (3 passes like source behavior). */
  void Build();

 private:
  void SmoothAndConstrain();

  // Constraint pass: scales interior velocities to equalize “speed” across time
  // buckets.
  void ConstrainOnce();
};

}  // namespace navary::math

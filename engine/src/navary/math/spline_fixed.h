#pragma once

/**
 * Fixed-point spline family (RN / SN / TN) using Fixed15p16 and FixedVec3.
 * - RN: rounded nonuniform spline (geometric distances as segment params)
 * - SN: RN + tangent smoothing passes
 * - TN: SN, but segment params are explicit time deltas (retiming support)
 *
 * Implementation is clean-room and mirrors the float version semantics.
 * Hermite evaluation is done in fixed arithmetic:
 *   p(t) = h00*p0 + h10*v0*L + h01*p1 + h11*v1*L
 *   where:
 *     h00 =  2t^3 - 3t^2 + 1
 *     h10 =   t^3 - 2t^2 + t
 *     h01 = -2t^3 + 3t^2
 *     h11 =   t^3 -   t^2
 * `L` is the segment parameter (geometric distance for RN/SN; time for TN).
 */

#include <vector>
#include <algorithm>

#include "navary/math/fixed.h"
#include "navary/math/fixed_vec3.h"

namespace navary::math {

using FX = Fixed15p16;

struct FixedSplineNode {
  FixedVec3 position{};
  FixedVec3 velocity{};  // Hermite tangent at node (unit-ish direction)
  FixedVec3 rotation{};  // payload (not used for position sampling)
  FX segment{
      FX::Zero()};  // param to *next* node: distance (RN/SN) or time (TN)
};

namespace detail {

// |a| helper for Fixed
inline FX Abs(FX v) {
  return (v < FX::FromRaw(0)) ? -v : v;
}

// Clamp t into [0,1]
inline FX Clamp01(FX t) {
  const FX zero = FX::Zero();
  const FX one  = FX::FromInt(1);
  if (t < zero)
    return zero;
  if (t > one)
    return one;
  return t;
}

// Hermite scalar basis (fixed)
inline void HermiteBasis(FX t, FX& h00, FX& h10, FX& h01, FX& h11) {
  const FX t2 = t * t;  // Mul with rounding
  const FX t3 = t2 * t;

  // h00 = 2t^3 - 3t^2 + 1
  h00 = FX::FromInt(2) * t3 - FX::FromInt(3) * t2 + FX::FromInt(1);
  // h10 = t^3 - 2t^2 + t
  h10 = t3 - FX::FromInt(2) * t2 + t;
  // h01 = -2t^3 + 3t^2
  h01 = -FX::FromInt(2) * t3 + FX::FromInt(3) * t2;
  // h11 = t^3 - t^2
  h11 = t3 - t2;
}

// FMA helpers on FixedVec3 with Fixed scalars
inline FixedVec3 Fmadd(const FixedVec3& a, FX s, const FixedVec3& b) {
  // a*s + b
  return FixedVec3{a.x * s + b.x, a.y * s + b.y, a.z * s + b.z};
}

inline FixedVec3 Scale(const FixedVec3& v, FX s) {
  return FixedVec3::MulScalar(v, s);
}

// |v| (geometric) for RN/SN distances
inline FX Dist(const FixedVec3& a, const FixedVec3& b) {
  FixedVec3 d{a.x - b.x, a.y - b.y, a.z - b.z};
  return d.Length();
}

// Normalize direction with safe-guard
inline FixedVec3 SafeNormalize(const FixedVec3& v) {
  FX l = v.Length();
  if (l.IsZero())
    return FixedVec3{};
  return FixedVec3{v.x / l, v.y / l, v.z / l};
}

inline FX Mul(const FX& a, const FX& b) {
  return a * b;
}

}  // namespace detail

// RN: Rounded Nonuniform spline Fixed15p16
class RnSplineFixed {
 public:
  void Clear() {
    nodes_.clear();
    total_ = FX::Zero();
  }
  int Count() const {
    return static_cast<int>(nodes_.size());
  }
  const std::vector<FixedSplineNode>& Nodes() const {
    return nodes_;
  }
  FX TotalParam() const {
    return total_;
  }

  // Add node (appends); for RN the previous node's segment becomes geometric
  // distance.
  void AddNode(const FixedVec3& p) {
    if (!nodes_.empty()) {
      FX seg                = detail::Dist(nodes_.back().position, p);
      nodes_.back().segment = seg;
      total_                = total_ + seg;
    }
    FixedSplineNode n;
    n.position = p;
    nodes_.push_back(n);
  }

  // Build tangents (Hermite)
  void Build() {
    const int n = Count();
    if (n < 2)
      return;

    if (n == 2) {
      nodes_.front().velocity = StartVelocity(0);
      nodes_.back().velocity  = EndVelocity(1);
      return;
    }

    // Interior: split-angle (normalize neighbors and subtract)
    for (int i = 1; i < n - 1; ++i) {
      FixedVec3 next =
          FixedVec3{nodes_[i + 1].position.x - nodes_[i].position.x,
                    nodes_[i + 1].position.y - nodes_[i].position.y,
                    nodes_[i + 1].position.z - nodes_[i].position.z};
      FixedVec3 prev =
          FixedVec3{nodes_[i - 1].position.x - nodes_[i].position.x,
                    nodes_[i - 1].position.y - nodes_[i].position.y,
                    nodes_[i - 1].position.z - nodes_[i].position.z};

      next = detail::SafeNormalize(next);
      prev = detail::SafeNormalize(prev);
      FixedVec3 v =
          FixedVec3{next.x - prev.x, next.y - prev.y, next.z - prev.z};
      nodes_[i].velocity = detail::SafeNormalize(v);
    }

    nodes_.front().velocity = StartVelocity(0);
    nodes_.back().velocity  = EndVelocity(n - 1);
  }

  // Sample position for t in [0,1] mapped over total_ param (geometric length
  // sum).
  FixedVec3 GetPosition(FX t) const {
    const int n = Count();
    if (n < 2)
      return FixedVec3{};

    t = detail::Clamp01(t);

    FX target = detail::Mul(t, total_);  // t * total_
    FX accum  = FX::Zero();
    int i     = 0;
    while (i < n - 1 && accum + nodes_[i].segment < target) {
      accum = accum + nodes_[i].segment;
      ++i;
    }
    if (i >= n - 1)
      i = n - 2;

    const FX L = nodes_[i].segment;
    if (L.IsZero() || nodes_[i].position == nodes_[i + 1].position)
      return nodes_[i + 1].position;

    // local t in [0,1]
    const FX lt = (target - accum) / L;

    // Hermite
    FX h00, h10, h01, h11;
    detail::HermiteBasis(lt, h00, h10, h01, h11);

    // v scaled by L (position units)
    const FixedVec3 v0 = detail::Scale(nodes_[i].velocity, L);
    const FixedVec3 v1 = detail::Scale(nodes_[i + 1].velocity, L);

    // p = h00*p0 + h10*v0 + h01*p1 + h11*v1
    FixedVec3 p = detail::Scale(nodes_[i].position, h00);
    p           = detail::Fmadd(v0, h10, p);
    p           = detail::Fmadd(nodes_[i + 1].position, h01, p);
    p           = detail::Fmadd(v1, h11, p);

    return p;
  }

 protected:
  // Endpoint velocity rules (mirrors float impl)
  FixedVec3 StartVelocity(int idx) const {
    // v0 = 0.5 * ( 3/L * (p1 - p0) - v1 )
    if (idx < 0 || idx >= Count() - 1)
      return FixedVec3{};

    const FX L =
        (nodes_[idx].segment.IsZero()) ? FX::FromInt(1) : nodes_[idx].segment;
    FixedVec3 d{(nodes_[idx + 1].position.x - nodes_[idx].position.x) *
                    (FX::FromInt(3) / L),
                (nodes_[idx + 1].position.y - nodes_[idx].position.y) *
                    (FX::FromInt(3) / L),
                (nodes_[idx + 1].position.z - nodes_[idx].position.z) *
                    (FX::FromInt(3) / L)};
    FixedVec3 v1 = (Count() >= 3) ? nodes_[idx + 1].velocity : FixedVec3{};

    return FixedVec3{(d.x - v1.x) / 2, (d.y - v1.y) / 2, (d.z - v1.z) / 2};
  }

  FixedVec3 EndVelocity(int idx) const {
    // vn = 0.5 * ( 3/L * (pn - p(n-1)) - v(n-1) )
    if (idx <= 0 || idx >= Count())
      return FixedVec3{};

    const FX L = (nodes_[idx - 1].segment.IsZero()) ? FX::FromInt(1)
                                                    : nodes_[idx - 1].segment;
    FixedVec3 d{(nodes_[idx].position.x - nodes_[idx - 1].position.x) *
                    (FX::FromInt(3) / L),
                (nodes_[idx].position.y - nodes_[idx - 1].position.y) *
                    (FX::FromInt(3) / L),
                (nodes_[idx].position.z - nodes_[idx - 1].position.z) *
                    (FX::FromInt(3) / L)};
    FixedVec3 vm1 = (Count() >= 3) ? nodes_[idx - 1].velocity : FixedVec3{};

    return FixedVec3{(d.x - vm1.x) / 2, (d.y - vm1.y) / 2, (d.z - vm1.z) / 2};
  }

  std::vector<FixedSplineNode> nodes_;
  FX total_{FX::Zero()};
};

// SN: Smoothed Nonuniform spline Fixed15p16
class SnSplineFixed : public RnSplineFixed {
 public:
  void Build() {
    RnSplineFixed::Build();
    for (int k = 0; k < 3; ++k) {
      SmoothOnce();
    }
  }

 protected:
  // One smoothing pass (matches float logic, but expressed in fixed)
  void SmoothOnce() {
    const int n = Count();
    if (n < 3)
      return;

    // Local copies for access
    const auto& nodes = Nodes();
    // Compute first "old" using base start rule
    FixedVec3 old = StartVelocity(0);

    // We need write access to velocities; cast away const via protected access.
    // (safe: RnSplineFixed owns nodes_).
    auto& mut = const_cast<std::vector<FixedSplineNode>&>(Nodes());

    for (int i = 1; i < n - 1; ++i) {
      const FX L0 =
          (mut[i - 1].segment.IsZero()) ? FX::FromInt(1) : mut[i - 1].segment;
      const FX L1 = (mut[i].segment.IsZero()) ? FX::FromInt(1) : mut[i].segment;

      // ve*L1 + vs*L0, then divide by (L0+L1)
      FixedVec3 ve = RnSplineFixed::EndVelocity(i);
      FixedVec3 vs = RnSplineFixed::StartVelocity(i);

      FixedVec3 num{ve.x * L1 + vs.x * L0, ve.y * L1 + vs.y * L0,
                    ve.z * L1 + vs.z * L0};
      const FX denom = L0 + L1;
      FixedVec3 v{num.x / denom, num.y / denom, num.z / denom};

      mut[i - 1].velocity = old;
      old                 = v;
    }
    mut.back().velocity = RnSplineFixed::EndVelocity(n - 1);
    mut[n - 2].velocity = old;
  }
};

// TN: Timed Nonuniform spline Fixed15p16
class TnSplineFixed : public SnSplineFixed {
 public:
  // time_period applies to the *previous* segment
  void AddNodeTimed(const FixedVec3& pos, const FixedVec3& rotation,
                    FX time_period) {
    if (!Nodes().empty()) {
      auto& mut = const_cast<std::vector<FixedSplineNode>&>(Nodes());
      mut.back().segment =
          (time_period < FX::Zero()) ? FX::Zero() : time_period;
      total_ = total_ + mut.back().segment;
    }

    FixedSplineNode n;
    n.position = pos;
    n.rotation = rotation;
    auto& mut  = const_cast<std::vector<FixedSplineNode>&>(Nodes());
    mut.push_back(n);
  }

  // Insert at index; time_period becomes new segment before this node.
  void InsertNodeTimed(int index, const FixedVec3& pos,
                       const FixedVec3& rotation, FX time_period) {
    auto& mut   = const_cast<std::vector<FixedSplineNode>&>(Nodes());
    const int n = Count();
    if (index < 0 || index > n)
      return;

    if (n > 0) {
      const FX add = (time_period < FX::Zero()) ? FX::Zero() : time_period;
      total_       = total_ + add;
    }

    FixedSplineNode node;
    node.position = pos;
    node.rotation = rotation;
    node.segment  = (time_period < FX::Zero()) ? FX::Zero() : time_period;

    mut.insert(mut.begin() + index, node);

    // Swap with previous to follow "time to previous" semantics.
    if (index > 0) {
      std::swap(mut[index].segment, mut[index - 1].segment);
    }
  }

  void RemoveNode(int index) {
    auto& mut   = const_cast<std::vector<FixedSplineNode>&>(Nodes());
    const int n = Count();
    if (n == 0 || index < 0 || index >= n)
      return;

    total_ = total_ - mut[index].segment;
    mut.erase(mut.begin() + index);
  }

  void UpdateNodePos(int index, const FixedVec3& pos) {
    auto& mut   = const_cast<std::vector<FixedSplineNode>&>(Nodes());
    const int n = Count();
    if (n == 0 || index < 0 || index >= n)
      return;

    mut[index].position = pos;
  }

  void UpdateNodeTime(int index, FX time_period) {
    auto& mut   = const_cast<std::vector<FixedSplineNode>&>(Nodes());
    const int n = Count();
    if (n == 0 || index < 0 || index >= n)
      return;

    const FX clamped   = (time_period < FX::Zero()) ? FX::Zero() : time_period;
    total_             = total_ + (clamped - mut[index].segment);
    mut[index].segment = clamped;
  }

  void Build() {
    SnSplineFixed::Build();
    SmoothAndConstrain();
  }

 private:
  void SmoothAndConstrain() {
    for (int i = 0; i < 3; ++i) {
      // call protected smooth in base
      SnSplineFixed::Build();  // NOTE: we want one SmoothOnce; re-run full
                               // build is too heavy
      // The above line redoes tangents; instead implement a single-pass
      // smoother: To avoid recursion, we duplicate SmoothOnce from
      // SnSplineFixed with access:
      SmoothOnceLocal_();
      ConstrainOnce();
    }
  }

  // Local copy of SmoothOnce logic to avoid calling Build() again.
  void SmoothOnceLocal_() {
    const int n = Count();
    if (n < 3)
      return;

    auto& mut = const_cast<std::vector<FixedSplineNode>&>(Nodes());

    FixedVec3 old = RnSplineFixed::StartVelocity(0);
    for (int i = 1; i < n - 1; ++i) {
      const FX L0 =
          (mut[i - 1].segment.IsZero()) ? FX::FromInt(1) : mut[i - 1].segment;
      const FX L1 = (mut[i].segment.IsZero()) ? FX::FromInt(1) : mut[i].segment;

      FixedVec3 ve = RnSplineFixed::EndVelocity(i);
      FixedVec3 vs = RnSplineFixed::StartVelocity(i);

      FixedVec3 num{ve.x * L1 + vs.x * L0, ve.y * L1 + vs.y * L0,
                    ve.z * L1 + vs.z * L0};
      const FX denom = L0 + L1;
      FixedVec3 v{num.x / denom, num.y / denom, num.z / denom};

      mut[i - 1].velocity = old;
      old                 = v;
    }
    mut.back().velocity = RnSplineFixed::EndVelocity(n - 1);
    mut[n - 2].velocity = old;
  }

  // Constraint pass (timing-aware speed equalization)
  void ConstrainOnce() {
    const int n = Count();
    if (n < 3)
      return;

    auto& mut = const_cast<std::vector<FixedSplineNode>&>(Nodes());

    for (int i = 1; i < n - 1; ++i) {
      const FX L0 = detail::Dist(mut[i].position, mut[i - 1].position);
      const FX L1 = detail::Dist(mut[i + 1].position, mut[i].position);
      const FX T0 =
          (mut[i - 1].segment.IsZero()) ? FX::FromInt(1) : mut[i - 1].segment;
      const FX T1 = (mut[i].segment.IsZero()) ? FX::FromInt(1) : mut[i].segment;

      const FX r0    = L0 / T0;
      const FX r1    = L1 / T1;
      const FX denom = r0 + r1;
      FX scale       = FX::FromInt(1);
      if (!(denom.IsZero())) {
        // 4 r0 r1 / (r0 + r1)^2
        scale = (FX::FromInt(4) * r0 * r1) / (denom * denom);
      }
      mut[i].velocity =
          FixedVec3{mut[i].velocity.x * scale, mut[i].velocity.y * scale,
                    mut[i].velocity.z * scale};
    }
  }

  // We need write access in TN helpers
  using RnSplineFixed::EndVelocity;
  using RnSplineFixed::StartVelocity;
  using SnSplineFixed::Nodes;

  using RnSplineFixed::total_;
};

}  // namespace navary::math

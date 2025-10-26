// navary/math/spline.cc

#include "navary/math/spline.h"

#include <algorithm>
#include <cmath>

namespace navary::math {

// -------------------------------------------------------------
// Hermite basis (column-major)
// -------------------------------------------------------------
Mat4 HermiteMixingMatrix() {
  // This is the standard cubic Hermite basis in column-major layout.
  // H = [  2 -2  1  1
  //       -3  3 -2 -1
  //        0  0  1  0
  //        1  0  0  0 ]
  // We construct with Identity() and then set entries.

  Mat4 H = Mat4::Identity();

  H.set(0, 0, 2.f);
  H.set(1, 0, -3.f);
  H.set(2, 0, 0.f);
  H.set(3, 0, 1.f);
  H.set(0, 1, -2.f);
  H.set(1, 1, 3.f);
  H.set(2, 1, 0.f);
  H.set(3, 1, 0.f);
  H.set(0, 2, 1.f);
  H.set(1, 2, -2.f);
  H.set(2, 2, 1.f);
  H.set(3, 2, 0.f);
  H.set(0, 3, 1.f);
  H.set(1, 3, -1.f);
  H.set(2, 3, 0.f);
  H.set(3, 3, 0.f);

  return H;
}

// Utility: evaluate cubic Hermite for one component triplet packed as columns.
// We build a 4x4 M with columns [p0, p1, v0, v1] for x/y/z independently.
static Vec3 HermiteEvaluate(const Vec3& p0, const Vec3& v0, const Vec3& p1,
                            const Vec3& v1, float t) {
  // Column-major: first make a matrix whose columns are the component rows.
  Mat4 M = Mat4::Identity();
  // Column 0: p0
  M.set(0, 0, p0.x);
  M.set(0, 1, p0.y);
  M.set(0, 2, p0.z);
  M.set(0, 3, 0.f);
  // Column 1: p1
  M.set(1, 0, p1.x);
  M.set(1, 1, p1.y);
  M.set(1, 2, p1.z);
  M.set(1, 3, 0.f);
  // Column 2: v0
  M.set(2, 0, v0.x);
  M.set(2, 1, v0.y);
  M.set(2, 2, v0.z);
  M.set(2, 3, 0.f);
  // Column 3: v1
  M.set(3, 0, v1.x);
  M.set(3, 1, v1.y);
  M.set(3, 2, v1.z);
  M.set(3, 3, 0.f);

  // Mix
  M = M * HermiteMixingMatrix();

  // Time vector T = [t^3, t^2, t, 1]
  const float t2 = t * t;
  const float t3 = t2 * t;
  // used with Transform(Vector) reading x,y,z rows
  const Vec3 time_x{t3, t2, t};

  // We want: result = [t^3,t^2,t,1] * M
  // Since our Mat4::TransformVector multiplies M * v (column-vector),
  // we can transpose logic by building a row multiply via picking rows.
  // Simpler: compute each component explicitly using matrix entries.
  // Let’s take M^T and multiply as column-vector; equivalent to:
  Vec4 T{t3, t2, t, 1.f};
  Vec4 r = M.Transform(T);  // r.x = x(t), r.y = y(t), r.z = z(t)

  return Vec3{r.x, r.y, r.z};
}

// -------------------------------------------------------------
// RnSpline
// -------------------------------------------------------------

void RnSpline::AddNode(const Vec3& p) {
  if (!nodes_.empty()) {
    // Update previous segment length to geometric distance.
    const Vec3 d          = p - nodes_.back().position;
    nodes_.back().segment = d.length();
    total_ += nodes_.back().segment;
  }
  SplineNode n;
  n.position = p;
  nodes_.push_back(n);
}

void RnSpline::Build() {
  const int n = Count();
  if (n < 2)
    return;

  if (n == 2) {
    // Endpoints only: use consistent endpoint velocity rules.
    nodes_.front().velocity = StartVelocity(0);
    nodes_.back().velocity  = EndVelocity(n - 1);
    return;
  }

  // Interior tangents: split-angle via normalized neighbors.
  for (int i = 1; i < n - 1; ++i) {
    Vec3 next      = nodes_[i + 1].position - nodes_[i].position;
    Vec3 prev      = nodes_[i - 1].position - nodes_[i].position;
    const float ln = next.length();
    const float lp = prev.length();
    if (ln > 0.f)
      next /= ln;
    if (lp > 0.f)
      prev /= lp;
    Vec3 v             = next - prev;
    const float lv     = v.length();
    nodes_[i].velocity = (lv > 0.f) ? (v / lv) : Vec3{0, 0, 0};
  }

  nodes_.front().velocity = StartVelocity(0);
  nodes_.back().velocity  = EndVelocity(n - 1);
}

Vec3 RnSpline::GetPosition(float t) const {
  const int n = Count();
  if (n < 2)
    return Vec3{0.f, 0.f, 0.f};

  if (t < 0.f)
    t = 0.f;
  if (t > 1.f)
    t = 1.f;

  const float target = t * total_;
  float accum        = 0.f;
  int i              = 0;
  // Find segment that contains target
  while (i < n - 1 && accum + nodes_[i].segment < target) {
    accum += nodes_[i].segment;
    ++i;
  }
  if (i >= n - 1)
    i = n - 2;

  const float seg_len = nodes_[i].segment;
  if (seg_len <= 1e-7f || nodes_[i].position == nodes_[i + 1].position) {
    return nodes_[i + 1].position;
  }

  // Normalize local t in [0,1] within segment.
  float lt = (target - accum) / seg_len;

  // Scale endpoint velocities to geometric scale (Hermite expects v in position
  // units).
  const Vec3 v0 = nodes_[i].velocity * seg_len;
  const Vec3 v1 = nodes_[i + 1].velocity * seg_len;

  return HermiteEvaluate(nodes_[i].position, v0, nodes_[i + 1].position, v1,
                         lt);
}

Vec3 RnSpline::StartVelocity(int idx) const {
  // Based on endpoint Hermite conditions using adjacent segment.
  // v0 = (3 / L) * (p1 - p0) * 0.5 - 0.5 * v1 (paired by interior def).
  // We follow the reference logic: scale by 3 / L and blend with neighbor.
  if (idx < 0 || idx >= Count() - 1)
    return Vec3{0, 0, 0};
  const float L = std::max(nodes_[idx].segment, 1e-7f);
  const Vec3 d = (nodes_[idx + 1].position - nodes_[idx].position) * (3.0f / L);
  // We don’t have v1 yet during interior build; for endpoints we compute after
  // interiors. Empirically stable: take 0.5*(d - next_vel)
  Vec3 next_vel = (Count() >= 3) ? nodes_[idx + 1].velocity : Vec3{0, 0, 0};
  return (d - next_vel) * 0.5f;
}

Vec3 RnSpline::EndVelocity(int idx) const {
  if (idx <= 0 || idx >= Count())
    return Vec3{0, 0, 0};
    
  const float L = std::max(nodes_[idx - 1].segment, 1e-7f);
  const Vec3 d = (nodes_[idx].position - nodes_[idx - 1].position) * (3.0f / L);

  Vec3 prev_vel = (Count() >= 3) ? nodes_[idx - 1].velocity : Vec3{0, 0, 0};

  return (d - prev_vel) * 0.5f;
}

// -------------------------------------------------------------
// SnSpline
// -------------------------------------------------------------

void SnSpline::Build() {
  RnSpline::Build();
  // Three smoothing iterations (mirrors source behavior).
  for (int k = 0; k < 3; ++k) {
    SmoothOnce();
  }
}

void SnSpline::SmoothOnce() {
  const int n = Count();
  if (n < 3)
    return;

  // Save first pass velocity to chain results (like moving window).
  Vec3 old = RnSpline::StartVelocity(0);
  for (int i = 1; i < n - 1; ++i) {
    // Weighted blend of local start/end velocities by neighboring segment
    // lengths.
    const float L0         = std::max(nodes_[i - 1].segment, 1e-7f);
    const float L1         = std::max(nodes_[i].segment, 1e-7f);
    const Vec3 ve          = RnSpline::EndVelocity(i) * L1;
    const Vec3 vs          = RnSpline::StartVelocity(i) * L0;
    Vec3 v                 = (ve + vs) * (1.0f / (L0 + L1));
    nodes_[i - 1].velocity = old;
    old                    = v;
  }
  nodes_.back().velocity = RnSpline::EndVelocity(n - 1);
  nodes_[n - 2].velocity = old;
}

// -------------------------------------------------------------
// TnSpline
// -------------------------------------------------------------

void TnSpline::AddNodeTimed(const Vec3& pos, const Vec3& rotation,
                            float time_period) {
  // time_period applies to previous segment
  if (!Nodes().empty()) {
    nodes_.back().segment = std::max(time_period, 0.0f);
    total_ += nodes_.back().segment;
  }
  SplineNode n;
  n.position = pos;
  n.rotation = rotation;
  nodes_.push_back(n);
}

void TnSpline::InsertNodeTimed(int index, const Vec3& pos, const Vec3& rotation,
                               float time_period) {
  const int n = Count();
  if (index < 0 || index > n)
    return;

  // Increase total by new segment that precedes the inserted node.
  if (n > 0)
    total_ += std::max(time_period, 0.0f);

  SplineNode node;
  node.position = pos;
  node.rotation = rotation;
  node.segment  = std::max(time_period, 0.0f);

  nodes_.insert(nodes_.begin() + index, node);

  // Swap segment with previous to match "time to previous" semantics.
  if (index > 0) {
    std::swap(nodes_[index].segment, nodes_[index - 1].segment);
  }
}

void TnSpline::RemoveNode(int index) {
  const int n = Count();
  if (n == 0 || index < 0 || index >= n)
    return;
  total_ -= nodes_[index].segment;
  nodes_.erase(nodes_.begin() + index);
}

void TnSpline::UpdateNodePos(int index, const Vec3& pos) {
  const int n = Count();
  if (n == 0 || index < 0 || index >= n)
    return;
  nodes_[index].position = pos;
}

void TnSpline::UpdateNodeTime(int index, float time_period) {
  const int n = Count();
  if (n == 0 || index < 0 || index >= n)
    return;
  // total_ tracks sum of segments; adjust for delta.
  const float clamped = std::max(time_period, 0.0f);
  total_ += (clamped - nodes_[index].segment);
  nodes_[index].segment = clamped;
}

void TnSpline::Build() {
  SnSpline::Build();
  // Apply smoothing/constraint a few times like the source code.
  SmoothAndConstrain();
}

void TnSpline::SmoothAndConstrain() {
  for (int i = 0; i < 3; ++i) {
    SnSpline::SmoothOnce();
    ConstrainOnce();
  }
}

void TnSpline::ConstrainOnce() {
  const int n = Count();
  if (n < 3)
    return;

  for (int i = 1; i < n - 1; ++i) {
    // r0 = |p_i - p_{i-1}| / time_{i-1}
    // r1 = |p_{i+1} - p_i| / time_{i}
    // scale v_i by 4 r0 r1 / (r0 + r1)^2   (accel smoothing under timing)
    const float L0    = (nodes_[i].position - nodes_[i - 1].position).length();
    const float L1    = (nodes_[i + 1].position - nodes_[i].position).length();
    const float T0    = std::max(nodes_[i - 1].segment, 1e-7f);
    const float T1    = std::max(nodes_[i].segment, 1e-7f);
    const float r0    = L0 / T0;
    const float r1    = L1 / T1;
    const float denom = (r0 + r1);
    const float scale = (denom > 0.f) ? (4.f * r0 * r1) / (denom * denom) : 1.f;
    nodes_[i].velocity *= scale;
  }
}

}  // namespace navary::math

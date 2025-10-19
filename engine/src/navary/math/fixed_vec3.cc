#include "navary/math/fixed_vec3.h"

namespace navary::math {

using F = FixedVec3::F;

FixedVec3 FixedVec3::RotateAroundX(F angle) const {
  F s, c;
  FixedTrig::SinCos(angle, s, c);
  // y' = y*c - z*s
  // z' = y*s + z*c
  F yp = (Mul(y, c) - Mul(z, s));
  F zp = (Mul(y, s) + Mul(z, c));

  return FixedVec3{x, yp, zp};
}

FixedVec3 FixedVec3::RotateAroundY(F angle) const {
  F s, c;
  FixedTrig::SinCos(angle, s, c);
  // x' = x*c + z*s
  // z' = -x*s + z*c
  F xp = (Mul(x, c) + Mul(z, s));
  F zp = (-Mul(x, s) + Mul(z, c));

  return FixedVec3{xp, y, zp};
}

FixedVec3 FixedVec3::RotateAroundZ(F angle) const {
  F s, c;
  FixedTrig::SinCos(angle, s, c);
  // x' = x*c - y*s
  // y' = x*s + y*c
  F xp = (Mul(x, c) - Mul(y, s));
  F yp = (Mul(x, s) + Mul(y, c));

  return FixedVec3{xp, yp, z};
}

FixedVec3 FixedVec3::RotateEuler(F yaw, F pitch, F roll) const {
  // Apply in Y (yaw), then X (pitch), then Z (roll).
  // v' = Rz(roll) * Rx(pitch) * Ry(yaw) * v   (column-vector convention)
  // We compose by applying to the vector in sequence: Ry -> Rx -> Rz.
  FixedVec3 v = this->RotateAroundY(yaw);
  v           = v.RotateAroundX(pitch);
  v           = v.RotateAroundZ(roll);
  
  return v;
}
}  // namespace navary::math

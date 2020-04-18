#ifndef VP_MATH_HPP
#define VP_MATH_HPP

#include <math.h>

namespace vpe {
namespace math
{
  inline void clampAngle(float& _angle) { _angle -= 360.0f * floor(_angle / 360.0f); }
}
}

#endif
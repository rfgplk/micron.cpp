#include <micron/math/quaternions/algebra.hpp>

int main()
{
  namespace q = micron::math::quaternions;
  q::quaternion<double> value(0, 0, 0, 2);
  auto unit = q::normalize(value);
  auto conjugate = q::conjugate(unit);
  return conjugate.w > 0.999 && conjugate.w < 1.001 ? 0 : 1;
}

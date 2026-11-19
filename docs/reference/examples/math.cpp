#include <micron/math/sqrt.hpp>

int main()
{
  double root = micron::math::fsqrt(9.0);
  return root > 2.999 && root < 3.001 ? 0 : 1;
}

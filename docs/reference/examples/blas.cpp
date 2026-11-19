#include <micron/math/blas/level1.hpp>

int main()
{
  const double x[]{ 1, 2, 3 };
  double y[]{ 10, 20, 30 };
  micron::math::blas::level1::axpy(2.0, x, x + 3, y);
  return y[0] == 12 && y[1] == 24 && y[2] == 36 ? 0 : 1;
}

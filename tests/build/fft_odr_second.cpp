#include "../../src/math/fft.hpp"

max_t
fft_other(const micron::math::fft::plan<double> &p, const micron::math::complex<double> *in, micron::math::complex<double> *out,
          micron::math::fft::workspace_view<double> w)
{
  return p.execute(in, out, w);
}

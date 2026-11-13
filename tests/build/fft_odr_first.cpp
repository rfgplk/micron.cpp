#include "../../src/math/fft.hpp"
extern max_t fft_other(const micron::math::fft::plan<double> &, const micron::math::complex<double> *, micron::math::complex<double> *,
                       micron::math::fft::workspace_view<double>);

#if defined(__micron_freestanding)
extern "C"
#endif
    int
    main()
{
  namespace f = micron::math::fft;
  f::plan<double> p(7);
  f::workspace<double> w(p);
  micron::math::complex<double> x[7]{}, y[7];
  x[0] = { 1 };
  if ( fft_other(p, x, y, w.view()) != sizeof(y) ) return 6;
  for ( auto z : y )
    if ( z.real != 1 || z.imag != 0 ) return 6;
  return 1;
}

//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft.hpp"
#include "../snowball/snowball.hpp"
namespace ft = micron::math::fft;
using ft::buffer;
using micron::math::complex;
#include "../support/fft/fixtures.hpp"
#include "../support/fft/oracle.hpp"

template<typename F>
static void
precision()
{
  for ( usize n : { usize(17), usize(32), usize(101), usize(256) } ) {
    ft::plan<F> p(n);
    ft::workspace<F> w(p);
    buffer<complex<F>> x(n), y(n);
    for ( usize j = 0; j < n; ++j ) x[j] = { F(int(j % 13) - 6) / F(8), F(int(j % 7) - 3) / F(16) };
    sb::require_true(p.execute(x.data(), y.data(), w.view()) > 0);
    for ( const auto &ref : fft_references )
      if ( ref.n == n ) {
        long double tolerance = epsilon<F>() * 64 * n;
        check(y[ref.k].real, ref.real, tolerance);
        check(y[ref.k].imag, ref.imag, tolerance);
      }
  }
}

int
main()
{
  sb::test_case("FFT against independent 256-bit fixtures in every supported scalar precision");
  precision<float>();
  precision<double>();
  precision<long double>();
  sb::end_test_case();
  return 1;
}

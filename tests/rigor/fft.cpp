//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft.hpp"
#include "../snowball/snowball.hpp"

namespace ft = micron::math::fft;
using ft::buffer;
using micron::math::complex;
using micron::math::conj;

#include "../support/fft/oracle.hpp"

template<typename F>
static void
complex_transforms()
{
  constexpr long double pi = 3.1415926535897932384626433832795028841971693993751L;
  for ( usize n = 1; n <= 180; ++n ) {
    ft::plan<F> p(n);
    ft::workspace<F> w(p);
    buffer<complex<F>> in(n), out(n), back(n);
    for ( usize j = 0; j < n; ++j ) in[j] = { F(sample()), F(sample()) };
    sb::require_true(p.execute(in.data(), out.data(), w.view()) == max_t(n * sizeof(complex<F>)));
    long double tolerance = (epsilon<F>() * 80 + __micron_ldbl_epsilon * 100) * n;
    for ( usize k = 0; k < n; ++k ) {
      long double re = 0, im = 0;
      for ( usize j = 0; j < n; ++j ) {
        long double phase = -2 * pi * static_cast<long double>((j * k) % n) / n;
        auto c = cosine(phase), s = sine(phase);
        re += in[j].real * c - in[j].imag * s;
        im += in[j].real * s + in[j].imag * c;
      }
      check(out[k].real, re, tolerance);
      check(out[k].imag, im, tolerance);
    }
    auto preserve = in;
    sb::require_true(p.execute(in.data(), in.data(), w.view()) > 0);
    for ( usize j = 0; j < n; ++j ) {
      check(in[j].real, out[j].real, tolerance);
      check(in[j].imag, out[j].imag, tolerance);
    }
    in = preserve;
    sb::require_true(p.execute(out.data(), back.data(), w.view(), ft::direction::backward) > 0);
    for ( usize j = 0; j < n; ++j ) {
      check(back[j].real, in[j].real, tolerance / n);
      check(back[j].imag, in[j].imag, tolerance / n);
    }
  }
}

static long double
real_oracle(const buffer<double> &x, usize k, ft::r2r_kind kind)
{
  const usize n = x.size();
  constexpr long double pi = 3.1415926535897932384626433832795028841971693993751L;
  long double y = 0;
  for ( usize j = 0; j < n; ++j ) {
    long double a = 0, w = 2;
    switch ( kind ) {
    case ft::r2r_kind::dct_i:
      a = pi * j * k / (n - 1);
      if ( j == 0 || j + 1 == n ) w = 1;
      break;
    case ft::r2r_kind::dct_ii:
      a = pi * (j + .5L) * k / n;
      break;
    case ft::r2r_kind::dct_iii:
      a = pi * j * (k + .5L) / n;
      if ( j == 0 ) w = 1;
      break;
    case ft::r2r_kind::dct_iv:
      a = pi * (j + .5L) * (k + .5L) / n;
      break;
    case ft::r2r_kind::dst_i:
      a = pi * (j + 1) * (k + 1) / (n + 1);
      break;
    case ft::r2r_kind::dst_ii:
      a = pi * (j + .5L) * (k + 1) / n;
      break;
    case ft::r2r_kind::dst_iii:
      a = pi * (j + 1) * (k + .5L) / n;
      if ( j + 1 == n ) w = 1;
      break;
    case ft::r2r_kind::dst_iv:
      a = pi * (j + .5L) * (k + .5L) / n;
      break;
    }
    y += w * x[j] * (unsigned(kind) < 4 ? cosine(a) : sine(a));
  }
  return y;
}

static void
real_transforms()
{
  for ( usize n = 1; n <= 35; ++n ) {
    buffer<double> in(n);
    for ( auto &x : in ) x = double(sample());
    auto spectrum = ft::rfft(in.data(), n);
    auto back = ft::irfft(spectrum.data(), n);
    buffer<complex<double>> full(n);
    for ( usize j = 0; j < n; ++j ) full[j] = { in[j] };
    auto ref = ft::transform(full.data(), n);
    for ( usize k = 0; k < spectrum.size(); ++k ) {
      check(spectrum[k].real, ref[k].real, 1e-11L);
      check(spectrum[k].imag, ref[k].imag, 1e-11L);
    }
    for ( usize j = 0; j < n; ++j ) check(back[j], in[j], 1e-12L);
    for ( unsigned t = 0; t < 8; ++t ) {
      if ( n == 1 && t == 0 ) continue;
      auto kind = ft::r2r_kind(t);
      for ( auto mode : { ft::normalization::backward, ft::normalization::ortho, ft::normalization::none, ft::normalization::forward } ) {
        ft::r2r_plan<double> p(n, kind, mode);
        ft::workspace<double> w(p);
        buffer<double> out(n), restored(n);
        sb::require_true(p.execute(in.data(), out.data(), w.view()) > 0);
        usize logical = t == 0 ? 2 * (n - 1) : t == 4 ? 2 * (n + 1) : 2 * n;
        if ( mode != ft::normalization::ortho )
          for ( usize k = 0; k < n; ++k )
            check(out[k], real_oracle(in, k, kind) / (mode == ft::normalization::forward ? logical : 1), 1e-10L);
        else {
          long double a = 0, b = 0;
          for ( usize k = 0; k < n; ++k ) {
            a += in[k] * in[k];
            b += out[k] * out[k];
          }
          check(b, a, 1e-11L);
        }
        sb::require_true(p.execute(out.data(), restored.data(), w.view(), ft::direction::backward) > 0);
        for ( usize j = 0; j < n; ++j ) check(restored[j], in[j] * (mode == ft::normalization::none ? logical : 1), 1e-10L);
      }
    }
  }
}

template<typename F>
static void
real_precision()
{
  for ( usize n = 1; n <= 25; ++n ) {
    buffer<F> x(n);
    buffer<double> reference(n);
    buffer<complex<F>> cx(n);
    for ( usize j = 0; j < n; ++j ) {
      reference[j] = double(int(j % 9) - 4) / 8;
      x[j] = F(reference[j]);
      cx[j] = { x[j] };
    }
    auto spectrum = ft::rfft(x.data(), n), full = ft::transform(cx.data(), n);
    auto back = ft::irfft(spectrum.data(), n);
    long double tolerance = (epsilon<F>() * 100 + __micron_ldbl_epsilon * 100) * n;
    for ( usize k = 0; k < spectrum.size(); ++k ) {
      check(spectrum[k].real, full[k].real, tolerance);
      check(spectrum[k].imag, full[k].imag, tolerance);
    }
    for ( usize j = 0; j < n; ++j ) check(back[j], x[j], tolerance);
    for ( unsigned t = 0; t < 8; ++t ) {
      if ( !t && n == 1 ) continue;
      auto result = ft::real_transform(x.data(), n, ft::r2r_kind(t));
      for ( usize k = 0; k < n; ++k ) check(result[k], real_oracle(reference, k, ft::r2r_kind(t)), tolerance);
    }
  }
}

int
main()
{
  sb::test_case("FFT float/double/long double against independent direct DFT, lengths 1..180");
  complex_transforms<float>();
  complex_transforms<double>();
  complex_transforms<long double>();
  sb::end_test_case();
  sb::test_case("Packed real, all DCT/DST definitions and orthonormal energy");
  real_transforms();
  real_precision<float>();
  real_precision<long double>();
  sb::end_test_case();
  return 1;
}

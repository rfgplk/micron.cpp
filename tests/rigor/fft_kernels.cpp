//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
#include "../../src/math/fft.hpp"
#include "../snowball/snowball.hpp"
namespace ft = micron::math::fft;
using ft::buffer;
using micron::math::complex;
#include "../support/fft/oracle.hpp"

template<typename F>
void
products()
{
  for ( usize n = 0; n <= 129; ++n )
    for ( usize offset = 0; offset < 8; ++offset ) {
      buffer<complex<F>> x(n + offset + 1), y(n + offset + 1), before;
      for ( usize j = 0; j < x.size(); ++j ) {
        x[j] = { F(sample()), F(sample()) };
        y[j] = { F(sample()), F(sample()) };
      }
      before = x;
      ft::__impl::__multiply(x.data() + offset, y.data() + offset, n);
      for ( usize j = 0; j < x.size(); ++j ) {
        if ( j < offset || j >= offset + n )
          sb::require_true(x[j] == before[j]);
        else {
          long double ar = before[j].real, ai = before[j].imag, br = y[j].real, bi = y[j].imag;
          check(x[j].real, ar * br - ai * bi, epsilon<F>() * 8);
          check(x[j].imag, ar * bi + ai * br, epsilon<F>() * 8);
        }
      }
      x = before;
      ft::__impl::__multiply(x.data() + offset, x.data() + offset, n);
      for ( usize j = offset; j < n + offset; ++j ) {
        long double a = before[j].real, b = before[j].imag;
        check(x[j].real, a * a - b * b, epsilon<F>() * 8);
        check(x[j].imag, 2 * a * b, epsilon<F>() * 8);
      }
    }
}

template<typename F, usize R, bool Inverse>
void
stages()
{
  constexpr long double pi = 3.1415926535897932384626433832795028841971693993751L;
  complex<long double> roots[R];
  for ( usize j = 0; j < R; ++j ) {
    long double phase = (Inverse ? 2 : -2) * pi * j / R;
    roots[j] = { cosine(phase), sine(phase) };
  }
  for ( usize m = 1; m <= 35; ++m )
    for ( usize offset = 0; offset < 4; ++offset ) {
      buffer<complex<F>> storage(R * m + offset + 1), tw((R - 1) * m + offset + 1), before;
      auto *x = storage.data() + offset, *w = tw.data() + offset;
      for ( auto &z : storage ) z = { F(sample()), F(sample()) };
      // Arbitrary twiddles catch assumptions about unit magnitude and k=0.
      for ( auto &z : tw ) z = { F(sample()), F(sample()) };
      before = storage;
      usize begin = offset < m ? offset : 0, end = m - (m > begin && (m & 1));
      ft::__impl::__stage<R, Inverse>(x, w, m, begin, end);
      long double tolerance = (epsilon<F>() + __micron_ldbl_epsilon) * R * 32;
      for ( usize i = 0; i < storage.size(); ++i ) {
        if ( i < offset || i == storage.size() - 1 ) {
          sb::require_true(storage[i] == before[i]);
          continue;
        }
        usize l = (i - offset) / m, k = (i - offset) % m;
        if ( k < begin || k >= end ) {
          sb::require_true(storage[i] == before[i]);
          continue;
        }
        long double re = 0, im = 0;
        for ( usize j = 0; j < R; ++j ) {
          long double ar = before[offset + j * m + k].real, ai = before[offset + j * m + k].imag;
          if ( j ) {
            long double br = w[(j - 1) * m + k].real, bi = w[(j - 1) * m + k].imag * (Inverse ? -1 : 1);
            long double t = ar * br - ai * bi;
            ai = ar * bi + ai * br;
            ar = t;
          }
          auto r = roots[(l * j) % R];
          re += ar * r.real - ai * r.imag;
          im += ar * r.imag + ai * r.real;
        }
        check(storage[i].real, re, tolerance);
        check(storage[i].imag, im, tolerance);
      }
    }
}

template<typename F>
void
large_transforms()
{
  for ( usize n : { usize(256), usize(343), usize(512), usize(625), usize(1000), usize(1009), usize(1024), usize(1536), usize(4093),
                    usize(4096), usize(16384), usize(65536) } ) {
    ft::plan<F> p(n);
    ft::workspace<F> w(p);
    buffer<complex<F>> x(n + 1), y(n + 1), back(n + 1);
    for ( usize j = 1; j <= n; ++j ) x[j] = { F(sample()), F(sample()) };
    sb::require_true(p.execute(x.data() + 1, y.data() + 1, w.view()) > 0);
    // A few independent DFT bins test phase/order; a round trip alone can hide paired defects.
    for ( usize k : { usize(0), usize(1), n / 3, n - 1 } ) {
#ifdef MICRON_FFT_TEST_MAX_ORACLE_N
      if ( n > MICRON_FFT_TEST_MAX_ORACLE_N ) break;
#endif
      long double re = 0, im = 0;
      for ( usize j = 0; j < n; ++j ) {
        long double phase = -6.2831853071795864769252867665590057683943387987502L * ((j * k) % n) / n;
        auto c = cosine(phase), s = sine(phase);
        re += x[j + 1].real * c - x[j + 1].imag * s;
        im += x[j + 1].real * s + x[j + 1].imag * c;
      }
      check(y[k + 1].real, re, (epsilon<F>() + __micron_ldbl_epsilon) * n * 16);
      check(y[k + 1].imag, im, (epsilon<F>() + __micron_ldbl_epsilon) * n * 16);
    }
    sb::require_true(p.execute(y.data() + 1, back.data() + 1, w.view(), ft::direction::backward) > 0);
    for ( usize j = 1; j <= n; ++j ) {
      check(back[j].real, x[j].real, epsilon<F>() * 256);
      check(back[j].imag, x[j].imag, epsilon<F>() * 256);
    }
  }
}

template<typename F>
void
check_type()
{
  products<F>();
  stages<F, 2, false>();
  stages<F, 2, true>();
  stages<F, 3, false>();
  stages<F, 3, true>();
  stages<F, 4, false>();
  stages<F, 4, true>();
  stages<F, 5, false>();
  stages<F, 5, true>();
  stages<F, 7, false>();
  stages<F, 7, true>();
  stages<F, 8, false>();
  stages<F, 8, true>();
}

int
main()
{
#if defined(__micron_arch_arm32) && defined(__micron_arm_neon) && !defined(MICRON_NO_SIMD) && !defined(__micron_simd_generic)
  sb::test_case("ARMv7 NEON native subnormal flushing");
  complex<float> small[4]
      = { { micron::numeric_limits<float>::min() / 4, 0 }, { micron::numeric_limits<float>::min(), 0 }, { 1, 0 }, { -1, 0 } };
  complex<float> factors[4] = { { 1, 0 }, { 0.5f, 0 }, { 2, 0 }, { 2, 0 } };
  ft::__impl::__multiply(small, factors, 4);
  sb::require_true(small[0].real == 0 && small[1].real == 0);
  sb::require_true(small[2].real == 2 && small[3].real == -2);
  sb::end_test_case();
#endif
  sb::test_case("FFT packets, scalar tails, bounded stage slices and independent DFT oracles");
  check_type<float>();
  check_type<double>();
  check_type<long double>();
  sb::end_test_case();
  sb::test_case("FFT large smooth/prime lengths and offset buffers");
  large_transforms<float>();
  large_transforms<double>();
  sb::end_test_case();
  return 1;
}

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "kernels.hpp"

namespace micron::math::fft::__impl
{

template<fft_scalar F> class __engine
{
  usize __n{}, __m{};
  buffer<complex<F>> __twiddles, __chirp, __kernel;
  buffer<usize> __sizes, __radices, __twiddle_offsets;

  static usize
  __radix(usize n) noexcept
  {
    if ( n == 8 ) return 8;
    for ( usize r : { usize(4), usize(2), usize(3), usize(5), usize(7) } )
      if ( n % r == 0 ) return r;
    return n;
  }

  template<bool Inverse>
  void
  __combine_stage(complex<F> *out, usize stage, usize begin, usize end) const noexcept
  {
    usize m = __sizes[stage + 1];
    const auto *tw = __twiddles.data() + __twiddle_offsets[stage];
    switch ( __radices[stage] ) {
    case 2:
      __stage<2, Inverse>(out, tw, m, begin, end);
      break;
    case 3:
      __stage<3, Inverse>(out, tw, m, begin, end);
      break;
    case 4:
      __stage<4, Inverse>(out, tw, m, begin, end);
      break;
    case 5:
      __stage<5, Inverse>(out, tw, m, begin, end);
      break;
    case 7:
      __stage<7, Inverse>(out, tw, m, begin, end);
      break;
    }
  }

  template<bool Inverse>
  void
  __smooth_stage(const complex<F> *in, complex<F> *out, usize stage, usize stride) const noexcept
  {
    if ( stage == __radices.size() ) {
      *out = *in;
      return;
    }
    usize r = __radices[stage], m = __sizes[stage + 1];
    if ( m == 1 ) {
      switch ( r ) {
      case 2:
        __leaf<2, Inverse>(in, out, stride);
        break;
      case 3:
        __leaf<3, Inverse>(in, out, stride);
        break;
      case 4:
        __leaf<4, Inverse>(in, out, stride);
        break;
      case 5:
        __leaf<5, Inverse>(in, out, stride);
        break;
      case 7:
        __leaf<7, Inverse>(in, out, stride);
        break;
      case 8:
        __leaf<8, Inverse>(in, out, stride);
        break;
      }
      return;
    }
    for ( usize j = 0; j < r; ++j ) __smooth_stage<Inverse>(in + j * stride, out + j * m, stage + 1, stride * r);
    __combine_stage<Inverse>(out, stage, 0, m);
  }

  void
  __smooth(const complex<F> *in, complex<F> *out, usize n, usize stride, bool inverse) const noexcept
  {
    usize stage = 0;
    while ( __sizes[stage] != n ) ++stage;
    if ( inverse )
      __smooth_stage<true>(in, out, stage, stride);
    else
      __smooth_stage<false>(in, out, stage, stride);
  }

  void
  __combine(complex<F> *out, usize, bool inverse, usize begin, usize end) const noexcept
  {
    if ( inverse )
      __combine_stage<true>(out, 0, begin, end);
    else
      __combine_stage<false>(out, 0, begin, end);
  }

  template<typename Runner>
  void
  __parallel_smooth(const complex<F> *in, complex<F> *out, bool inverse, Runner &runner) const noexcept
  {
    if ( __m < 1024 || runner.workers == 1 ) {
      __smooth(in, out, __m, 1, inverse);
      return;
    }
    usize r = __radix(__m), m = __m / r;
    runner(r, out, 0, [&](usize j, complex<F> *) noexcept { __smooth(in + j, out + j * m, m, r, inverse); });
    constexpr usize block = 64;
    runner((m + block - 1) / block, out, 0, [&](usize j, complex<F> *) noexcept {
      usize end = (j + 1) * block < m ? (j + 1) * block : m;
      __combine(out, __m, inverse, j * block, end);
    });
  }

public:
  explicit __engine(usize n) : __n(n)
  {
    if ( !n || n > __limit / (16 * sizeof(complex<F>)) ) exc<except::length_error>("fft: unsupported length");
    usize remaining = n;
    for ( usize r : { usize(2), usize(3), usize(5), usize(7) } )
      while ( remaining % r == 0 ) remaining /= r;
    __m = remaining == 1 ? n : next_fast_len(__add(n, n) - 1);
    usize offset = 0;
    for ( usize size = __m; size > 1; ) {
      usize radix = __radix(size);
      __sizes.push_back(size);
      __radices.push_back(radix);
      __twiddle_offsets.push_back(offset);
      offset += (radix - 1) * (size / radix);
      size /= radix;
    }
    __sizes.push_back(1);
    __twiddles.resize(offset);
    for ( usize stage = 0; stage < __radices.size(); ++stage ) {
      usize r = __radices[stage], m = __sizes[stage + 1], step = __m / __sizes[stage];
      auto *tw = __twiddles.data() + __twiddle_offsets[stage];
      for ( usize j = 1; j < r; ++j )
        for ( usize k = 0; k < m; ++k ) {
          auto w = __root<F>(j * k * step, __m);
          tw[(j - 1) * m + k] = w;
        }
    }
    if ( __m == __n ) return;
    __chirp.resize(n);
    __kernel.resize(__m);
    buffer<complex<F>> time(__m);
    usize square = 0, delta = 1, modulus = 2 * n;
    for ( usize k = 0; k < n; ++k ) {
      __chirp[k] = __root<F>(square, modulus);
      time[k] = conj(__chirp[k]);
      if ( k ) time[__m - k] = time[k];
      square = (square + delta) % modulus;
      delta = (delta + 2) % modulus;
    }
    __smooth(time.data(), __kernel.data(), __m, 1, false);
  }

  usize
  size() const noexcept
  {
    return __n;
  }

  usize
  scratch_size() const noexcept
  {
    return __m == __n ? 0 : 2 * __m;
  }

  bool
  uses_bluestein() const noexcept
  {
    return __m != __n;
  }

  template<typename Runner>
  void
  execute_parallel(const complex<F> *in, complex<F> *out, complex<F> *scratch, bool inverse, Runner &runner) const noexcept
  {
    if ( __m == __n ) {
      __parallel_smooth(in, out, inverse, runner);
      return;
    }
    auto *a = scratch, *b = scratch + __m;
    for ( usize k = 0; k < __m; ++k ) a[k] = k < __n ? (inverse ? conj(in[k]) : in[k]) * __chirp[k] : complex<F>{};
    __parallel_smooth(a, b, false, runner);
    constexpr usize block = 256;
    runner((__m + block - 1) / block, b, 0, [&](usize j, complex<F> *) noexcept {
      usize begin = j * block, count = __m - begin < block ? __m - begin : block;
      __multiply(b + begin, __kernel.data() + begin, count);
    });
    __parallel_smooth(b, a, true, runner);
    for ( usize k = 0; k < __n; ++k ) {
      auto z = (a[k] * __chirp[k]) / F(__m);
      out[k] = inverse ? conj(z) : z;
    }
  }

  void
  execute(const complex<F> *in, complex<F> *out, complex<F> *scratch, bool inverse) const noexcept
  {
    if ( __m == __n ) {
      __smooth(in, out, __n, 1, inverse);
      return;
    }
    auto *a = scratch, *b = scratch + __m;
    for ( usize k = 0; k < __m; ++k ) a[k] = k < __n ? (inverse ? conj(in[k]) : in[k]) * __chirp[k] : complex<F>{};
    __smooth(a, b, __m, 1, false);
    __multiply(b, __kernel.data(), __m);
    __smooth(b, a, __m, 1, true);
    for ( usize k = 0; k < __n; ++k ) {
      auto z = (a[k] * __chirp[k]) / F(__m);
      out[k] = inverse ? conj(z) : z;
    }
  }
};

};      // namespace micron::math::fft::__impl

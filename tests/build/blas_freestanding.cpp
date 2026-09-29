// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt

#include <micron/math/blas/level2.hpp>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// linked freestanding regression: LTO must retain data-dependent BLAS arithmetic

volatile f32 blas_input = 0.25f;
volatile f32 blas_actual = 0;

template<class F>
[[gnu::noinline]] bool
check_blas(F v)
{
  F weights[64 * 8]{}, input[8]{}, output[64]{};
  input[0] = v;
  for ( usize j = 0; j < 64; ++j ) weights[j * 8] = 1;
  micron::math::blas::level2::gemv_row(false, 64, 8, F(1), weights, 8, input, 1, F(0), output, 1);
  blas_actual = output[0];
  for ( F y : output )
    if ( y != v ) return false;
  return true;
}

int
main()
{
  const f32 v = blas_input;
#if defined(__AVX2__) && defined(__FMA__)
  float a[8]{ 1, 0, 0, 0, 0, 0, 0, 0 }, b[8]{ float(v), 0, 0, 0, 0, 0, 0, 0 }, c[8]{};
  const auto mul
      = micron::simd::fma::fma_f32(micron::simd::avx::loadu_f32(a), micron::simd::avx::loadu_f32(b), micron::simd::avx::zero_f32());
  micron::simd::avx::storeu_f32(c, mul);
  if ( c[0] != v ) return 30;
  const auto sum = micron::simd::sse::add_f32(micron::simd::avx::cast_f32_to_lo128(mul), micron::simd::avx::extract_f128_f32<1>(mul));
  const auto pair = micron::simd::sse::hadd_f32(sum, sum);
  const auto single = micron::simd::sse::hadd_f32(pair, pair);
  if ( micron::simd::sse::extract_low_f32(single) != v ) return 31;
  if ( micron::math::fma<float>(1.f, micron::simd::sse::extract_low_f32(single), 0.f) != v ) return 32;
#endif
  if ( micron::math::fma<f32>(v, f32(2), f32(3)) != f32(3.5) ) return 20;
  if ( micron::math::fma<float>(float(v), 2.0f, 3.0f) != 3.5f ) return 21;
  if ( !check_blas<float>(v) ) return 22;
  if ( !check_blas<f32>(v) ) return 23;
  return 1;
}

// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#include "../src/bits/wasm/math/special.hpp"
#include "../src/simd/simd.hpp"
namespace s = micron::simd;
namespace w = micron::wasm::math;
static float input[4096], output[4096];

extern "C" __attribute__((export_name("bench_simd_width"))) unsigned
bench_simd_width()
{
  return __micron_simd_width;
}

extern "C" __attribute__((export_name("bench_initialize"))) void
bench_initialize()
{
  for ( unsigned i = 0; i < 4096; ++i ) input[i] = float(i % 113) / 113;
}

extern "C" __attribute__((export_name("bench_simd"))) double
bench_simd(unsigned iterations)
{
  s::vfloat multiplier(0.9999f), increment(0.0001f);
  for ( unsigned repeat = 0; repeat < iterations; ++repeat )
    for ( unsigned i = 0; i < 4096; i += 4 ) {
      s::vfloat value;
      value.load(input + i);
      value = value * multiplier + increment;
      value.get(output + i);
      s::vfloat again;
      again.load(output + i);
      again = again * multiplier + increment;
      again.get(input + i);
    }
  double sum = 0;
  for ( float x : input ) sum += x;
  return sum;
}

extern "C" __attribute__((export_name("bench_quad_fma"))) double
bench_quad_fma(unsigned iterations)
{
  long double value = 1.25L;
  for ( unsigned i = 0; i < iterations; ++i ) value = w::fma(value, 0.9999L, 0.0001L);
  return double(value);
}

extern "C" __attribute__((export_name("bench_quad_sqrt"))) double
bench_quad_sqrt(unsigned iterations)
{
  long double value = 1.25L;
  for ( unsigned i = 0; i < iterations; ++i ) value = w::sqrt(value + 1);
  return double(value);
}

extern "C" __attribute__((export_name("bench_quad_sin"))) double
bench_quad_sin(unsigned iterations)
{
  long double value = 1.25L;
  for ( unsigned i = 0; i < iterations; ++i ) value = w::sin(value + 0.001L);
  return double(value);
}

int
main()
{
  bench_initialize();
  return 1;
}

extern "C" __attribute__((export_name("bench_quad_lgamma"))) double
bench_quad_lgamma(unsigned iterations)
{
  long double sum = 0;
  for ( unsigned i = 0; i < iterations; ++i ) sum += w::lgamma(1.L + (i % 257 + 1) * 0x1p-12L);
  return double(sum);
}

extern "C" __attribute__((export_name("bench_quad_gamma"))) double
bench_quad_gamma(unsigned iterations)
{
  long double sum = 0;
  for ( unsigned i = 0; i < iterations; ++i ) sum += w::tgamma(0.25L + (i % 128));
  return double(sum);
}

extern "C" __attribute__((export_name("bench_quad_pow"))) double
bench_quad_pow(unsigned iterations)
{
  long double sum = 0;
  for ( unsigned i = 0; i < iterations; ++i ) sum += w::pow(1.L + (i % 257 + 1) * 0x1p-24L, 65535.L);
  return double(sum);
}

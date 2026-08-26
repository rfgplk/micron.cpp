//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../__vec_simd.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace __integrate_arch
{

[[nodiscard, gnu::always_inline]] inline f32
__horizontal(simd::f128 value) noexcept
{
  value = simd::sse::add_f32(value, simd::sse::shuffle_f32<0x4e>(value, value));
  value = simd::sse::add_f32(value, simd::sse::shuffle_f32<0xb1>(value, value));
  return f32(simd::sse::extract_low_f32(value));
}

[[nodiscard, gnu::always_inline]] inline f64
__horizontal(simd::d128 value) noexcept
{
  value = simd::sse::add_f64(value, simd::sse::shuffle_f64<1>(value, value));
  return f64(simd::sse::extract_low_f64(value));
}

[[nodiscard, gnu::always_inline]] inline f32
sum_fast(const f32 *values, usize count) noexcept
{
  simd::f128 a0 = simd::sse::zero_f32(), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
#if defined(__micron_x86_avx2)
  simd::f256 b0 = simd::avx::zero_f32(), b1 = b0, b2 = b0, b3 = b0;
  for ( ; i + 32 <= count; i += 32 ) {
    b0 = simd::avx::add_f32(b0, simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i)));
    b1 = simd::avx::add_f32(b1, simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 8)));
    b2 = simd::avx::add_f32(b2, simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 16)));
    b3 = simd::avx::add_f32(b3, simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 24)));
  }
  b0 = simd::avx::add_f32(simd::avx::add_f32(b0, b1), simd::avx::add_f32(b2, b3));
  a0 = simd::sse::add_f32(simd::avx::cast_f32_to_lo128(b0), simd::avx::extract_f128_f32<1>(b0));
#endif
  for ( ; i + 16 <= count; i += 16 ) {
    a0 = simd::sse::add_f32(a0, simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i)));
    a1 = simd::sse::add_f32(a1, simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 4)));
    a2 = simd::sse::add_f32(a2, simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 8)));
    a3 = simd::sse::add_f32(a3, simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 12)));
  }
  f32 result = __horizontal(simd::sse::add_f32(simd::sse::add_f32(a0, a1), simd::sse::add_f32(a2, a3)));
  for ( ; i < count; ++i ) result += values[i];
  return result;
}

[[nodiscard, gnu::always_inline]] inline f64
sum_fast(const f64 *values, usize count) noexcept
{
  simd::d128 a0 = simd::sse::zero_f64(), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
#if defined(__micron_x86_avx2)
  simd::d256 b0 = simd::avx::zero_f64(), b1 = b0, b2 = b0, b3 = b0;
  for ( ; i + 16 <= count; i += 16 ) {
    b0 = simd::avx::add_f64(b0, simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i)));
    b1 = simd::avx::add_f64(b1, simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 4)));
    b2 = simd::avx::add_f64(b2, simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 8)));
    b3 = simd::avx::add_f64(b3, simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 12)));
  }
  b0 = simd::avx::add_f64(simd::avx::add_f64(b0, b1), simd::avx::add_f64(b2, b3));
  a0 = simd::sse::add_f64(simd::avx::cast_f64_to_lo128(b0), simd::avx::extract_f128_f64<1>(b0));
#endif
  for ( ; i + 8 <= count; i += 8 ) {
    a0 = simd::sse::add_f64(a0, simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i)));
    a1 = simd::sse::add_f64(a1, simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 2)));
    a2 = simd::sse::add_f64(a2, simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 4)));
    a3 = simd::sse::add_f64(a3, simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 6)));
  }
  f64 result = __horizontal(simd::sse::add_f64(simd::sse::add_f64(a0, a1), simd::sse::add_f64(a2, a3)));
  for ( ; i < count; ++i ) result += values[i];
  return result;
}

[[nodiscard, gnu::always_inline]] inline f32
weighted_sum_fast(const f32 *weights, const f32 *values, usize count) noexcept
{
  simd::f128 a0 = simd::sse::zero_f32(), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
#if defined(__micron_x86_avx2)
  simd::f256 b0 = simd::avx::zero_f32(), b1 = b0, b2 = b0, b3 = b0;
  for ( ; i + 32 <= count; i += 32 ) {
#if defined(__micron_x86_fma)
    b0 = simd::fma::fma_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i)),
                            simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i)), b0);
    b1 = simd::fma::fma_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 8)),
                            simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 8)), b1);
    b2 = simd::fma::fma_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 16)),
                            simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 16)), b2);
    b3 = simd::fma::fma_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 24)),
                            simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 24)), b3);
#else
    b0 = simd::avx::add_f32(b0, simd::avx::mul_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i)),
                                                   simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i))));
    b1 = simd::avx::add_f32(b1, simd::avx::mul_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 8)),
                                                   simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 8))));
    b2 = simd::avx::add_f32(b2, simd::avx::mul_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 16)),
                                                   simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 16))));
    b3 = simd::avx::add_f32(b3, simd::avx::mul_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(weights + i + 24)),
                                                   simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i + 24))));
#endif
  }
  b0 = simd::avx::add_f32(simd::avx::add_f32(b0, b1), simd::avx::add_f32(b2, b3));
  a0 = simd::sse::add_f32(simd::avx::cast_f32_to_lo128(b0), simd::avx::extract_f128_f32<1>(b0));
#endif
  for ( ; i + 16 <= count; i += 16 ) {
    a0 = simd::sse::add_f32(a0, simd::sse::mul_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(weights + i)),
                                                   simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i))));
    a1 = simd::sse::add_f32(a1, simd::sse::mul_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(weights + i + 4)),
                                                   simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 4))));
    a2 = simd::sse::add_f32(a2, simd::sse::mul_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(weights + i + 8)),
                                                   simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 8))));
    a3 = simd::sse::add_f32(a3, simd::sse::mul_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(weights + i + 12)),
                                                   simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 12))));
  }
  f32 result = __horizontal(simd::sse::add_f32(simd::sse::add_f32(a0, a1), simd::sse::add_f32(a2, a3)));
  for ( ; i < count; ++i ) result += weights[i] * values[i];
  return result;
}

[[nodiscard, gnu::always_inline]] inline f64
weighted_sum_fast(const f64 *weights, const f64 *values, usize count) noexcept
{
  simd::d128 a0 = simd::sse::zero_f64(), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
#if defined(__micron_x86_avx2)
  simd::d256 b0 = simd::avx::zero_f64(), b1 = b0, b2 = b0, b3 = b0;
  for ( ; i + 16 <= count; i += 16 ) {
#if defined(__micron_x86_fma)
    b0 = simd::fma::fma_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i)),
                            simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i)), b0);
    b1 = simd::fma::fma_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 4)),
                            simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 4)), b1);
    b2 = simd::fma::fma_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 8)),
                            simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 8)), b2);
    b3 = simd::fma::fma_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 12)),
                            simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 12)), b3);
#else
    b0 = simd::avx::add_f64(b0, simd::avx::mul_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i)),
                                                   simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i))));
    b1 = simd::avx::add_f64(b1, simd::avx::mul_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 4)),
                                                   simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 4))));
    b2 = simd::avx::add_f64(b2, simd::avx::mul_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 8)),
                                                   simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 8))));
    b3 = simd::avx::add_f64(b3, simd::avx::mul_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(weights + i + 12)),
                                                   simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i + 12))));
#endif
  }
  b0 = simd::avx::add_f64(simd::avx::add_f64(b0, b1), simd::avx::add_f64(b2, b3));
  a0 = simd::sse::add_f64(simd::avx::cast_f64_to_lo128(b0), simd::avx::extract_f128_f64<1>(b0));
#endif
  for ( ; i + 8 <= count; i += 8 ) {
    a0 = simd::sse::add_f64(a0, simd::sse::mul_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(weights + i)),
                                                   simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i))));
    a1 = simd::sse::add_f64(a1, simd::sse::mul_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(weights + i + 2)),
                                                   simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 2))));
    a2 = simd::sse::add_f64(a2, simd::sse::mul_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(weights + i + 4)),
                                                   simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 4))));
    a3 = simd::sse::add_f64(a3, simd::sse::mul_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(weights + i + 6)),
                                                   simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 6))));
  }
  f64 result = __horizontal(simd::sse::add_f64(simd::sse::add_f64(a0, a1), simd::sse::add_f64(a2, a3)));
  for ( ; i < count; ++i ) result += weights[i] * values[i];
  return result;
}

inline void
diff_uniform(const f32 *values, f32 *out, usize count, f32 dx) noexcept
{
  if ( count < 2 ) {
    if ( count == 1 ) out[0] = f32(0);
    return;
  }
  out[0] = (values[1] - values[0]) / dx;
  const simd::f128 scale = simd::sse::splat_f32(float(f32(0.5) / dx));
  usize i = 1;
  for ( ; i + 4 <= count - 1; i += 4 ) {
    const simd::f128 lo = simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i - 1));
    const simd::f128 hi = simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i + 1));
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + i), simd::sse::mul_f32(simd::sse::sub_f32(hi, lo), scale));
  }
  for ( ; i + 1 < count; ++i ) out[i] = (values[i + 1] - values[i - 1]) * f32(0.5) / dx;
  out[count - 1] = (values[count - 1] - values[count - 2]) / dx;
}

inline void
diff_uniform(const f64 *values, f64 *out, usize count, f64 dx) noexcept
{
  if ( count < 2 ) {
    if ( count == 1 ) out[0] = f64(0);
    return;
  }
  out[0] = (values[1] - values[0]) / dx;
  const simd::d128 scale = simd::sse::splat_f64(double(f64(0.5) / dx));
  usize i = 1;
  for ( ; i + 2 <= count - 1; i += 2 ) {
    const simd::d128 lo = simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i - 1));
    const simd::d128 hi = simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i + 1));
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + i), simd::sse::mul_f64(simd::sse::sub_f64(hi, lo), scale));
  }
  for ( ; i + 1 < count; ++i ) out[i] = (values[i + 1] - values[i - 1]) * f64(0.5) / dx;
  out[count - 1] = (values[count - 1] - values[count - 2]) / dx;
}

inline void
cum_trapezoid_uniform(const f32 *values, f32 *out, usize count, f32 dx) noexcept
{
  if ( count == 0 ) return;
  out[0] = f32(0);
  const simd::f128 scale = simd::sse::splat_f32(float(f32(0.5) * dx));
  f32 carry = f32(0);
  usize i = 1;
  for ( ; i + 4 <= count; i += 4 ) {
    simd::f128 v = simd::sse::mul_f32(simd::sse::add_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i - 1)),
                                                         simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i))),
                                      scale);
    v = simd::sse::add_f32(v, simd::sse::cast_i128_to_f32(simd::sse::bsll_i128<4>(simd::sse::cast_f32_to_i128(v))));
    v = simd::sse::add_f32(v, simd::sse::cast_i128_to_f32(simd::sse::bsll_i128<8>(simd::sse::cast_f32_to_i128(v))));
    v = simd::sse::add_f32(v, simd::sse::splat_f32(float(carry)));
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + i), v);
    carry = f32(simd::sse::extract_low_f32(simd::sse::shuffle_f32<0xff>(v, v)));
  }
  for ( ; i < count; ++i ) {
    carry += f32(0.5) * dx * (values[i - 1] + values[i]);
    out[i] = carry;
  }
}

inline void
cum_trapezoid_uniform(const f64 *values, f64 *out, usize count, f64 dx) noexcept
{
  if ( count == 0 ) return;
  out[0] = f64(0);
  const simd::d128 scale = simd::sse::splat_f64(double(f64(0.5) * dx));
  f64 carry = f64(0);
  usize i = 1;
  for ( ; i + 2 <= count; i += 2 ) {
    simd::d128 v = simd::sse::mul_f64(simd::sse::add_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i - 1)),
                                                         simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i))),
                                      scale);
    v = simd::sse::add_f64(v, simd::sse::cast_i128_to_f64(simd::sse::bsll_i128<8>(simd::sse::cast_f64_to_i128(v))));
    v = simd::sse::add_f64(v, simd::sse::splat_f64(double(carry)));
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + i), v);
    carry = f64(simd::sse::extract_low_f64(simd::sse::shuffle_f64<1>(v, v)));
  }
  for ( ; i < count; ++i ) {
    carry += f64(0.5) * dx * (values[i - 1] + values[i]);
    out[i] = carry;
  }
}

inline void
affine_transform(const f32 *values, f32 *out, usize count, f32 scale, f32 offset) noexcept
{
  const simd::f128 s = simd::sse::splat_f32(float(scale));
  const simd::f128 o = simd::sse::splat_f32(float(offset));
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 v = simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i));
#if defined(__micron_x86_fma)
    const simd::f128 r = simd::fma::fma_f32(v, s, o);
#else
    const simd::f128 r = simd::sse::add_f32(simd::sse::mul_f32(v, s), o);
#endif
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + i), r);
  }
  for ( ; i < count; ++i ) out[i] = values[i] * scale + offset;
}

inline void
affine_transform(const f64 *values, f64 *out, usize count, f64 scale, f64 offset) noexcept
{
  const simd::d128 s = simd::sse::splat_f64(double(scale));
  const simd::d128 o = simd::sse::splat_f64(double(offset));
  usize i = 0;
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 v = simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i));
#if defined(__micron_x86_fma)
    const simd::d128 r = simd::fma::fma_f64(v, s, o);
#else
    const simd::d128 r = simd::sse::add_f64(simd::sse::mul_f64(v, s), o);
#endif
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + i), r);
  }
  for ( ; i < count; ++i ) out[i] = values[i] * scale + offset;
}

};      // namespace __integrate_arch
};      // namespace integrate
};      // namespace math
};      // namespace micron

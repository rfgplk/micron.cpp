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
namespace splines
{
namespace __spline_arch
{

[[nodiscard, gnu::always_inline]] inline simd::f128
__madd(simd::f128 a, simd::f128 b, simd::f128 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fma_f32(a, b, c);
#else
  return simd::sse::add_f32(simd::sse::mul_f32(a, b), c);
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::d128
__madd(simd::d128 a, simd::d128 b, simd::d128 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fma_f64(a, b, c);
#else
  return simd::sse::add_f64(simd::sse::mul_f64(a, b), c);
#endif
}

#if defined(__micron_x86_avx2)
[[nodiscard, gnu::always_inline]] inline simd::f256
__madd(simd::f256 a, simd::f256 b, simd::f256 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fma_f32(a, b, c);
#else
  return simd::avx::add_f32(simd::avx::mul_f32(a, b), c);
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::d256
__madd(simd::d256 a, simd::d256 b, simd::d256 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fma_f64(a, b, c);
#else
  return simd::avx::add_f64(simd::avx::mul_f64(a, b), c);
#endif
}
#endif

inline void
cubic_horner_batch(const poly_coeffs<f32, 3> &p, f32 origin, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::f256 o8 = simd::avx::splat_f32(float(origin));
  const simd::f256 c08 = simd::avx::splat_f32(float(p[0]));
  const simd::f256 c18 = simd::avx::splat_f32(float(p[1]));
  const simd::f256 c28 = simd::avx::splat_f32(float(p[2]));
  const simd::f256 c38 = simd::avx::splat_f32(float(p[3]));
  for ( ; i + 8 <= count; i += 8 ) {
    const simd::f256 u = simd::avx::sub_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(x + i)), o8);
    simd::f256 value = __madd(c38, u, c28);
    value = __madd(value, u, c18);
    simd::avx::storeu_f32(reinterpret_cast<float *>(out + i), __madd(value, u, c08));
  }
#endif
  const simd::f128 o4 = simd::sse::splat_f32(float(origin));
  const simd::f128 c04 = simd::sse::splat_f32(float(p[0]));
  const simd::f128 c14 = simd::sse::splat_f32(float(p[1]));
  const simd::f128 c24 = simd::sse::splat_f32(float(p[2]));
  const simd::f128 c34 = simd::sse::splat_f32(float(p[3]));
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 u = simd::sse::sub_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(x + i)), o4);
    simd::f128 value = __madd(c34, u, c24);
    value = __madd(value, u, c14);
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + i), __madd(value, u, c04));
  }
  for ( ; i < count; ++i ) {
    const f32 u = x[i] - origin;
    f32 value = math::fma<f32>(p[3], u, p[2]);
    value = math::fma<f32>(value, u, p[1]);
    out[i] = math::fma<f32>(value, u, p[0]);
  }
}

inline void
cubic_horner_batch(const poly_coeffs<f64, 3> &p, f64 origin, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::d256 o4 = simd::avx::splat_f64(double(origin));
  const simd::d256 c04 = simd::avx::splat_f64(double(p[0]));
  const simd::d256 c14 = simd::avx::splat_f64(double(p[1]));
  const simd::d256 c24 = simd::avx::splat_f64(double(p[2]));
  const simd::d256 c34 = simd::avx::splat_f64(double(p[3]));
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::d256 u = simd::avx::sub_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(x + i)), o4);
    simd::d256 value = __madd(c34, u, c24);
    value = __madd(value, u, c14);
    simd::avx::storeu_f64(reinterpret_cast<double *>(out + i), __madd(value, u, c04));
  }
#endif
  const simd::d128 o2 = simd::sse::splat_f64(double(origin));
  const simd::d128 c02 = simd::sse::splat_f64(double(p[0]));
  const simd::d128 c12 = simd::sse::splat_f64(double(p[1]));
  const simd::d128 c22 = simd::sse::splat_f64(double(p[2]));
  const simd::d128 c32 = simd::sse::splat_f64(double(p[3]));
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 u = simd::sse::sub_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(x + i)), o2);
    simd::d128 value = __madd(c32, u, c22);
    value = __madd(value, u, c12);
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + i), __madd(value, u, c02));
  }
  for ( ; i < count; ++i ) {
    const f64 u = x[i] - origin;
    f64 value = math::fma<f64>(p[3], u, p[2]);
    value = math::fma<f64>(value, u, p[1]);
    out[i] = math::fma<f64>(value, u, p[0]);
  }
}

inline void
linear_batch(f32 origin, f32 y0, f32 slope, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::f256 o8 = simd::avx::splat_f32(float(origin));
  const simd::f256 y8 = simd::avx::splat_f32(float(y0));
  const simd::f256 m8 = simd::avx::splat_f32(float(slope));
  for ( ; i + 8 <= count; i += 8 ) {
    const simd::f256 u = simd::avx::sub_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(x + i)), o8);
    simd::avx::storeu_f32(reinterpret_cast<float *>(out + i), __madd(m8, u, y8));
  }
#endif
  const simd::f128 o4 = simd::sse::splat_f32(float(origin));
  const simd::f128 y4 = simd::sse::splat_f32(float(y0));
  const simd::f128 m4 = simd::sse::splat_f32(float(slope));
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 u = simd::sse::sub_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(x + i)), o4);
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + i), __madd(m4, u, y4));
  }
  for ( ; i < count; ++i ) out[i] = math::fma<f32>(slope, x[i] - origin, y0);
}

inline void
linear_batch(f64 origin, f64 y0, f64 slope, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::d256 o4 = simd::avx::splat_f64(double(origin));
  const simd::d256 y4 = simd::avx::splat_f64(double(y0));
  const simd::d256 m4 = simd::avx::splat_f64(double(slope));
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::d256 u = simd::avx::sub_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(x + i)), o4);
    simd::avx::storeu_f64(reinterpret_cast<double *>(out + i), __madd(m4, u, y4));
  }
#endif
  const simd::d128 o2 = simd::sse::splat_f64(double(origin));
  const simd::d128 y2 = simd::sse::splat_f64(double(y0));
  const simd::d128 m2 = simd::sse::splat_f64(double(slope));
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 u = simd::sse::sub_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(x + i)), o2);
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + i), __madd(m2, u, y2));
  }
  for ( ; i < count; ++i ) out[i] = math::fma<f64>(slope, x[i] - origin, y0);
}

inline void
cubic_horner_stream_batch(const poly_coeffs<f32, 3> &p, f32 origin, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
  while ( i < count && (reinterpret_cast<uintptr_t>(out + i) & 63u) != 0 ) {
    const f32 u = x[i] - origin;
    f32 value = math::fma<f32>(p[3], u, p[2]);
    value = math::fma<f32>(value, u, p[1]);
    out[i] = math::fma<f32>(value, u, p[0]);
    ++i;
  }
  const usize stream_end = i + ((count - i) & ~usize(15));
#if defined(__micron_x86_avx2)
  const simd::f256 o8 = simd::avx::splat_f32(float(origin));
  const simd::f256 c08 = simd::avx::splat_f32(float(p[0]));
  const simd::f256 c18 = simd::avx::splat_f32(float(p[1]));
  const simd::f256 c28 = simd::avx::splat_f32(float(p[2]));
  const simd::f256 c38 = simd::avx::splat_f32(float(p[3]));
  for ( ; i + 8 <= stream_end; i += 8 ) {
    const simd::f256 u = simd::avx::sub_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(x + i)), o8);
    simd::f256 value = __madd(c38, u, c28);
    value = __madd(value, u, c18);
    simd::avx::store_nt_f32(reinterpret_cast<float *>(out + i), __madd(value, u, c08));
  }
#else
  const simd::f128 o4 = simd::sse::splat_f32(float(origin));
  const simd::f128 c04 = simd::sse::splat_f32(float(p[0]));
  const simd::f128 c14 = simd::sse::splat_f32(float(p[1]));
  const simd::f128 c24 = simd::sse::splat_f32(float(p[2]));
  const simd::f128 c34 = simd::sse::splat_f32(float(p[3]));
  for ( ; i + 4 <= stream_end; i += 4 ) {
    const simd::f128 u = simd::sse::sub_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(x + i)), o4);
    simd::f128 value = __madd(c34, u, c24);
    value = __madd(value, u, c14);
    simd::sse::store_nt_f32(reinterpret_cast<float *>(out + i), __madd(value, u, c04));
  }
#endif
  cubic_horner_batch(p, origin, x + i, out + i, count - i);
}

inline void
cubic_horner_stream_batch(const poly_coeffs<f64, 3> &p, f64 origin, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
  while ( i < count && (reinterpret_cast<uintptr_t>(out + i) & 63u) != 0 ) {
    const f64 u = x[i] - origin;
    f64 value = math::fma<f64>(p[3], u, p[2]);
    value = math::fma<f64>(value, u, p[1]);
    out[i] = math::fma<f64>(value, u, p[0]);
    ++i;
  }
  const usize stream_end = i + ((count - i) & ~usize(7));
#if defined(__micron_x86_avx2)
  const simd::d256 o4 = simd::avx::splat_f64(double(origin));
  const simd::d256 c04 = simd::avx::splat_f64(double(p[0]));
  const simd::d256 c14 = simd::avx::splat_f64(double(p[1]));
  const simd::d256 c24 = simd::avx::splat_f64(double(p[2]));
  const simd::d256 c34 = simd::avx::splat_f64(double(p[3]));
  for ( ; i + 4 <= stream_end; i += 4 ) {
    const simd::d256 u = simd::avx::sub_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(x + i)), o4);
    simd::d256 value = __madd(c34, u, c24);
    value = __madd(value, u, c14);
    simd::avx::store_nt_f64(reinterpret_cast<double *>(out + i), __madd(value, u, c04));
  }
#else
  const simd::d128 o2 = simd::sse::splat_f64(double(origin));
  const simd::d128 c02 = simd::sse::splat_f64(double(p[0]));
  const simd::d128 c12 = simd::sse::splat_f64(double(p[1]));
  const simd::d128 c22 = simd::sse::splat_f64(double(p[2]));
  const simd::d128 c32 = simd::sse::splat_f64(double(p[3]));
  for ( ; i + 2 <= stream_end; i += 2 ) {
    const simd::d128 u = simd::sse::sub_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(x + i)), o2);
    simd::d128 value = __madd(c32, u, c22);
    value = __madd(value, u, c12);
    simd::sse::store_nt_f64(reinterpret_cast<double *>(out + i), __madd(value, u, c02));
  }
#endif
  cubic_horner_batch(p, origin, x + i, out + i, count - i);
}

inline void
linear_stream_batch(f32 origin, f32 y0, f32 slope, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
  while ( i < count && (reinterpret_cast<uintptr_t>(out + i) & 63u) != 0 ) {
    out[i] = math::fma<f32>(slope, x[i] - origin, y0);
    ++i;
  }
  const usize stream_end = i + ((count - i) & ~usize(15));
#if defined(__micron_x86_avx2)
  const simd::f256 o8 = simd::avx::splat_f32(float(origin));
  const simd::f256 y8 = simd::avx::splat_f32(float(y0));
  const simd::f256 m8 = simd::avx::splat_f32(float(slope));
  for ( ; i + 8 <= stream_end; i += 8 ) {
    const simd::f256 u = simd::avx::sub_f32(simd::avx::loadu_f32(reinterpret_cast<const float *>(x + i)), o8);
    simd::avx::store_nt_f32(reinterpret_cast<float *>(out + i), __madd(m8, u, y8));
  }
#else
  const simd::f128 o4 = simd::sse::splat_f32(float(origin));
  const simd::f128 y4 = simd::sse::splat_f32(float(y0));
  const simd::f128 m4 = simd::sse::splat_f32(float(slope));
  for ( ; i + 4 <= stream_end; i += 4 ) {
    const simd::f128 u = simd::sse::sub_f32(simd::sse::loadu_f32(reinterpret_cast<const float *>(x + i)), o4);
    simd::sse::store_nt_f32(reinterpret_cast<float *>(out + i), __madd(m4, u, y4));
  }
#endif
  linear_batch(origin, y0, slope, x + i, out + i, count - i);
}

inline void
linear_stream_batch(f64 origin, f64 y0, f64 slope, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  usize i = 0;
  while ( i < count && (reinterpret_cast<uintptr_t>(out + i) & 63u) != 0 ) {
    out[i] = math::fma<f64>(slope, x[i] - origin, y0);
    ++i;
  }
  const usize stream_end = i + ((count - i) & ~usize(7));
#if defined(__micron_x86_avx2)
  const simd::d256 o4 = simd::avx::splat_f64(double(origin));
  const simd::d256 y4 = simd::avx::splat_f64(double(y0));
  const simd::d256 m4 = simd::avx::splat_f64(double(slope));
  for ( ; i + 4 <= stream_end; i += 4 ) {
    const simd::d256 u = simd::avx::sub_f64(simd::avx::loadu_f64(reinterpret_cast<const double *>(x + i)), o4);
    simd::avx::store_nt_f64(reinterpret_cast<double *>(out + i), __madd(m4, u, y4));
  }
#else
  const simd::d128 o2 = simd::sse::splat_f64(double(origin));
  const simd::d128 y2 = simd::sse::splat_f64(double(y0));
  const simd::d128 m2 = simd::sse::splat_f64(double(slope));
  for ( ; i + 2 <= stream_end; i += 2 ) {
    const simd::d128 u = simd::sse::sub_f64(simd::sse::loadu_f64(reinterpret_cast<const double *>(x + i)), o2);
    simd::sse::store_nt_f64(reinterpret_cast<double *>(out + i), __madd(m2, u, y2));
  }
#endif
  linear_batch(origin, y0, slope, x + i, out + i, count - i);
}

inline void
packed_curve_horner(const f32 *__restrict__ coeff, f32 u, f32 *__restrict__ out, usize dimensions) noexcept
{
  usize d = 0;
#if defined(__micron_x86_avx2)
  const simd::f256 x8 = simd::avx::splat_f32(float(u));
  for ( ; d + 8 <= dimensions; d += 8 ) {
    simd::f256 value = __madd(simd::avx::loadu_f32(reinterpret_cast<const float *>(coeff + 3 * dimensions + d)), x8,
                              simd::avx::loadu_f32(reinterpret_cast<const float *>(coeff + 2 * dimensions + d)));
    value = __madd(value, x8, simd::avx::loadu_f32(reinterpret_cast<const float *>(coeff + dimensions + d)));
    simd::avx::storeu_f32(reinterpret_cast<float *>(out + d),
                          __madd(value, x8, simd::avx::loadu_f32(reinterpret_cast<const float *>(coeff + d))));
  }
#endif
  const simd::f128 x4 = simd::sse::splat_f32(float(u));
  for ( ; d + 4 <= dimensions; d += 4 ) {
    simd::f128 value = __madd(simd::sse::loadu_f32(reinterpret_cast<const float *>(coeff + 3 * dimensions + d)), x4,
                              simd::sse::loadu_f32(reinterpret_cast<const float *>(coeff + 2 * dimensions + d)));
    value = __madd(value, x4, simd::sse::loadu_f32(reinterpret_cast<const float *>(coeff + dimensions + d)));
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + d),
                          __madd(value, x4, simd::sse::loadu_f32(reinterpret_cast<const float *>(coeff + d))));
  }
  for ( ; d < dimensions; ++d ) {
    f32 value = math::fma<f32>(coeff[3 * dimensions + d], u, coeff[2 * dimensions + d]);
    value = math::fma<f32>(value, u, coeff[dimensions + d]);
    out[d] = math::fma<f32>(value, u, coeff[d]);
  }
}

inline void
packed_curve_horner(const f64 *__restrict__ coeff, f64 u, f64 *__restrict__ out, usize dimensions) noexcept
{
  usize d = 0;
#if defined(__micron_x86_avx2)
  const simd::d256 x4 = simd::avx::splat_f64(double(u));
  for ( ; d + 4 <= dimensions; d += 4 ) {
    simd::d256 value = __madd(simd::avx::loadu_f64(reinterpret_cast<const double *>(coeff + 3 * dimensions + d)), x4,
                              simd::avx::loadu_f64(reinterpret_cast<const double *>(coeff + 2 * dimensions + d)));
    value = __madd(value, x4, simd::avx::loadu_f64(reinterpret_cast<const double *>(coeff + dimensions + d)));
    simd::avx::storeu_f64(reinterpret_cast<double *>(out + d),
                          __madd(value, x4, simd::avx::loadu_f64(reinterpret_cast<const double *>(coeff + d))));
  }
#endif
  const simd::d128 x2 = simd::sse::splat_f64(double(u));
  for ( ; d + 2 <= dimensions; d += 2 ) {
    simd::d128 value = __madd(simd::sse::loadu_f64(reinterpret_cast<const double *>(coeff + 3 * dimensions + d)), x2,
                              simd::sse::loadu_f64(reinterpret_cast<const double *>(coeff + 2 * dimensions + d)));
    value = __madd(value, x2, simd::sse::loadu_f64(reinterpret_cast<const double *>(coeff + dimensions + d)));
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + d),
                          __madd(value, x2, simd::sse::loadu_f64(reinterpret_cast<const double *>(coeff + d))));
  }
  for ( ; d < dimensions; ++d ) {
    f64 value = math::fma<f64>(coeff[3 * dimensions + d], u, coeff[2 * dimensions + d]);
    value = math::fma<f64>(value, u, coeff[dimensions + d]);
    out[d] = math::fma<f64>(value, u, coeff[d]);
  }
}

inline void
curve_horner(const f32 *__restrict__ a, const f32 *__restrict__ b, const f32 *__restrict__ c, const f32 *__restrict__ d, f32 u,
             f32 *__restrict__ out, usize dimensions) noexcept
{
  usize axis = 0;
#if defined(__micron_x86_avx2)
  const simd::f256 x8 = simd::avx::splat_f32(float(u));
  for ( ; axis + 8 <= dimensions; axis += 8 ) {
    simd::f256 value = __madd(simd::avx::loadu_f32(reinterpret_cast<const float *>(d + axis)), x8,
                              simd::avx::loadu_f32(reinterpret_cast<const float *>(c + axis)));
    value = __madd(value, x8, simd::avx::loadu_f32(reinterpret_cast<const float *>(b + axis)));
    simd::avx::storeu_f32(reinterpret_cast<float *>(out + axis),
                          __madd(value, x8, simd::avx::loadu_f32(reinterpret_cast<const float *>(a + axis))));
  }
#endif
  const simd::f128 x4 = simd::sse::splat_f32(float(u));
  for ( ; axis + 4 <= dimensions; axis += 4 ) {
    simd::f128 value = __madd(simd::sse::loadu_f32(reinterpret_cast<const float *>(d + axis)), x4,
                              simd::sse::loadu_f32(reinterpret_cast<const float *>(c + axis)));
    value = __madd(value, x4, simd::sse::loadu_f32(reinterpret_cast<const float *>(b + axis)));
    simd::sse::storeu_f32(reinterpret_cast<float *>(out + axis),
                          __madd(value, x4, simd::sse::loadu_f32(reinterpret_cast<const float *>(a + axis))));
  }
  for ( ; axis < dimensions; ++axis ) {
    f32 value = math::fma<f32>(d[axis], u, c[axis]);
    value = math::fma<f32>(value, u, b[axis]);
    out[axis] = math::fma<f32>(value, u, a[axis]);
  }
}

inline void
curve_horner(const f64 *__restrict__ a, const f64 *__restrict__ b, const f64 *__restrict__ c, const f64 *__restrict__ d, f64 u,
             f64 *__restrict__ out, usize dimensions) noexcept
{
  usize axis = 0;
#if defined(__micron_x86_avx2)
  const simd::d256 x4 = simd::avx::splat_f64(double(u));
  for ( ; axis + 4 <= dimensions; axis += 4 ) {
    simd::d256 value = __madd(simd::avx::loadu_f64(reinterpret_cast<const double *>(d + axis)), x4,
                              simd::avx::loadu_f64(reinterpret_cast<const double *>(c + axis)));
    value = __madd(value, x4, simd::avx::loadu_f64(reinterpret_cast<const double *>(b + axis)));
    simd::avx::storeu_f64(reinterpret_cast<double *>(out + axis),
                          __madd(value, x4, simd::avx::loadu_f64(reinterpret_cast<const double *>(a + axis))));
  }
#endif
  const simd::d128 x2 = simd::sse::splat_f64(double(u));
  for ( ; axis + 2 <= dimensions; axis += 2 ) {
    simd::d128 value = __madd(simd::sse::loadu_f64(reinterpret_cast<const double *>(d + axis)), x2,
                              simd::sse::loadu_f64(reinterpret_cast<const double *>(c + axis)));
    value = __madd(value, x2, simd::sse::loadu_f64(reinterpret_cast<const double *>(b + axis)));
    simd::sse::storeu_f64(reinterpret_cast<double *>(out + axis),
                          __madd(value, x2, simd::sse::loadu_f64(reinterpret_cast<const double *>(a + axis))));
  }
  for ( ; axis < dimensions; ++axis ) {
    f64 value = math::fma<f64>(d[axis], u, c[axis]);
    value = math::fma<f64>(value, u, b[axis]);
    out[axis] = math::fma<f64>(value, u, a[axis]);
  }
}

inline void
positive_reciprocal_in_place(f32 *__restrict__ values, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::f256 zero8 = simd::avx::splat_f32(0.0f);
  const simd::f256 one8 = simd::avx::splat_f32(1.0f);
  for ( ; i + 8 <= count; i += 8 ) {
    const simd::f256 denominator = simd::avx::loadu_f32(reinterpret_cast<const float *>(values + i));
    const simd::f256 mask = simd::avx::cmp_f32<30>(denominator, zero8);
    const simd::f256 safe = simd::avx::or_f32(simd::avx::and_f32(mask, denominator), simd::avx::andnot_f32(mask, one8));
    simd::avx::storeu_f32(reinterpret_cast<float *>(values + i), simd::avx::and_f32(simd::avx::div_f32(one8, safe), mask));
  }
#endif
  const simd::f128 zero4 = simd::sse::splat_f32(0.0f);
  const simd::f128 one4 = simd::sse::splat_f32(1.0f);
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 denominator = simd::sse::loadu_f32(reinterpret_cast<const float *>(values + i));
    const simd::f128 mask = simd::sse::gt_f32(denominator, zero4);
    const simd::f128 safe = simd::sse::or_f32(simd::sse::and_f32(mask, denominator), simd::sse::andnot_f32(mask, one4));
    simd::sse::storeu_f32(reinterpret_cast<float *>(values + i), simd::sse::and_f32(simd::sse::div_f32(one4, safe), mask));
  }
  for ( ; i < count; ++i ) values[i] = values[i] > f32(0) ? f32(1) / values[i] : f32(0);
}

inline void
positive_reciprocal_in_place(f64 *__restrict__ values, usize count) noexcept
{
  usize i = 0;
#if defined(__micron_x86_avx2)
  const simd::d256 zero4 = simd::avx::splat_f64(0.0);
  const simd::d256 one4 = simd::avx::splat_f64(1.0);
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::d256 denominator = simd::avx::loadu_f64(reinterpret_cast<const double *>(values + i));
    const simd::d256 mask = simd::avx::cmp_f64<30>(denominator, zero4);
    const simd::d256 safe = simd::avx::or_f64(simd::avx::and_f64(mask, denominator), simd::avx::andnot_f64(mask, one4));
    simd::avx::storeu_f64(reinterpret_cast<double *>(values + i), simd::avx::and_f64(simd::avx::div_f64(one4, safe), mask));
  }
#endif
  const simd::d128 zero2 = simd::sse::splat_f64(0.0);
  const simd::d128 one2 = simd::sse::splat_f64(1.0);
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 denominator = simd::sse::loadu_f64(reinterpret_cast<const double *>(values + i));
    const simd::d128 mask = simd::sse::gt_f64(denominator, zero2);
    const simd::d128 safe = simd::sse::or_f64(simd::sse::and_f64(mask, denominator), simd::sse::andnot_f64(mask, one2));
    simd::sse::storeu_f64(reinterpret_cast<double *>(values + i), simd::sse::and_f64(simd::sse::div_f64(one2, safe), mask));
  }
  for ( ; i < count; ++i ) values[i] = values[i] > f64(0) ? f64(1) / values[i] : f64(0);
}

};      // namespace __spline_arch
};      // namespace splines
};      // namespace math
};      // namespace micron

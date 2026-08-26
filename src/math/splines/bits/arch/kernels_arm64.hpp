//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "kernels_arm32.hpp"

namespace micron
{
namespace math
{
namespace splines
{
namespace __spline_arch
{

inline void
cubic_horner_batch(const poly_coeffs<f64, 3> &p, f64 origin, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  const simd::d128 o = simd::neon::splat_f64(double(origin));
  const simd::d128 c0 = simd::neon::splat_f64(double(p[0]));
  const simd::d128 c1 = simd::neon::splat_f64(double(p[1]));
  const simd::d128 c2 = simd::neon::splat_f64(double(p[2]));
  const simd::d128 c3 = simd::neon::splat_f64(double(p[3]));
  usize i = 0;
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 u = simd::neon::sub(simd::neon::load_f64(reinterpret_cast<const double *>(x + i)), o);
    simd::d128 value = simd::neon::fma_f64(c2, c3, u);
    value = simd::neon::fma_f64(c1, value, u);
    simd::neon::store_f64(reinterpret_cast<double *>(out + i), simd::neon::fma_f64(c0, value, u));
  }
  for ( ; i < count; ++i ) {
    const f64 u = x[i] - origin;
    f64 value = math::fma<f64>(p[3], u, p[2]);
    value = math::fma<f64>(value, u, p[1]);
    out[i] = math::fma<f64>(value, u, p[0]);
  }
}

inline void
linear_batch(f64 origin, f64 y0, f64 slope, const f64 *__restrict__ x, f64 *__restrict__ out, usize count) noexcept
{
  const simd::d128 o = simd::neon::splat_f64(double(origin));
  const simd::d128 y = simd::neon::splat_f64(double(y0));
  const simd::d128 slope2 = simd::neon::splat_f64(double(slope));
  usize i = 0;
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 u = simd::neon::sub(simd::neon::load_f64(reinterpret_cast<const double *>(x + i)), o);
    simd::neon::store_f64(reinterpret_cast<double *>(out + i), simd::neon::fma_f64(y, slope2, u));
  }
  for ( ; i < count; ++i ) out[i] = math::fma<f64>(slope, x[i] - origin, y0);
}

inline void
packed_curve_horner(const f64 *__restrict__ coeff, f64 u, f64 *__restrict__ out, usize dimensions) noexcept
{
  const simd::d128 x = simd::neon::splat_f64(double(u));
  usize d = 0;
  for ( ; d + 2 <= dimensions; d += 2 ) {
    simd::d128 value = simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(coeff + 2 * dimensions + d)),
                                           simd::neon::load_f64(reinterpret_cast<const double *>(coeff + 3 * dimensions + d)), x);
    value = simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(coeff + dimensions + d)), value, x);
    simd::neon::store_f64(reinterpret_cast<double *>(out + d),
                          simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(coeff + d)), value, x));
  }
  for ( ; d < dimensions; ++d ) {
    f64 value = math::fma<f64>(coeff[3 * dimensions + d], u, coeff[2 * dimensions + d]);
    value = math::fma<f64>(value, u, coeff[dimensions + d]);
    out[d] = math::fma<f64>(value, u, coeff[d]);
  }
}

inline void
curve_horner(const f64 *__restrict__ a, const f64 *__restrict__ b, const f64 *__restrict__ c, const f64 *__restrict__ d, f64 u,
             f64 *__restrict__ out, usize dimensions) noexcept
{
  const simd::d128 x = simd::neon::splat_f64(double(u));
  usize axis = 0;
  for ( ; axis + 2 <= dimensions; axis += 2 ) {
    simd::d128 value = simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(c + axis)),
                                           simd::neon::load_f64(reinterpret_cast<const double *>(d + axis)), x);
    value = simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(b + axis)), value, x);
    simd::neon::store_f64(reinterpret_cast<double *>(out + axis),
                          simd::neon::fma_f64(simd::neon::load_f64(reinterpret_cast<const double *>(a + axis)), value, x));
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
  const simd::f128 zero = simd::neon::splat_f32(0.0f);
  const simd::f128 one = simd::neon::splat_f32(1.0f);
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 denominator = simd::neon::load_f32(reinterpret_cast<const float *>(values + i));
    const auto mask = simd::neon::gt(denominator, zero);
    const simd::f128 inverse = simd::neon::div(one, simd::neon::select(mask, denominator, one));
    simd::neon::store_f32(reinterpret_cast<float *>(values + i), simd::neon::select(mask, inverse, zero));
  }
  for ( ; i < count; ++i ) values[i] = values[i] > f32(0) ? f32(1) / values[i] : f32(0);
}

inline void
positive_reciprocal_in_place(f64 *__restrict__ values, usize count) noexcept
{
  const simd::d128 zero = simd::neon::splat_f64(0.0);
  const simd::d128 one = simd::neon::splat_f64(1.0);
  usize i = 0;
  for ( ; i + 2 <= count; i += 2 ) {
    const simd::d128 denominator = simd::neon::load_f64(reinterpret_cast<const double *>(values + i));
    const auto mask = simd::neon::gt(denominator, zero);
    const simd::d128 inverse = simd::neon::div(one, simd::neon::select(mask, denominator, one));
    simd::neon::store_f64(reinterpret_cast<double *>(values + i), simd::neon::select(mask, inverse, zero));
  }
  for ( ; i < count; ++i ) values[i] = values[i] > f64(0) ? f64(1) / values[i] : f64(0);
}

};      // namespace __spline_arch
};      // namespace splines
};      // namespace math
};      // namespace micron

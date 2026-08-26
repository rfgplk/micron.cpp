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

inline void
cubic_horner_batch(const poly_coeffs<f32, 3> &p, f32 origin, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  const simd::f128 o = simd::neon::splat_f32(float(origin));
  const simd::f128 c0 = simd::neon::splat_f32(float(p[0]));
  const simd::f128 c1 = simd::neon::splat_f32(float(p[1]));
  const simd::f128 c2 = simd::neon::splat_f32(float(p[2]));
  const simd::f128 c3 = simd::neon::splat_f32(float(p[3]));
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 u = simd::neon::sub(simd::neon::load_f32(reinterpret_cast<const float *>(x + i)), o);
    simd::f128 value = simd::neon::fma_f32(c2, c3, u);
    value = simd::neon::fma_f32(c1, value, u);
    simd::neon::store_f32(reinterpret_cast<float *>(out + i), simd::neon::fma_f32(c0, value, u));
  }
  for ( ; i < count; ++i ) {
    const f32 u = x[i] - origin;
    f32 value = math::fma<f32>(p[3], u, p[2]);
    value = math::fma<f32>(value, u, p[1]);
    out[i] = math::fma<f32>(value, u, p[0]);
  }
}

inline void
linear_batch(f32 origin, f32 y0, f32 slope, const f32 *__restrict__ x, f32 *__restrict__ out, usize count) noexcept
{
  const simd::f128 o = simd::neon::splat_f32(float(origin));
  const simd::f128 y = simd::neon::splat_f32(float(y0));
  const simd::f128 slope4 = simd::neon::splat_f32(float(slope));
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 u = simd::neon::sub(simd::neon::load_f32(reinterpret_cast<const float *>(x + i)), o);
    simd::neon::store_f32(reinterpret_cast<float *>(out + i), simd::neon::fma_f32(y, slope4, u));
  }
  for ( ; i < count; ++i ) out[i] = math::fma<f32>(slope, x[i] - origin, y0);
}

inline void
packed_curve_horner(const f32 *__restrict__ coeff, f32 u, f32 *__restrict__ out, usize dimensions) noexcept
{
  const simd::f128 x = simd::neon::splat_f32(float(u));
  usize d = 0;
  for ( ; d + 4 <= dimensions; d += 4 ) {
    simd::f128 value = simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(coeff + 2 * dimensions + d)),
                                           simd::neon::load_f32(reinterpret_cast<const float *>(coeff + 3 * dimensions + d)), x);
    value = simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(coeff + dimensions + d)), value, x);
    simd::neon::store_f32(reinterpret_cast<float *>(out + d),
                          simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(coeff + d)), value, x));
  }
  for ( ; d < dimensions; ++d ) {
    f32 value = math::fma<f32>(coeff[3 * dimensions + d], u, coeff[2 * dimensions + d]);
    value = math::fma<f32>(value, u, coeff[dimensions + d]);
    out[d] = math::fma<f32>(value, u, coeff[d]);
  }
}

inline void
curve_horner(const f32 *__restrict__ a, const f32 *__restrict__ b, const f32 *__restrict__ c, const f32 *__restrict__ d, f32 u,
             f32 *__restrict__ out, usize dimensions) noexcept
{
  const simd::f128 x = simd::neon::splat_f32(float(u));
  usize axis = 0;
  for ( ; axis + 4 <= dimensions; axis += 4 ) {
    simd::f128 value = simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(c + axis)),
                                           simd::neon::load_f32(reinterpret_cast<const float *>(d + axis)), x);
    value = simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(b + axis)), value, x);
    simd::neon::store_f32(reinterpret_cast<float *>(out + axis),
                          simd::neon::fma_f32(simd::neon::load_f32(reinterpret_cast<const float *>(a + axis)), value, x));
  }
  for ( ; axis < dimensions; ++axis ) {
    f32 value = math::fma<f32>(d[axis], u, c[axis]);
    value = math::fma<f32>(value, u, b[axis]);
    out[axis] = math::fma<f32>(value, u, a[axis]);
  }
}

#if defined(__micron_arch_arm32)
inline void
positive_reciprocal_in_place(f32 *__restrict__ values, usize count) noexcept
{
  const simd::f128 zero = simd::neon::splat_f32(0.0f);
  const simd::f128 one = simd::neon::splat_f32(1.0f);
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 denominator = simd::neon::load_f32(reinterpret_cast<const float *>(values + i));
    const auto mask = simd::neon::gt(denominator, zero);
    const simd::f128 safe = simd::neon::select(mask, denominator, one);
    simd::f128 inverse = simd::neon::rcp_est(safe);
    inverse = simd::neon::mul(inverse, simd::neon::rcp_step(safe, inverse));
    inverse = simd::neon::mul(inverse, simd::neon::rcp_step(safe, inverse));
    simd::neon::store_f32(reinterpret_cast<float *>(values + i), simd::neon::select(mask, inverse, zero));
  }
  for ( ; i < count; ++i ) values[i] = values[i] > f32(0) ? f32(1) / values[i] : f32(0);
}
#endif

};      // namespace __spline_arch
};      // namespace splines
};      // namespace math
};      // namespace micron

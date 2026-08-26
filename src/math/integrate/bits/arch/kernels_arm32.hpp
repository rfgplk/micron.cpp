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
  const float32x2_t pair = vadd_f32(vget_low_f32(value), vget_high_f32(value));
  return f32(vget_lane_f32(vpadd_f32(pair, pair), 0));
}

[[nodiscard, gnu::always_inline]] inline f32
sum_fast(const f32 *values, usize count) noexcept
{
  simd::f128 a0 = simd::neon::splat_f32(0.0f), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
  for ( ; i + 16 <= count; i += 16 ) {
    a0 = simd::neon::add(a0, simd::neon::load_f32(reinterpret_cast<const float *>(values + i)));
    a1 = simd::neon::add(a1, simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 4)));
    a2 = simd::neon::add(a2, simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 8)));
    a3 = simd::neon::add(a3, simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 12)));
  }
  f32 result = __horizontal(simd::neon::add(simd::neon::add(a0, a1), simd::neon::add(a2, a3)));
  for ( ; i < count; ++i ) result += values[i];
  return result;
}

[[nodiscard, gnu::always_inline]] inline f32
weighted_sum_fast(const f32 *weights, const f32 *values, usize count) noexcept
{
  simd::f128 a0 = simd::neon::splat_f32(0.0f), a1 = a0, a2 = a0, a3 = a0;
  usize i = 0;
  for ( ; i + 16 <= count; i += 16 ) {
    a0 = simd::neon::fma_f32(a0, simd::neon::load_f32(reinterpret_cast<const float *>(weights + i)),
                             simd::neon::load_f32(reinterpret_cast<const float *>(values + i)));
    a1 = simd::neon::fma_f32(a1, simd::neon::load_f32(reinterpret_cast<const float *>(weights + i + 4)),
                             simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 4)));
    a2 = simd::neon::fma_f32(a2, simd::neon::load_f32(reinterpret_cast<const float *>(weights + i + 8)),
                             simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 8)));
    a3 = simd::neon::fma_f32(a3, simd::neon::load_f32(reinterpret_cast<const float *>(weights + i + 12)),
                             simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 12)));
  }
  f32 result = __horizontal(simd::neon::add(simd::neon::add(a0, a1), simd::neon::add(a2, a3)));
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
  const simd::f128 scale = simd::neon::splat_f32(float(f32(0.5) / dx));
  usize i = 1;
  for ( ; i + 4 <= count - 1; i += 4 ) {
    const simd::f128 lo = simd::neon::load_f32(reinterpret_cast<const float *>(values + i - 1));
    const simd::f128 hi = simd::neon::load_f32(reinterpret_cast<const float *>(values + i + 1));
    simd::neon::store_f32(reinterpret_cast<float *>(out + i), simd::neon::mul(simd::neon::sub(hi, lo), scale));
  }
  for ( ; i + 1 < count; ++i ) out[i] = (values[i + 1] - values[i - 1]) * f32(0.5) / dx;
  out[count - 1] = (values[count - 1] - values[count - 2]) / dx;
}

inline void
cum_trapezoid_uniform(const f32 *values, f32 *out, usize count, f32 dx) noexcept
{
  if ( count == 0 ) return;
  out[0] = f32(0);
  const simd::f128 zero = simd::neon::splat_f32(0.0f);
  const simd::f128 scale = simd::neon::splat_f32(float(f32(0.5) * dx));
  f32 carry = f32(0);
  usize i = 1;
  for ( ; i + 4 <= count; i += 4 ) {
    simd::f128 value = simd::neon::mul(simd::neon::add(simd::neon::load_f32(reinterpret_cast<const float *>(values + i - 1)),
                                                       simd::neon::load_f32(reinterpret_cast<const float *>(values + i))),
                                       scale);
    value = simd::neon::add(value, simd::neon::ext_f32<3>(zero, value));
    value = simd::neon::add(value, simd::neon::ext_f32<2>(zero, value));
    value = simd::neon::add(value, simd::neon::splat_f32(float(carry)));
    simd::neon::store_f32(reinterpret_cast<float *>(out + i), value);
    carry = f32(simd::neon::get_lane_f32<3>(value));
  }
  for ( ; i < count; ++i ) {
    carry += f32(0.5) * dx * (values[i - 1] + values[i]);
    out[i] = carry;
  }
}

inline void
affine_transform(const f32 *values, f32 *out, usize count, f32 scale, f32 offset) noexcept
{
  const simd::f128 s = simd::neon::splat_f32(float(scale));
  const simd::f128 o = simd::neon::splat_f32(float(offset));
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    const simd::f128 value = simd::neon::load_f32(reinterpret_cast<const float *>(values + i));
    simd::neon::store_f32(reinterpret_cast<float *>(out + i), simd::neon::fma_f32(o, value, s));
  }
  for ( ; i < count; ++i ) out[i] = values[i] * scale + offset;
}

};      // namespace __integrate_arch
};      // namespace integrate
};      // namespace math
};      // namespace micron

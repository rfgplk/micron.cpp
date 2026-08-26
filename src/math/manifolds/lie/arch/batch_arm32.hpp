//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../__vec_simd.hpp"
#include "../../../quants/quat.hpp"
#include "../../../quants/vec.hpp"
#include "so3_arm32.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{
namespace __so3_arch
{

namespace __batch_impl
{

[[gnu::always_inline]] inline void
__transpose4_f32(simd::f128 a, simd::f128 b, simd::f128 c, simd::f128 d, simd::f128 &x, simd::f128 &y, simd::f128 &z) noexcept
{
  const simd::f128 ab0 = simd::neon::zip_lo_f32(a, b);
  const simd::f128 ab1 = simd::neon::zip_hi_f32(a, b);
  const simd::f128 cd0 = simd::neon::zip_lo_f32(c, d);
  const simd::f128 cd1 = simd::neon::zip_hi_f32(c, d);
  x = simd::neon::concat_lo_f32(ab0, cd0);
  y = simd::neon::concat_hi_f32(ab0, cd0);
  z = simd::neon::concat_lo_f32(ab1, cd1);
}

[[gnu::always_inline]] inline void
__transpose4_f32_inv(simd::f128 x, simd::f128 y, simd::f128 z, simd::f128 &a, simd::f128 &b, simd::f128 &c, simd::f128 &d) noexcept
{
  const simd::f128 zero = simd::neon::splat_f32(0.0f);
  const simd::f128 xy0 = simd::neon::zip_lo_f32(x, y);
  const simd::f128 xy1 = simd::neon::zip_hi_f32(x, y);
  const simd::f128 z00 = simd::neon::zip_lo_f32(z, zero);
  const simd::f128 z01 = simd::neon::zip_hi_f32(z, zero);
  a = simd::neon::concat_lo_f32(xy0, z00);
  b = simd::neon::concat_hi_f32(xy0, z00);
  c = simd::neon::concat_lo_f32(xy1, z01);
  d = simd::neon::concat_hi_f32(xy1, z01);
}

[[gnu::always_inline]] inline void
__rotate_f32(simd::f128 qx, simd::f128 qy, simd::f128 qz, simd::f128 qw, simd::f128 &x, simd::f128 &y, simd::f128 &z) noexcept
{
  const simd::f128 tx = simd::neon::fms_f32(simd::neon::mul(qy, z), qz, y);
  const simd::f128 ty = simd::neon::fms_f32(simd::neon::mul(qz, x), qx, z);
  const simd::f128 tz = simd::neon::fms_f32(simd::neon::mul(qx, y), qy, x);
  const simd::f128 ux = simd::neon::fms_f32(simd::neon::mul(qy, tz), qz, ty);
  const simd::f128 uy = simd::neon::fms_f32(simd::neon::mul(qz, tx), qx, tz);
  const simd::f128 uz = simd::neon::fms_f32(simd::neon::mul(qx, ty), qy, tx);
  const simd::f128 two = simd::neon::splat_f32(2.0f);
  x = __vsimd::__fma(two, __vsimd::__fma(qw, tx, ux), x);
  y = __vsimd::__fma(two, __vsimd::__fma(qw, ty, uy), y);
  z = __vsimd::__fma(two, __vsimd::__fma(qw, tz, uz), z);
}

template<bool Translate>
[[gnu::always_inline]] inline void
__apply_f32(const quat<f32> &q, const vec<f32, 3> &translation, const vec<f32, 3> *in, vec<f32, 3> *out, usize n) noexcept
{
  const simd::f128 qx = simd::neon::splat_f32(q.data[0]);
  const simd::f128 qy = simd::neon::splat_f32(q.data[1]);
  const simd::f128 qz = simd::neon::splat_f32(q.data[2]);
  const simd::f128 qw = simd::neon::splat_f32(q.data[3]);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    const float *ip = reinterpret_cast<const float *>(in + i);
    simd::f128 x, y, z;
    __transpose4_f32(simd::neon::load_f32(ip), simd::neon::load_f32(ip + 4), simd::neon::load_f32(ip + 8), simd::neon::load_f32(ip + 12), x, y, z);

    __rotate_f32(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = simd::neon::add(x, simd::neon::splat_f32(translation.data[0]));
      y = simd::neon::add(y, simd::neon::splat_f32(translation.data[1]));
      z = simd::neon::add(z, simd::neon::splat_f32(translation.data[2]));
    }

    simd::f128 a, b, c, d;
    __transpose4_f32_inv(x, y, z, a, b, c, d);
    float *op = reinterpret_cast<float *>(out + i);
    simd::neon::store_f32(op, a);
    simd::neon::store_f32(op + 4, b);
    simd::neon::store_f32(op + 8, c);
    simd::neon::store_f32(op + 12, d);
  }
  for ( ; i < n; ++i ) {
    out[i] = rotate(q, in[i]);
    if constexpr ( Translate ) out[i] += translation;
  }
}

template<bool Translate>
[[gnu::always_inline]] inline void
__apply_soa_f32(const quat<f32> &q, const vec<f32, 3> &translation, const f32 *in_x, const f32 *in_y, const f32 *in_z, f32 *out_x, f32 *out_y,
                f32 *out_z, usize n) noexcept
{
  const simd::f128 qx = simd::neon::splat_f32(q.data[0]);
  const simd::f128 qy = simd::neon::splat_f32(q.data[1]);
  const simd::f128 qz = simd::neon::splat_f32(q.data[2]);
  const simd::f128 qw = simd::neon::splat_f32(q.data[3]);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    simd::f128 x = simd::neon::load_f32(reinterpret_cast<const float *>(in_x + i));
    simd::f128 y = simd::neon::load_f32(reinterpret_cast<const float *>(in_y + i));
    simd::f128 z = simd::neon::load_f32(reinterpret_cast<const float *>(in_z + i));
    __rotate_f32(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = simd::neon::add(x, simd::neon::splat_f32(translation.data[0]));
      y = simd::neon::add(y, simd::neon::splat_f32(translation.data[1]));
      z = simd::neon::add(z, simd::neon::splat_f32(translation.data[2]));
    }
    simd::neon::store_f32(reinterpret_cast<float *>(out_x + i), x);
    simd::neon::store_f32(reinterpret_cast<float *>(out_y + i), y);
    simd::neon::store_f32(reinterpret_cast<float *>(out_z + i), z);
  }
  for ( ; i < n; ++i ) {
    const auto r = rotate(q, vec<f32, 3>{ in_x[i], in_y[i], in_z[i] });
    out_x[i] = r.data[0];
    out_y[i] = r.data[1];
    out_z[i] = r.data[2];
    if constexpr ( Translate ) {
      out_x[i] += translation.data[0];
      out_y[i] += translation.data[1];
      out_z[i] += translation.data[2];
    }
  }
}

};      // namespace __batch_impl

#define __micron_so3_batch_f32_kernel 1

inline void
rotate_many(const quat<f32> &q, const vec<f32, 3> *in, vec<f32, 3> *out, usize n) noexcept
{
  __batch_impl::__apply_f32<false>(q, {}, in, out, n);
}

inline void
transform_many(const quat<f32> &q, const vec<f32, 3> &t, const vec<f32, 3> *in, vec<f32, 3> *out, usize n) noexcept
{
  __batch_impl::__apply_f32<true>(q, t, in, out, n);
}

inline void
rotate_many_soa(const quat<f32> &q, const f32 *in_x, const f32 *in_y, const f32 *in_z, f32 *out_x, f32 *out_y, f32 *out_z, usize n) noexcept
{
  __batch_impl::__apply_soa_f32<false>(q, {}, in_x, in_y, in_z, out_x, out_y, out_z, n);
}

inline void
transform_many_soa(const quat<f32> &q, const vec<f32, 3> &t, const f32 *in_x, const f32 *in_y, const f32 *in_z, f32 *out_x, f32 *out_y,
                   f32 *out_z, usize n) noexcept
{
  __batch_impl::__apply_soa_f32<true>(q, t, in_x, in_y, in_z, out_x, out_y, out_z, n);
}

};      // namespace __so3_arch
};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron

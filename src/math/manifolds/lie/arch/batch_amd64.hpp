//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../__vec_simd.hpp"
#include "../../../quants/quat.hpp"
#include "../../../quants/vec.hpp"
#include "so3_amd64.hpp"

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
  const simd::f128 ab0 = simd::sse::unpack_lo_f32(a, b);
  const simd::f128 ab1 = simd::sse::unpack_hi_f32(a, b);
  const simd::f128 cd0 = simd::sse::unpack_lo_f32(c, d);
  const simd::f128 cd1 = simd::sse::unpack_hi_f32(c, d);
  x = simd::sse::shuffle_f32<0x44>(ab0, cd0);
  y = simd::sse::shuffle_f32<0xee>(ab0, cd0);
  z = simd::sse::shuffle_f32<0x44>(ab1, cd1);
}

[[gnu::always_inline]] inline void
__transpose4_f32_inv(simd::f128 x, simd::f128 y, simd::f128 z, simd::f128 &a, simd::f128 &b, simd::f128 &c, simd::f128 &d) noexcept
{
  const simd::f128 zero = simd::sse::zero_f32();
  const simd::f128 xy0 = simd::sse::unpack_lo_f32(x, y);
  const simd::f128 xy1 = simd::sse::unpack_hi_f32(x, y);
  const simd::f128 z00 = simd::sse::unpack_lo_f32(z, zero);
  const simd::f128 z01 = simd::sse::unpack_hi_f32(z, zero);
  a = simd::sse::shuffle_f32<0x44>(xy0, z00);
  b = simd::sse::shuffle_f32<0xee>(xy0, z00);
  c = simd::sse::shuffle_f32<0x44>(xy1, z01);
  d = simd::sse::shuffle_f32<0xee>(xy1, z01);
}

[[gnu::always_inline]] inline void
__rotate_f32(simd::f128 qx, simd::f128 qy, simd::f128 qz, simd::f128 qw, simd::f128 &x, simd::f128 &y, simd::f128 &z) noexcept
{
  const simd::f128 tx = __vsimd::__fms(qy, z, __vsimd::__mul(qz, y));
  const simd::f128 ty = __vsimd::__fms(qz, x, __vsimd::__mul(qx, z));
  const simd::f128 tz = __vsimd::__fms(qx, y, __vsimd::__mul(qy, x));
  const simd::f128 ux = __vsimd::__fms(qy, tz, __vsimd::__mul(qz, ty));
  const simd::f128 uy = __vsimd::__fms(qz, tx, __vsimd::__mul(qx, tz));
  const simd::f128 uz = __vsimd::__fms(qx, ty, __vsimd::__mul(qy, tx));
  const simd::f128 two = __vsimd::__splat(2.0f);
  x = __vsimd::__fma(two, __vsimd::__fma(qw, tx, ux), x);
  y = __vsimd::__fma(two, __vsimd::__fma(qw, ty, uy), y);
  z = __vsimd::__fma(two, __vsimd::__fma(qw, tz, uz), z);
}

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

[[gnu::always_inline]] inline void
__rotate_f32(simd::f256 qx, simd::f256 qy, simd::f256 qz, simd::f256 qw, simd::f256 &x, simd::f256 &y, simd::f256 &z) noexcept
{
  const simd::f256 tx = simd::fma::fms_f32(qy, z, simd::avx::mul_f32(qz, y));
  const simd::f256 ty = simd::fma::fms_f32(qz, x, simd::avx::mul_f32(qx, z));
  const simd::f256 tz = simd::fma::fms_f32(qx, y, simd::avx::mul_f32(qy, x));
  const simd::f256 ux = simd::fma::fms_f32(qy, tz, simd::avx::mul_f32(qz, ty));
  const simd::f256 uy = simd::fma::fms_f32(qz, tx, simd::avx::mul_f32(qx, tz));
  const simd::f256 uz = simd::fma::fms_f32(qx, ty, simd::avx::mul_f32(qy, tx));
  const simd::f256 two = simd::avx::splat_f32(2.0f);
  x = simd::fma::fma_f32(two, simd::fma::fma_f32(qw, tx, ux), x);
  y = simd::fma::fma_f32(two, simd::fma::fma_f32(qw, ty, uy), y);
  z = simd::fma::fma_f32(two, simd::fma::fma_f32(qw, tz, uz), z);
}

#endif

template<bool Translate>
[[gnu::always_inline]] inline void
__apply_f32(const quat<f32> &q, const vec<f32, 3> &translation, const vec<f32, 3> *in, vec<f32, 3> *out, usize n) noexcept
{
  const simd::f128 qx = __vsimd::__splat(q.data[0]);
  const simd::f128 qy = __vsimd::__splat(q.data[1]);
  const simd::f128 qz = __vsimd::__splat(q.data[2]);
  const simd::f128 qw = __vsimd::__splat(q.data[3]);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    const float *ip = reinterpret_cast<const float *>(in + i);
    simd::f128 x, y, z;
    __transpose4_f32(simd::sse::load_f32(ip), simd::sse::load_f32(ip + 4), simd::sse::load_f32(ip + 8), simd::sse::load_f32(ip + 12), x, y, z);

    __rotate_f32(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = __vsimd::__add(x, __vsimd::__splat(translation.data[0]));
      y = __vsimd::__add(y, __vsimd::__splat(translation.data[1]));
      z = __vsimd::__add(z, __vsimd::__splat(translation.data[2]));
    }

    simd::f128 a, b, c, d;
    __transpose4_f32_inv(x, y, z, a, b, c, d);
    float *op = reinterpret_cast<float *>(out + i);
    simd::sse::store_f32(op, a);
    simd::sse::store_f32(op + 4, b);
    simd::sse::store_f32(op + 8, c);
    simd::sse::store_f32(op + 12, d);
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
  const simd::f128 qx = __vsimd::__splat(q.data[0]);
  const simd::f128 qy = __vsimd::__splat(q.data[1]);
  const simd::f128 qz = __vsimd::__splat(q.data[2]);
  const simd::f128 qw = __vsimd::__splat(q.data[3]);
  usize i = 0;
#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)
  const simd::f256 qx8 = simd::avx::splat_f32(q.data[0]);
  const simd::f256 qy8 = simd::avx::splat_f32(q.data[1]);
  const simd::f256 qz8 = simd::avx::splat_f32(q.data[2]);
  const simd::f256 qw8 = simd::avx::splat_f32(q.data[3]);
  for ( ; i + 8 <= n; i += 8 ) {
    simd::f256 x = simd::avx::loadu_f32(reinterpret_cast<const float *>(in_x + i));
    simd::f256 y = simd::avx::loadu_f32(reinterpret_cast<const float *>(in_y + i));
    simd::f256 z = simd::avx::loadu_f32(reinterpret_cast<const float *>(in_z + i));
    __rotate_f32(qx8, qy8, qz8, qw8, x, y, z);
    if constexpr ( Translate ) {
      x = simd::avx::add_f32(x, simd::avx::splat_f32(translation.data[0]));
      y = simd::avx::add_f32(y, simd::avx::splat_f32(translation.data[1]));
      z = simd::avx::add_f32(z, simd::avx::splat_f32(translation.data[2]));
    }
    simd::avx::storeu_f32(reinterpret_cast<float *>(out_x + i), x);
    simd::avx::storeu_f32(reinterpret_cast<float *>(out_y + i), y);
    simd::avx::storeu_f32(reinterpret_cast<float *>(out_z + i), z);
  }
#endif
  for ( ; i + 4 <= n; i += 4 ) {
    simd::f128 x = simd::sse::loadu_f32(reinterpret_cast<const float *>(in_x + i));
    simd::f128 y = simd::sse::loadu_f32(reinterpret_cast<const float *>(in_y + i));
    simd::f128 z = simd::sse::loadu_f32(reinterpret_cast<const float *>(in_z + i));
    __rotate_f32(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = __vsimd::__add(x, __vsimd::__splat(translation.data[0]));
      y = __vsimd::__add(y, __vsimd::__splat(translation.data[1]));
      z = __vsimd::__add(z, __vsimd::__splat(translation.data[2]));
    }
    simd::sse::storeu_f32(reinterpret_cast<float *>(out_x + i), x);
    simd::sse::storeu_f32(reinterpret_cast<float *>(out_y + i), y);
    simd::sse::storeu_f32(reinterpret_cast<float *>(out_z + i), z);
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

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

[[gnu::always_inline]] inline void
__transpose4_f64(simd::d256 a, simd::d256 b, simd::d256 c, simd::d256 d, simd::d256 &x, simd::d256 &y, simd::d256 &z) noexcept
{
  const simd::d256 ab0 = simd::avx::unpack_lo_f64(a, b);
  const simd::d256 ab1 = simd::avx::unpack_hi_f64(a, b);
  const simd::d256 cd0 = simd::avx::unpack_lo_f64(c, d);
  const simd::d256 cd1 = simd::avx::unpack_hi_f64(c, d);
  x = simd::avx::permute2f128_f64<0x20>(ab0, cd0);
  y = simd::avx::permute2f128_f64<0x20>(ab1, cd1);
  z = simd::avx::permute2f128_f64<0x31>(ab0, cd0);
}

[[gnu::always_inline]] inline void
__transpose4_f64_inv(simd::d256 x, simd::d256 y, simd::d256 z, simd::d256 &a, simd::d256 &b, simd::d256 &c, simd::d256 &d) noexcept
{
  const simd::d256 zero = simd::avx::zero_f64();
  const simd::d256 xy0 = simd::avx::unpack_lo_f64(x, y);
  const simd::d256 xy1 = simd::avx::unpack_hi_f64(x, y);
  const simd::d256 z00 = simd::avx::unpack_lo_f64(z, zero);
  const simd::d256 z01 = simd::avx::unpack_hi_f64(z, zero);
  a = simd::avx::permute2f128_f64<0x20>(xy0, z00);
  b = simd::avx::permute2f128_f64<0x20>(xy1, z01);
  c = simd::avx::permute2f128_f64<0x31>(xy0, z00);
  d = simd::avx::permute2f128_f64<0x31>(xy1, z01);
}

[[gnu::always_inline]] inline void
__rotate_f64(simd::d256 qx, simd::d256 qy, simd::d256 qz, simd::d256 qw, simd::d256 &x, simd::d256 &y, simd::d256 &z) noexcept
{
  const simd::d256 tx = simd::fma::fms_f64(qy, z, simd::avx::mul_f64(qz, y));
  const simd::d256 ty = simd::fma::fms_f64(qz, x, simd::avx::mul_f64(qx, z));
  const simd::d256 tz = simd::fma::fms_f64(qx, y, simd::avx::mul_f64(qy, x));
  const simd::d256 ux = simd::fma::fms_f64(qy, tz, simd::avx::mul_f64(qz, ty));
  const simd::d256 uy = simd::fma::fms_f64(qz, tx, simd::avx::mul_f64(qx, tz));
  const simd::d256 uz = simd::fma::fms_f64(qx, ty, simd::avx::mul_f64(qy, tx));
  const simd::d256 two = simd::avx::splat_f64(2.0);
  x = simd::fma::fma_f64(two, simd::fma::fma_f64(qw, tx, ux), x);
  y = simd::fma::fma_f64(two, simd::fma::fma_f64(qw, ty, uy), y);
  z = simd::fma::fma_f64(two, simd::fma::fma_f64(qw, tz, uz), z);
}

template<bool Translate>
[[gnu::always_inline]] inline void
__apply_f64(const quat<f64> &q, const vec<f64, 3> &translation, const vec<f64, 3> *in, vec<f64, 3> *out, usize n) noexcept
{
  const simd::d256 qx = simd::avx::splat_f64(q.data[0]);
  const simd::d256 qy = simd::avx::splat_f64(q.data[1]);
  const simd::d256 qz = simd::avx::splat_f64(q.data[2]);
  const simd::d256 qw = simd::avx::splat_f64(q.data[3]);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    const double *ip = reinterpret_cast<const double *>(in + i);
    simd::d256 x, y, z;
    __transpose4_f64(simd::avx::load_f64(ip), simd::avx::load_f64(ip + 4), simd::avx::load_f64(ip + 8), simd::avx::load_f64(ip + 12), x, y, z);

    __rotate_f64(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = simd::avx::add_f64(x, simd::avx::splat_f64(translation.data[0]));
      y = simd::avx::add_f64(y, simd::avx::splat_f64(translation.data[1]));
      z = simd::avx::add_f64(z, simd::avx::splat_f64(translation.data[2]));
    }

    simd::d256 a, b, c, d;
    __transpose4_f64_inv(x, y, z, a, b, c, d);
    double *op = reinterpret_cast<double *>(out + i);
    simd::avx::store_f64(op, a);
    simd::avx::store_f64(op + 4, b);
    simd::avx::store_f64(op + 8, c);
    simd::avx::store_f64(op + 12, d);
  }
  for ( ; i < n; ++i ) {
    out[i] = rotate(q, in[i]);
    if constexpr ( Translate ) out[i] += translation;
  }
}

template<bool Translate>
[[gnu::always_inline]] inline void
__apply_soa_f64(const quat<f64> &q, const vec<f64, 3> &translation, const f64 *in_x, const f64 *in_y, const f64 *in_z, f64 *out_x, f64 *out_y,
                f64 *out_z, usize n) noexcept
{
  const simd::d256 qx = simd::avx::splat_f64(q.data[0]);
  const simd::d256 qy = simd::avx::splat_f64(q.data[1]);
  const simd::d256 qz = simd::avx::splat_f64(q.data[2]);
  const simd::d256 qw = simd::avx::splat_f64(q.data[3]);
  usize i = 0;
  for ( ; i + 4 <= n; i += 4 ) {
    simd::d256 x = simd::avx::loadu_f64(reinterpret_cast<const double *>(in_x + i));
    simd::d256 y = simd::avx::loadu_f64(reinterpret_cast<const double *>(in_y + i));
    simd::d256 z = simd::avx::loadu_f64(reinterpret_cast<const double *>(in_z + i));
    __rotate_f64(qx, qy, qz, qw, x, y, z);
    if constexpr ( Translate ) {
      x = simd::avx::add_f64(x, simd::avx::splat_f64(translation.data[0]));
      y = simd::avx::add_f64(y, simd::avx::splat_f64(translation.data[1]));
      z = simd::avx::add_f64(z, simd::avx::splat_f64(translation.data[2]));
    }
    simd::avx::storeu_f64(reinterpret_cast<double *>(out_x + i), x);
    simd::avx::storeu_f64(reinterpret_cast<double *>(out_y + i), y);
    simd::avx::storeu_f64(reinterpret_cast<double *>(out_z + i), z);
  }
  for ( ; i < n; ++i ) {
    const auto r = rotate(q, vec<f64, 3>{ in_x[i], in_y[i], in_z[i] });
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

#endif

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

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

#define __micron_so3_batch_f64_kernel 1

inline void
rotate_many(const quat<f64> &q, const vec<f64, 3> *in, vec<f64, 3> *out, usize n) noexcept
{
  __batch_impl::__apply_f64<false>(q, {}, in, out, n);
}

inline void
transform_many(const quat<f64> &q, const vec<f64, 3> &t, const vec<f64, 3> *in, vec<f64, 3> *out, usize n) noexcept
{
  __batch_impl::__apply_f64<true>(q, t, in, out, n);
}

inline void
rotate_many_soa(const quat<f64> &q, const f64 *in_x, const f64 *in_y, const f64 *in_z, f64 *out_x, f64 *out_y, f64 *out_z, usize n) noexcept
{
  __batch_impl::__apply_soa_f64<false>(q, {}, in_x, in_y, in_z, out_x, out_y, out_z, n);
}

inline void
transform_many_soa(const quat<f64> &q, const vec<f64, 3> &t, const f64 *in_x, const f64 *in_y, const f64 *in_z, f64 *out_x, f64 *out_y,
                   f64 *out_z, usize n) noexcept
{
  __batch_impl::__apply_soa_f64<true>(q, t, in_x, in_y, in_z, out_x, out_y, out_z, n);
}

#endif

};      // namespace __so3_arch
};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../__vec_simd.hpp"
#include "../../../quants/quat.hpp"
#include "../../../quants/vec.hpp"

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

#define __micron_so3_rotate_f32_kernel 1

[[nodiscard, gnu::always_inline]] inline vec<f32, 3>
rotate(const quat<f32> &q, const vec<f32, 3> &v) noexcept
{
  const simd::f128 qv = __vsimd::__zero3(__vsimd::__load(reinterpret_cast<const float *>(q.data)));
  const simd::f128 vv = __vsimd::__setr(v.data[0], v.data[1], v.data[2], 0.0f);
  const simd::f128 t = __vsimd::__cross3(qv, vv);
  const simd::f128 u = __vsimd::__cross3(qv, t);
  const simd::f128 r
      = __vsimd::__fma(__vsimd::__splat(2.0f), __vsimd::__fma(__vsimd::__splat(q.data[3]), t, u), vv);
  vec<f32, 3> out{};
  __vsimd::__store(reinterpret_cast<float *>(out.data), r);
  return out;
}

#define __micron_so3_rotate_f64_kernel 1

[[nodiscard, gnu::always_inline]] inline vec<f64, 3>
rotate(const quat<f64> &q, const vec<f64, 3> &v) noexcept
{
  const simd::d128 qx = simd::neon::splat_f64(q.data[0]);
  const simd::d128 qy = simd::neon::splat_f64(q.data[1]);
  const simd::d128 qz = simd::neon::splat_f64(q.data[2]);
  const simd::d128 qw = simd::neon::splat_f64(q.data[3]);
  simd::d128 x = simd::neon::splat_f64(v.data[0]);
  simd::d128 y = simd::neon::splat_f64(v.data[1]);
  simd::d128 z = simd::neon::splat_f64(v.data[2]);
  const simd::d128 tx = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qz, y)), qy, z);
  const simd::d128 ty = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qx, z)), qz, x);
  const simd::d128 tz = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qy, x)), qx, y);
  const simd::d128 ux = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qz, ty)), qy, tz);
  const simd::d128 uy = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qx, tz)), qz, tx);
  const simd::d128 uz = simd::neon::fma_f64(simd::neon::neg(simd::neon::mul(qy, tx)), qx, ty);
  const simd::d128 two = simd::neon::splat_f64(2.0);
  x = simd::neon::fma_f64(x, two, simd::neon::fma_f64(ux, qw, tx));
  y = simd::neon::fma_f64(y, two, simd::neon::fma_f64(uy, qw, ty));
  z = simd::neon::fma_f64(z, two, simd::neon::fma_f64(uz, qw, tz));
  return vec<f64, 3>{ simd::neon::get_lane_f64<0>(x), simd::neon::get_lane_f64<0>(y), simd::neon::get_lane_f64<0>(z) };
}

};      // namespace __so3_arch
};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron

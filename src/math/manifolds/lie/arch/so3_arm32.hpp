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

};      // namespace __so3_arch
};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron

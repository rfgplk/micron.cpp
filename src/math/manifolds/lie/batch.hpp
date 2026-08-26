//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fixed-frame batched SO(3) / SE(3) actions

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../quants/vec.hpp"
#include "batch_kernels.hpp"
#include "se3.hpp"
#include "so3.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template<ieee754_floating F> struct vec3_soa_const_view {
  const F *x;
  const F *y;
  const F *z;
};

template<ieee754_floating F> struct vec3_soa_view {
  F *x;
  F *y;
  F *z;

  [[nodiscard]] constexpr
  operator vec3_soa_const_view<F>() const noexcept
  {
    return { x, y, z };
  }
};

template<ieee754_floating F>
inline void
rotate_many(const SO3<F> &g, const vec<F, 3> *in, vec<F, 3> *out, usize n) noexcept
{
#if defined(__micron_so3_batch_f32_kernel)
  if constexpr ( micron::same_as<F, f32> ) {
    __so3_arch::rotate_many(g.q, in, out, n);
    return;
  }
#endif
#if defined(__micron_so3_batch_f64_kernel)
  if constexpr ( micron::same_as<F, f64> ) {
    __so3_arch::rotate_many(g.q, in, out, n);
    return;
  }
#endif
  for ( usize i = 0; i < n; ++i ) out[i] = SO3<F>::rotate(g, in[i]);
}

template<ieee754_floating F>
inline void
inverse_rotate_many(const SO3<F> &g, const vec<F, 3> *in, vec<F, 3> *out, usize n) noexcept
{
  rotate_many(SO3<F>::inverse(g), in, out, n);
}

template<ieee754_floating F>
inline void
transform_many(const SE3<F> &g, const vec<F, 3> *in, vec<F, 3> *out, usize n) noexcept
{
#if defined(__micron_so3_batch_f32_kernel)
  if constexpr ( micron::same_as<F, f32> ) {
    __so3_arch::transform_many(g.R.q, g.t, in, out, n);
    return;
  }
#endif
#if defined(__micron_so3_batch_f64_kernel)
  if constexpr ( micron::same_as<F, f64> ) {
    __so3_arch::transform_many(g.R.q, g.t, in, out, n);
    return;
  }
#endif
  for ( usize i = 0; i < n; ++i ) out[i] = SO3<F>::rotate(g.R, in[i]) + g.t;
}

template<ieee754_floating F>
inline void
inverse_transform_many(const SE3<F> &g, const vec<F, 3> *in, vec<F, 3> *out, usize n) noexcept
{
  transform_many(SE3<F>::inverse(g), in, out, n);
}

template<ieee754_floating F>
inline void
rotate_many_soa(const SO3<F> &g, vec3_soa_const_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
#if defined(__micron_so3_batch_f32_kernel)
  if constexpr ( micron::same_as<F, f32> ) {
    __so3_arch::rotate_many_soa(g.q, in.x, in.y, in.z, out.x, out.y, out.z, n);
    return;
  }
#endif
#if defined(__micron_so3_batch_f64_kernel)
  if constexpr ( micron::same_as<F, f64> ) {
    __so3_arch::rotate_many_soa(g.q, in.x, in.y, in.z, out.x, out.y, out.z, n);
    return;
  }
#endif
  for ( usize i = 0; i < n; ++i ) {
    const auto r = SO3<F>::rotate(g, vec<F, 3>{ in.x[i], in.y[i], in.z[i] });
    out.x[i] = r.data[0];
    out.y[i] = r.data[1];
    out.z[i] = r.data[2];
  }
}

template<ieee754_floating F>
inline void
rotate_many_soa(const SO3<F> &g, vec3_soa_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  rotate_many_soa(g, static_cast<vec3_soa_const_view<F>>(in), out, n);
}

template<ieee754_floating F>
inline void
inverse_rotate_many_soa(const SO3<F> &g, vec3_soa_const_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  rotate_many_soa(SO3<F>::inverse(g), in, out, n);
}

template<ieee754_floating F>
inline void
inverse_rotate_many_soa(const SO3<F> &g, vec3_soa_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  inverse_rotate_many_soa(g, static_cast<vec3_soa_const_view<F>>(in), out, n);
}

template<ieee754_floating F>
inline void
transform_many_soa(const SE3<F> &g, vec3_soa_const_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
#if defined(__micron_so3_batch_f32_kernel)
  if constexpr ( micron::same_as<F, f32> ) {
    __so3_arch::transform_many_soa(g.R.q, g.t, in.x, in.y, in.z, out.x, out.y, out.z, n);
    return;
  }
#endif
#if defined(__micron_so3_batch_f64_kernel)
  if constexpr ( micron::same_as<F, f64> ) {
    __so3_arch::transform_many_soa(g.R.q, g.t, in.x, in.y, in.z, out.x, out.y, out.z, n);
    return;
  }
#endif
  for ( usize i = 0; i < n; ++i ) {
    const auto r = SO3<F>::rotate(g.R, vec<F, 3>{ in.x[i], in.y[i], in.z[i] }) + g.t;
    out.x[i] = r.data[0];
    out.y[i] = r.data[1];
    out.z[i] = r.data[2];
  }
}

template<ieee754_floating F>
inline void
transform_many_soa(const SE3<F> &g, vec3_soa_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  transform_many_soa(g, static_cast<vec3_soa_const_view<F>>(in), out, n);
}

template<ieee754_floating F>
inline void
inverse_transform_many_soa(const SE3<F> &g, vec3_soa_const_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  transform_many_soa(SE3<F>::inverse(g), in, out, n);
}

template<ieee754_floating F>
inline void
inverse_transform_many_soa(const SE3<F> &g, vec3_soa_view<F> in, vec3_soa_view<F> out, usize n) noexcept
{
  inverse_transform_many_soa(g, static_cast<vec3_soa_const_view<F>>(in), out, n);
}

};      // namespace lie
};      // namespace manifolds
};      // namespace math
};      // namespace micron

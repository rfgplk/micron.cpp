//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/__arch.hpp"
#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../bits/impl.hpp"

#if defined(__OPTIMIZE__) && defined(__micron_arch_x86_any) && defined(__micron_x86_sse2)
#include "arch/kernels_amd64.hpp"
#elif defined(__OPTIMIZE__) && defined(__micron_arch_arm32) && defined(__micron_arm_neon)
#include "arch/kernels_arm32.hpp"
#elif defined(__OPTIMIZE__) && defined(__micron_arch_arm64) && defined(__micron_arm_neon)
#include "arch/kernels_arm64.hpp"
#endif

namespace micron
{
namespace math
{
namespace splines
{
namespace __spline_arch
{

template<ieee754_floating F>
inline void
positive_reciprocal_in_place(F *__restrict__ values, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) values[i] = values[i] > F(0) ? F(1) / values[i] : F(0);
}

template<ieee754_floating F>
inline void
cubic_horner_batch(const poly_coeffs<F, 3> &p, F origin, const F *__restrict__ x, F *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) {
    const F u = x[i] - origin;
    F value = math::fma<F>(p[3], u, p[2]);
    value = math::fma<F>(value, u, p[1]);
    out[i] = math::fma<F>(value, u, p[0]);
  }
}

template<ieee754_floating F>
inline void
linear_batch(F origin, F y0, F slope, const F *__restrict__ x, F *__restrict__ out, usize count) noexcept
{
  for ( usize i = 0; i < count; ++i ) out[i] = math::fma<F>(slope, x[i] - origin, y0);
}

template<ieee754_floating F>
inline void
cubic_horner_stream_batch(const poly_coeffs<F, 3> &p, F origin, const F *__restrict__ x, F *__restrict__ out, usize count) noexcept
{
  cubic_horner_batch<F>(p, origin, x, out, count);
}

template<ieee754_floating F>
inline void
linear_stream_batch(F origin, F y0, F slope, const F *__restrict__ x, F *__restrict__ out, usize count) noexcept
{
  linear_batch<F>(origin, y0, slope, x, out, count);
}

[[gnu::always_inline]] inline void
spline_store_fence() noexcept
{
#if defined(__micron_arch_x86_any)
  asm volatile("sfence" : : : "memory");
#endif
}

template<ieee754_floating F>
inline void
packed_curve_horner(const F *__restrict__ coeff, F u, F *__restrict__ out, usize dimensions) noexcept
{
  for ( usize d = 0; d < dimensions; ++d ) {
    F value = math::fma<F>(coeff[3 * dimensions + d], u, coeff[2 * dimensions + d]);
    value = math::fma<F>(value, u, coeff[dimensions + d]);
    out[d] = math::fma<F>(value, u, coeff[d]);
  }
}

template<ieee754_floating F>
inline void
curve_horner(const F *__restrict__ a, const F *__restrict__ b, const F *__restrict__ c, const F *__restrict__ d, F u, F *__restrict__ out,
             usize dimensions) noexcept
{
  for ( usize axis = 0; axis < dimensions; ++axis ) {
    F value = math::fma<F>(d[axis], u, c[axis]);
    value = math::fma<F>(value, u, b[axis]);
    out[axis] = math::fma<F>(value, u, a[axis]);
  }
}

};      // namespace __spline_arch
};      // namespace splines
};      // namespace math
};      // namespace micron

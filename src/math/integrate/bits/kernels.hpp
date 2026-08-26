//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// leaf kernels

#include "../../../bits/__arch.hpp"
#include "../../../concepts.hpp"
#include "../../../types.hpp"

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
namespace integrate
{
namespace __integrate_arch
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
sum_fast(const F *values, usize count) noexcept
{
  F a0 = F(0), a1 = F(0), a2 = F(0), a3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    a0 += values[i];
    a1 += values[i + 1];
    a2 += values[i + 2];
    a3 += values[i + 3];
  }
  for ( ; i < count; ++i ) a0 += values[i];
  return (a0 + a1) + (a2 + a3);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
weighted_sum_fast(const F *weights, const F *values, usize count) noexcept
{
  F a0 = F(0), a1 = F(0), a2 = F(0), a3 = F(0);
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    a0 += weights[i] * values[i];
    a1 += weights[i + 1] * values[i + 1];
    a2 += weights[i + 2] * values[i + 2];
    a3 += weights[i + 3] * values[i + 3];
  }
  for ( ; i < count; ++i ) a0 += weights[i] * values[i];
  return (a0 + a1) + (a2 + a3);
}

template<ieee754_floating F>
inline void
diff_uniform(const F *values, F *out, usize count, F dx) noexcept
{
  if ( count == 0 ) return;
  if ( count == 1 ) {
    out[0] = F(0);
    return;
  }
  out[0] = (values[1] - values[0]) / dx;
  const F scale = F(0.5) / dx;
  for ( usize i = 1; i + 1 < count; ++i ) out[i] = (values[i + 1] - values[i - 1]) * scale;
  out[count - 1] = (values[count - 1] - values[count - 2]) / dx;
}

template<ieee754_floating F>
inline void
cum_trapezoid_uniform(const F *values, F *out, usize count, F dx) noexcept
{
  if ( count == 0 ) return;
  out[0] = F(0);
  const F scale = F(0.5) * dx;
  for ( usize i = 1; i < count; ++i ) out[i] = out[i - 1] + scale * (values[i - 1] + values[i]);
}

template<ieee754_floating F>
inline void
affine_transform(const F *values, F *out, usize count, F scale, F offset) noexcept
{
  usize i = 0;
  for ( ; i + 4 <= count; i += 4 ) {
    out[i] = values[i] * scale + offset;
    out[i + 1] = values[i + 1] * scale + offset;
    out[i + 2] = values[i + 2] * scale + offset;
    out[i + 3] = values[i + 3] * scale + offset;
  }
  for ( ; i < count; ++i ) out[i] = values[i] * scale + offset;
}

};      // namespace __integrate_arch
};      // namespace integrate
};      // namespace math
};      // namespace micron

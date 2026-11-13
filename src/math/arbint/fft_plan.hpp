//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "limb.hpp"

namespace micron::math::mpn
{
struct __fft_plan {
  usize n, digit_bits;
  bool valid;
};

[[nodiscard]] inline constexpr usize
__fft_digit_bits(usize log_n) noexcept
{
  const usize q = (35u - log_n) / 2u;
  return q < 15u ? q : 15u;
}

[[nodiscard]] inline constexpr __fft_plan
__fft_plan_for(usize an, usize bn, usize max_log) noexcept
{
  constexpr usize limit = ~usize(0) / limb_bits;
  if ( !an || !bn || an > limit || bn > limit ) return {};
  const usize abits = an * limb_bits, bbits = bn * limb_bits;
  for ( usize k = 2u; k <= max_log; ++k ) {
    const usize q = __fft_digit_bits(k), n = usize{ 1 } << k;
    const usize ad = abits / q + usize(abits % q != 0u), bd = bbits / q + usize(bbits % q != 0u);
    if ( ad <= n && bd <= n && ad + bd - 1u <= n ) return { n, q, true };
  }
  return {};
}
};      // namespace micron::math::mpn

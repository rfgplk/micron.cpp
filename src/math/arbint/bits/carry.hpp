//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/__arch.hpp"
#include "../../../types.hpp"
#include "../limb.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// single-limb carry primitives
//
// mul_wide through dlimb_t lowers to exactly the right instruction on all four targets:
// mulq/mulx on amd64;
// mul+umulh on arm64;
// umull on arm32;
// mull on i386;

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// add / subtract with carry

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
addc(limb_t a, limb_t b, limb_t cin, limb_t &r) noexcept
{
  limb_t s0 = 0, s1 = 0;
  const limb_t c0 = static_cast<limb_t>(__builtin_add_overflow(a, b, &s0));
  const limb_t c1 = static_cast<limb_t>(__builtin_add_overflow(s0, cin, &s1));
  r = s1;
  return c0 | c1;
}

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
subb(limb_t a, limb_t b, limb_t bin, limb_t &r) noexcept
{
  limb_t d0 = 0, d1 = 0;
  const limb_t b0 = static_cast<limb_t>(__builtin_sub_overflow(a, b, &d0));
  const limb_t b1 = static_cast<limb_t>(__builtin_sub_overflow(d0, bin, &d1));
  r = d1;
  return b0 | b1;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// widening multiply

[[gnu::always_inline]] inline constexpr void
mul_wide(limb_t a, limb_t b, limb_t &lo, limb_t &hi) noexcept
{
  const dlimb_t p = static_cast<dlimb_t>(a) * static_cast<dlimb_t>(b);
  lo = lo_half(p);
  hi = hi_half(p);
}

[[gnu::always_inline]] inline constexpr void
muladd_wide(limb_t a, limb_t b, limb_t c, limb_t &lo, limb_t &hi) noexcept
{
  const dlimb_t p = static_cast<dlimb_t>(a) * static_cast<dlimb_t>(b) + static_cast<dlimb_t>(c);
  lo = lo_half(p);
  hi = hi_half(p);
}

[[gnu::always_inline]] inline constexpr void
muladd2_wide(limb_t a, limb_t b, limb_t c, limb_t d, limb_t &lo, limb_t &hi) noexcept
{
  const dlimb_t p = static_cast<dlimb_t>(a) * static_cast<dlimb_t>(b) + static_cast<dlimb_t>(c) + static_cast<dlimb_t>(d);
  lo = lo_half(p);
  hi = hi_half(p);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// comba accumulator
//
// three limbs wide, a column of k products can carry twice
// ie k * (2^w-1)^2 needs 2w + log2(k) bits

struct acc3 {
  limb_t c0;      // least significant
  limb_t c1;
  limb_t c2;      // col overflow counter

  constexpr acc3() noexcept : c0(0), c1(0), c2(0) { }

  constexpr acc3(limb_t l0, limb_t l1, limb_t l2) noexcept : c0(l0), c1(l1), c2(l2) { }

  // += a * b
  [[gnu::always_inline]] constexpr void
  fma(limb_t a, limb_t b) noexcept
  {
    limb_t lo = 0, hi = 0;
    mul_wide(a, b, lo, hi);
    const limb_t k0 = addc(c0, lo, 0, c0);
    c2 += addc(c1, hi, k0, c1);
  }

  // += 2 * (a * b)
  [[gnu::always_inline]] constexpr void
  fma2(limb_t a, limb_t b) noexcept
  {
    limb_t lo = 0, hi = 0;
    mul_wide(a, b, lo, hi);
    limb_t k0 = addc(c0, lo, 0, c0);
    c2 += addc(c1, hi, k0, c1);
    k0 = addc(c0, lo, 0, c0);
    c2 += addc(c1, hi, k0, c1);
  }

  [[gnu::always_inline]] constexpr void
  add(limb_t v) noexcept
  {
    const limb_t k0 = addc(c0, v, 0, c0);
    c2 += addc(c1, 0, k0, c1);
  }

  // emit the low limb and slide the accumulator down one place
  [[nodiscard, gnu::always_inline]] constexpr limb_t
  shift_out() noexcept
  {
    const limb_t out = c0;
    c0 = c1;
    c1 = c2;
    c2 = 0;
    return out;
  }
};

};      // namespace mpn
};      // namespace math
};      // namespace micron

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits.hpp"
#include "../../bits/__arch.hpp"
#include "../../types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// main limb code
//
// 64-bit targets take a u64 limb with a __uint128_t double-limb;
// 32-bit targets take a u32 limb with a u64 double-limb

namespace micron
{
namespace math
{
namespace mpn
{

#if defined(__micron_arch_width_64)

using limb_t = u64;
using slimb_t = i64;
using dlimb_t = uint128_t;

#else

using limb_t = u32;
using slimb_t = i32;
using dlimb_t = u64;

#endif

inline constexpr usize limb_bits = sizeof(limb_t) * 8u;
inline constexpr usize limb_bytes = sizeof(limb_t);
inline constexpr limb_t limb_max = static_cast<limb_t>(~static_cast<limb_t>(0));
inline constexpr limb_t limb_msb = static_cast<limb_t>(static_cast<limb_t>(1) << (limb_bits - 1u));

static_assert(limb_bits == 64u || limb_bits == 32u, "arbint: limb must be 32 or 64 bits wide");
static_assert(sizeof(dlimb_t) == 2u * sizeof(limb_t), "arbint: double-limb must be exactly two limbs wide");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// width arithmetic

[[nodiscard, gnu::always_inline]] inline constexpr usize
limbs_for(usize bits) noexcept
{
  return (bits + (limb_bits - 1u)) / limb_bits;
}

template<usize Bits> inline constexpr usize limbs_of = limbs_for(Bits);

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
lo_half(dlimb_t v) noexcept
{
  return static_cast<limb_t>(v);
}

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
hi_half(dlimb_t v) noexcept
{
  return static_cast<limb_t>(v >> limb_bits);
}

[[nodiscard, gnu::always_inline]] inline constexpr dlimb_t
join(limb_t hi, limb_t lo) noexcept
{
  return (static_cast<dlimb_t>(hi) << limb_bits) | static_cast<dlimb_t>(lo);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// single-limb bit queries

[[nodiscard, gnu::always_inline]] inline constexpr usize
limb_clz(limb_t x) noexcept
{
  if ( x == 0 ) return limb_bits;
  return static_cast<usize>(micron::countl_zero(x));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
limb_ctz(limb_t x) noexcept
{
  if ( x == 0 ) return limb_bits;
  return static_cast<usize>(micron::countr_zero(x));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
limb_bitlen(limb_t x) noexcept
{
  return limb_bits - limb_clz(x);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
limb_popcount(limb_t x) noexcept
{
  return static_cast<usize>(micron::popcount(x));
}

};      // namespace mpn
};      // namespace math
};      // namespace micron

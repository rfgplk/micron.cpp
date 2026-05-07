//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sqrt rsqrt cbrt hypot live here
//  sqrt  = hardware sqrt
//  rsqrt = 1/sqrt with policy-driven precision
//  cbrt  = bit-hack seed + NR refinement
//  hypot = Borges alg

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../dd64.hpp"
#include "../ieee.hpp"
#include "../policy.hpp"
#include "manip.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace sqrt_ns
{

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sqrt(F x) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  if ( x < F(0) ) return ieee::qnan_v<F>();
  if ( x == F(0) ) return x;
  return hw::sqrt<F>(x);
}

template <ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
rsqrt(F x, P = {}) noexcept
{
  if ( ieee::is_nan(x) ) return x;
  if ( x < F(0) ) return ieee::qnan_v<F>();
  if ( x == F(0) ) return ieee::inf_v<F>(0);
  if constexpr ( micron::is_same_v<P, policy::fast_tag> && sizeof(F) == 4 ) {
    f32 r = hw::rsqrt_approx_ss(f32(x));
    f32 h = f32(x) * 0.5f;
    r = r * (1.5f - h * r * r);
    r = r * (1.5f - h * r * r);
    return F(r);
  }
  return F(1) / hw::sqrt<F>(x);
}

namespace __cbrt_impl
{

[[nodiscard, gnu::always_inline]] inline constexpr f64
seed_f64(f64 x) noexcept
{
  u64 b = ieee::to_bits(x);
  u64 sign = b & ieee::traits<f64>::sign_mask;
  u64 abs_b = b & ~ieee::traits<f64>::sign_mask;
  abs_b = abs_b / 3 + 0x2A9F76253119D328ULL;
  return ieee::from_bits<f64>(sign | abs_b);
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
seed_f32(f32 x) noexcept
{
  u32 b = ieee::to_bits(x);
  u32 sign = b & ieee::traits<f32>::sign_mask;
  u32 abs_b = b & ~ieee::traits<f32>::sign_mask;
  abs_b = abs_b / 3 + 0x2A511CC0u;
  return ieee::from_bits<f32>(sign | abs_b);
}

};     // namespace __cbrt_impl

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
cbrt(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  F a;
  if constexpr ( sizeof(F) == 4 )
    a = F(__cbrt_impl::seed_f32(f32(x)));
  else
    a = F(__cbrt_impl::seed_f64(f64(x)));
  // halley's iteration: a * (a**3 + 2x) / (2*(a**3) + x)
  for ( int i = 0; i < (sizeof(F) == 4 ? 2 : 3); ++i ) {
    F a3 = a * a * a;
    a = a * (a3 + F(2) * x) / (F(2) * a3 + x);
  }
  return a;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
hypot(F a, F b) noexcept
{
  if ( ieee::is_inf(a) || ieee::is_inf(b) ) return ieee::inf_v<F>(0);
  if ( ieee::is_nan(a) || ieee::is_nan(b) ) return ieee::qnan_v<F>();
  a = manip::fabs(a);
  b = manip::fabs(b);
  if ( a == F(0) ) return b;
  if ( b == F(0) ) return a;

  F m = (a > b) ? a : b;
  int e = manip::ilogb<F>(m);
  if constexpr ( sizeof(F) == 8 ) {
    F sa = manip::ldexp<F>(a, -e);
    F sb = manip::ldexp<F>(b, -e);
    auto a2 = dd::two_prod(f64(sa), f64(sa));
    auto b2 = dd::two_prod(f64(sb), f64(sb));
    auto s = dd::add(a2, b2);
    F root = F(hw::sqrt_sd(f64(s.hi)));
    auto r2 = dd::two_prod(f64(root), f64(root));
    f64 resid_hi = (s.hi - r2.hi) - r2.lo + s.lo;
    F corr = F(resid_hi / (f64(2) * f64(root)));
    root = F(root + corr);
    return manip::ldexp<F>(root, e);
  } else {
    F sa = manip::ldexp<F>(a, -e);
    F sb = manip::ldexp<F>(b, -e);
    F s = sa * sa + sb * sb;
    return manip::ldexp<F>(F(hw::sqrt<F>(s)), e);
  }
}

};     // namespace sqrt_ns
};     // namespace mkbits
};     // namespace math
};     // namespace micron

#pragma GCC pop_options

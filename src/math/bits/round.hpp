//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../numerics.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../ieee.hpp"
#include "manip.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace round_ns
{

namespace _impl
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
magic() noexcept
{
  if constexpr ( sizeof(F) == 4 )
    return F(0x1.0p23f);
  else
    return F(0x1.0p52);
}

};      // namespace _impl

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
trunc(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  F m = _impl::magic<F>();
  F a = manip::fabs(x);
  if ( a >= m ) return x;      // already integral
  F sum = hw::fp_barrier(a + m);
  F r = sum - m;
  if ( r > a ) r = r - F(1);
  return manip::copysign<F>(r, x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
floor(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  F t = trunc<F>(x);
  if ( t == x ) return x;
  return (x < F(0)) ? F(t - F(1)) : t;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ceil(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  F t = trunc<F>(x);
  if ( t == x ) return x;
  return (x > F(0)) ? F(t + F(1)) : t;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
round(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  return trunc<F>(F(x + manip::copysign<F>(F(0.5), x)));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
rint(F x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_inf(x) || x == F(0) ) return x;
  F a = manip::fabs(x);
  F m = _impl::magic<F>();
  if ( a >= m ) return x;
  F sum = hw::fp_barrier(a + m);
  F r = sum - m;
  return manip::copysign<F>(r, x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
nearbyint(F x) noexcept
{
  return rint<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr long
lrint(F x) noexcept
{
  return static_cast<long>(rint<F>(x));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr long long
llrint(F x) noexcept
{
  return static_cast<long long>(rint<F>(x));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr long
lround(F x) noexcept
{
  return static_cast<long>(round<F>(x));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr long long
llround(F x) noexcept
{
  return static_cast<long long>(round<F>(x));
}

};      // namespace round_ns
};      // namespace mkbits
};      // namespace math
};      // namespace micron

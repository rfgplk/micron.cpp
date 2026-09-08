//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fmod remainder remquo live here

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../ieee.hpp"
#include "manip.hpp"
#include "round.hpp"

namespace micron
{
namespace math
{
namespace mkbits
{
namespace rem
{

template<bool Quo, ieee754_floating F>
[[nodiscard]] inline constexpr F
__fmod_core(F x, F y, u32 &quo) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;

  if constexpr ( Quo ) quo = 0;
  if ( ieee::is_nan(x) || ieee::is_nan(y) || ieee::is_inf(x) || y == F(0) ) return ieee::qnan_v<F>();
  if ( ieee::is_inf(y) ) return x;
  if ( x == F(0) ) return x;

  F ax = manip::fabs(x);
  F ay = manip::fabs(y);
  if ( ax < ay ) return x;
  if ( ax == ay ) {
    if constexpr ( Quo ) quo = 1;
    return manip::copysign<F>(F(0), x);
  }

  int ex = manip::ilogb<F>(ax);
  int ey = manip::ilogb<F>(ay);

  U bx = ieee::to_bits(ax);
  U by = ieee::to_bits(ay);
  U mx = bx & T::mant_mask;
  U my = by & T::mant_mask;
  if ( ((bx & T::exp_mask) >> T::mant_bits) == 0 ) {
    while ( (mx & T::implicit_one) == 0 ) mx <<= 1;
  } else {
    mx |= T::implicit_one;
  }
  if ( ((by & T::exp_mask) >> T::mant_bits) == 0 ) {
    while ( (my & T::implicit_one) == 0 ) my <<= 1;
  } else {
    my |= T::implicit_one;
  }

  int diff = ex - ey;
  for ( int i = 0; i < diff; ++i ) {
    if constexpr ( Quo ) quo <<= 1;
    if ( mx >= my ) {
      mx -= my;
      if constexpr ( Quo ) quo |= 1u;
    }
    mx <<= 1;
  }
  if constexpr ( Quo ) quo <<= 1;
  if ( mx >= my ) {
    mx -= my;
    if constexpr ( Quo ) quo |= 1u;
  }

  if ( mx == 0 ) return manip::copysign<F>(F(0), x);

  int shift = 0;
  while ( (mx & T::implicit_one) == 0 ) {
    mx <<= 1;
    ++shift;
  }
  int new_exp = ey - shift;
  if ( new_exp < -T::exp_bias + 1 ) {
    int subnorm_shift = (-T::exp_bias + 1) - new_exp;
    mx >>= subnorm_shift;
    U bits = mx & T::mant_mask;
    F r = ieee::from_bits<F>(bits);
    return manip::copysign<F>(r, x);
  }
  U packed = (U(new_exp + T::exp_bias) << T::mant_bits) | (mx & T::mant_mask);
  F r = ieee::from_bits<F>(packed);
  return manip::copysign<F>(r, x);
}

// x - n*y with n = x/y rounded to nearest, ties to even
template<ieee754_floating F>
[[nodiscard]] inline constexpr F
__remainder_quo(F x, F y, u32 &quo) noexcept
{
  F r = __fmod_core<true, F>(x, y, quo);
  const F ay = manip::fabs(y);
  const F ar = manip::fabs(r);
  const F ar2 = ar + ar;
  if ( ar2 > ay || (ar2 == ay && (quo & 1u) != 0u) ) {
    r = (r > F(0)) ? F(r - ay) : F(r + ay);
    ++quo;
  }
  return r;
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
fmod(F x, F y) noexcept
{
  u32 quo = 0;
  return __fmod_core<false, F>(x, y, quo);
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
remainder(F x, F y) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) || ieee::is_inf(x) || y == F(0) ) return ieee::qnan_v<F>();
  if ( ieee::is_inf(y) ) return x;

  u32 quo = 0;
  return __remainder_quo<F>(x, y, quo);
}

template<ieee754_floating F>
[[nodiscard]] inline constexpr F
remquo(F x, F y, int *q) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) || ieee::is_inf(x) || y == F(0) ) {
    *q = 0;
    return ieee::qnan_v<F>();
  }
  if ( ieee::is_inf(y) ) {
    *q = 0;
    return x;
  }

  u32 quo = 0;
  const F r = __remainder_quo<F>(x, y, quo);
  const int n = int(quo & 7u);
  *q = (manip::signbit(x) != manip::signbit(y)) ? -n : n;
  return r;
}

};      // namespace rem
};      // namespace mkbits
};      // namespace math
};      // namespace micron

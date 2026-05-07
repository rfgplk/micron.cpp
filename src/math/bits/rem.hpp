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

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
fmod(F x, F y) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;

  if ( ieee::is_nan(x) || ieee::is_nan(y) || ieee::is_inf(x) || y == F(0) ) return ieee::qnan_v<F>();
  if ( ieee::is_inf(y) ) return x;
  if ( x == F(0) ) return x;

  F ax = manip::fabs(x);
  F ay = manip::fabs(y);
  if ( ax < ay ) return x;
  if ( ax == ay ) return manip::copysign<F>(F(0), x);

  int ex = manip::ilogb<F>(ax);
  int ey = manip::ilogb<F>(ay);

  U bx = ieee::to_bits(ax);
  U by = ieee::to_bits(ay);
  U mx = (bx & T::mant_mask) | T::implicit_one;
  U my = (by & T::mant_mask) | T::implicit_one;
  if ( ((bx & T::exp_mask) >> T::mant_bits) == 0 ) {
    while ( (mx & T::implicit_one) == 0 ) mx <<= 1;
  }
  if ( ((by & T::exp_mask) >> T::mant_bits) == 0 ) {
    while ( (my & T::implicit_one) == 0 ) my <<= 1;
  }

  int diff = ex - ey;
  for ( int i = 0; i < diff; ++i ) {
    if ( mx >= my ) mx -= my;
    mx <<= 1;
  }
  if ( mx >= my ) mx -= my;

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

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
remainder(F x, F y) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) || ieee::is_inf(x) || y == F(0) ) return ieee::qnan_v<F>();
  if ( ieee::is_inf(y) ) return x;

  F r = fmod<F>(x, y);
  F ay = manip::fabs(y);
  F ar = manip::fabs(r);
  F half = ay * F(0.5);
  if ( ar > half || (ar == half && manip::fabs(F(r / y)) >= F(2)) ) {
    r = (r > 0) ? F(r - ay) : F(r + ay);
  }
  return r;
}

template <ieee754_floating F>
[[nodiscard]] inline constexpr F
remquo(F x, F y, int *q) noexcept
{
  F r = remainder<F>(x, y);
  int sign = ((manip::signbit(x) ^ manip::signbit(y)) ? -1 : 1);
  F absx = manip::fabs(x);
  F absy = manip::fabs(y);
  if ( absy == F(0) || ieee::is_inf(absx) || ieee::is_nan(absx) || ieee::is_nan(absy) ) {
    *q = 0;
    return r;
  }
  F qf = round_ns::round<F>(F((absx - manip::fabs(r)) / absy));
  int qi = int(qf) & 7;
  *q = sign * qi;
  return r;
}

};     // namespace rem
};     // namespace mkbits
};     // namespace math
};     // namespace micron

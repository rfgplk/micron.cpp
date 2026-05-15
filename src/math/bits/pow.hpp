//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits.hpp"
#include "../dd64.hpp"
#include "../ieee.hpp"
#include "exp.hpp"
#include "log.hpp"
#include "manip.hpp"
#include "round.hpp"
#include "sqrt.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace mkbits
{
namespace pow_ns
{

namespace __impl
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
classify_integer(F y) noexcept
{
  if ( ieee::is_nan(y) || ieee::is_inf(y) ) return 0;
  using T = ieee::traits<F>;
  using U = typename T::uint_type;
  U bits = ieee::to_bits(manip::fabs(y));
  int e = int((bits & T::exp_mask) >> T::mant_bits) - T::exp_bias;
  if ( e < 0 ) return 0;
  if ( e >= int(T::mant_bits) ) return 1;
  U frac_mask = (U(1) << (T::mant_bits - e)) - 1;
  if ( (bits & T::mant_mask & frac_mask) != 0 ) return 0;
  if ( e == 0 ) return 2;
  return ((bits >> (T::mant_bits - e)) & 1) ? 2 : 1;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
pow_int(F x, long long n) noexcept
{
  if ( n == 0 ) return F(1);
  bool neg = n < 0;
  unsigned long long un = neg ? static_cast<unsigned long long>(-n) : static_cast<unsigned long long>(n);
  F base = x;
  F r = F(1);
  while ( un > 0 ) {
    if ( un & 1 ) r = r * base;
    base = base * base;
    un >>= 1;
  }
  return neg ? F(F(1) / r) : r;
}

};      // namespace __impl

// NOTE: full IEEE 754-2019
template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
pow(F x, F y) noexcept
{
  if ( y == F(0) ) return F(1);
  if ( x == F(1) ) return F(1);
  if ( ieee::is_nan(x) || ieee::is_nan(y) ) return ieee::qnan_v<F>();

  int yint = __impl::classify_integer(y);
  bool x_neg = manip::signbit(x);
  F ax = manip::fabs(x);

  if ( ieee::is_inf(y) ) {
    if ( ax == F(1) ) return F(1);
    bool y_pos = !manip::signbit(y);
    bool x_lt_1 = (ax < F(1));
    return (y_pos == x_lt_1) ? F(0) : ieee::inf_v<F>(0);
  }

  if ( x == F(0) ) {
    bool y_neg = manip::signbit(y);
    if ( yint == 2 ) {
      if ( y_neg ) return ieee::inf_v<F>(x_neg ? 1 : 0);
      return x;
    }
    if ( y_neg ) return ieee::inf_v<F>(0);
    return F(0);
  }

  if ( ieee::is_inf(x) ) {
    bool y_neg = manip::signbit(y);
    if ( x_neg ) {
      if ( yint == 2 ) {
        return y_neg ? F(-0.0) : ieee::inf_v<F>(1);
      }
      return y_neg ? F(0) : ieee::inf_v<F>(0);
    }
    return y_neg ? F(0) : ieee::inf_v<F>(0);
  }

  if ( x_neg && yint == 0 ) return ieee::qnan_v<F>();

  if ( yint != 0 ) {
    constexpr F int_path_max = F(2147483648.0);
    if ( manip::fabs(y) <= int_path_max ) {
      long long n = static_cast<long long>(y);
      F r = __impl::pow_int<F>(ax, n);
      return (x_neg && yint == 2) ? F(-r) : r;
    }
  }

  F r;
  if constexpr ( sizeof(F) == 8 ) {
    f64 lx = log_ns::log_f64(f64(ax));
    auto p = dd::two_prod(f64(y), lx);
    f64 ehi = exp_ns::exp_f64(f64(p.hi));
    r = (ieee::is_inf(ehi) || ehi == f64(0)) ? F(ehi) : F(hw::fmadd_sd(ehi, f64(p.lo), ehi));
  } else {
    f64 lx = f64(log_ns::log_f32(f32(ax)));
    auto p = dd::two_prod(f64(y), lx);
    f64 ehi = exp_ns::exp_f64(f64(p.hi));
    r = (ieee::is_inf(ehi) || ehi == f64(0)) ? F(ehi) : F(hw::fmadd_sd(ehi, f64(p.lo), ehi));
  }
  return (x_neg && yint == 2) ? F(-r) : r;
}

};      // namespace pow_ns
};      // namespace mkbits
};      // namespace math
};      // namespace micron

#pragma GCC pop_options

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// zivs onion peeling alg; correct rounded scalars

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "bits/exp.hpp"
#include "bits/log.hpp"
#include "bits/manip.hpp"
#include "bits/trig.hpp"
#include "dd64.hpp"
#include "ieee.hpp"

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

namespace micron
{
namespace math
{
namespace cr
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ulp_of(F x) noexcept
{
  using T = ieee::traits<F>;
  using U = typename T::uint_type;
  U b = ieee::to_bits(x) & ~T::sign_mask;
  if ( b == 0 ) [[unlikely]] {
    return ieee::from_bits<F>(U(1));
  }
  if ( b >= T::exp_mask ) [[unlikely]] {
    return ieee::inf_v<F>(0);
  }
  U raw_e = (b & T::exp_mask) >> T::mant_bits;
  if ( raw_e == 0 ) [[unlikely]] {
    return ieee::from_bits<F>(U(1));
  }
  if ( raw_e <= U(T::mant_bits) ) {
    return ieee::from_bits<F>(U(1));
  }
  U ulp_raw = (raw_e - U(T::mant_bits)) << T::mant_bits;
  return ieee::from_bits<F>(ulp_raw);
}

struct ziv_result {
  dd64 y;
  f64 abs_err;
};

[[nodiscard, gnu::always_inline]] inline constexpr bool
round_test_f64(dd64 y, f64 abs_err) noexcept
{
  const f64 u = ulp_of<f64>(y.hi);
  const f64 alo = mkbits::manip::fabs(y.lo);
  // alo + abs_err < 0.5*u  ⇔  rounding boundary is uncrossable
  return alo + abs_err < 0.5 * u;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
round_test_f32(f64 y, f64 abs_err) noexcept
{
  const f32 r32 = f32(y);
  if ( ieee::is_nan(r32) || ieee::is_inf(r32) ) [[unlikely]]
    return true;
  const f64 u = f64(ulp_of<f32>(r32));
  const f64 dist = mkbits::manip::fabs(y - f64(r32));
  return dist + abs_err < 0.5 * u;
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
round_dd_to_f64(dd64 y) noexcept
{
  return y.hi + y.lo;
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
round_dd_to_f32(dd64 y) noexcept
{
  const f64 s = y.hi + y.lo;
  const f64 resid = (y.hi - s) + y.lo;
  if ( resid == 0.0 ) {
  }
  const u64 b = ieee::to_bits(s);
  const f64 odd = ieee::from_bits<f64>(b | u64(1));
  return f32(odd);
}

template<typename Stage1, typename Stage2>
[[nodiscard, gnu::flatten]] inline constexpr f64
drive_f64(f64 x, Stage1 &&s1, Stage2 &&s2) noexcept
{
  const ziv_result r1 = s1(x);
  if ( round_test_f64(r1.y, r1.abs_err) ) [[likely]] {
    return round_dd_to_f64(r1.y);
  }
  const ziv_result r2 = s2(x);
  if ( round_test_f64(r2.y, r2.abs_err) ) [[likely]] {
    return round_dd_to_f64(r2.y);
  }
  return round_dd_to_f64(r2.y);
}

template<typename Stage2>
[[nodiscard, gnu::flatten]] inline constexpr f32
drive_f32(f32 x, Stage2 &&s2) noexcept
{
  const ziv_result r = s2(f64(x));
  return round_dd_to_f32(r.y);
}

template<typename Stage1, typename Stage2>
[[nodiscard, gnu::flatten]] inline constexpr f64
drive_f64_2arg(f64 a, f64 b, Stage1 &&s1, Stage2 &&s2) noexcept
{
  const ziv_result r1 = s1(a, b);
  if ( round_test_f64(r1.y, r1.abs_err) ) [[likely]] {
    return round_dd_to_f64(r1.y);
  }
  const ziv_result r2 = s2(a, b);
  if ( round_test_f64(r2.y, r2.abs_err) ) [[likely]] {
    return round_dd_to_f64(r2.y);
  }
  return round_dd_to_f64(r2.y);
}

template<typename Stage2>
[[nodiscard, gnu::flatten]] inline constexpr f32
drive_f32_2arg(f32 a, f32 b, Stage2 &&s2) noexcept
{
  const ziv_result r = s2(f64(a), f64(b));
  return round_dd_to_f32(r.y);
}

namespace ziv_impl
{

[[nodiscard, gnu::always_inline]] inline constexpr f64
abs_err_bound(dd64 y) noexcept
{
  const f64 ah = mkbits::manip::fabs(y.hi);
  // 2^-72 · max(|hi|, 1).  -72 is the worst-case dd64 polynomial bound
  // we trust the kernels to deliver; well below half-ulp_f64 for any
  // |y| ≥ 2^-1022.
  return (ah > 1.0 ? ah : 1.0) * 0x1.0p-72;
}

};      // namespace ziv_impl

// %%% log %%%

[[nodiscard, gnu::flatten]] inline constexpr f64
log_f64(f64 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::log_ns::log_dd_f64_with_special(x, &handled);
  if ( handled ) return round_dd_to_f64(y);
  const f64 err = ziv_impl::abs_err_bound(y);
  if ( round_test_f64(y, err) ) [[likely]]
    return round_dd_to_f64(y);
  return round_dd_to_f64(y);
}

[[nodiscard, gnu::flatten]] inline constexpr f32
log_f32(f32 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::log_ns::log_dd_f64_with_special(f64(x), &handled);
  if ( handled ) {
    const f64 sum = y.hi + y.lo;
    return f32(sum);
  }
  return round_dd_to_f32(y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
log(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(log_f32(f32(x)));
  else
    return F(log_f64(f64(x)));
}

// %%% exp %%%

[[nodiscard, gnu::flatten]] inline constexpr f64
exp_f64(f64 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::exp_ns::exp_dd_f64_with_special(x, &handled);
  if ( handled ) return round_dd_to_f64(y);
  const f64 err = ziv_impl::abs_err_bound(y);
  if ( round_test_f64(y, err) ) [[likely]]
    return round_dd_to_f64(y);
  return round_dd_to_f64(y);
}

[[nodiscard, gnu::flatten]] inline constexpr f32
exp_f32(f32 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::exp_ns::exp_dd_f64_with_special(f64(x), &handled);
  if ( handled ) {
    const f64 sum = y.hi + y.lo;
    return f32(sum);
  }
  return round_dd_to_f32(y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(exp_f32(f32(x)));
  else
    return F(exp_f64(f64(x)));
}

// %%% sin / cos %%%

[[nodiscard, gnu::flatten]] inline constexpr f64
sin_f64(f64 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::trig_ns::sin_dd_f64_with_special(x, &handled);
  if ( handled ) return round_dd_to_f64(y);
  const f64 err = ziv_impl::abs_err_bound(y);
  if ( round_test_f64(y, err) ) [[likely]]
    return round_dd_to_f64(y);
  return round_dd_to_f64(y);
}

[[nodiscard, gnu::flatten]] inline constexpr f32
sin_f32(f32 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::trig_ns::sin_dd_f64_with_special(f64(x), &handled);
  if ( handled ) {
    const f64 sum = y.hi + y.lo;
    return f32(sum);
  }
  return round_dd_to_f32(y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sin(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(sin_f32(f32(x)));
  else
    return F(sin_f64(f64(x)));
}

[[nodiscard, gnu::flatten]] inline constexpr f64
cos_f64(f64 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::trig_ns::cos_dd_f64_with_special(x, &handled);
  if ( handled ) return round_dd_to_f64(y);
  const f64 err = ziv_impl::abs_err_bound(y);
  if ( round_test_f64(y, err) ) [[likely]]
    return round_dd_to_f64(y);
  return round_dd_to_f64(y);
}

[[nodiscard, gnu::flatten]] inline constexpr f32
cos_f32(f32 x) noexcept
{
  bool handled = false;
  const dd64 y = mkbits::trig_ns::cos_dd_f64_with_special(f64(x), &handled);
  if ( handled ) {
    const f64 sum = y.hi + y.lo;
    return f32(sum);
  }
  return round_dd_to_f32(y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cos(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(cos_f32(f32(x)));
  else
    return F(cos_f64(f64(x)));
}

};      // namespace cr
};      // namespace math
};      // namespace micron

#pragma GCC pop_options

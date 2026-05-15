//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: currently the only __builtins we use are bit_cast, bswap, clz, ctz, popcount, parity, rotateleft, rotateright, add_overflow,
// sub_overflow, mul_overflow, umul_overflow, isnan, isinf_sign, isfinite, isnormal, signbit, fma since those don't actually pull in libm
// nor any libc code but are evaluated by the compiler in situ

#include "../concepts.hpp"
#include "../types.hpp"
#include "bits/impl.hpp"
#include "constants.hpp"
#include "cr.hpp"
#include "dispatch.hpp"
#include "generic.hpp"
#include "ieee.hpp"
#include "policy.hpp"

#include "bits/cordic.hpp"
#include "bits/exp.hpp"
#include "bits/hyp.hpp"
#include "bits/log.hpp"
#include "bits/manip.hpp"
#include "bits/pow.hpp"
#include "bits/rem.hpp"
#include "bits/round.hpp"
#include "bits/special.hpp"
#include "bits/sqrt.hpp"
#include "bits/trig.hpp"
#include "simd.hpp"

namespace micron
{
namespace math
{

namespace mkbits
{

#define __micron_math_builtin_fn(name)                                                                                                     \
  template<ieee754_floating F> [[nodiscard, gnu::always_inline]] inline constexpr F bi_##name(F x) noexcept                                \
  {                                                                                                                                        \
    if constexpr ( sizeof(F) == sizeof(float) )                                                                                            \
      return F(__builtin_##name##f(float(x)));                                                                                             \
    else if constexpr ( sizeof(F) == sizeof(double) )                                                                                      \
      return F(__builtin_##name(double(x)));                                                                                               \
    else                                                                                                                                   \
      return F(__builtin_##name##l(static_cast<long double>(x)));                                                                          \
  }

#define __micron_math_builtin_fn2(name)                                                                                                    \
  template<ieee754_floating F> [[nodiscard, gnu::always_inline]] inline constexpr F bi_##name(F a, F b) noexcept                           \
  {                                                                                                                                        \
    if constexpr ( sizeof(F) == sizeof(float) )                                                                                            \
      return F(__builtin_##name##f(float(a), float(b)));                                                                                   \
    else if constexpr ( sizeof(F) == sizeof(double) )                                                                                      \
      return F(__builtin_##name(double(a), double(b)));                                                                                    \
    else                                                                                                                                   \
      return F(__builtin_##name##l(static_cast<long double>(a), static_cast<long double>(b)));                                             \
  }

__micron_math_builtin_fn(sin);
__micron_math_builtin_fn(cos);
__micron_math_builtin_fn(tan);
__micron_math_builtin_fn(asin);
__micron_math_builtin_fn(acos);
__micron_math_builtin_fn(atan);
__micron_math_builtin_fn(sinh);
__micron_math_builtin_fn(cosh);
__micron_math_builtin_fn(tanh);
__micron_math_builtin_fn(asinh);
__micron_math_builtin_fn(acosh);
__micron_math_builtin_fn(atanh);
__micron_math_builtin_fn(exp);
__micron_math_builtin_fn(exp2);
__micron_math_builtin_fn(expm1);
__micron_math_builtin_fn(log);
__micron_math_builtin_fn(log2);
__micron_math_builtin_fn(log10);
__micron_math_builtin_fn(log1p);
__micron_math_builtin_fn(sqrt);
__micron_math_builtin_fn(cbrt);
__micron_math_builtin_fn(erf);
__micron_math_builtin_fn(erfc);
__micron_math_builtin_fn(tgamma);
__micron_math_builtin_fn(lgamma);
__micron_math_builtin_fn(j0);
__micron_math_builtin_fn(j1);
__micron_math_builtin_fn(y0);
__micron_math_builtin_fn(y1);
__micron_math_builtin_fn(floor);
__micron_math_builtin_fn(ceil);
__micron_math_builtin_fn(trunc);
__micron_math_builtin_fn(round);
__micron_math_builtin_fn(rint);
__micron_math_builtin_fn(nearbyint);

__micron_math_builtin_fn2(atan2);
__micron_math_builtin_fn2(pow);
__micron_math_builtin_fn2(hypot);
__micron_math_builtin_fn2(fmod);
__micron_math_builtin_fn2(remainder);
__micron_math_builtin_fn2(copysign);

#undef __micron_math_builtin_fn
#undef __micron_math_builtin_fn2

};      // namespace mkbits

namespace mk
{
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trigs
namespace trig
{
template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
sin(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_sin<F>(x);
  }
  if constexpr ( micron::is_same_v<P, policy::cr_tag> )
    return cr::sin<F>(x);
  else
    return mkbits::trig_ns::sin<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
cos(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_cos<F>(x);
  }
  if constexpr ( micron::is_same_v<P, policy::cr_tag> )
    return cr::cos<F>(x);
  else
    return mkbits::trig_ns::cos<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
tan(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_tan<F>(x);
  }
  return mkbits::trig_ns::tan<F>(x);
}

template<ieee754_floating F>
[[gnu::flatten]] inline constexpr void
sincos(F x, F &s, F &c) noexcept
{
  if consteval {
    s = mkbits::bi_sin<F>(x);
    c = mkbits::bi_cos<F>(x);
    return;
  }
  mkbits::trig_ns::sincos<F>(x, s, c);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
asin(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_asin<F>(x);
  }
  return mkbits::trig_ns::asin<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
acos(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_acos<F>(x);
  }
  return mkbits::trig_ns::acos<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_atan<F>(x);
  }
  return mkbits::trig_ns::atan<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan2(F y, F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_atan2<F>(y, x);
  }
  return mkbits::trig_ns::atan2<F>(y, x);
}

};      // namespace trig

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// CORDIC-based trig (shift+add)
namespace cordic
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
sin(F x) noexcept
{
  if consteval {
    return mkbits::bi_sin<F>(x);
  }
  return mkbits::cordic_ns::sin<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
cos(F x) noexcept
{
  if consteval {
    return mkbits::bi_cos<F>(x);
  }
  return mkbits::cordic_ns::cos<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr F
tan(F x) noexcept
{
  if consteval {
    return mkbits::bi_tan<F>(x);
  }
  return mkbits::cordic_ns::tan<F>(x);
}

template<ieee754_floating F>
[[gnu::flatten]] inline constexpr void
sincos(F x, F &s, F &c) noexcept
{
  if consteval {
    s = mkbits::bi_sin<F>(x);
    c = mkbits::bi_cos<F>(x);
    return;
  }
  mkbits::cordic_ns::sincos<F>(x, s, c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan(F x) noexcept
{
  if consteval {
    return mkbits::bi_atan<F>(x);
  }
  return mkbits::cordic_ns::atan<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan2(F y, F x) noexcept
{
  if consteval {
    return mkbits::bi_atan2<F>(y, x);
  }
  return mkbits::cordic_ns::atan2<F>(y, x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
asin(F x) noexcept
{
  if consteval {
    return mkbits::bi_asin<F>(x);
  }
  return mkbits::cordic_ns::asin<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
acos(F x) noexcept
{
  if consteval {
    return mkbits::bi_acos<F>(x);
  }
  return mkbits::cordic_ns::acos<F>(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
sin(V x) noexcept
{
  return mk::sin_cordic(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
cos(V x) noexcept
{
  return mk::cos_cordic(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
tan(V x) noexcept
{
  return mk::tan_cordic(x);
}

template<mk::packed_real V>
[[gnu::always_inline]] inline void
sincos(V x, V &s, V &c) noexcept
{
  s = mk::sin_cordic(x);
  c = mk::cos_cordic(x);
}
};      // namespace cordic

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// trig hyps
namespace hyp
{

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
sinh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_sinh<F>(x);
  }
  return mkbits::hyp_ns::sinh<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
cosh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_cosh<F>(x);
  }
  return mkbits::hyp_ns::cosh<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
tanh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_tanh<F>(x);
  }
  return mkbits::hyp_ns::tanh<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
asinh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_asinh<F>(x);
  }
  return mkbits::hyp_ns::asinh<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
acosh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_acosh<F>(x);
  }
  return mkbits::hyp_ns::acosh<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
atanh(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_atanh<F>(x);
  }
  return mkbits::hyp_ns::atanh<F>(x);
}

};      // namespace hyp

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exponents
namespace exp_ns
{
template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_exp<F>(x);
  }
  if constexpr ( micron::is_same_v<P, policy::cr_tag> )
    return cr::exp<F>(x);
  else
    return mkbits::exp_ns::exp<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp2(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_exp2<F>(x);
  }
  return mkbits::exp_ns::exp2<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
exp10(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_exp<F>(F(x * constant_ln10<F>));
  }
  return mkbits::exp_ns::exp10<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
expm1(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_expm1<F>(x);
  }
  return mkbits::exp_ns::expm1<F>(x);
}

};      // namespace exp_ns

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// logs
namespace log_ns
{
template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
log(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_log<F>(x);
  }
  if constexpr ( micron::is_same_v<P, policy::cr_tag> )
    return cr::log<F>(x);
  else
    return mkbits::log_ns::log<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
log2(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_log2<F>(x);
  }
  return mkbits::log_ns::log2<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
log10(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_log10<F>(x);
  }
  return mkbits::log_ns::log10<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
log1p(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_log1p<F>(x);
  }
  return mkbits::log_ns::log1p<F>(x);
}

};      // namespace log_ns

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pows

namespace pow_ns
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sqrt(F x) noexcept
{
  if consteval {
    return mkbits::bi_sqrt<F>(x);
  }
  return mkbits::sqrt_ns::sqrt<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cbrt(F x) noexcept
{
  if consteval {
    return mkbits::bi_cbrt<F>(x);
  }
  return mkbits::sqrt_ns::cbrt<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
pow(F x, F y) noexcept
{
  if consteval {
    return mkbits::bi_pow<F>(x, y);
  }
  return mkbits::pow_ns::pow<F>(x, y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
hypot(F x, F y) noexcept
{
  if consteval {
    return mkbits::bi_hypot<F>(x, y);
  }
  return mkbits::sqrt_ns::hypot<F>(x, y);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
rsqrt(F x, P p = {}) noexcept
{
  if consteval {
    return F(1) / mkbits::bi_sqrt<F>(x);
  }
  return mkbits::sqrt_ns::rsqrt<F>(x, p);
}

};      // namespace pow_ns

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// roundings

namespace round_ns
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
floor(F x) noexcept
{
  if consteval {
    return mkbits::bi_floor<F>(x);
  }
  return mkbits::round_ns::floor<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ceil(F x) noexcept
{
  if consteval {
    return mkbits::bi_ceil<F>(x);
  }
  return mkbits::round_ns::ceil<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
trunc(F x) noexcept
{
  if consteval {
    return mkbits::bi_trunc<F>(x);
  }
  return mkbits::round_ns::trunc<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
round(F x) noexcept
{
  if consteval {
    return mkbits::bi_round<F>(x);
  }
  return mkbits::round_ns::round<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
rint(F x) noexcept
{
  if consteval {
    return mkbits::bi_rint<F>(x);
  }
  return mkbits::round_ns::rint<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
nearbyint(F x) noexcept
{
  if consteval {
    return mkbits::bi_nearbyint<F>(x);
  }
  return mkbits::round_ns::nearbyint<F>(x);
}

};      // namespace round_ns

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mods and remainders

namespace rem
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fmod(F x, F y) noexcept
{
  if consteval {
    return mkbits::bi_fmod<F>(x, y);
  }
  return mkbits::rem::fmod<F>(x, y);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
remainder(F x, F y) noexcept
{
  if consteval {
    return mkbits::bi_remainder<F>(x, y);
  }
  return mkbits::rem::remainder<F>(x, y);
}

};      // namespace rem

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// misc

namespace manip
{

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fabs(F x) noexcept
{
  return mkbits::manip::fabs<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
frexp(F x, int *e) noexcept
{
  return mkbits::manip::frexp<F>(x, e);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ldexp(F x, int n) noexcept
{
  return mkbits::manip::ldexp<F>(x, n);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
copysign(F a, F b) noexcept
{
  return mkbits::manip::copysign<F>(a, b);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
ilogb(F x) noexcept
{
  return mkbits::manip::ilogb<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
logb(F x) noexcept
{
  return mkbits::manip::logb<F>(x);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
nextafter(F a, F b) noexcept
{
  return mkbits::manip::nextafter<F>(a, b);
}

};      // namespace manip

namespace special
{
template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
erf(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_erf<F>(x);
  }
  return mkbits::special_ns::erf<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
erfc(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_erfc<F>(x);
  }
  return mkbits::special_ns::erfc<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
tgamma(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_tgamma<F>(x);
  }
  return mkbits::special_ns::tgamma<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
lgamma(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_lgamma<F>(x);
  }
  return mkbits::special_ns::lgamma<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
j0(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_j0<F>(x);
  }
  return mkbits::special_ns::j0<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
j1(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_j1<F>(x);
  }
  return mkbits::special_ns::j1<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
y0(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_y0<F>(x);
  }
  return mkbits::special_ns::y0<F>(x);
}

template<ieee754_floating F, policy::policy_tag P = policy::faithful_tag>
[[nodiscard, gnu::always_inline]] inline constexpr F
y1(F x, P = {}) noexcept
{
  if consteval {
    return mkbits::bi_y1<F>(x);
  }
  return mkbits::special_ns::y1<F>(x);
}

};      // namespace special

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  fmas

namespace fused
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fma(F a, F b, F c) noexcept
{
  return math::fma<F>(a, b, c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fms(F a, F b, F c) noexcept
{
  return math::fma<F>(a, b, -c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fnma(F a, F b, F c) noexcept
{
  return math::fma<F>(-a, b, c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
dot2(F a0, F a1, F b0, F b1) noexcept
{
  return math::fma<F>(a0, b0, a1 * b1);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
dot3(F a0, F a1, F a2, F b0, F b1, F b2) noexcept
{
  return math::fma<F>(a0, b0, math::fma<F>(a1, b1, a2 * b2));
}

};      // namespace fused

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// simd overloads
// TODO: think about moving this to a separate file

namespace trig
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
sin(V x) noexcept
{
  return mk::sin(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
cos(V x) noexcept
{
  return mk::cos(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
tan(V x) noexcept
{
  return mk::tan(x);
}

template<mk::packed_real V>
[[gnu::always_inline]] inline void
sincos(V x, V &s, V &c) noexcept
{
  s = mk::sin(x);
  c = mk::cos(x);
}
};      // namespace trig

namespace exp_ns
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
exp(V x) noexcept
{
  return mk::exp(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
exp2(V x) noexcept
{
  return mk::exp2(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
expm1(V x) noexcept
{
  return mk::expm1(x);
}
};      // namespace exp_ns

namespace log_ns
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
log(V x) noexcept
{
  return mk::log(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
log2(V x) noexcept
{
  return mk::log2(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
log10(V x) noexcept
{
  return mk::log10(x);
}
};      // namespace log_ns

namespace pow_ns
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
sqrt(V x) noexcept
{
  return mk::sqrt(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
rsqrt(V x) noexcept
{
  return mk::rsqrt(x);
}
};      // namespace pow_ns

namespace round_ns
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
floor(V x) noexcept
{
  return mk::floor(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
ceil(V x) noexcept
{
  return mk::ceil(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
trunc(V x) noexcept
{
  return mk::trunc(x);
}

template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
rint(V x) noexcept
{
  return mk::rint(x);
}
};      // namespace round_ns

namespace manip
{
template<mk::packed_real V>
[[nodiscard, gnu::always_inline]] inline V
fabs(V x) noexcept
{
  return mk::fabs(x);
}
};      // namespace manip

};      // namespace mk

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// identities

namespace id
{
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cos_from_sin(F s, F sign = F(1)) noexcept
{
  return sign * mkbits::sqrt_ns::sqrt<F>(F(1) - s * s);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sin_from_cos(F c, F sign = F(1)) noexcept
{
  return sign * mkbits::sqrt_ns::sqrt<F>(F(1) - c * c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sin2x(F s, F c) noexcept
{
  return F(2) * s * c;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cos2x(F s, F c) noexcept
{
  return c * c - s * s;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
tan_half(F s, F c) noexcept
{
  return s / (F(1) + c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sin_cos_product(F a, F b) noexcept
{
  return F(0.5) * (mk::trig::sin(a + b) + mk::trig::sin(a - b));
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
cos_cos_product(F a, F b) noexcept
{
  return F(0.5) * (mk::trig::cos(a - b) + mk::trig::cos(a + b));
}

};      // namespace id

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// porcelain layer
// WARNING: due to spaghetti, be extra careful when moving fns around to prevent catastrophic namespace collisions

#define __micron_math_export_fn(NAME, NS)                                                                                                  \
  template<ieee754_floating F> [[nodiscard, gnu::always_inline]] inline constexpr F NAME(F x) noexcept { return mk::NS::NAME<F>(x); }

#define __micron_math_export_fn2(NAME, NS)                                                                                                 \
  template<ieee754_floating F> [[nodiscard, gnu::always_inline]] inline constexpr F NAME(F a, F b) noexcept                                \
  {                                                                                                                                        \
    return mk::NS::NAME<F>(a, b);                                                                                                          \
  }

// trig
__micron_math_export_fn(sin, trig);
__micron_math_export_fn(cos, trig);
__micron_math_export_fn(tan, trig);
__micron_math_export_fn(asin, trig);
__micron_math_export_fn(acos, trig);
__micron_math_export_fn(atan, trig);
__micron_math_export_fn2(atan2, trig);

template<ieee754_floating F>
[[gnu::always_inline]] inline constexpr void
sincos(F x, F &s, F &c) noexcept
{
  mk::trig::sincos<F>(x, s, c);
}

#define __micron_math_export_cordic(NAME)                                                                                                  \
  template<ieee754_floating F> [[nodiscard, gnu::always_inline]] inline constexpr F NAME##_cordic(F x) noexcept                            \
  {                                                                                                                                        \
    return mk::cordic::NAME<F>(x);                                                                                                         \
  }

__micron_math_export_cordic(sin);
__micron_math_export_cordic(cos);
__micron_math_export_cordic(tan);
__micron_math_export_cordic(asin);
__micron_math_export_cordic(acos);
__micron_math_export_cordic(atan);

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
atan2_cordic(F y, F x) noexcept
{
  return mk::cordic::atan2<F>(y, x);
}

template<ieee754_floating F>
[[gnu::always_inline]] inline constexpr void
sincos_cordic(F x, F &s, F &c) noexcept
{
  mk::cordic::sincos<F>(x, s, c);
}

#undef __micron_math_export_cordic

using mk::cos;
using mk::cos_cordic;
using mk::sin;
using mk::sin_cordic;
using mk::sincos;
using mk::sincos_cordic;
using mk::tan;
using mk::tan_cordic;

// hyp
__micron_math_export_fn(sinh, hyp);
__micron_math_export_fn(cosh, hyp);
__micron_math_export_fn(tanh, hyp);
__micron_math_export_fn(asinh, hyp);
__micron_math_export_fn(acosh, hyp);
__micron_math_export_fn(atanh, hyp);

// exp
__micron_math_export_fn(exp, exp_ns);
__micron_math_export_fn(exp2, exp_ns);
__micron_math_export_fn(exp10, exp_ns);
__micron_math_export_fn(expm1, exp_ns);

// log
__micron_math_export_fn(log, log_ns);
__micron_math_export_fn(log2, log_ns);
__micron_math_export_fn(log10, log_ns);
__micron_math_export_fn(log1p, log_ns);

// pow
__micron_math_export_fn(sqrt, pow_ns);
__micron_math_export_fn(cbrt, pow_ns);
__micron_math_export_fn2(pow, pow_ns);
__micron_math_export_fn2(hypot, pow_ns);
__micron_math_export_fn(rsqrt, pow_ns);

// round
__micron_math_export_fn(floor, round_ns);
__micron_math_export_fn(ceil, round_ns);
__micron_math_export_fn(trunc, round_ns);
__micron_math_export_fn(round, round_ns);
__micron_math_export_fn(rint, round_ns);
__micron_math_export_fn(nearbyint, round_ns);

// rem
__micron_math_export_fn2(fmod, rem);
__micron_math_export_fn2(remainder, rem);

// manip
__micron_math_export_fn(fabs, manip);
__micron_math_export_fn2(copysign, manip);
__micron_math_export_fn(logb, manip);
__micron_math_export_fn2(nextafter, manip);

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
frexp(F x, int *e) noexcept
{
  return mk::manip::frexp<F>(x, e);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
ldexp(F x, int n) noexcept
{
  return mk::manip::ldexp<F>(x, n);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr int
ilogb(F x) noexcept
{
  return mk::manip::ilogb<F>(x);
}

// special
__micron_math_export_fn(erf, special);
__micron_math_export_fn(erfc, special);
__micron_math_export_fn(tgamma, special);
__micron_math_export_fn(lgamma, special);
__micron_math_export_fn(j0, special);
__micron_math_export_fn(j1, special);
__micron_math_export_fn(y0, special);
__micron_math_export_fn(y1, special);

#undef __micron_math_export_fn
#undef __micron_math_export_fn2

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fms(F a, F b, F c) noexcept
{
  return math::fma<F>(a, b, -c);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fnma(F a, F b, F c) noexcept
{
  return math::fma<F>(-a, b, c);
}

};      // namespace math
};      // namespace micron

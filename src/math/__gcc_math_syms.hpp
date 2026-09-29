//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// FREESTANDING LIBM SYMBOLS
//
// WARNING: THESE AREN'T FOR EXTERNAL USE, THE COMPILER EMITS CALLS TO THESE IF USING __BUILTIN_* QUALIFIED FNS FOR ERRNO CALLBACK OR IF NO
// HARDWARE FORM EXISTS ALL DEFINITIONS ARE WEAK SO WE DON'T COLLIDE WITH REAL LIBM OR TU INCLUDE SPAGHETTI; THIS SHOULD EXCLUSIVELY BE
// INCLUDED IN START.CPP
//
// NEVER USE THIS IN REGULAR CODE, PREFER USING REGULAR MICRON MATH FNS

#include "../types.hpp"

#include "__asm/hw.hpp"
#include "cr.hpp"
#include "generic.hpp"

#include "bits/exp.hpp"
#include "bits/hyp.hpp"
#include "bits/log.hpp"
#include "bits/manip.hpp"
#include "bits/rem.hpp"
#include "bits/round.hpp"
#include "bits/special.hpp"

namespace micron
{
namespace math
{
namespace __shim
{

#if defined(__micron_x86_fma) || (defined(__micron_arch_arm64) && defined(__micron_arm_neon))                                              \
    || (defined(__micron_arch_arm32) && defined(__micron_arm_fma))
#define __MC_SHIM_HW_FMA 1
#endif

// mkbits::rem::fmod ORs in the implicit one before its subnormal-normalisation loop
template<ieee754_floating F>
[[nodiscard]] inline F
fmod(F x, F y) noexcept
{
  if ( !ieee::is_subnormal(y) ) return mkbits::rem::fmod<F>(x, y);
  constexpr int k = ieee::traits<F>::mant_bits + 1;
  const F ys = mkbits::manip::scalbn<F>(y, k);
  const F r = mkbits::rem::fmod<F>(x, ys);
  return mkbits::manip::scalbn<F>(mkbits::rem::fmod<F>(mkbits::manip::scalbn<F>(r, k), ys), -k);
}

template<ieee754_floating F>
[[nodiscard]] inline F
hypot(F x, F y) noexcept
{
  F ax = mkbits::manip::fabs(x);
  F ay = mkbits::manip::fabs(y);
  if ( ieee::is_inf(ax) || ieee::is_inf(ay) ) return ieee::inf_v<F>();
  if ( ieee::is_nan(ax) || ieee::is_nan(ay) ) return ieee::qnan_v<F>();
  if ( ax < ay ) {
    const F t = ax;
    ax = ay;
    ay = t;
  }
  if ( ay == F(0) ) return ax;

  const int e = mkbits::manip::ilogb<F>(ax);
  const F sx = mkbits::manip::scalbn<F>(ax, -e);
  const F sy = mkbits::manip::scalbn<F>(ay, -e);
  const F q = F(sx * sx + sy * sy);
  F r;
  if constexpr ( sizeof(F) == 4 )
    r = hw::sqrt_ss(q);
  else
    r = hw::sqrt_sd(q);
  return mkbits::manip::scalbn<F>(r, e);
}

#if !defined(__MC_SHIM_HW_FMA)
// s + e == a + b exactly
template<ieee754_floating F>
[[gnu::always_inline]] inline void
two_sum(F a, F b, F &s, F &e) noexcept
{
  s = hw::fp_barrier(F(a + b));
  const F bp = hw::fp_barrier(F(s - a));
  const F lo = hw::fp_barrier(F(a - hw::fp_barrier(F(s - bp))));
  e = hw::fp_barrier(F(lo + hw::fp_barrier(F(b - bp))));
}
#endif

[[nodiscard]] inline f64
fma(f64 a, f64 b, f64 c) noexcept
{
#if defined(__MC_SHIM_HW_FMA)
  return hw::fmadd_sd(a, b, c);
#else
  const f64 p = hw::fp_barrier(f64(a * b));
  const f64 pa = mkbits::manip::fabs(p);
  // WARNING: dekker's split overflows above 2^996
  if ( !(mkbits::manip::fabs(a) <= 0x1p996) || !(mkbits::manip::fabs(b) <= 0x1p996) || !(pa >= 0x1p-960) || !(pa <= 0x1p1000)
       || !(mkbits::manip::fabs(c) <= 0x1p1000) )
    return f64(p + c);

  constexpr f64 split = 0x1.0p27 + 1.0;
  const f64 at = hw::fp_barrier(f64(split * a));
  const f64 ah = hw::fp_barrier(f64(at - hw::fp_barrier(f64(at - a))));
  const f64 al = hw::fp_barrier(f64(a - ah));
  const f64 bt = hw::fp_barrier(f64(split * b));
  const f64 bh = hw::fp_barrier(f64(bt - hw::fp_barrier(f64(bt - b))));
  const f64 bl = hw::fp_barrier(f64(b - bh));

  f64 e = hw::fp_barrier(f64(hw::fp_barrier(f64(ah * bh)) - p));
  e = hw::fp_barrier(f64(e + hw::fp_barrier(f64(ah * bl))));
  e = hw::fp_barrier(f64(e + hw::fp_barrier(f64(al * bh))));
  e = hw::fp_barrier(f64(e + hw::fp_barrier(f64(al * bl))));

  f64 sh, sl, vh, vl, zh, zl;
  two_sum<f64>(p, c, sh, sl);        // sh + sl == p + c
  two_sum<f64>(sl, e, vh, vl);       // vh + vl == sl + e
  two_sum<f64>(sh, vh, zh, zl);      // zh + zl + vl == a * b + c, exactly
  return f64(zh + hw::fp_barrier(f64(zl + vl)));
#endif
}

[[nodiscard]] inline f32
fma(f32 a, f32 b, f32 c) noexcept
{
#if defined(__MC_SHIM_HW_FMA)
  return hw::fmadd_ss(a, b, c);
#else
  // 24 + 24 significand bits, and f32's exponent range cannot over- or underflow f64, so the
  // product is exact and the sum below rounds once -- into f64
  const f64 p = hw::fp_barrier(f64(f64(a) * f64(b)));
  if ( !ieee::is_finite(p) || !ieee::is_finite(f64(c)) ) return f32(p + f64(c));

  f64 sh, sl;
  two_sum<f64>(p, f64(c), sh, sl);
  if ( sl != 0.0 && mkbits::manip::fabs(sh) >= 0x1p-126 ) {
    // sh sits on an f32 halfway point, where the cast below would round on a tie the exact
    // residual sl has already decided; step sh off it in sl's direction
    u64 u = ieee::to_bits(sh);
    if ( (u & 0x1fffffffULL) == 0x10000000ULL ) u += ((sl < 0.0) == (sh < 0.0)) ? u64(1) : ~u64(0);
    sh = ieee::from_bits<f64>(u);
  }
  return f32(sh);
#endif
}

};      // namespace __shim
};      // namespace math
};      // namespace micron

__micron_diagnostic_push      // we are deliberately (re)defining functions the compiler knows as builtins
    __micron_diagnostic_ignored("-Wbuiltin-declaration-mismatch")
#define __MC_M1(NAME, DBODY, FBODY, LBODY)                                                                                                 \
  extern "C" __attribute__((weak)) double NAME(double x) noexcept { return (DBODY); }                                                      \
  extern "C" __attribute__((weak)) float NAME##f(float x) noexcept { return (FBODY); }                                                     \
  extern "C" __attribute__((weak)) long double NAME##l(long double x) noexcept { return (LBODY); }

#define __MC_M2(NAME, DBODY, FBODY, LBODY)                                                                                                 \
  extern "C" __attribute__((weak)) double NAME(double x, double y) noexcept { return (DBODY); }                                            \
  extern "C" __attribute__((weak)) float NAME##f(float x, float y) noexcept { return (FBODY); }                                            \
  extern "C" __attribute__((weak)) long double NAME##l(long double x, long double y) noexcept { return (LBODY); }

        __MC_M1(sqrt, micron::math::hw::sqrt_sd(x), micron::math::hw::sqrt_ss(x),
                static_cast<long double>(micron::math::hw::sqrt_sd(static_cast<double>(x))))

    // __builtin_fabs is a pure bit-op on every arch
    __MC_M1(cbrt, __builtin_copysign(micron::math::powerf(__builtin_fabs(x), 1.0 / 3.0), x),
            __builtin_copysignf(micron::math::powerf32(__builtin_fabsf(x), static_cast<float>(1.0 / 3.0)), x),
            static_cast<long double>(__builtin_copysign(micron::math::powerf(__builtin_fabs(static_cast<double>(x)), 1.0 / 3.0),
                                                        static_cast<double>(x))))

        __MC_M1(exp, micron::math::expf64(x), micron::math::expf32(x), micron::math::expf128(x))
            __MC_M1(exp2, micron::math::powerf(2.0, x), micron::math::powerf32(2.0f, x), micron::math::powerflong(2.0L, x))

                __MC_M1(log, micron::math::logf64(x), micron::math::logf32(x), micron::math::logf128(x))
                    __MC_M1(log2, micron::math::flog2(x), micron::math::flog2(x), micron::math::flog2(x))
                        __MC_M1(log10, micron::math::log10f64(x), micron::math::log10f32(x), micron::math::log10f128(x))

                            __MC_M1(sin, micron::math::cr::sin_f64(x), micron::math::cr::sin_f32(x),
                                    static_cast<long double>(micron::math::cr::sin_f64(static_cast<double>(x))))
                                __MC_M1(cos, micron::math::cr::cos_f64(x), micron::math::cr::cos_f32(x),
                                        static_cast<long double>(micron::math::cr::cos_f64(static_cast<double>(x))))
                                    __MC_M1(tan, micron::math::cr::sin_f64(x) / micron::math::cr::cos_f64(x),
                                            micron::math::cr::sin_f32(x) / micron::math::cr::cos_f32(x),
                                            static_cast<long double>(micron::math::cr::sin_f64(static_cast<double>(x))
                                                                     / micron::math::cr::cos_f64(static_cast<double>(x))))

    // these are libcalls on armv7-a (no vrintX)
    __MC_M1(ceil, micron::math::ceil(x), micron::math::ceil(x), micron::math::ceil(x))
        __MC_M1(floor, micron::math::floor(x), micron::math::floor(x), micron::math::floor(x))
            __MC_M1(round, micron::math::round(x), micron::math::round(x), micron::math::round(x))
                __MC_M1(trunc, (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)),
                        (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)), (x < 0 ? micron::math::ceil(x) : micron::math::floor(x)))
                    __MC_M1(rint, micron::math::rint(x), micron::math::rint(x),
                            static_cast<long double>(micron::math::rint(static_cast<double>(x))))
                        __MC_M1(nearbyint, micron::math::nearbyint(x), micron::math::nearbyint(x),
                                static_cast<long double>(micron::math::nearbyint(static_cast<double>(x))))

    // __builtin_fabs/copysign are pure bit-ops, never libcalls
    __MC_M1(fabs, __builtin_fabs(x), __builtin_fabsf(x), __builtin_fabsl(x))
        __MC_M2(copysign, __builtin_copysign(x, y), __builtin_copysignf(x, y), __builtin_copysignl(x, y))

            __MC_M2(pow, micron::math::powerf(x, y), micron::math::powerf32(x, y), micron::math::powerflong(x, y))
                __MC_M2(remainder, micron::math::remainder(x, y), micron::math::remainder(x, y),
                        static_cast<long double>(micron::math::remainder(static_cast<double>(x), static_cast<double>(y))))
                    __MC_M2(hypot, micron::math::__shim::hypot<double>(x, y), micron::math::__shim::hypot<float>(x, y),
                            static_cast<long double>(micron::math::hw::sqrt_sd(static_cast<double>(x) * static_cast<double>(x)
                                                                               + static_cast<double>(y) * static_cast<double>(y))))

                        extern "C" __attribute__((weak)) double fmod(double x, double y) noexcept
{
  return micron::math::__shim::fmod<double>(x, y);
}

extern "C" __attribute__((weak)) float
fmodf(float x, float y) noexcept
{
  return micron::math::__shim::fmod<float>(x, y);
}

extern "C" __attribute__((weak)) long double
fmodl(long double x, long double y) noexcept
{
  long double q = x / y;
  long double t = (q < 0 ? micron::math::ceil(q) : micron::math::floor(q));
  return x - t * y;
}

// GCC 16 LTO can commute builtin operands after specializing these definitions,
// leaving a constant-propagation clone with the wrong operand
#if defined(__micron_compiler_gcc)
#define __MC_SHIM_FMA_BOUNDARY __attribute__((weak, noipa))
#else
#define __MC_SHIM_FMA_BOUNDARY __attribute__((weak))
#endif

extern "C" __MC_SHIM_FMA_BOUNDARY double
fma(double a, double b, double c) noexcept
{
  return micron::math::__shim::fma(a, b, c);
}

extern "C" __MC_SHIM_FMA_BOUNDARY float
fmaf(float a, float b, float c) noexcept
{
  return micron::math::__shim::fma(a, b, c);
}

extern "C" __MC_SHIM_FMA_BOUNDARY long double
fmal(long double a, long double b, long double c) noexcept
{
  return a * b + c;
}

#undef __MC_SHIM_FMA_BOUNDARY

// missing __builtin_* symbols (these should be almost all of them, although these libm fns fmax fmin fdim ldexp frexp modf scalbn nextafter
// ilogb remquo lrint llrint lround llround still don't exist in micron)

// inverse trig
__MC_M1(asin, micron::math::mkbits::trig_ns::asin<f64>(x), micron::math::mkbits::trig_ns::asin<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::asin<f64>(static_cast<double>(x))))
__MC_M1(acos, micron::math::mkbits::trig_ns::acos<f64>(x), micron::math::mkbits::trig_ns::acos<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::acos<f64>(static_cast<double>(x))))
__MC_M1(atan, micron::math::mkbits::trig_ns::atan<f64>(x), micron::math::mkbits::trig_ns::atan<f32>(x),
        static_cast<long double>(micron::math::mkbits::trig_ns::atan<f64>(static_cast<double>(x))))

// hyperbolic
__MC_M1(sinh, micron::math::mkbits::hyp_ns::sinh<f64>(x), micron::math::mkbits::hyp_ns::sinh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::sinh<f64>(static_cast<double>(x))))
__MC_M1(cosh, micron::math::mkbits::hyp_ns::cosh<f64>(x), micron::math::mkbits::hyp_ns::cosh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::cosh<f64>(static_cast<double>(x))))
__MC_M1(tanh, micron::math::mkbits::hyp_ns::tanh<f64>(x), micron::math::mkbits::hyp_ns::tanh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::tanh<f64>(static_cast<double>(x))))
__MC_M1(asinh, micron::math::mkbits::hyp_ns::asinh<f64>(x), micron::math::mkbits::hyp_ns::asinh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::asinh<f64>(static_cast<double>(x))))
__MC_M1(acosh, micron::math::mkbits::hyp_ns::acosh<f64>(x), micron::math::mkbits::hyp_ns::acosh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::acosh<f64>(static_cast<double>(x))))
__MC_M1(atanh, micron::math::mkbits::hyp_ns::atanh<f64>(x), micron::math::mkbits::hyp_ns::atanh<f32>(x),
        static_cast<long double>(micron::math::mkbits::hyp_ns::atanh<f64>(static_cast<double>(x))))

// exp/log tail
__MC_M1(exp10, micron::math::mkbits::exp_ns::exp10<f64>(x), micron::math::mkbits::exp_ns::exp10<f32>(x),
        static_cast<long double>(micron::math::mkbits::exp_ns::exp10<f64>(static_cast<double>(x))))
__MC_M1(expm1, micron::math::mkbits::exp_ns::expm1<f64>(x), micron::math::mkbits::exp_ns::expm1<f32>(x),
        static_cast<long double>(micron::math::mkbits::exp_ns::expm1<f64>(static_cast<double>(x))))
__MC_M1(log1p, micron::math::mkbits::log_ns::log1p<f64>(x), micron::math::mkbits::log_ns::log1p<f32>(x),
        static_cast<long double>(micron::math::mkbits::log_ns::log1p<f64>(static_cast<double>(x))))
__MC_M1(logb, micron::math::mkbits::manip::logb<f64>(x), micron::math::mkbits::manip::logb<f32>(x),
        static_cast<long double>(micron::math::mkbits::manip::logb<f64>(static_cast<double>(x))))

// error fn / gamma
__MC_M1(erf, micron::math::mkbits::special_ns::erf<f64>(x), micron::math::mkbits::special_ns::erf<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::erf<f64>(static_cast<double>(x))))
__MC_M1(erfc, micron::math::mkbits::special_ns::erfc<f64>(x), micron::math::mkbits::special_ns::erfc<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::erfc<f64>(static_cast<double>(x))))
__MC_M1(tgamma, micron::math::mkbits::special_ns::tgamma<f64>(x), micron::math::mkbits::special_ns::tgamma<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::tgamma<f64>(static_cast<double>(x))))
__MC_M1(lgamma, micron::math::mkbits::special_ns::lgamma<f64>(x), micron::math::mkbits::special_ns::lgamma<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::lgamma<f64>(static_cast<double>(x))))

// bessel
__MC_M1(j0, micron::math::mkbits::special_ns::j0<f64>(x), micron::math::mkbits::special_ns::j0<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::j0<f64>(static_cast<double>(x))))
__MC_M1(j1, micron::math::mkbits::special_ns::j1<f64>(x), micron::math::mkbits::special_ns::j1<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::j1<f64>(static_cast<double>(x))))
__MC_M1(y0, micron::math::mkbits::special_ns::y0<f64>(x), micron::math::mkbits::special_ns::y0<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::y0<f64>(static_cast<double>(x))))
__MC_M1(y1, micron::math::mkbits::special_ns::y1<f64>(x), micron::math::mkbits::special_ns::y1<f32>(x),
        static_cast<long double>(micron::math::mkbits::special_ns::y1<f64>(static_cast<double>(x))))

extern "C" __attribute__((weak)) double
atan2(double y, double x) noexcept
{
  return micron::math::mkbits::trig_ns::atan2<f64>(y, x);
}

extern "C" __attribute__((weak)) float
atan2f(float y, float x) noexcept
{
  return micron::math::mkbits::trig_ns::atan2<f32>(y, x);
}

extern "C" __attribute__((weak)) long double
atan2l(long double y, long double x) noexcept
{
  return static_cast<long double>(micron::math::mkbits::trig_ns::atan2<f64>(static_cast<double>(y), static_cast<double>(x)));
}

static_assert(micron::math::mkbits::trig_ns::atan2<f64>(1.0, 0.0) > 1.5707 && micron::math::mkbits::trig_ns::atan2<f64>(1.0, 0.0) < 1.5709,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=1, x=0) must be +pi/2");
static_assert(micron::math::mkbits::trig_ns::atan2<f64>(0.0, -1.0) > 3.1415
                  && micron::math::mkbits::trig_ns::atan2<f64>(0.0, -1.0) < 3.1417,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=0, x=-1) must be +pi");
static_assert(micron::math::mkbits::trig_ns::atan2<f64>(-1.0, 0.0) < -1.5707,
              "atan2 kernel takes (ordinate, abscissa): atan2(y=-1, x=0) must be -pi/2");

#undef __MC_M1
#undef __MC_M2
#undef __MC_SHIM_HW_FMA

__micron_diagnostic_pop
// libgcc integer fns
#include "__gcc_int_syms.hpp"

// libgcc binary128 soft fp fns
#include "__gcc_fp128_syms.hpp"

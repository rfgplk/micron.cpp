//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../concepts.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace hw
{

template <typename F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fp_barrier(F x) noexcept
{
  if consteval {
    return x;
  }
#if defined(__has_builtin) && __has_builtin(__builtin_assoc_barrier)
  return __builtin_assoc_barrier(x);
#else
  volatile F v = x;
  return v;
#endif
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
sqrt_ss(f32 x) noexcept
{
  if consteval {
    return f32(__builtin_sqrtf(x));
  }
#if defined(__micron_arch_x86_any)
  f32 r;
  __asm__("sqrtss %1, %0" : "=x"(r) : "x"(x));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vgetq_lane_f32(vsqrtq_f32(vdupq_n_f32(x)), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  f32 r;
  __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(x));
  return r;
#else
  return f32(__builtin_sqrtf(x));
#endif
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
sqrt_sd(f64 x) noexcept
{
  if consteval {
    return f64(__builtin_sqrt(x));
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse2)
  f64 r;
  __asm__("sqrtsd %1, %0" : "=x"(r) : "x"(x));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vgetq_lane_f64(vsqrtq_f64(vdupq_n_f64(x)), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  f64 r;
  __asm__("vsqrt.f64 %P0, %P1" : "=w"(r) : "w"(x));
  return r;
#else
  return f64(__builtin_sqrt(x));
#endif
}

template <typename F>
[[nodiscard, gnu::always_inline]] inline constexpr F
sqrt(F x) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(sqrt_ss(f32(x)));
  else
    return F(sqrt_sd(f64(x)));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
rsqrt_approx_ss(f32 x) noexcept
{
  if consteval {
    return 1.0f / f32(__builtin_sqrtf(x));
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse)
  f32 r;
  __asm__("rsqrtss %1, %0" : "=x"(r) : "x"(x));
  return r;
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
  return vgetq_lane_f32(vrsqrteq_f32(vdupq_n_f32(x)), 0);
#else
  return 1.0f / sqrt_ss(x);
#endif
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
fmadd_ss(f32 a, f32 b, f32 c) noexcept
{
  if consteval {
    // clang SOMEHOW doesn't support fma for constexprs?!?!
#if defined(__micron_compiler_gcc)
    return f32(__builtin_fmaf(a, b, c));
#else
    return f32(a * b + c);
#endif
  }
#if defined(__micron_x86_fma)
  f32 r = c;
  __asm__("vfmadd231ss %2, %1, %0" : "+x"(r) : "x"(a), "x"(b));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vfmas_lane_f32(c, a, vdup_n_f32(b), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_fma) && defined(__micron_arm_neon)
  f32 r = c;
  __asm__("vfma.f32 %0, %1, %2" : "+t"(r) : "t"(a), "t"(b));
  return r;
#else
  return f32(__builtin_fmaf(a, b, c));
#endif
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
fmadd_sd(f64 a, f64 b, f64 c) noexcept
{
  if consteval {
#if defined(__micron_compiler_gcc)
    return f64(__builtin_fma(a, b, c));
#else
    return f64(a * b + c);
#endif
  }
#if defined(__micron_x86_fma)
  f64 r = c;
  __asm__("vfmadd231sd %2, %1, %0" : "+x"(r) : "x"(a), "x"(b));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vfmad_lane_f64(c, a, vdup_n_f64(b), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_fma)
  f64 r = c;
  __asm__("vfma.f64 %P0, %P1, %P2" : "+w"(r) : "w"(a), "w"(b));
  return r;
#else
  return f64(__builtin_fma(a, b, c));
#endif
}

template <typename F>
[[nodiscard, gnu::always_inline]] inline constexpr F
fmadd(F a, F b, F c) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(fmadd_ss(f32(a), f32(b), f32(c)));
  else
    return F(fmadd_sd(f64(a), f64(b), f64(c)));
}

// current modes are
//  0 = nearest (IEEE round-to-even)
//  1 = down    (toward -inf)
//  2 = up      (toward +inf)
//  3 = trunc   (toward zero)

template <int Mode>
[[nodiscard, gnu::always_inline]] inline constexpr f32
round_ss(f32 x) noexcept
{
  static_assert(Mode >= 0 && Mode < 4, "round mode must be 0..3");
  if consteval {
    if constexpr ( Mode == 0 )
      return f32(__builtin_nearbyintf(x));
    else if constexpr ( Mode == 1 )
      return f32(__builtin_floorf(x));
    else if constexpr ( Mode == 2 )
      return f32(__builtin_ceilf(x));
    else
      return f32(__builtin_truncf(x));
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse4_1)
  f32 r;
  if constexpr ( Mode == 0 )
    __asm__("roundss $0, %1, %0" : "=x"(r) : "x"(x));
  else if constexpr ( Mode == 1 )
    __asm__("roundss $1, %1, %0" : "=x"(r) : "x"(x));
  else if constexpr ( Mode == 2 )
    __asm__("roundss $2, %1, %0" : "=x"(r) : "x"(x));
  else
    __asm__("roundss $3, %1, %0" : "=x"(r) : "x"(x));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( Mode == 0 )
    return vgetq_lane_f32(vrndnq_f32(vdupq_n_f32(x)), 0);
  else if constexpr ( Mode == 1 )
    return vgetq_lane_f32(vrndmq_f32(vdupq_n_f32(x)), 0);
  else if constexpr ( Mode == 2 )
    return vgetq_lane_f32(vrndpq_f32(vdupq_n_f32(x)), 0);
  else
    return vgetq_lane_f32(vrndq_f32(vdupq_n_f32(x)), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon) && defined(__micron_arm_directed_rounding)
  if constexpr ( Mode == 0 )
    return vgetq_lane_f32(vrndnq_f32(vdupq_n_f32(x)), 0);
  else if constexpr ( Mode == 1 )
    return vgetq_lane_f32(vrndmq_f32(vdupq_n_f32(x)), 0);
  else if constexpr ( Mode == 2 )
    return vgetq_lane_f32(vrndpq_f32(vdupq_n_f32(x)), 0);
  else
    return vgetq_lane_f32(vrndq_f32(vdupq_n_f32(x)), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_directed_rounding)
  f32 r;
  if constexpr ( Mode == 0 )
    __asm__("vrintn.f32 %0, %1" : "=t"(r) : "t"(x));
  else if constexpr ( Mode == 1 )
    __asm__("vrintm.f32 %0, %1" : "=t"(r) : "t"(x));
  else if constexpr ( Mode == 2 )
    __asm__("vrintp.f32 %0, %1" : "=t"(r) : "t"(x));
  else
    __asm__("vrintz.f32 %0, %1" : "=t"(r) : "t"(x));
  return r;
#else
  if constexpr ( Mode == 0 )
    return f32(__builtin_nearbyintf(x));
  else if constexpr ( Mode == 1 )
    return f32(__builtin_floorf(x));
  else if constexpr ( Mode == 2 )
    return f32(__builtin_ceilf(x));
  else
    return f32(__builtin_truncf(x));
#endif
}

template <int Mode>
[[nodiscard, gnu::always_inline]] inline constexpr f64
round_sd(f64 x) noexcept
{
  static_assert(Mode >= 0 && Mode < 4, "round mode must be 0..3");
  if consteval {
    if constexpr ( Mode == 0 )
      return f64(__builtin_nearbyint(x));
    else if constexpr ( Mode == 1 )
      return f64(__builtin_floor(x));
    else if constexpr ( Mode == 2 )
      return f64(__builtin_ceil(x));
    else
      return f64(__builtin_trunc(x));
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse4_1)
  f64 r;
  if constexpr ( Mode == 0 )
    __asm__("roundsd $0, %1, %0" : "=x"(r) : "x"(x));
  else if constexpr ( Mode == 1 )
    __asm__("roundsd $1, %1, %0" : "=x"(r) : "x"(x));
  else if constexpr ( Mode == 2 )
    __asm__("roundsd $2, %1, %0" : "=x"(r) : "x"(x));
  else
    __asm__("roundsd $3, %1, %0" : "=x"(r) : "x"(x));
  return r;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( Mode == 0 )
    return vgetq_lane_f64(vrndnq_f64(vdupq_n_f64(x)), 0);
  else if constexpr ( Mode == 1 )
    return vgetq_lane_f64(vrndmq_f64(vdupq_n_f64(x)), 0);
  else if constexpr ( Mode == 2 )
    return vgetq_lane_f64(vrndpq_f64(vdupq_n_f64(x)), 0);
  else
    return vgetq_lane_f64(vrndq_f64(vdupq_n_f64(x)), 0);
#elif defined(__micron_arch_arm32) && defined(__micron_arm_directed_rounding)
  // Scalar VFP VRINT* on f64 D-register.  No NEON-quad-f64 on AArch32, so
  // the per-element NEON form doesn't apply.
  f64 r;
  if constexpr ( Mode == 0 )
    __asm__("vrintn.f64 %P0, %P1" : "=w"(r) : "w"(x));
  else if constexpr ( Mode == 1 )
    __asm__("vrintm.f64 %P0, %P1" : "=w"(r) : "w"(x));
  else if constexpr ( Mode == 2 )
    __asm__("vrintp.f64 %P0, %P1" : "=w"(r) : "w"(x));
  else
    __asm__("vrintz.f64 %P0, %P1" : "=w"(r) : "w"(x));
  return r;
#else
  if constexpr ( Mode == 0 )
    return f64(__builtin_nearbyint(x));
  else if constexpr ( Mode == 1 )
    return f64(__builtin_floor(x));
  else if constexpr ( Mode == 2 )
    return f64(__builtin_ceil(x));
  else
    return f64(__builtin_trunc(x));
#endif
}

};     // namespace hw
};     // namespace math
};     // namespace micron

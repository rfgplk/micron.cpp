//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// register-level cores for the graphics kernels
// (normalize / cross / inv4 / look_at)
//
// two accuracy tiers:
//   exact = hardware sqrt + true division (the default-path contract)
//   fast  = rsqrt estimate + Newton refinement to ~2^-22 (policy::fast)

#include "../bits/__arch.hpp"
#include "../types.hpp"
#include "__asm/hw.hpp"
#include "ieee.hpp"

// x86 needs SSE2
#if ( defined(__micron_arch_x86_any) && defined(__micron_x86_sse2) ) || (defined(__micron_arch_arm_any) && defined(__micron_arm_neon))
#define __micron_gfx_simd 1
#include "../simd/aliases.hpp"
#include "../simd/types.hpp"
#endif

// NOTE: we __must__ disable fast-math here: the Newton step r*(1.5 - h*r*r)
// and the exact 1/sqrt(x) forms must not be reassociated or rewritten into
// reciprocal estimates by -Ofast
__micron_push_options
__micron_optimize_no_fast_math
namespace micron
{
namespace math
{
namespace __vsimd
{

// %%%%%%%%%%%%%%%%%%%
// scalar cores

// IEEE division
[[nodiscard, gnu::always_inline]] inline constexpr f32
__div_exact_ss(f32 a, f32 b) noexcept
{
  if consteval {
    return a / b;
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse)
  f32 r;
#if defined(__micron_x86_avx)
  __asm__("vdivss %2, %1, %0" : "=x"(r) : "x"(a), "x"(b));
#else
  r = a;
  __asm__("divss %1, %0" : "+x"(r) : "x"(b));
#endif
  return r;
#else
  return a / b;
#endif
}

[[nodiscard, gnu::always_inline]] inline constexpr f64
__div_exact_sd(f64 a, f64 b) noexcept
{
  if consteval {
    return a / b;
  }
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse2)
  f64 r;
#if defined(__micron_x86_avx)
  __asm__("vdivsd %2, %1, %0" : "=x"(r) : "x"(a), "x"(b));
#else
  r = a;
  __asm__("divsd %1, %0" : "+x"(r) : "x"(b));
#endif
  return r;
#else
  return a / b;
#endif
}

// n2 must be > 0 finite
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
__inv_sqrt_exact_s(F n2) noexcept
{
  if constexpr ( sizeof(F) == sizeof(f32) )
    return F(__div_exact_ss(1.0f, hw::sqrt_ss(f32(n2))));
  else
    return F(__div_exact_sd(1.0, hw::sqrt_sd(f64(n2))));
}

[[nodiscard, gnu::always_inline]] inline constexpr f32
__inv_sqrt_fast_s(f32 n2) noexcept
{
  // estimate + one NR step, ~2^-22 relative (consteval: exact)
  f32 r = hw::rsqrt_approx_ss(n2);
  f32 h = n2 * 0.5f;
  return r * (1.5f - h * r * r);
}

#if defined(__micron_gfx_simd)

#if defined(__micron_arch_x86_any)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// amd64 / i386+SSE2

[[nodiscard, gnu::always_inline]] inline simd::f128
__load(const float *p) noexcept
{
  return simd::sse::loadu_f32(p);
}

[[gnu::always_inline]] inline void
__store(float *p, simd::f128 v) noexcept
{
  simd::sse::storeu_f32(p, v);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__mul(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::mul_f32(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__add(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::add_f32(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__sub(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::sub_f32(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__splat(float v) noexcept
{
  return simd::sse::splat_f32(v);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__setr(float a, float b, float c, float d) noexcept
{
  return simd::sse::setr_f32(a, b, c, d);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__xor(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::xor_f32(a, b);
}

// (~mask) & v
[[nodiscard, gnu::always_inline]] inline simd::f128
__andnot(simd::f128 mask, simd::f128 v) noexcept
{
  return simd::sse::andnot_f32(mask, v);
}

// a*b + c / a*b - c, fused where the ISA provides it
[[nodiscard, gnu::always_inline]] inline simd::f128
__fma(simd::f128 a, simd::f128 b, simd::f128 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fma_f32(a, b, c);
#else
  return simd::sse::add_f32(simd::sse::mul_f32(a, b), c);
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__fms(simd::f128 a, simd::f128 b, simd::f128 c) noexcept
{
#if defined(__micron_x86_fma)
  return simd::fma::fms_f32(a, b, c);
#else
  return simd::sse::sub_f32(simd::sse::mul_f32(a, b), c);
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__mask3(void) noexcept
{
  return simd::sse::cast_i128_to_f32(simd::sse::setr_i32(-1, -1, -1, 0));
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__zero3(simd::f128 p) noexcept
{
  return simd::sse::and_f32(p, __mask3());
}

// (v[X], v[Y], v[Z], v[W])
template<int X, int Y, int Z, int W>
[[nodiscard, gnu::always_inline]] inline simd::f128
__swz(simd::f128 v) noexcept
{
  return simd::sse::shuffle_f32<(X | (Y << 2) | (Z << 4) | (W << 6))>(v, v);
}

// (a[X], a[Y], b[Z], b[W])
template<int X, int Y, int Z, int W>
[[nodiscard, gnu::always_inline]] inline simd::f128
__shuf2(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::shuffle_f32<(X | (Y << 2) | (Z << 4) | (W << 6))>(a, b);
}

// all-lanes splat of p0+p1+p2+p3; lane-0 association is (p0+p1) + (p2+p3)
[[nodiscard, gnu::always_inline]] inline simd::f128
__sum_splat(simd::f128 p) noexcept
{
  const simd::f128 t = simd::sse::add_f32(p, simd::sse::shuffle_f32<0xB1>(p, p));      // pair sums
  return simd::sse::add_f32(t, simd::sse::shuffle_f32<0x4E>(t, t));                    // + swapped halves
}

// all-lanes splat of a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
[[nodiscard, gnu::always_inline]] inline simd::f128
__dot3_splat(simd::f128 a, simd::f128 b) noexcept
{
  return __sum_splat(__zero3(simd::sse::mul_f32(a, b)));
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__dot4_splat(simd::f128 a, simd::f128 b) noexcept
{
  return __sum_splat(simd::sse::mul_f32(a, b));
}

// IEEE divps. _mm_div_ps is a plain a / b, which gcc expands through
// ix86_emit_swdivsf (rcpps + Newton) under RECIP_MASK_VEC_DIV
[[nodiscard, gnu::always_inline]] inline simd::f128
__div_exact(simd::f128 a, simd::f128 b) noexcept
{
  simd::f128 r;
#if defined(__micron_x86_avx)
  __asm__("vdivps %2, %1, %0" : "=x"(r) : "x"(a), "x"(b));
#else
  r = a;
  __asm__("divps %1, %0" : "+x"(r) : "x"(b));
#endif
  return r;
}

// signs / denom, exact
[[nodiscard, gnu::always_inline]] inline simd::f128
__recip_signed(simd::f128 signs, simd::f128 denom) noexcept
{
  return __div_exact(signs, denom);
}

// 1/sqrt of an all-lanes splat, full precision (sqrtps + divps)
[[nodiscard, gnu::always_inline]] inline simd::f128
__inv_sqrt_exact(simd::f128 n2) noexcept
{
  return __div_exact(simd::sse::splat_f32(1.0f), simd::sse::sqrt_f32(n2));
}

// 1/sqrt via rsqrtps + one NR step, ~2^-22 relative
[[nodiscard, gnu::always_inline]] inline simd::f128
__inv_sqrt_fast(simd::f128 n2) noexcept
{
  const simd::f128 r = simd::sse::rsqrt_f32(n2);
  const simd::f128 hr = simd::sse::mul_f32(simd::sse::mul_f32(simd::sse::splat_f32(0.5f), n2), r);
#if defined(__micron_x86_fma)
  const simd::f128 t = simd::fma::fnma_f32(hr, r, simd::sse::splat_f32(1.5f));
#else
  const simd::f128 t = simd::sse::sub_f32(simd::sse::splat_f32(1.5f), simd::sse::mul_f32(hr, r));
#endif
  return simd::sse::mul_f32(r, t);
}

// cross product in lanes 0..2; lane 3 = a3*b3 - a3*b3 (0 for finite padding)
[[nodiscard, gnu::always_inline]] inline simd::f128
__cross3(simd::f128 a, simd::f128 b) noexcept
{
  const simd::f128 ayzx = simd::sse::shuffle_f32<0xC9>(a, a);      // (a1 a2 a0 a3)
  const simd::f128 byzx = simd::sse::shuffle_f32<0xC9>(b, b);
#if defined(__micron_x86_fma)
  const simd::f128 t = simd::fma::fms_f32(a, byzx, simd::sse::mul_f32(ayzx, b));
#else
  const simd::f128 t = simd::sse::sub_f32(simd::sse::mul_f32(a, byzx), simd::sse::mul_f32(ayzx, b));
#endif
  return simd::sse::shuffle_f32<0xC9>(t, t);
}

// (v0 v1 v2 t0)
[[nodiscard, gnu::always_inline]] inline simd::f128
__insert_lane3(simd::f128 v, simd::f128 t) noexcept
{
  const simd::f128 m = simd::sse::shuffle_f32<0x02>(v, t);      // (v2 v0 t0 t0)
  return simd::sse::shuffle_f32<0x84>(v, m);
}

// branchless select: mask ? a : b   (mask lanes all-ones/all-zeros)
[[nodiscard, gnu::always_inline]] inline simd::f128
__select(simd::f128 mask, simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::or_f32(simd::sse::and_f32(mask, a), simd::sse::andnot_f32(mask, b));
}

#elif defined(__micron_arch_arm_any)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// NEON backend, f32

[[nodiscard, gnu::always_inline]] inline simd::f128
__load(const float *p) noexcept
{
  return simd::neon::load_f32(p);
}

[[gnu::always_inline]] inline void
__store(float *p, simd::f128 v) noexcept
{
  simd::neon::store_f32(p, v);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__mul(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::mul(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__add(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::add(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__sub(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::sub(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__splat(float v) noexcept
{
  return simd::neon::splat_f32(v);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__setr(float a, float b, float c, float d) noexcept
{
  const float t[4] = { a, b, c, d };
  return simd::neon::load_f32(t);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__xor(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::reinterpret_f32_from_u32(
      simd::neon::xor_(simd::neon::reinterpret_u32_from_f32(a), simd::neon::reinterpret_u32_from_f32(b)));
}

// (~mask) & v
[[nodiscard, gnu::always_inline]] inline simd::f128
__andnot(simd::f128 mask, simd::f128 v) noexcept
{
  return simd::neon::reinterpret_f32_from_u32(
      simd::neon::and_(simd::neon::not_(simd::neon::reinterpret_u32_from_f32(mask)), simd::neon::reinterpret_u32_from_f32(v)));
}

// a*b + c / a*b - c; neon fma_f32 is accumulator-first (vmla/vfma)
[[nodiscard, gnu::always_inline]] inline simd::f128
__fma(simd::f128 a, simd::f128 b, simd::f128 c) noexcept
{
  return simd::neon::fma_f32(c, a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__fms(simd::f128 a, simd::f128 b, simd::f128 c) noexcept
{
  return simd::neon::sub(simd::neon::mul(a, b), c);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__yzxw(simd::f128 v) noexcept
{
  return __builtin_shufflevector(v, v, 1, 2, 0, 3);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__zero3(simd::f128 p) noexcept
{
  const uint32x4_t m = simd::neon::set_lane_u32<3>(0u, simd::neon::splat_u32(~0u));
  return simd::neon::reinterpret_f32_from_u32(simd::neon::and_(simd::neon::reinterpret_u32_from_f32(p), m));
}

// (v[X], v[Y], v[Z], v[W])
template<int X, int Y, int Z, int W>
[[nodiscard, gnu::always_inline]] inline simd::f128
__swz(simd::f128 v) noexcept
{
  return __builtin_shufflevector(v, v, X, Y, Z, W);
}

// (a[X], a[Y], b[Z], b[W])
template<int X, int Y, int Z, int W>
[[nodiscard, gnu::always_inline]] inline simd::f128
__shuf2(simd::f128 a, simd::f128 b) noexcept
{
  return __builtin_shufflevector(a, b, X, Y, Z + 4, W + 4);
}

// all-lanes splat of p0+p1+p2+p3; lane-0 association is (p0+p1) + (p2+p3)
[[nodiscard, gnu::always_inline]] inline simd::f128
__sum_splat(simd::f128 p) noexcept
{
  const simd::f128 t = simd::neon::add(p, simd::neon::rev64(p));      // pair sums
  return simd::neon::add(t, simd::neon::ext_f32<2>(t, t));            // + swapped halves
}

// all-lanes splat of a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
[[nodiscard, gnu::always_inline]] inline simd::f128
__dot3_splat(simd::f128 a, simd::f128 b) noexcept
{
  return __sum_splat(__zero3(simd::neon::mul(a, b)));
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__dot4_splat(simd::f128 a, simd::f128 b) noexcept
{
  return __sum_splat(simd::neon::mul(a, b));
}

// signs / denom, exact
[[nodiscard, gnu::always_inline]] inline simd::f128
__recip_signed(simd::f128 signs, simd::f128 denom) noexcept
{
#if defined(__micron_arch_arm64)
  return simd::neon::div(signs, denom);
#else
  return simd::neon::mul(signs, simd::neon::splat_f32(__div_exact_ss(1.0f, simd::neon::get_lane_f32<0>(denom))));
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__inv_sqrt_exact(simd::f128 n2) noexcept
{
#if defined(__micron_arch_arm64)
  return simd::neon::div(simd::neon::splat_f32(1.0f), simd::neon::sqrt(n2));
#else
  // armv7 has no vector sqrt/div: exact via scalar VFP on the splat lane
  return simd::neon::splat_f32(__div_exact_ss(1.0f, hw::sqrt_ss(simd::neon::get_lane_f32<0>(n2))));
#endif
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__inv_sqrt_fast(simd::f128 n2) noexcept
{
  // vrsqrte is only ~8 bits; two vrsqrts rounds land ~2^-22, matching x86
  simd::f128 r = simd::neon::rsqrt_est(n2);
  r = simd::neon::mul(simd::neon::rsqrt_step(simd::neon::mul(n2, r), r), r);
  r = simd::neon::mul(simd::neon::rsqrt_step(simd::neon::mul(n2, r), r), r);
  return r;
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__cross3(simd::f128 a, simd::f128 b) noexcept
{
  const simd::f128 ayzx = __yzxw(a);
  const simd::f128 byzx = __yzxw(b);
  // neon fms_f32 is accumulator-first: acc - a*b
  const simd::f128 t = simd::neon::fms_f32(simd::neon::mul(a, byzx), ayzx, b);
  return __yzxw(t);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__insert_lane3(simd::f128 v, simd::f128 t) noexcept
{
  return __builtin_shufflevector(v, t, 0, 1, 2, 4);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__select(simd::f128 mask, simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::select(simd::neon::reinterpret_u32_from_f32(mask), a, b);
}

// comparison results as f32-typed masks, mirroring the SSE cmpps convention
[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_eq(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::reinterpret_f32_from_u32(simd::neon::eq(a, b));
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_le(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::reinterpret_f32_from_u32(simd::neon::le(a, b));
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_gt(simd::f128 a, simd::f128 b) noexcept
{
  return simd::neon::reinterpret_f32_from_u32(simd::neon::gt(a, b));
}

#endif

#if defined(__micron_arch_x86_any)
// x86 spellings of the mask-producing compares
[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_eq(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::eq_f32(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_le(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::le_f32(a, b);
}

[[nodiscard, gnu::always_inline]] inline simd::f128
__cmp_gt(simd::f128 a, simd::f128 b) noexcept
{
  return simd::sse::gt_f32(a, b);
}
#endif

#endif      // __micron_gfx_simd

};      // namespace __vsimd
};      // namespace math
};      // namespace micron

__micron_pop_options
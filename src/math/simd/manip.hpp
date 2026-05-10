//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/round.hpp"
#include "_dispatch.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wignored-attributes"

namespace micron
{
namespace math
{
namespace mk
{

#if defined(__micron_x86_avx2)

[[gnu::always_inline]] inline simd::d256
fabs(simd::d256 x) noexcept
{
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), x);
}

[[gnu::always_inline]] inline simd::f256
fabs(simd::f256 x) noexcept
{
  return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), x);
}

[[gnu::always_inline]] inline simd::d256
fneg(simd::d256 x) noexcept
{
  return _mm256_xor_pd(x, _mm256_set1_pd(-0.0));
}

[[gnu::always_inline]] inline simd::f256
fneg(simd::f256 x) noexcept
{
  return _mm256_xor_ps(x, _mm256_set1_ps(-0.0f));
}

[[gnu::always_inline]] inline simd::d256
copysign(simd::d256 mag, simd::d256 sgn) noexcept
{
  const simd::d256 sm = _mm256_set1_pd(-0.0);
  return _mm256_or_pd(_mm256_andnot_pd(sm, mag), _mm256_and_pd(sm, sgn));
}

[[gnu::always_inline]] inline simd::f256
copysign(simd::f256 mag, simd::f256 sgn) noexcept
{
  const simd::f256 sm = _mm256_set1_ps(-0.0f);
  return _mm256_or_ps(_mm256_andnot_ps(sm, mag), _mm256_and_ps(sm, sgn));
}

[[gnu::always_inline]] inline simd::d256
floor(simd::d256 x) noexcept
{
  return _mm256_round_pd(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f256
floor(simd::f256 x) noexcept
{
  return _mm256_round_ps(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d256
ceil(simd::d256 x) noexcept
{
  return _mm256_round_pd(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f256
ceil(simd::f256 x) noexcept
{
  return _mm256_round_ps(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d256
trunc(simd::d256 x) noexcept
{
  return _mm256_round_pd(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f256
trunc(simd::f256 x) noexcept
{
  return _mm256_round_ps(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d256
rint(simd::d256 x) noexcept
{
  return _mm256_round_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f256
rint(simd::f256 x) noexcept
{
  return _mm256_round_ps(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d128
fabs(simd::d128 x) noexcept
{
  return _mm_andnot_pd(_mm_set1_pd(-0.0), x);
}

[[gnu::always_inline]] inline simd::f128
fabs(simd::f128 x) noexcept
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), x);
}

[[gnu::always_inline]] inline simd::d128
floor(simd::d128 x) noexcept
{
  return _mm_round_pd(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return _mm_round_ps(x, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d128
ceil(simd::d128 x) noexcept
{
  return _mm_round_pd(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return _mm_round_ps(x, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d128
trunc(simd::d128 x) noexcept
{
  return _mm_round_pd(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return _mm_round_ps(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d128
rint(simd::d128 x) noexcept
{
  return _mm_round_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return _mm_round_ps(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::always_inline]] inline simd::f128
fabs(simd::f128 x) noexcept
{
  return vabsq_f32(x);
}

[[gnu::always_inline]] inline simd::f128
fneg(simd::f128 x) noexcept
{
  return vnegq_f32(x);
}

[[gnu::always_inline]] inline simd::f128
copysign(simd::f128 mag, simd::f128 sgn) noexcept
{
  const uint32x4_t sm = vdupq_n_u32(0x80000000u);
  const uint32x4_t mb = vbicq_u32(vreinterpretq_u32_f32(mag), sm);
  const uint32x4_t sb = vandq_u32(vreinterpretq_u32_f32(sgn), sm);
  return vreinterpretq_f32_u32(vorrq_u32(mb, sb));
}

#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
fabs(simd::d128 x) noexcept
{
  return vabsq_f64(x);
}

[[gnu::always_inline]] inline simd::d128
fneg(simd::d128 x) noexcept
{
  return vnegq_f64(x);
}

[[gnu::always_inline]] inline simd::d128
copysign(simd::d128 mag, simd::d128 sgn) noexcept
{
  const uint64x2_t sm = vdupq_n_u64(0x8000000000000000ULL);
  const uint64x2_t mb = vbicq_u64(vreinterpretq_u64_f64(mag), sm);
  const uint64x2_t sb = vandq_u64(vreinterpretq_u64_f64(sgn), sm);
  return vreinterpretq_f64_u64(vorrq_u64(mb, sb));
}
#endif

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return vrndmq_f32(x);
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return vrndpq_f32(x);
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return vrndq_f32(x);
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return vrndnq_f32(x);
}
#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
floor(simd::d128 x) noexcept
{
  return vrndmq_f64(x);
}

[[gnu::always_inline]] inline simd::d128
ceil(simd::d128 x) noexcept
{
  return vrndpq_f64(x);
}

[[gnu::always_inline]] inline simd::d128
trunc(simd::d128 x) noexcept
{
  return vrndq_f64(x);
}

[[gnu::always_inline]] inline simd::d128
rint(simd::d128 x) noexcept
{
  return vrndnq_f64(x);
}
#endif
#else

[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::floor<f32>(vgetq_lane_f32(x, 0)),
    mkbits::round_ns::floor<f32>(vgetq_lane_f32(x, 1)),
    mkbits::round_ns::floor<f32>(vgetq_lane_f32(x, 2)),
    mkbits::round_ns::floor<f32>(vgetq_lane_f32(x, 3)),
  };
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::ceil<f32>(vgetq_lane_f32(x, 0)),
    mkbits::round_ns::ceil<f32>(vgetq_lane_f32(x, 1)),
    mkbits::round_ns::ceil<f32>(vgetq_lane_f32(x, 2)),
    mkbits::round_ns::ceil<f32>(vgetq_lane_f32(x, 3)),
  };
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::trunc<f32>(vgetq_lane_f32(x, 0)),
    mkbits::round_ns::trunc<f32>(vgetq_lane_f32(x, 1)),
    mkbits::round_ns::trunc<f32>(vgetq_lane_f32(x, 2)),
    mkbits::round_ns::trunc<f32>(vgetq_lane_f32(x, 3)),
  };
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::nearbyint<f32>(vgetq_lane_f32(x, 0)),
    mkbits::round_ns::nearbyint<f32>(vgetq_lane_f32(x, 1)),
    mkbits::round_ns::nearbyint<f32>(vgetq_lane_f32(x, 2)),
    mkbits::round_ns::nearbyint<f32>(vgetq_lane_f32(x, 3)),
  };
}
#endif

#endif

#if defined(__micron_x86_avx512f)
[[gnu::always_inline]] inline simd::d512
fabs(simd::d512 x) noexcept
{
  return _mm512_abs_pd(x);
}

[[gnu::always_inline]] inline simd::f512
fabs(simd::f512 x) noexcept
{
  return _mm512_abs_ps(x);
}

[[gnu::always_inline]] inline simd::d512
floor(simd::d512 x) noexcept
{
  return _mm512_floor_pd(x);
}

[[gnu::always_inline]] inline simd::f512
floor(simd::f512 x) noexcept
{
  return _mm512_floor_ps(x);
}

[[gnu::always_inline]] inline simd::d512
ceil(simd::d512 x) noexcept
{
  return _mm512_ceil_pd(x);
}

[[gnu::always_inline]] inline simd::f512
ceil(simd::f512 x) noexcept
{
  return _mm512_ceil_ps(x);
}

[[gnu::always_inline]] inline simd::d512
trunc(simd::d512 x) noexcept
{
  return _mm512_roundscale_pd(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f512
trunc(simd::f512 x) noexcept
{
  return _mm512_roundscale_ps(x, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::d512
rint(simd::d512 x) noexcept
{
  return _mm512_roundscale_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}

[[gnu::always_inline]] inline simd::f512
rint(simd::f512 x) noexcept
{
  return _mm512_roundscale_ps(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}
#endif

};     // namespace mk
};     // namespace math
};     // namespace micron

#pragma GCC diagnostic pop

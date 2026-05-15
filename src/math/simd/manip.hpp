//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/aliases.hpp"
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
  return simd::avx::andnot_f64(simd::avx::splat_f64(-0.0), x);
}

[[gnu::always_inline]] inline simd::f256
fabs(simd::f256 x) noexcept
{
  return simd::avx::andnot_f32(simd::avx::splat_f32(-0.0f), x);
}

[[gnu::always_inline]] inline simd::d256
fneg(simd::d256 x) noexcept
{
  return simd::avx::xor_f64(x, simd::avx::splat_f64(-0.0));
}

[[gnu::always_inline]] inline simd::f256
fneg(simd::f256 x) noexcept
{
  return simd::avx::xor_f32(x, simd::avx::splat_f32(-0.0f));
}

[[gnu::always_inline]] inline simd::d256
copysign(simd::d256 mag, simd::d256 sgn) noexcept
{
  const simd::d256 sm = simd::avx::splat_f64(-0.0);
  return simd::avx::or_f64(simd::avx::andnot_f64(sm, mag), simd::avx::and_f64(sm, sgn));
}

[[gnu::always_inline]] inline simd::f256
copysign(simd::f256 mag, simd::f256 sgn) noexcept
{
  const simd::f256 sm = simd::avx::splat_f32(-0.0f);
  return simd::avx::or_f32(simd::avx::andnot_f32(sm, mag), simd::avx::and_f32(sm, sgn));
}

[[gnu::always_inline]] inline simd::d256
floor(simd::d256 x) noexcept
{
  return simd::avx::round_f64<_MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f256
floor(simd::f256 x) noexcept
{
  return simd::avx::round_f32<_MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d256
ceil(simd::d256 x) noexcept
{
  return simd::avx::round_f64<_MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f256
ceil(simd::f256 x) noexcept
{
  return simd::avx::round_f32<_MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d256
trunc(simd::d256 x) noexcept
{
  return simd::avx::round_f64<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f256
trunc(simd::f256 x) noexcept
{
  return simd::avx::round_f32<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d256
rint(simd::d256 x) noexcept
{
  return simd::avx::round_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f256
rint(simd::f256 x) noexcept
{
  return simd::avx::round_f32<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d128
fabs(simd::d128 x) noexcept
{
  return simd::sse::andnot_f64(simd::sse::splat_f64(-0.0), x);
}

[[gnu::always_inline]] inline simd::f128
fabs(simd::f128 x) noexcept
{
  return simd::sse::andnot_f32(simd::sse::splat_f32(-0.0f), x);
}

[[gnu::always_inline]] inline simd::d128
floor(simd::d128 x) noexcept
{
  return simd::sse::round_f64<_MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return simd::sse::round_f32<_MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d128
ceil(simd::d128 x) noexcept
{
  return simd::sse::round_f64<_MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return simd::sse::round_f32<_MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d128
trunc(simd::d128 x) noexcept
{
  return simd::sse::round_f64<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return simd::sse::round_f32<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d128
rint(simd::d128 x) noexcept
{
  return simd::sse::round_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return simd::sse::round_f32<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::always_inline]] inline simd::f128
fabs(simd::f128 x) noexcept
{
  return simd::neon::abs(x);
}

[[gnu::always_inline]] inline simd::f128
fneg(simd::f128 x) noexcept
{
  return simd::neon::neg(x);
}

[[gnu::always_inline]] inline simd::f128
copysign(simd::f128 mag, simd::f128 sgn) noexcept
{
  const uint32x4_t sm = simd::neon::splat_u32(0x80000000u);
  const uint32x4_t mb = simd::neon::bic(simd::neon::reinterpret_u32_from_f32(mag), sm);
  const uint32x4_t sb = simd::neon::and_(simd::neon::reinterpret_u32_from_f32(sgn), sm);
  return simd::neon::reinterpret_f32_from_u32(simd::neon::or_(mb, sb));
}

#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
fabs(simd::d128 x) noexcept
{
  return simd::neon::abs(x);
}

[[gnu::always_inline]] inline simd::d128
fneg(simd::d128 x) noexcept
{
  return simd::neon::neg(x);
}

[[gnu::always_inline]] inline simd::d128
copysign(simd::d128 mag, simd::d128 sgn) noexcept
{
  const uint64x2_t sm = simd::neon::splat_u64(0x8000000000000000ULL);
  const uint64x2_t mb = simd::neon::bic(simd::neon::reinterpret_u64_from_f64(mag), sm);
  const uint64x2_t sb = simd::neon::and_(simd::neon::reinterpret_u64_from_f64(sgn), sm);
  return simd::neon::reinterpret_f64_from_u64(simd::neon::or_(mb, sb));
}
#endif

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return simd::neon::floor(x);
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return simd::neon::ceil(x);
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return simd::neon::trunc(x);
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return simd::neon::rint(x);
}
#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
floor(simd::d128 x) noexcept
{
  return simd::neon::floor(x);
}

[[gnu::always_inline]] inline simd::d128
ceil(simd::d128 x) noexcept
{
  return simd::neon::ceil(x);
}

[[gnu::always_inline]] inline simd::d128
trunc(simd::d128 x) noexcept
{
  return simd::neon::trunc(x);
}

[[gnu::always_inline]] inline simd::d128
rint(simd::d128 x) noexcept
{
  return simd::neon::rint(x);
}
#endif
#else

[[gnu::always_inline]] inline simd::f128
floor(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::floor<f32>(simd::neon::get_lane_f32<0>(x)),
    mkbits::round_ns::floor<f32>(simd::neon::get_lane_f32<1>(x)),
    mkbits::round_ns::floor<f32>(simd::neon::get_lane_f32<2>(x)),
    mkbits::round_ns::floor<f32>(simd::neon::get_lane_f32<3>(x)),
  };
}

[[gnu::always_inline]] inline simd::f128
ceil(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::ceil<f32>(simd::neon::get_lane_f32<0>(x)),
    mkbits::round_ns::ceil<f32>(simd::neon::get_lane_f32<1>(x)),
    mkbits::round_ns::ceil<f32>(simd::neon::get_lane_f32<2>(x)),
    mkbits::round_ns::ceil<f32>(simd::neon::get_lane_f32<3>(x)),
  };
}

[[gnu::always_inline]] inline simd::f128
trunc(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::trunc<f32>(simd::neon::get_lane_f32<0>(x)),
    mkbits::round_ns::trunc<f32>(simd::neon::get_lane_f32<1>(x)),
    mkbits::round_ns::trunc<f32>(simd::neon::get_lane_f32<2>(x)),
    mkbits::round_ns::trunc<f32>(simd::neon::get_lane_f32<3>(x)),
  };
}

[[gnu::always_inline]] inline simd::f128
rint(simd::f128 x) noexcept
{
  return (simd::f128){
    mkbits::round_ns::nearbyint<f32>(simd::neon::get_lane_f32<0>(x)),
    mkbits::round_ns::nearbyint<f32>(simd::neon::get_lane_f32<1>(x)),
    mkbits::round_ns::nearbyint<f32>(simd::neon::get_lane_f32<2>(x)),
    mkbits::round_ns::nearbyint<f32>(simd::neon::get_lane_f32<3>(x)),
  };
}
#endif

#endif

#if defined(__micron_x86_avx512f)
[[gnu::always_inline]] inline simd::d512
fabs(simd::d512 x) noexcept
{
  return simd::avx512::abs_f64(x);
}

[[gnu::always_inline]] inline simd::f512
fabs(simd::f512 x) noexcept
{
  return simd::avx512::abs_f32(x);
}

[[gnu::always_inline]] inline simd::d512
floor(simd::d512 x) noexcept
{
  return simd::avx512::floor_f64(x);
}

[[gnu::always_inline]] inline simd::f512
floor(simd::f512 x) noexcept
{
  return simd::avx512::floor_f32(x);
}

[[gnu::always_inline]] inline simd::d512
ceil(simd::d512 x) noexcept
{
  return simd::avx512::ceil_f64(x);
}

[[gnu::always_inline]] inline simd::f512
ceil(simd::f512 x) noexcept
{
  return simd::avx512::ceil_f32(x);
}

[[gnu::always_inline]] inline simd::d512
trunc(simd::d512 x) noexcept
{
  return simd::avx512::roundscale_f64<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f512
trunc(simd::f512 x) noexcept
{
  return simd::avx512::roundscale_f32<_MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::d512
rint(simd::d512 x) noexcept
{
  return simd::avx512::roundscale_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}

[[gnu::always_inline]] inline simd::f512
rint(simd::f512 x) noexcept
{
  return simd::avx512::roundscale_f32<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(x);
}
#endif

};      // namespace mk
};      // namespace math
};      // namespace micron

#pragma GCC diagnostic pop

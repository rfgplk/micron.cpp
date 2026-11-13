// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../math/bits/sqrt.hpp"
#include "../../../math/generic.hpp"
#include "types.hpp"

namespace micron::simd
{
template<typename B>
concept __wasm_real_packet = is_simd_type<B> && micron::is_floating_point_v<typename __wasm_packet_traits<B>::element>;

template<typename F, is_simd_type B, typename Fn>
inline B
__wasm_math_map(B value, Fn fn) noexcept
{
  auto a = __wasm_unpack<F>(value);
  for ( auto &x : a.lane ) x = fn(x);
  return __wasm_pack<B>(a);
}

template<typename F, is_simd_type B, typename Fn>
inline B
__wasm_math_zip(B a, B b, Fn fn) noexcept
{
  auto av = __wasm_unpack<F>(a);
  const auto bv = __wasm_unpack<F>(b);
  for ( usize i = 0; i < sizeof(B) / sizeof(F); ++i ) av.lane[i] = fn(av.lane[i], bv.lane[i]);
  return __wasm_pack<B>(av);
}

template<typename F, is_simd_type B, typename M>
inline B
__wasm_math_mask(B src, M mask, B value) noexcept
{
  auto out = __wasm_unpack<F>(src);
  const auto v = __wasm_unpack<F>(value);
  for ( usize i = 0; i < sizeof(B) / sizeof(F); ++i )
    if ( (u64(mask) >> i) & 1 ) out.lane[i] = v.lane[i];
  return __wasm_pack<B>(out);
}

#define __MC_WASM_MATH1(NAME, BODY)                                                                                                        \
  template<__wasm_real_packet B> inline B NAME(B value) noexcept                                                                           \
  {                                                                                                                                        \
    using F = typename __wasm_packet_traits<B>::element;                                                                                   \
    return __wasm_math_map<F>(value, [](F x) { return BODY; });                                                                            \
  }                                                                                                                                        \
  template<__wasm_real_packet B, typename M> inline B mask_##NAME(B src, M k, B value) noexcept                                            \
  {                                                                                                                                        \
    return __wasm_math_mask<typename __wasm_packet_traits<B>::element>(src, k, NAME(value));                                               \
  }                                                                                                                                        \
  template<__wasm_real_packet B, typename M> inline B maskz_##NAME(M k, B value) noexcept                                                  \
  {                                                                                                                                        \
    return __wasm_math_mask<typename __wasm_packet_traits<B>::element>(B{}, k, NAME(value));                                               \
  }
__MC_WASM_MATH1(sqrt, math::mkbits::sqrt_ns::sqrt<F>(x))
__MC_WASM_MATH1(cbrt, math::mkbits::sqrt_ns::cbrt<F>(x))
__MC_WASM_MATH1(invsqrt, F(1) / math::mkbits::sqrt_ns::sqrt<F>(x))
__MC_WASM_MATH1(invcbrt, F(1) / math::mkbits::sqrt_ns::cbrt<F>(x))
__MC_WASM_MATH1(rcp14, F(1) / x)
__MC_WASM_MATH1(rsqrt14, F(1) / math::mkbits::sqrt_ns::sqrt<F>(x))
__MC_WASM_MATH1(exp, math::mkbits::exp_ns::exp<F>(x))
__MC_WASM_MATH1(exp2, math::mkbits::exp_ns::exp2<F>(x))
__MC_WASM_MATH1(exp10, math::mkbits::exp_ns::exp10<F>(x))
__MC_WASM_MATH1(expm1, math::mkbits::exp_ns::expm1<F>(x))
__MC_WASM_MATH1(log, math::mkbits::log_ns::log<F>(x))
__MC_WASM_MATH1(log2, math::mkbits::log_ns::log2<F>(x))
__MC_WASM_MATH1(log10, math::mkbits::log_ns::log10<F>(x))
__MC_WASM_MATH1(log1p, math::mkbits::log_ns::log1p<F>(x))
__MC_WASM_MATH1(logb, math::mkbits::manip::logb<F>(x))
__MC_WASM_MATH1(ceil, math::mkbits::round_ns::ceil<F>(x))
__MC_WASM_MATH1(floor, math::mkbits::round_ns::floor<F>(x))
__MC_WASM_MATH1(trunc, math::mkbits::round_ns::trunc<F>(x))
__MC_WASM_MATH1(rint, math::mkbits::round_ns::rint<F>(x))
__MC_WASM_MATH1(nearbyint, math::mkbits::round_ns::rint<F>(x))
__MC_WASM_MATH1(svml_sqrt, math::mkbits::sqrt_ns::sqrt<F>(x))
__MC_WASM_MATH1(svml_ceil, math::mkbits::round_ns::ceil<F>(x))
__MC_WASM_MATH1(svml_floor, math::mkbits::round_ns::floor<F>(x))
__MC_WASM_MATH1(svml_round, math::mkbits::round_ns::round<F>(x))
#undef __MC_WASM_MATH1
#define __MC_WASM_MATH2(NAME, BODY)                                                                                                        \
  template<__wasm_real_packet B> inline B NAME(B a, B b) noexcept                                                                          \
  {                                                                                                                                        \
    using F = typename __wasm_packet_traits<B>::element;                                                                                   \
    return __wasm_math_zip<F>(a, b, [](F x, F y) { return BODY; });                                                                        \
  }                                                                                                                                        \
  template<__wasm_real_packet B, typename M> inline B mask_##NAME(B src, M k, B a, B b) noexcept                                           \
  {                                                                                                                                        \
    return __wasm_math_mask<typename __wasm_packet_traits<B>::element>(src, k, NAME(a, b));                                                \
  }
__MC_WASM_MATH2(min, x < y ? x : y)
__MC_WASM_MATH2(max, x > y ? x : y)
__MC_WASM_MATH2(pow, math::mkbits::pow_ns::pow<F>(x, y))
__MC_WASM_MATH2(hypot, math::mkbits::sqrt_ns::hypot<F>(x, y))
#undef __MC_WASM_MATH2
#define __MC_WASM_INT_MATH(F, N)                                                                                                           \
  template<is_simd_type B> inline B min_##F##N(B a, B b) noexcept                                                                          \
  {                                                                                                                                        \
    return __wasm_math_zip<F##N>(a, b, [](F##N x, F##N y) { return x < y ? x : y; });                                                      \
  }                                                                                                                                        \
  template<is_simd_type B> inline B max_##F##N(B a, B b) noexcept                                                                          \
  {                                                                                                                                        \
    return __wasm_math_zip<F##N>(a, b, [](F##N x, F##N y) { return x > y ? x : y; });                                                      \
  }                                                                                                                                        \
  template<is_simd_type B> inline F##N reduce_min_##F##N(B a) noexcept                                                                     \
  {                                                                                                                                        \
    auto v = __wasm_unpack<F##N>(a);                                                                                                       \
    F##N r = v.lane[0];                                                                                                                    \
    for ( auto x : v.lane )                                                                                                                \
      if ( x < r ) r = x;                                                                                                                  \
    return r;                                                                                                                              \
  }                                                                                                                                        \
  template<is_simd_type B> inline F##N reduce_max_##F##N(B a) noexcept                                                                     \
  {                                                                                                                                        \
    auto v = __wasm_unpack<F##N>(a);                                                                                                       \
    F##N r = v.lane[0];                                                                                                                    \
    for ( auto x : v.lane )                                                                                                                \
      if ( x > r ) r = x;                                                                                                                  \
    return r;                                                                                                                              \
  }
__MC_WASM_INT_MATH(i, 8)
__MC_WASM_INT_MATH(i, 16)
__MC_WASM_INT_MATH(i, 32)
__MC_WASM_INT_MATH(i, 64)
__MC_WASM_INT_MATH(u, 8)
__MC_WASM_INT_MATH(u, 16)
__MC_WASM_INT_MATH(u, 32)
__MC_WASM_INT_MATH(u, 64)
#undef __MC_WASM_INT_MATH
#define __MC_WASM_ABS(N)                                                                                                                   \
  template<is_simd_type B> inline B abs_##N(B a) noexcept                                                                                  \
  {                                                                                                                                        \
    return __wasm_math_map<i##N>(a, [](i##N x) { return x < 0 ? i##N(u##N(0) - u##N(x)) : x; });                                           \
  }                                                                                                                                        \
  template<is_simd_type B, typename M> inline B mask_abs_##N(B src, M k, B a) noexcept                                                     \
  {                                                                                                                                        \
    return __wasm_math_mask<i##N>(src, k, abs_##N(a));                                                                                     \
  }                                                                                                                                        \
  template<is_simd_type B, typename M> inline B maskz_abs_##N(M k, B a) noexcept { return __wasm_math_mask<i##N>(B{}, k, abs_##N(a)); }
__MC_WASM_ABS(8)
__MC_WASM_ABS(16)
__MC_WASM_ABS(32)
__MC_WASM_ABS(64)
#undef __MC_WASM_ABS

template<is_simd_type B>
inline B
abs(B a) noexcept
{
  return abs_32(a);
}

template<__wasm_real_packet B>
inline B
round(B a, int mode) noexcept
{
  switch ( mode & 7 ) {
  case 1:
    return floor(a);
  case 2:
    return ceil(a);
  case 3:
    return trunc(a);
  default:
    return rint(a);
  }
}

template<__wasm_real_packet B>
inline B
sqrt_round(B a, int) noexcept
{
  return sqrt(a);
}

template<__wasm_real_packet B>
inline B
min_round(B a, B b, int) noexcept
{
  return min(a, b);
}

template<__wasm_real_packet B>
inline B
max_round(B a, B b, int) noexcept
{
  return max(a, b);
}

inline f128
rcp_ps(f128 a) noexcept
{
  return rcp14(a);
}

inline f128
rsqrt_ps(f128 a) noexcept
{
  return rsqrt14(a);
}
};      // namespace micron::simd

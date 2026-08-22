//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__vector_types_arm64.hpp"

#if !defined(__clang__) || !defined(__micron_arch_arm64)
#error "__clang_compat_arm64.hpp requires Clang targeting AArch64"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_absdiff(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = a[i] < b[i] ? b[i] - a[i] : a[i] - b[i];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_half_add(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = (static_cast<long long>(a[i]) + static_cast<long long>(b[i])) >> 1;
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_round_half_add(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = (static_cast<unsigned long long>(a[i]) + static_cast<unsigned long long>(b[i]) + 1) >> 1;
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_round_half_add_signed(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = (static_cast<long long>(a[i]) + static_cast<long long>(b[i]) + 1) >> 1;
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_half_sub(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = (static_cast<long long>(a[i]) - static_cast<long long>(b[i])) >> 1;
  return out;
}

template<typename Out, bool High, typename V>
[[gnu::always_inline]] static inline Out
__clang_arm_widen_absdiff(V a, V b) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  const unsigned base = High ? n : 0;
  for ( unsigned i = 0; i < n; ++i ) {
    long long av = a[base + i];
    long long bv = b[base + i];
    out[i] = av < bv ? bv - av : av - bv;
  }
  return out;
}

template<typename Acc, typename V>
[[gnu::always_inline]] static inline Acc
__clang_arm_widen_acc_absdiff(Acc acc, V a, V b) noexcept
{
  constexpr unsigned n = sizeof(Acc) / sizeof(acc[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    long long av = a[i];
    long long bv = b[i];
    acc[i] += av < bv ? bv - av : av - bv;
  }
  return acc;
}

template<typename Out, typename V>
[[gnu::always_inline]] static inline Out
__clang_arm_narrow_sat(V v, long long low, unsigned long long high) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    long long value = v[i];
    if ( value < low ) value = low;
    if ( value > static_cast<long long>(high) ) value = high;
    out[i] = value;
  }
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline auto
__clang_arm_reduce_add(V v) noexcept
{
  auto value = v[0];
  constexpr unsigned n = sizeof(V) / sizeof(v[0]);
  for ( unsigned i = 1; i < n; ++i ) value += v[i];
  return value;
}

template<typename V>
[[gnu::always_inline]] static inline auto
__clang_arm_reduce_min(V v) noexcept
{
  auto value = v[0];
  constexpr unsigned n = sizeof(V) / sizeof(v[0]);
  for ( unsigned i = 1; i < n; ++i ) value = value < v[i] ? value : v[i];
  return value;
}

template<typename V>
[[gnu::always_inline]] static inline auto
__clang_arm_reduce_max(V v) noexcept
{
  auto value = v[0];
  constexpr unsigned n = sizeof(V) / sizeof(v[0]);
  for ( unsigned i = 1; i < n; ++i ) value = value > v[i] ? value : v[i];
  return value;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_recip(V v) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(v[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = 1 / v[i];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_rsqrt(V v) noexcept
{
  return __clang_arm_recip(__builtin_elementwise_sqrt(v));
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_arm_tbl(V table, V indexes) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(table[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    const unsigned index = static_cast<unsigned char>(indexes[i]);
    out[i] = index < n ? table[index] : 0;
  }
  return out;
}

#define __mc_arm_unary(name, expression)                                                                                                   \
  template<typename V> [[gnu::always_inline]] static inline V name(V a) noexcept { return (expression); }
#define __mc_arm_binary(name, expression)                                                                                                  \
  template<typename V> [[gnu::always_inline]] static inline V name(V a, V b) noexcept { return (expression); }
#define __mc_arm_ternary(name, expression)                                                                                                 \
  template<typename V> [[gnu::always_inline]] static inline V name(V acc, V a, V b) noexcept { return (expression); }

__mc_arm_unary(__builtin_aarch64_absv16qi, __builtin_elementwise_abs(a))
__mc_arm_unary(__builtin_aarch64_absv8hi, __builtin_elementwise_abs(a))
__mc_arm_unary(__builtin_aarch64_absv4si, __builtin_elementwise_abs(a))
__mc_arm_unary(__builtin_aarch64_absv2di, __builtin_elementwise_abs(a))
__mc_arm_unary(__builtin_aarch64_absv4sf, __builtin_elementwise_abs(a))
__mc_arm_unary(__builtin_aarch64_absv2df, __builtin_elementwise_abs(a))

__mc_arm_binary(__builtin_aarch64_sminv16qi, __builtin_elementwise_min(a, b))
__mc_arm_binary(__builtin_aarch64_sminv8hi, __builtin_elementwise_min(a, b))
__mc_arm_binary(__builtin_aarch64_sminv4si, __builtin_elementwise_min(a, b))
__mc_arm_binary(__builtin_aarch64_smaxv16qi, __builtin_elementwise_max(a, b))
__mc_arm_binary(__builtin_aarch64_smaxv8hi, __builtin_elementwise_max(a, b))
__mc_arm_binary(__builtin_aarch64_smaxv4si, __builtin_elementwise_max(a, b))
__mc_arm_binary(__builtin_aarch64_fminv4sf, __builtin_elementwise_min(a, b))
__mc_arm_binary(__builtin_aarch64_fminv2df, __builtin_elementwise_min(a, b))
__mc_arm_binary(__builtin_aarch64_fmaxv4sf, __builtin_elementwise_max(a, b))
__mc_arm_binary(__builtin_aarch64_fmaxv2df, __builtin_elementwise_max(a, b))

[[gnu::always_inline]] static inline __Int8x16_t
__builtin_aarch64_uminv16qi(__Int8x16_t a, __Int8x16_t b) noexcept
{
  return (__Int8x16_t)__builtin_elementwise_min((uint8x16_t)a, (uint8x16_t)b);
}

[[gnu::always_inline]] static inline __Int16x8_t
__builtin_aarch64_uminv8hi(__Int16x8_t a, __Int16x8_t b) noexcept
{
  return (__Int16x8_t)__builtin_elementwise_min((uint16x8_t)a, (uint16x8_t)b);
}

[[gnu::always_inline]] static inline __Int32x4_t
__builtin_aarch64_uminv4si(__Int32x4_t a, __Int32x4_t b) noexcept
{
  return (__Int32x4_t)__builtin_elementwise_min((uint32x4_t)a, (uint32x4_t)b);
}

[[gnu::always_inline]] static inline __Int8x16_t
__builtin_aarch64_umaxv16qi(__Int8x16_t a, __Int8x16_t b) noexcept
{
  return (__Int8x16_t)__builtin_elementwise_max((uint8x16_t)a, (uint8x16_t)b);
}

[[gnu::always_inline]] static inline __Int16x8_t
__builtin_aarch64_umaxv8hi(__Int16x8_t a, __Int16x8_t b) noexcept
{
  return (__Int16x8_t)__builtin_elementwise_max((uint16x8_t)a, (uint16x8_t)b);
}

[[gnu::always_inline]] static inline __Int32x4_t
__builtin_aarch64_umaxv4si(__Int32x4_t a, __Int32x4_t b) noexcept
{
  return (__Int32x4_t)__builtin_elementwise_max((uint32x4_t)a, (uint32x4_t)b);
}

#define __mc_arm_absdiff(name) __mc_arm_binary(name, __clang_arm_absdiff(a, b))
__mc_arm_absdiff(__builtin_aarch64_sabdv16qi)
__mc_arm_absdiff(__builtin_aarch64_sabdv8hi)
__mc_arm_absdiff(__builtin_aarch64_sabdv4si)
__mc_arm_absdiff(__builtin_aarch64_sabdv8qi)
__mc_arm_absdiff(__builtin_aarch64_sabdv4hi)
__mc_arm_absdiff(__builtin_aarch64_sabdv2si)
__mc_arm_absdiff(__builtin_aarch64_uabdv16qi_uuu)
__mc_arm_absdiff(__builtin_aarch64_uabdv8hi_uuu)
__mc_arm_absdiff(__builtin_aarch64_uabdv4si_uuu)
__mc_arm_absdiff(__builtin_aarch64_uabdv8qi_uuu)
__mc_arm_absdiff(__builtin_aarch64_uabdv4hi_uuu)
__mc_arm_absdiff(__builtin_aarch64_uabdv2si_uuu)
__mc_arm_absdiff(__builtin_aarch64_fabdv4sf)
__mc_arm_absdiff(__builtin_aarch64_fabdv2sf)
__mc_arm_absdiff(__builtin_aarch64_fabdv2df)

#define __mc_arm_absacc(name) __mc_arm_ternary(name, acc + __clang_arm_absdiff(a, b))
__mc_arm_absacc(__builtin_aarch64_sabav16qi)
__mc_arm_absacc(__builtin_aarch64_sabav8hi)
__mc_arm_absacc(__builtin_aarch64_sabav4si)
__mc_arm_absacc(__builtin_aarch64_sabav8qi)
__mc_arm_absacc(__builtin_aarch64_sabav4hi)
__mc_arm_absacc(__builtin_aarch64_sabav2si)
__mc_arm_absacc(__builtin_aarch64_uabav16qi_uuuu)
__mc_arm_absacc(__builtin_aarch64_uabav8hi_uuuu)
__mc_arm_absacc(__builtin_aarch64_uabav4si_uuuu)
__mc_arm_absacc(__builtin_aarch64_uabav8qi_uuuu)
__mc_arm_absacc(__builtin_aarch64_uabav4hi_uuuu)
__mc_arm_absacc(__builtin_aarch64_uabav2si_uuuu)

#define __mc_arm_half_add(name) __mc_arm_binary(name, __clang_arm_half_add(a, b))
#define __mc_arm_half_sub(name) __mc_arm_binary(name, __clang_arm_half_sub(a, b))
#define __mc_arm_rhalf_add_signed(name) __mc_arm_binary(name, __clang_arm_round_half_add_signed(a, b))
#define __mc_arm_rhalf_add_unsigned(name) __mc_arm_binary(name, __clang_arm_round_half_add(a, b))
__mc_arm_half_add(__builtin_aarch64_shaddv16qi)
__mc_arm_half_add(__builtin_aarch64_shaddv8hi)
__mc_arm_half_add(__builtin_aarch64_shaddv4si)
__mc_arm_half_add(__builtin_aarch64_uhaddv16qi_uuu)
__mc_arm_half_add(__builtin_aarch64_uhaddv8hi_uuu)
__mc_arm_half_add(__builtin_aarch64_uhaddv4si_uuu)
__mc_arm_half_sub(__builtin_aarch64_shsubv16qi)
__mc_arm_half_sub(__builtin_aarch64_shsubv8hi)
__mc_arm_half_sub(__builtin_aarch64_shsubv4si)
__mc_arm_half_sub(__builtin_aarch64_uhsubv16qi_uuu)
__mc_arm_half_sub(__builtin_aarch64_uhsubv8hi_uuu)
__mc_arm_half_sub(__builtin_aarch64_uhsubv4si_uuu)
__mc_arm_rhalf_add_signed(__builtin_aarch64_srhaddv16qi)
__mc_arm_rhalf_add_signed(__builtin_aarch64_srhaddv8hi)
__mc_arm_rhalf_add_signed(__builtin_aarch64_srhaddv4si)
__mc_arm_rhalf_add_unsigned(__builtin_aarch64_urhaddv16qi_uuu)
__mc_arm_rhalf_add_unsigned(__builtin_aarch64_urhaddv8hi_uuu)
__mc_arm_rhalf_add_unsigned(__builtin_aarch64_urhaddv4si_uuu)

#define __mc_arm_add_sat(name) __mc_arm_binary(name, __builtin_elementwise_add_sat(a, b))
#define __mc_arm_sub_sat(name) __mc_arm_binary(name, __builtin_elementwise_sub_sat(a, b))
__mc_arm_add_sat(__builtin_aarch64_ssaddv16qi)
__mc_arm_add_sat(__builtin_aarch64_ssaddv8hi)
__mc_arm_add_sat(__builtin_aarch64_ssaddv4si)
__mc_arm_add_sat(__builtin_aarch64_ssaddv2di)
__mc_arm_add_sat(__builtin_aarch64_ssaddv8qi)
__mc_arm_add_sat(__builtin_aarch64_ssaddv4hi)
__mc_arm_add_sat(__builtin_aarch64_ssaddv2si)
__mc_arm_add_sat(__builtin_aarch64_usaddv16qi_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv8hi_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv4si_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv2di_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv8qi_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv4hi_uuu)
__mc_arm_add_sat(__builtin_aarch64_usaddv2si_uuu)
__mc_arm_sub_sat(__builtin_aarch64_sssubv16qi)
__mc_arm_sub_sat(__builtin_aarch64_sssubv8hi)
__mc_arm_sub_sat(__builtin_aarch64_sssubv4si)
__mc_arm_sub_sat(__builtin_aarch64_sssubv2di)
__mc_arm_sub_sat(__builtin_aarch64_sssubv8qi)
__mc_arm_sub_sat(__builtin_aarch64_sssubv4hi)
__mc_arm_sub_sat(__builtin_aarch64_sssubv2si)
__mc_arm_sub_sat(__builtin_aarch64_ussubv16qi_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv8hi_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv4si_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv2di_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv8qi_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv4hi_uuu)
__mc_arm_sub_sat(__builtin_aarch64_ussubv2si_uuu)

__mc_arm_unary(__builtin_aarch64_sqrtv4sf, __builtin_elementwise_sqrt(a))
__mc_arm_unary(__builtin_aarch64_sqrtv2sf, __builtin_elementwise_sqrt(a))
__mc_arm_unary(__builtin_aarch64_sqrtv2df, __builtin_elementwise_sqrt(a))
__mc_arm_unary(__builtin_aarch64_floorv4sf, __builtin_elementwise_floor(a))
__mc_arm_unary(__builtin_aarch64_floorv2df, __builtin_elementwise_floor(a))
__mc_arm_unary(__builtin_aarch64_ceilv4sf, __builtin_elementwise_ceil(a))
__mc_arm_unary(__builtin_aarch64_ceilv2df, __builtin_elementwise_ceil(a))
__mc_arm_unary(__builtin_aarch64_btruncv4sf, __builtin_elementwise_trunc(a))
__mc_arm_unary(__builtin_aarch64_btruncv2df, __builtin_elementwise_trunc(a))
__mc_arm_unary(__builtin_aarch64_roundv4sf, __builtin_elementwise_round(a))
__mc_arm_unary(__builtin_aarch64_roundv2df, __builtin_elementwise_round(a))
__mc_arm_unary(__builtin_aarch64_nearbyintv4sf, __builtin_elementwise_roundeven(a))
__mc_arm_unary(__builtin_aarch64_nearbyintv2df, __builtin_elementwise_roundeven(a))
__mc_arm_unary(__builtin_aarch64_rintv4sf, __builtin_elementwise_roundeven(a))
__mc_arm_unary(__builtin_aarch64_rintv2df, __builtin_elementwise_roundeven(a))

__mc_arm_unary(__builtin_aarch64_frecpev4sf, __clang_arm_recip(a))
__mc_arm_unary(__builtin_aarch64_frecpev2df, __clang_arm_recip(a))
__mc_arm_binary(__builtin_aarch64_frecpsv4sf, (a * 0 + 2) - a * b)
__mc_arm_binary(__builtin_aarch64_frecpsv2df, (a * 0 + 2) - a * b)
__mc_arm_unary(__builtin_aarch64_rsqrtev4sf, __clang_arm_rsqrt(a))
__mc_arm_unary(__builtin_aarch64_rsqrtev2df, __clang_arm_rsqrt(a))
__mc_arm_binary(__builtin_aarch64_rsqrtsv4sf, ((a * 0 + 3) - a * b) / (a * 0 + 2))
__mc_arm_binary(__builtin_aarch64_rsqrtsv2df, ((a * 0 + 3) - a * b) / (a * 0 + 2))

[[gnu::always_inline]] static inline int16x8_t
__builtin_aarch64_sabdlv8qi(int8x8_t a, int8x8_t b) noexcept
{
  return __clang_arm_widen_absdiff<int16x8_t, false>(a, b);
}

[[gnu::always_inline]] static inline int32x4_t
__builtin_aarch64_sabdlv4hi(int16x4_t a, int16x4_t b) noexcept
{
  return __clang_arm_widen_absdiff<int32x4_t, false>(a, b);
}

[[gnu::always_inline]] static inline int64x2_t
__builtin_aarch64_sabdlv2si(int32x2_t a, int32x2_t b) noexcept
{
  return __clang_arm_widen_absdiff<int64x2_t, false>(a, b);
}

[[gnu::always_inline]] static inline uint16x8_t
__builtin_aarch64_uabdlv8qi_uuu(uint8x8_t a, uint8x8_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint16x8_t, false>(a, b);
}

[[gnu::always_inline]] static inline uint32x4_t
__builtin_aarch64_uabdlv4hi_uuu(uint16x4_t a, uint16x4_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint32x4_t, false>(a, b);
}

[[gnu::always_inline]] static inline uint64x2_t
__builtin_aarch64_uabdlv2si_uuu(uint32x2_t a, uint32x2_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint64x2_t, false>(a, b);
}

[[gnu::always_inline]] static inline int16x8_t
__builtin_aarch64_sabdl2v16qi(int8x16_t a, int8x16_t b) noexcept
{
  return __clang_arm_widen_absdiff<int16x8_t, true>(a, b);
}

[[gnu::always_inline]] static inline int32x4_t
__builtin_aarch64_sabdl2v8hi(int16x8_t a, int16x8_t b) noexcept
{
  return __clang_arm_widen_absdiff<int32x4_t, true>(a, b);
}

[[gnu::always_inline]] static inline int64x2_t
__builtin_aarch64_sabdl2v4si(int32x4_t a, int32x4_t b) noexcept
{
  return __clang_arm_widen_absdiff<int64x2_t, true>(a, b);
}

[[gnu::always_inline]] static inline uint16x8_t
__builtin_aarch64_uabdl2v16qi_uuu(uint8x16_t a, uint8x16_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint16x8_t, true>(a, b);
}

[[gnu::always_inline]] static inline uint32x4_t
__builtin_aarch64_uabdl2v8hi_uuu(uint16x8_t a, uint16x8_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint32x4_t, true>(a, b);
}

[[gnu::always_inline]] static inline uint64x2_t
__builtin_aarch64_uabdl2v4si_uuu(uint32x4_t a, uint32x4_t b) noexcept
{
  return __clang_arm_widen_absdiff<uint64x2_t, true>(a, b);
}

#define __mc_arm_widen_acc(name)                                                                                                           \
  template<typename Acc, typename V> [[gnu::always_inline]] static inline Acc name(Acc acc, V a, V b) noexcept                             \
  {                                                                                                                                        \
    return __clang_arm_widen_acc_absdiff(acc, a, b);                                                                                       \
  }
__mc_arm_widen_acc(__builtin_aarch64_sabalv8qi)
__mc_arm_widen_acc(__builtin_aarch64_sabalv4hi)
__mc_arm_widen_acc(__builtin_aarch64_sabalv2si)
__mc_arm_widen_acc(__builtin_aarch64_uabalv8qi_uuuu)
__mc_arm_widen_acc(__builtin_aarch64_uabalv4hi_uuuu)
__mc_arm_widen_acc(__builtin_aarch64_uabalv2si_uuuu)

#define __mc_arm_reduce(name, expression)                                                                                                  \
  template<typename V> [[gnu::always_inline]] static inline auto name(V v) noexcept { return (expression); }
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v16qi, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v8hi, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v4si, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v2di, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v16qi_uu, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v8hi_uu, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v4si_uu, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v2di_uu, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v4sf, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_plus_scal_v2df, __clang_arm_reduce_add(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smin_scal_v16qi, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smin_scal_v8hi, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smin_scal_v4si, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umin_scal_v16qi_uu, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umin_scal_v8hi_uu, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umin_scal_v4si_uu, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smin_nan_scal_v4sf, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smin_nan_scal_v2df, __clang_arm_reduce_min(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smax_scal_v16qi, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smax_scal_v8hi, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smax_scal_v4si, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umax_scal_v16qi_uu, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umax_scal_v8hi_uu, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_umax_scal_v4si_uu, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smax_nan_scal_v4sf, __clang_arm_reduce_max(v))
__mc_arm_reduce(__builtin_aarch64_reduc_smax_nan_scal_v2df, __clang_arm_reduce_max(v))

[[gnu::always_inline]] static inline int8x8_t
__builtin_aarch64_sqmovnv8hi(int16x8_t v) noexcept
{
  return __clang_arm_narrow_sat<int8x8_t>(v, -128, 127);
}

[[gnu::always_inline]] static inline int16x4_t
__builtin_aarch64_sqmovnv4si(int32x4_t v) noexcept
{
  return __clang_arm_narrow_sat<int16x4_t>(v, -32768, 32767);
}

[[gnu::always_inline]] static inline int32x2_t
__builtin_aarch64_sqmovnv2di(int64x2_t v) noexcept
{
  return __clang_arm_narrow_sat<int32x2_t>(v, -2147483647ll - 1, 2147483647);
}

[[gnu::always_inline]] static inline uint8x8_t
__builtin_aarch64_sqmovunv8hi_us(int16x8_t v) noexcept
{
  return __clang_arm_narrow_sat<uint8x8_t>(v, 0, 255);
}

[[gnu::always_inline]] static inline uint16x4_t
__builtin_aarch64_sqmovunv4si_us(int32x4_t v) noexcept
{
  return __clang_arm_narrow_sat<uint16x4_t>(v, 0, 65535);
}

[[gnu::always_inline]] static inline uint32x2_t
__builtin_aarch64_sqmovunv2di_us(int64x2_t v) noexcept
{
  return __clang_arm_narrow_sat<uint32x2_t>(v, 0, 4294967295ull);
}

[[gnu::always_inline]] static inline int8x16_t
__builtin_aarch64_qtbl1v16qi(int8x16_t table, int8x16_t indexes) noexcept
{
  return __clang_arm_tbl(table, indexes);
}

#undef __mc_arm_unary
#undef __mc_arm_binary
#undef __mc_arm_ternary
#undef __mc_arm_absdiff
#undef __mc_arm_absacc
#undef __mc_arm_half_add
#undef __mc_arm_half_sub
#undef __mc_arm_rhalf_add_signed
#undef __mc_arm_rhalf_add_unsigned
#undef __mc_arm_add_sat
#undef __mc_arm_sub_sat
#undef __mc_arm_widen_acc
#undef __mc_arm_reduce

};      // namespace __bits
};      // namespace simd
};      // namespace micron

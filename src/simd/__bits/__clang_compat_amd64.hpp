//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__vector_types_amd64.hpp"

#if !defined(__clang__)
#error "__clang_compat_amd64.hpp is a Clang-only compatibility layer"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

// NOTE: clang deliberately exposes a smaller set of GCC-compatible __builtin_ia32_* spellings

__micron_diagnostic_push
__micron_diagnostic_nan
template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_add(V a, V b) noexcept
{
  a[0] += b[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_sub(V a, V b) noexcept
{
  a[0] -= b[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_mul(V a, V b) noexcept
{
  a[0] *= b[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_div(V a, V b) noexcept
{
  a[0] /= b[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_sqrt(V a) noexcept
{
  a[0] = static_cast<__typeof__(a[0])>(__builtin_elementwise_sqrt(a[0]));
  return a;
}

template<typename V, typename S>
[[gnu::always_inline]] static inline V
__clang_scalar_convert(V a, S value) noexcept
{
  a[0] = static_cast<__typeof__(a[0])>(value);
  return a;
}

template<typename Out, typename In>
[[gnu::always_inline]] static inline Out
__clang_convert_low(In in) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = static_cast<__typeof__(out[0])>(in[i]);
  return out;
}

template<typename Out, unsigned Period = 1, typename In>
[[gnu::always_inline]] static inline Out
__clang_broadcast(In in) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = static_cast<__typeof__(out[0])>(in[i % Period]);
  return out;
}

template<typename Out, typename In>
[[gnu::always_inline]] static inline Out
__clang_broadcast_scalar(In in) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = static_cast<__typeof__(out[0])>(in);
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline unsigned long long
__clang_compare_mask(V a, V b, int predicate) noexcept
{
  unsigned long long mask = 0;
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    const bool unord = __builtin_isunordered(a[i], b[i]);
    bool match = false;
    switch ( predicate & 31 ) {
    case 0:
      match = !unord and a[i] == b[i];
      break;
    case 1:
      match = !unord and a[i] < b[i];
      break;
    case 2:
      match = !unord and a[i] <= b[i];
      break;
    case 3:
      match = unord;
      break;
    case 4:
      match = unord or a[i] != b[i];
      break;
    case 5:
      match = unord or !(a[i] < b[i]);
      break;
    case 6:
      match = unord or !(a[i] <= b[i]);
      break;
    case 7:
      match = !unord;
      break;
    case 8:
      match = unord or a[i] == b[i];
      break;
    case 9:
      match = unord or !(a[i] >= b[i]);
      break;
    case 10:
      match = unord or !(a[i] > b[i]);
      break;
    case 11:
      match = false;
      break;
    case 12:
      match = !unord and a[i] != b[i];
      break;
    case 13:
      match = !unord and a[i] >= b[i];
      break;
    case 14:
      match = !unord and a[i] > b[i];
      break;
    case 15:
      match = true;
      break;
    default:
      match = __clang_compare_mask(a, b, predicate & 15) & (1ull << i);
      break;
    }
    if ( match ) mask |= 1ull << i;
  }
  return mask;
}
__micron_diagnostic_pop

template<bool High, typename V>
[[gnu::always_inline]] static inline V
__clang_unpack(V a, V b) noexcept
{
  V out{};
  constexpr unsigned lanes = sizeof(V) / sizeof(a[0]);
  constexpr unsigned block_lanes = 16 / sizeof(a[0]);
  constexpr unsigned half = block_lanes / 2;
  for ( unsigned block = 0; block < lanes; block += block_lanes ) {
    const unsigned base = block + (High ? half : 0);
    for ( unsigned i = 0; i < half; ++i ) {
      out[block + i * 2] = a[base + i];
      out[block + i * 2 + 1] = b[base + i];
    }
  }
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_shift_left(V a, unsigned count) noexcept
{
  constexpr unsigned width = sizeof(a[0]) * 8;
  if ( count >= width ) return V{};
  return a << count;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_shift_right_logical(V a, unsigned count) noexcept
{
  constexpr unsigned width = sizeof(a[0]) * 8;
  if ( count >= width ) return V{};
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = static_cast<__typeof__(out[0])>(static_cast<unsigned long long>(a[i]) >> count);
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_shift_right_arithmetic(V a, unsigned count) noexcept
{
  constexpr unsigned width = sizeof(a[0]) * 8;
  if ( count >= width ) count = width - 1;
  return a >> count;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_fmadd(V a, V b, V c) noexcept
{
  return __builtin_elementwise_fma(a, b, c);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_fmsub(V a, V b, V c) noexcept
{
  return __builtin_elementwise_fma(a, b, -c);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_fnmadd(V a, V b, V c) noexcept
{
  return __builtin_elementwise_fma(-a, b, c);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_fnmsub(V a, V b, V c) noexcept
{
  return __builtin_elementwise_fma(-a, b, -c);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_fmadd(V a, V b, V c) noexcept
{
  V value = __clang_fmadd(a, b, c);
  a[0] = value[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_fmsub(V a, V b, V c) noexcept
{
  V value = __clang_fmsub(a, b, c);
  a[0] = value[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_fnmadd(V a, V b, V c) noexcept
{
  V value = __clang_fnmadd(a, b, c);
  a[0] = value[0];
  return a;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_scalar_fnmsub(V a, V b, V c) noexcept
{
  V value = __clang_fnmsub(a, b, c);
  a[0] = value[0];
  return a;
}

template<typename V, typename Base, typename I, typename M>
[[gnu::always_inline]] static inline V
__clang_gather(V src, Base base, I indexes, M mask, int scale) noexcept
{
  constexpr unsigned n = sizeof(V) / sizeof(src[0]);
  const char *bytes = (const char *)base;
  for ( unsigned i = 0; i < n; ++i ) {
    if ( mask[i] != 0 ) {
      __typeof__(src[0]) value;
      __builtin_memcpy(&value, bytes + indexes[i] * scale, sizeof(value));
      src[i] = value;
    }
  }
  return src;
}

#define __mc_clang_gather(name)                                                                                                            \
  template<typename V, typename Base, typename I, typename M>                                                                              \
  [[gnu::always_inline]] static inline V name(V src, Base base, I indexes, M mask, int scale) noexcept                                     \
  {                                                                                                                                        \
    return __clang_gather(src, base, indexes, mask, scale);                                                                                \
  }

__mc_clang_gather(__builtin_ia32_gathersiv4sf)
__mc_clang_gather(__builtin_ia32_gathersiv8sf)
__mc_clang_gather(__builtin_ia32_gathersiv2df)
__mc_clang_gather(__builtin_ia32_gathersiv4df)
__mc_clang_gather(__builtin_ia32_gathersiv4si)
__mc_clang_gather(__builtin_ia32_gathersiv8si)
__mc_clang_gather(__builtin_ia32_gathersiv2di)
__mc_clang_gather(__builtin_ia32_gathersiv4di)

#undef __mc_clang_gather

template<typename V, typename P>
[[gnu::always_inline]] static inline V
__clang_load_low(V a, P p) noexcept
{
  a[0] = (*p)[0];
  a[1] = (*p)[1];
  return a;
}

template<typename V, typename P>
[[gnu::always_inline]] static inline V
__clang_load_high(V a, P p) noexcept
{
  a[2] = (*p)[0];
  a[3] = (*p)[1];
  return a;
}

template<typename P, typename V>
[[gnu::always_inline]] static inline void
__clang_store_low(P p, V a) noexcept
{
  (*p)[0] = a[0];
  (*p)[1] = a[1];
}

template<typename P, typename V>
[[gnu::always_inline]] static inline void
__clang_store_high(P p, V a) noexcept
{
  (*p)[0] = a[2];
  (*p)[1] = a[3];
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_byte_shift_left(V a, unsigned bits) noexcept
{
  V out{};
  const unsigned count = bits / 8;
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  if ( count >= n ) return out;
  for ( unsigned i = count; i < n; ++i ) out[i] = a[i - count];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_byte_shift_right(V a, unsigned bits) noexcept
{
  V out{};
  const unsigned count = bits / 8;
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  if ( count >= n ) return out;
  for ( unsigned i = 0; i + count < n; ++i ) out[i] = a[i + count];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_avg_unsigned(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i )
    out[i] = static_cast<__typeof__(out[0])>((static_cast<unsigned long long>(a[i]) + static_cast<unsigned long long>(b[i]) + 1ULL) >> 1);
  return out;
}

template<bool Unsigned, typename V>
[[gnu::always_inline]] static inline V
__clang_mulhi16(V a, V b) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    if constexpr ( Unsigned )
      out[i] = static_cast<__typeof__(out[0])>((static_cast<unsigned int>(static_cast<unsigned short>(a[i]))
                                                * static_cast<unsigned int>(static_cast<unsigned short>(b[i])))
                                               >> 16);
    else
      out[i] = static_cast<__typeof__(out[0])>((static_cast<int>(a[i]) * static_cast<int>(b[i])) >> 16);
  }
  return out;
}

template<typename Out, bool Unsigned, typename V>
[[gnu::always_inline]] static inline Out
__clang_mul_even32(V a, V b) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i ) {
    if constexpr ( Unsigned )
      out[i] = static_cast<__typeof__(out[0])>(static_cast<unsigned long long>(static_cast<unsigned int>(a[i * 2]))
                                               * static_cast<unsigned long long>(static_cast<unsigned int>(b[i * 2])));
    else
      out[i] = static_cast<__typeof__(out[0])>(static_cast<long long>(a[i * 2]) * static_cast<long long>(b[i * 2]));
  }
  return out;
}

template<typename Out, typename V>
[[gnu::always_inline]] static inline Out
__clang_madd16(V a, V b) noexcept
{
  Out out{};
  constexpr unsigned n = sizeof(Out) / sizeof(out[0]);
  for ( unsigned i = 0; i < n; ++i )
    out[i] = static_cast<__typeof__(out[0])>(a[i * 2] * b[i * 2] + a[i * 2 + 1] * b[i * 2 + 1]);
  return out;
}

template<typename Out, typename In>
[[gnu::always_inline]] static inline Out
__clang_pack(In a, In b, long long low, unsigned long long high) noexcept
{
  Out out{};
  const long long high_value = static_cast<long long>(high);
  constexpr unsigned blocks = sizeof(In) / 16;
  constexpr unsigned in_lanes = 16 / sizeof(a[0]);
  constexpr unsigned out_lanes = 16 / sizeof(out[0]);
  for ( unsigned block = 0; block < blocks; ++block ) {
    for ( unsigned i = 0; i < in_lanes; ++i ) {
      long long av = a[block * in_lanes + i];
      long long bv = b[block * in_lanes + i];
      if ( av < low ) av = low;
      if ( bv < low ) bv = low;
      if ( av > high_value ) av = high_value;
      if ( bv > high_value ) bv = high_value;
      out[block * out_lanes + i] = static_cast<__typeof__(out[0])>(av);
      out[block * out_lanes + in_lanes + i] = static_cast<__typeof__(out[0])>(bv);
    }
  }
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_movshdup(V a) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = a[i | 1u];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_movsldup(V a) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = a[i & ~1u];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_movddup(V a) noexcept
{
  V out{};
  constexpr unsigned n = sizeof(V) / sizeof(a[0]);
  for ( unsigned i = 0; i < n; ++i ) out[i] = a[i & ~1u];
  return out;
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_abs(V a) noexcept
{
  return __builtin_elementwise_abs(a);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_min(V a, V b) noexcept
{
  return __builtin_elementwise_min(a, b);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_max(V a, V b) noexcept
{
  return __builtin_elementwise_max(a, b);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_add_sat(V a, V b) noexcept
{
  return __builtin_elementwise_add_sat(a, b);
}

template<typename V>
[[gnu::always_inline]] static inline V
__clang_sub_sat(V a, V b) noexcept
{
  return __builtin_elementwise_sub_sat(a, b);
}

#define __builtin_ia32_addss(a, b) ::micron::simd::__bits::__clang_scalar_add((a), (b))
#define __builtin_ia32_subss(a, b) ::micron::simd::__bits::__clang_scalar_sub((a), (b))
#define __builtin_ia32_mulss(a, b) ::micron::simd::__bits::__clang_scalar_mul((a), (b))
#define __builtin_ia32_divss(a, b) ::micron::simd::__bits::__clang_scalar_div((a), (b))
#define __builtin_ia32_addsd(a, b) ::micron::simd::__bits::__clang_scalar_add((a), (b))
#define __builtin_ia32_subsd(a, b) ::micron::simd::__bits::__clang_scalar_sub((a), (b))
#define __builtin_ia32_mulsd(a, b) ::micron::simd::__bits::__clang_scalar_mul((a), (b))
#define __builtin_ia32_divsd(a, b) ::micron::simd::__bits::__clang_scalar_div((a), (b))
#define __builtin_ia32_sqrtss(a) ::micron::simd::__bits::__clang_scalar_sqrt((a))
#define __builtin_ia32_sqrtsd(a) ::micron::simd::__bits::__clang_scalar_sqrt((a))
#define __builtin_ia32_cvtsi2ss(a, v) ::micron::simd::__bits::__clang_scalar_convert((a), (v))
#define __builtin_ia32_cvtsi642ss(a, v) ::micron::simd::__bits::__clang_scalar_convert((a), (v))
#define __builtin_ia32_cvtsi2sd(a, v) ::micron::simd::__bits::__clang_scalar_convert((a), (v))
#define __builtin_ia32_cvtsi642sd(a, v) ::micron::simd::__bits::__clang_scalar_convert((a), (v))
#define __builtin_ia32_cvtss2sd(a, b) ::micron::simd::__bits::__clang_scalar_convert((a), (b)[0])

#define __builtin_ia32_loadlps(a, p) ::micron::simd::__bits::__clang_load_low((a), (p))
#define __builtin_ia32_loadhps(a, p) ::micron::simd::__bits::__clang_load_high((a), (p))
#define __builtin_ia32_storelps(p, a) ::micron::simd::__bits::__clang_store_low((p), (a))
#define __builtin_ia32_storehps(p, a) ::micron::simd::__bits::__clang_store_high((p), (a))
#define __builtin_ia32_movhlps(a, b) __builtin_shufflevector((b), (a), 2, 3, 6, 7)
#define __builtin_ia32_movlhps(a, b) __builtin_shufflevector((a), (b), 0, 1, 4, 5)

#define __builtin_ia32_ps256_ps(a) ::micron::simd::__bits::__clang_convert_low<__v8sf>((a))
#define __builtin_ia32_pd256_pd(a) ::micron::simd::__bits::__clang_convert_low<__v4df>((a))
#define __builtin_ia32_si256_si(a) ::micron::simd::__bits::__clang_convert_low<__v8si>((a))
#define __builtin_ia32_ps_ps256(a) ::micron::simd::__bits::__clang_convert_low<__v4sf>((a))
#define __builtin_ia32_pd_pd256(a) ::micron::simd::__bits::__clang_convert_low<__v2df>((a))
#define __builtin_ia32_si_si256(a) ::micron::simd::__bits::__clang_convert_low<__v4si>((a))

#define __builtin_ia32_sqrtps(a) __builtin_elementwise_sqrt((a))
#define __builtin_ia32_sqrtpd(a) __builtin_elementwise_sqrt((a))
#define __builtin_ia32_sqrtps256(a) __builtin_elementwise_sqrt((a))
#define __builtin_ia32_sqrtpd256(a) __builtin_elementwise_sqrt((a))
#define __builtin_ia32_sqrtps512_mask(a, passthrough, mask, rounding) __builtin_elementwise_sqrt((a))
#define __builtin_ia32_sqrtpd512_mask(a, passthrough, mask, rounding) __builtin_elementwise_sqrt((a))

#define __builtin_ia32_andps256(a, b) ((__v8sf)((__v8su)(a) & (__v8su)(b)))
#define __builtin_ia32_andnps256(a, b) ((__v8sf)(~(__v8su)(a) & (__v8su)(b)))
#define __builtin_ia32_orps256(a, b) ((__v8sf)((__v8su)(a) | (__v8su)(b)))
#define __builtin_ia32_xorps256(a, b) ((__v8sf)((__v8su)(a) ^ (__v8su)(b)))
#define __builtin_ia32_andpd256(a, b) ((__v4df)((__v4du)(a) & (__v4du)(b)))
#define __builtin_ia32_andnpd256(a, b) ((__v4df)(~(__v4du)(a) & (__v4du)(b)))
#define __builtin_ia32_orpd256(a, b) ((__v4df)((__v4du)(a) | (__v4du)(b)))
#define __builtin_ia32_xorpd256(a, b) ((__v4df)((__v4du)(a) ^ (__v4du)(b)))
#define __builtin_ia32_andps512_mask(a, b, passthrough, mask) ((__v16sf)((__v16su)(a) & (__v16su)(b)))
#define __builtin_ia32_andpd512_mask(a, b, passthrough, mask) ((__v8df)((__v8du)(a) & (__v8du)(b)))

#define __builtin_ia32_movshdup(a) ::micron::simd::__bits::__clang_movshdup((a))
#define __builtin_ia32_movsldup(a) ::micron::simd::__bits::__clang_movsldup((a))
#define __builtin_ia32_movshdup256(a) ::micron::simd::__bits::__clang_movshdup((a))
#define __builtin_ia32_movsldup256(a) ::micron::simd::__bits::__clang_movsldup((a))
#define __builtin_ia32_movddup256(a) ::micron::simd::__bits::__clang_movddup((a))

#define __mc_clang_movnt(p, v) __builtin_nontemporal_store((v), (__typeof__(v) *)(p))
#define __builtin_ia32_movntdq(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntpd(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntps(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntdq256(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntpd256(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntps256(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntdq512(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntpd512(p, v) __mc_clang_movnt((p), (v))
#define __builtin_ia32_movntps512(p, v) __mc_clang_movnt((p), (v))

#define __builtin_ia32_vbroadcastss256(p) ::micron::simd::__bits::__clang_broadcast_scalar<__v8sf>(*p)
#define __builtin_ia32_vbroadcastsd256(p) ::micron::simd::__bits::__clang_broadcast_scalar<__v4df>(*p)
#define __builtin_ia32_vbroadcastss(p) ::micron::simd::__bits::__clang_broadcast_scalar<__v4sf>(*p)
#define __builtin_ia32_vbroadcastf128_ps256(p) ::micron::simd::__bits::__clang_broadcast<__v8sf, 4>(*(p))
#define __builtin_ia32_vbroadcastf128_pd256(p) ::micron::simd::__bits::__clang_broadcast<__v4df, 2>(*(p))
#define __builtin_ia32_broadcastss512(a, ...) ::micron::simd::__bits::__clang_broadcast<__v16sf>((a))
#define __builtin_ia32_broadcastsd512(a, ...) ::micron::simd::__bits::__clang_broadcast<__v8df>((a))
#define __builtin_ia32_broadcastf32x2_512_mask(a, passthrough, mask) ::micron::simd::__bits::__clang_broadcast<__v16sf, 2>((a))
#define __builtin_ia32_broadcasti32x2_512_mask(a, passthrough, mask) ::micron::simd::__bits::__clang_broadcast<__v16si, 2>((a))
#define __builtin_ia32_broadcastf64x2_512_mask(a, passthrough, mask) ::micron::simd::__bits::__clang_broadcast<__v8df, 2>((a))

#define __builtin_ia32_cvtdq2ps(a) __builtin_convertvector((a), __v4sf)
#define __builtin_ia32_cvtdq2ps256(a) __builtin_convertvector((a), __v8sf)
#define __builtin_ia32_cvtdq2pd(a) ::micron::simd::__bits::__clang_convert_low<__v2df>((a))
#define __builtin_ia32_cvtdq2pd256(a) __builtin_convertvector((a), __v4df)
#define __builtin_ia32_cvtps2pd(a) ::micron::simd::__bits::__clang_convert_low<__v2df>((a))
#define __builtin_ia32_cvtps2pd256(a) __builtin_convertvector((a), __v4df)
#define __builtin_ia32_cvtdq2ps512_mask(a, passthrough, mask, rounding) __builtin_convertvector((a), __v16sf)
#define __builtin_ia32_cvtdq2pd512_mask(a, passthrough, mask) __builtin_convertvector((a), __v8df)
#define __builtin_ia32_cvtps2pd512_mask(a, passthrough, mask, rounding) __builtin_convertvector((a), __v8df)

#define __builtin_ia32_unpckhps(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_unpcklps(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_unpckhpd(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_unpcklpd(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_unpckhps256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_unpcklps256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_unpckhpd256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_unpcklpd256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))

#define __builtin_ia32_punpckhbw128(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhwd128(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhdq128(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhqdq128(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpcklbw128(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpcklwd128(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpckldq128(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpcklqdq128(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpckhbw256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhwd256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhdq256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhqdq256(a, b) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpcklbw256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpcklwd256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpckldq256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpcklqdq256(a, b) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpckhbw512_mask(a, b, passthrough, mask) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpckhwd512_mask(a, b, passthrough, mask) ::micron::simd::__bits::__clang_unpack<true>((a), (b))
#define __builtin_ia32_punpcklbw512_mask(a, b, passthrough, mask) ::micron::simd::__bits::__clang_unpack<false>((a), (b))
#define __builtin_ia32_punpcklwd512_mask(a, b, passthrough, mask) ::micron::simd::__bits::__clang_unpack<false>((a), (b))

#define __builtin_ia32_pmovsxbw128(a) ::micron::simd::__bits::__clang_convert_low<__v8hi>((__v16qi)(a))
#define __builtin_ia32_pmovsxbd128(a) ::micron::simd::__bits::__clang_convert_low<__v4si>((__v16qi)(a))
#define __builtin_ia32_pmovsxbq128(a) ::micron::simd::__bits::__clang_convert_low<__v2di>((__v16qi)(a))
#define __builtin_ia32_pmovsxwd128(a) ::micron::simd::__bits::__clang_convert_low<__v4si>((__v8hi)(a))
#define __builtin_ia32_pmovsxwq128(a) ::micron::simd::__bits::__clang_convert_low<__v2di>((__v8hi)(a))
#define __builtin_ia32_pmovsxdq128(a) ::micron::simd::__bits::__clang_convert_low<__v2di>((__v4si)(a))
#define __builtin_ia32_pmovzxbw128(a) ::micron::simd::__bits::__clang_convert_low<__v8hu>((__v16qu)(a))
#define __builtin_ia32_pmovzxbd128(a) ::micron::simd::__bits::__clang_convert_low<__v4su>((__v16qu)(a))
#define __builtin_ia32_pmovzxbq128(a) ::micron::simd::__bits::__clang_convert_low<__v2du>((__v16qu)(a))
#define __builtin_ia32_pmovzxwd128(a) ::micron::simd::__bits::__clang_convert_low<__v4su>((__v8hu)(a))
#define __builtin_ia32_pmovzxwq128(a) ::micron::simd::__bits::__clang_convert_low<__v2du>((__v8hu)(a))
#define __builtin_ia32_pmovzxdq128(a) ::micron::simd::__bits::__clang_convert_low<__v2du>((__v4su)(a))
#define __builtin_ia32_pmovsxbw256(a) ::micron::simd::__bits::__clang_convert_low<__v16hi>((__v16qi)(a))
#define __builtin_ia32_pmovsxbd256(a) ::micron::simd::__bits::__clang_convert_low<__v8si>((__v16qi)(a))
#define __builtin_ia32_pmovsxbq256(a) ::micron::simd::__bits::__clang_convert_low<__v4di>((__v16qi)(a))
#define __builtin_ia32_pmovsxwd256(a) ::micron::simd::__bits::__clang_convert_low<__v8si>((__v8hi)(a))
#define __builtin_ia32_pmovsxwq256(a) ::micron::simd::__bits::__clang_convert_low<__v4di>((__v8hi)(a))
#define __builtin_ia32_pmovsxdq256(a) ::micron::simd::__bits::__clang_convert_low<__v4di>((__v4si)(a))
#define __builtin_ia32_pmovzxbw256(a) ::micron::simd::__bits::__clang_convert_low<__v16hu>((__v16qu)(a))
#define __builtin_ia32_pmovzxbd256(a) ::micron::simd::__bits::__clang_convert_low<__v8su>((__v16qu)(a))
#define __builtin_ia32_pmovzxbq256(a) ::micron::simd::__bits::__clang_convert_low<__v4du>((__v16qu)(a))
#define __builtin_ia32_pmovzxwd256(a) ::micron::simd::__bits::__clang_convert_low<__v8su>((__v8hu)(a))
#define __builtin_ia32_pmovzxwq256(a) ::micron::simd::__bits::__clang_convert_low<__v4du>((__v8hu)(a))
#define __builtin_ia32_pmovzxdq256(a) ::micron::simd::__bits::__clang_convert_low<__v4du>((__v4su)(a))

#define __builtin_ia32_vbroadcastsi256(a) ::micron::simd::__bits::__clang_broadcast<__v4di, 2>((a))
#define __builtin_ia32_pbroadcastb128(a) ::micron::simd::__bits::__clang_broadcast<__v16qi>((a))
#define __builtin_ia32_pbroadcastw128(a) ::micron::simd::__bits::__clang_broadcast<__v8hi>((a))
#define __builtin_ia32_pbroadcastd128(a) ::micron::simd::__bits::__clang_broadcast<__v4si>((a))
#define __builtin_ia32_pbroadcastq128(a) ::micron::simd::__bits::__clang_broadcast<__v2di>((a))
#define __builtin_ia32_pbroadcastb256(a) ::micron::simd::__bits::__clang_broadcast<__v32qi>((a))
#define __builtin_ia32_pbroadcastw256(a) ::micron::simd::__bits::__clang_broadcast<__v16hi>((a))
#define __builtin_ia32_pbroadcastd256(a) ::micron::simd::__bits::__clang_broadcast<__v8si>((a))
#define __builtin_ia32_pbroadcastq256(a) ::micron::simd::__bits::__clang_broadcast<__v4di>((a))
#define __builtin_ia32_vbroadcastss_ps(a) ::micron::simd::__bits::__clang_broadcast<__v4sf>((a))
#define __builtin_ia32_vbroadcastss_ps256(a) ::micron::simd::__bits::__clang_broadcast<__v8sf>((a))
#define __builtin_ia32_vbroadcastsd_pd256(a) ::micron::simd::__bits::__clang_broadcast<__v4df>((a))
#define __builtin_ia32_movntdqa(p) (*(p))
#define __builtin_ia32_movntdqa256(p) (*(p))

#define __builtin_ia32_minps512_mask(a, b, passthrough, mask, rounding) __builtin_elementwise_min((a), (b))
#define __builtin_ia32_maxps512_mask(a, b, passthrough, mask, rounding) __builtin_elementwise_max((a), (b))
#define __builtin_ia32_minpd512_mask(a, b, passthrough, mask, rounding) __builtin_elementwise_min((a), (b))
#define __builtin_ia32_maxpd512_mask(a, b, passthrough, mask, rounding) __builtin_elementwise_max((a), (b))
#define __builtin_ia32_cmpps512_mask(a, b, predicate, mask, rounding) ::micron::simd::__bits::__clang_compare_mask((a), (b), (predicate))
#define __builtin_ia32_cmppd512_mask(a, b, predicate, mask, rounding) ::micron::simd::__bits::__clang_compare_mask((a), (b), (predicate))

#define __builtin_ia32_pslldqi128(a, bits) ::micron::simd::__bits::__clang_byte_shift_left((__v16qi)(a), (bits))
#define __builtin_ia32_psrldqi128(a, bits) ::micron::simd::__bits::__clang_byte_shift_right((__v16qi)(a), (bits))

#define __builtin_ia32_psllwi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_left((a), (count))
#define __builtin_ia32_pslldi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_left((a), (count))
#define __builtin_ia32_psllqi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_left((a), (count))
#define __builtin_ia32_psrlwi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_logical((a), (count))
#define __builtin_ia32_psrldi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_logical((a), (count))
#define __builtin_ia32_psrlqi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_logical((a), (count))
#define __builtin_ia32_psrawi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_arithmetic((a), (count))
#define __builtin_ia32_psradi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_arithmetic((a), (count))
#define __builtin_ia32_psraqi512_mask(a, count, ...) ::micron::simd::__bits::__clang_shift_right_arithmetic((a), (count))

#define __builtin_ia32_pavgb512_mask(a, b, ...) ::micron::simd::__bits::__clang_avg_unsigned((__v64qu)(a), (__v64qu)(b))
#define __builtin_ia32_pavgw512_mask(a, b, ...) ::micron::simd::__bits::__clang_avg_unsigned((__v32hu)(a), (__v32hu)(b))
#define __builtin_ia32_pmulhw512_mask(a, b, ...) ::micron::simd::__bits::__clang_mulhi16<false>((a), (b))
#define __builtin_ia32_pmulhuw512_mask(a, b, ...) ::micron::simd::__bits::__clang_mulhi16<true>((a), (b))
#define __builtin_ia32_pmaddwd512_mask(a, b, ...) ::micron::simd::__bits::__clang_madd16<__v16si>((a), (b))
#define __builtin_ia32_pmuldq512_mask(a, b, ...) ::micron::simd::__bits::__clang_mul_even32<__v8di, false>((a), (b))
#define __builtin_ia32_pmuludq512_mask(a, b, ...) ::micron::simd::__bits::__clang_mul_even32<__v8du, true>((a), (b))
#define __builtin_ia32_packsswb512_mask(a, b, ...) ::micron::simd::__bits::__clang_pack<__v64qi>((a), (b), -128, 127)
#define __builtin_ia32_packssdw512_mask(a, b, ...) ::micron::simd::__bits::__clang_pack<__v32hi>((a), (b), -32768, 32767)
#define __builtin_ia32_packuswb512_mask(a, b, ...) ::micron::simd::__bits::__clang_pack<__v64qu>((a), (b), 0, 255)
#define __builtin_ia32_packusdw512_mask(a, b, ...) ::micron::simd::__bits::__clang_pack<__v32hu>((a), (b), 0, 65535)

#define __builtin_ia32_prefetch(address, rw, locality, cache) __builtin_prefetch((address), (rw), (locality))

#define __builtin_ia32_vfmaddps(a, b, c) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddpd(a, b, c) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddps256(a, b, c) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddpd256(a, b, c) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddps512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddpd512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fmadd((a), (b), (c))
#define __builtin_ia32_vfmsubps(a, b, c) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubpd(a, b, c) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubps256(a, b, c) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubpd256(a, b, c) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubps512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubpd512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fmsub((a), (b), (c))
#define __builtin_ia32_vfnmaddps(a, b, c) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddpd(a, b, c) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddps256(a, b, c) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddpd256(a, b, c) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddps512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddpd512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmsubps(a, b, c) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubpd(a, b, c) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubps256(a, b, c) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubpd256(a, b, c) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubps512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubpd512_mask(a, b, c, ...) ::micron::simd::__bits::__clang_fnmsub((a), (b), (c))
#define __builtin_ia32_vfmaddss3(a, b, c) ::micron::simd::__bits::__clang_scalar_fmadd((a), (b), (c))
#define __builtin_ia32_vfmaddsd3(a, b, c) ::micron::simd::__bits::__clang_scalar_fmadd((a), (b), (c))
#define __builtin_ia32_vfmsubss3(a, b, c) ::micron::simd::__bits::__clang_scalar_fmsub((a), (b), (c))
#define __builtin_ia32_vfmsubsd3(a, b, c) ::micron::simd::__bits::__clang_scalar_fmsub((a), (b), (c))
#define __builtin_ia32_vfnmaddss3(a, b, c) ::micron::simd::__bits::__clang_scalar_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmaddsd3(a, b, c) ::micron::simd::__bits::__clang_scalar_fnmadd((a), (b), (c))
#define __builtin_ia32_vfnmsubss3(a, b, c) ::micron::simd::__bits::__clang_scalar_fnmsub((a), (b), (c))
#define __builtin_ia32_vfnmsubsd3(a, b, c) ::micron::simd::__bits::__clang_scalar_fnmsub((a), (b), (c))

#define __mc_clang_abs(name)                                                                                                               \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, Tail...) noexcept { return __clang_abs(a); }
#define __mc_clang_min(name)                                                                                                               \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return __clang_min(a, b);                                                                                                              \
  }
#define __mc_clang_max(name)                                                                                                               \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return __clang_max(a, b);                                                                                                              \
  }
#define __mc_clang_add_sat(name)                                                                                                           \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return __clang_add_sat(a, b);                                                                                                          \
  }
#define __mc_clang_sub_sat(name)                                                                                                           \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return __clang_sub_sat(a, b);                                                                                                          \
  }

__mc_clang_abs(__builtin_ia32_pabsb128)
__mc_clang_abs(__builtin_ia32_pabsw128)
__mc_clang_abs(__builtin_ia32_pabsd128)
__mc_clang_abs(__builtin_ia32_pabsb256)
__mc_clang_abs(__builtin_ia32_pabsw256)
__mc_clang_abs(__builtin_ia32_pabsd256)
__mc_clang_abs(__builtin_ia32_pabsb512_mask)
__mc_clang_abs(__builtin_ia32_pabsw512_mask)
__mc_clang_abs(__builtin_ia32_pabsd512_mask)
__mc_clang_abs(__builtin_ia32_pabsq128_mask)
__mc_clang_abs(__builtin_ia32_pabsq256_mask)
__mc_clang_abs(__builtin_ia32_pabsq512_mask)

__mc_clang_min(__builtin_ia32_pminsb128)
__mc_clang_min(__builtin_ia32_pminsw128)
__mc_clang_min(__builtin_ia32_pminsd128)
__mc_clang_min(__builtin_ia32_pminsb256)
__mc_clang_min(__builtin_ia32_pminsw256)
__mc_clang_min(__builtin_ia32_pminsd256)
__mc_clang_min(__builtin_ia32_pminsq128_mask)
__mc_clang_min(__builtin_ia32_pminsq256_mask)
__mc_clang_min(__builtin_ia32_pminsq512_mask)
__mc_clang_min(__builtin_ia32_pminsb512_mask)
__mc_clang_min(__builtin_ia32_pminsw512_mask)
__mc_clang_min(__builtin_ia32_pminsd512_mask)
__mc_clang_max(__builtin_ia32_pmaxsb128)
__mc_clang_max(__builtin_ia32_pmaxsw128)
__mc_clang_max(__builtin_ia32_pmaxsd128)
__mc_clang_max(__builtin_ia32_pmaxsb256)
__mc_clang_max(__builtin_ia32_pmaxsw256)
__mc_clang_max(__builtin_ia32_pmaxsd256)
__mc_clang_max(__builtin_ia32_pmaxsq128_mask)
__mc_clang_max(__builtin_ia32_pmaxsq256_mask)
__mc_clang_max(__builtin_ia32_pmaxsq512_mask)
__mc_clang_max(__builtin_ia32_pmaxsb512_mask)
__mc_clang_max(__builtin_ia32_pmaxsw512_mask)
__mc_clang_max(__builtin_ia32_pmaxsd512_mask)

#define __mc_unsigned_min(name, U)                                                                                                         \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return (V)__clang_min((U)a, (U)b);                                                                                                     \
  }
#define __mc_unsigned_max(name, U)                                                                                                         \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return (V)__clang_max((U)a, (U)b);                                                                                                     \
  }
#define __mc_unsigned_add_sat(name, U)                                                                                                     \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return (V)__clang_add_sat((U)a, (U)b);                                                                                                 \
  }
#define __mc_unsigned_sub_sat(name, U)                                                                                                     \
  template<typename V, typename... Tail> [[gnu::always_inline]] static inline V name(V a, V b, Tail...) noexcept                           \
  {                                                                                                                                        \
    return (V)__clang_sub_sat((U)a, (U)b);                                                                                                 \
  }

__mc_unsigned_min(__builtin_ia32_pminub128, __v16qu)
__mc_unsigned_min(__builtin_ia32_pminuw128, __v8hu)
__mc_unsigned_min(__builtin_ia32_pminud128, __v4su)
__mc_unsigned_min(__builtin_ia32_pminub256, __v32qu)
__mc_unsigned_min(__builtin_ia32_pminuw256, __v16hu)
__mc_unsigned_min(__builtin_ia32_pminud256, __v8su)
__mc_unsigned_min(__builtin_ia32_pminuq128_mask, __v2du)
__mc_unsigned_min(__builtin_ia32_pminuq256_mask, __v4du)
__mc_unsigned_min(__builtin_ia32_pminuq512_mask, __v8du)
__mc_unsigned_min(__builtin_ia32_pminub512_mask, __v64qu)
__mc_unsigned_min(__builtin_ia32_pminuw512_mask, __v32hu)
__mc_unsigned_min(__builtin_ia32_pminud512_mask, __v16su)
__mc_unsigned_max(__builtin_ia32_pmaxub128, __v16qu)
__mc_unsigned_max(__builtin_ia32_pmaxuw128, __v8hu)
__mc_unsigned_max(__builtin_ia32_pmaxud128, __v4su)
__mc_unsigned_max(__builtin_ia32_pmaxub256, __v32qu)
__mc_unsigned_max(__builtin_ia32_pmaxuw256, __v16hu)
__mc_unsigned_max(__builtin_ia32_pmaxud256, __v8su)
__mc_unsigned_max(__builtin_ia32_pmaxuq128_mask, __v2du)
__mc_unsigned_max(__builtin_ia32_pmaxuq256_mask, __v4du)
__mc_unsigned_max(__builtin_ia32_pmaxuq512_mask, __v8du)
__mc_unsigned_max(__builtin_ia32_pmaxub512_mask, __v64qu)
__mc_unsigned_max(__builtin_ia32_pmaxuw512_mask, __v32hu)
__mc_unsigned_max(__builtin_ia32_pmaxud512_mask, __v16su)

__mc_clang_add_sat(__builtin_ia32_paddsb128)
__mc_clang_add_sat(__builtin_ia32_paddsw128)
__mc_clang_add_sat(__builtin_ia32_paddsb256)
__mc_clang_add_sat(__builtin_ia32_paddsw256)
__mc_clang_add_sat(__builtin_ia32_paddsb512_mask)
__mc_clang_add_sat(__builtin_ia32_paddsw512_mask)
__mc_clang_sub_sat(__builtin_ia32_psubsb128)
__mc_clang_sub_sat(__builtin_ia32_psubsw128)
__mc_clang_sub_sat(__builtin_ia32_psubsb256)
__mc_clang_sub_sat(__builtin_ia32_psubsw256)
__mc_clang_sub_sat(__builtin_ia32_psubsb512_mask)
__mc_clang_sub_sat(__builtin_ia32_psubsw512_mask)
__mc_unsigned_add_sat(__builtin_ia32_paddusb128, __v16qu)
__mc_unsigned_add_sat(__builtin_ia32_paddusw128, __v8hu)
__mc_unsigned_add_sat(__builtin_ia32_paddusb256, __v32qu)
__mc_unsigned_add_sat(__builtin_ia32_paddusw256, __v16hu)
__mc_unsigned_add_sat(__builtin_ia32_paddusb512_mask, __v64qu)
__mc_unsigned_add_sat(__builtin_ia32_paddusw512_mask, __v32hu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusb128, __v16qu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusw128, __v8hu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusb256, __v32qu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusw256, __v16hu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusb512_mask, __v64qu)
__mc_unsigned_sub_sat(__builtin_ia32_psubusw512_mask, __v32hu)

#undef __mc_clang_abs
#undef __mc_clang_min
#undef __mc_clang_max
#undef __mc_clang_add_sat
#undef __mc_clang_sub_sat
#undef __mc_unsigned_min
#undef __mc_unsigned_max
#undef __mc_unsigned_add_sat
#undef __mc_unsigned_sub_sat

};      // namespace __bits
};      // namespace simd
};      // namespace micron

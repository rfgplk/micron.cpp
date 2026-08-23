//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "fixed.hpp"
#include "floating_point.hpp"
#include "integral.hpp"
#include "parse_float.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%
// integers

template<typename I>
concept __tc_int = micron::is_integral_v<I> && !micron::is_same_v<micron::remove_cv_t<I>, bool>
                   && !micron::is_same_v<micron::remove_cv_t<I>, u128> && !micron::is_same_v<micron::remove_cv_t<I>, i128>;

constexpr static const usize __tc_int_max = 72;

template<__tc_int I>
constexpr usize
to_chars(char *buf, usize cap, I v, u32 base = 10u, bool upper = false)
{
  if ( buf == nullptr || base < 2u || base > 36u ) return 0;

  using U = micron::make_unsigned_t<I>;
  U mag;
  bool neg = false;
  if constexpr ( micron::is_signed_v<I> ) {
    if ( v < 0 ) {
      neg = true;
      mag = static_cast<U>(0) - static_cast<U>(v);      // two's complement, safe at the minimum
    } else {
      mag = static_cast<U>(v);
    }
  } else {
    mag = static_cast<U>(v);
  }

  char tmp[__tc_int_max];
  char *tend = tmp + __tc_int_max;
  char *start = (base == 10u) ? __impl::uint_to_buf_backward(tend, static_cast<u64>(mag))
                              : __impl::uint_to_buf_base_backward(tend, static_cast<u64>(mag), base, upper);

  const usize len = static_cast<usize>(tend - start) + (neg ? 1u : 0u);
  if ( cap < len ) return 0;

  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  for ( char *q = start; q != tend; ++q ) buf[pos++] = *q;
  return pos;
}

template<__tc_int I>
constexpr bool
from_chars(I &out, const char *p, usize n, u32 base = 10u)
{
  out = static_cast<I>(0);
  if ( p == nullptr || n == 0 || base < 2u || base > 36u ) return false;

  const char *ptr = p;
  const char *const end = p + n;
  bool neg = false;
  if ( *ptr == '-' ) {
    if constexpr ( !micron::is_signed_v<I> ) return false;
    neg = true;
    ++ptr;
  } else if ( *ptr == '+' ) {
    ++ptr;
  }
  if ( ptr == end ) return false;

  using U = micron::make_unsigned_t<I>;
  constexpr u64 umax = static_cast<u64>(static_cast<U>(~static_cast<U>(0)));
  u64 lim;
  if constexpr ( micron::is_signed_v<I> )
    lim = neg ? (umax >> 1) + 1ull : (umax >> 1);
  else
    lim = umax;

  u64 acc = 0;
  for ( ; ptr != end; ++ptr ) {
    const int d = __impl::digit_val(*ptr, base);
    if ( d < 0 ) return false;
    const u64 du = static_cast<u64>(d);
    if ( du > lim || acc > (lim - du) / base ) return false;      // out of range for I
    acc = acc * base + du;
  }

  if constexpr ( micron::is_signed_v<I> ) {
    if ( neg )
      out = static_cast<I>(static_cast<U>(0ull - acc));
    else
      out = static_cast<I>(static_cast<U>(acc));
  } else {
    out = static_cast<I>(static_cast<U>(acc));
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// 128-bit

inline constexpr bool
__i128_neg(const i128 &v) noexcept
{
#if defined(__micron_arch_width_64)
  return v < 0;
#else
  return v.__is_negative();
#endif
}

inline constexpr u128
__i128_mag(const i128 &v) noexcept
{
#if defined(__micron_arch_width_64)
  return __i128_neg(v) ? (static_cast<u128>(0) - static_cast<u128>(v)) : static_cast<u128>(v);
#else
  return v.__abs();
#endif
}

inline constexpr u128
__i128_bits(const i128 &v) noexcept
{
#if defined(__micron_arch_width_64)
  return static_cast<u128>(v);
#else
  return v.v;
#endif
}

constexpr static const usize __tc_int128_max = 136;

inline constexpr usize
to_chars(char *buf, usize cap, u128 v, u32 base = 10u, bool upper = false)
{
  if ( buf == nullptr || base < 2u || base > 36u ) return 0;
  char tmp[__tc_int128_max];
  char *tend = tmp + __tc_int128_max;
  char *start = __impl::uint128_to_buf_base_backward(tend, __impl::__u128_hi(v), __impl::__u128_lo(v), base, upper);
  const usize len = static_cast<usize>(tend - start);
  if ( cap < len ) return 0;
  for ( usize i = 0; i < len; ++i ) buf[i] = start[i];
  return len;
}

inline constexpr usize
to_chars(char *buf, usize cap, i128 v, u32 base = 10u, bool upper = false)
{
  if ( buf == nullptr || base < 2u || base > 36u ) return 0;
  const bool neg = __i128_neg(v);
  const u128 mag = __i128_mag(v);
  char tmp[__tc_int128_max];
  char *tend = tmp + __tc_int128_max;
  char *start = __impl::uint128_to_buf_base_backward(tend, __impl::__u128_hi(mag), __impl::__u128_lo(mag), base, upper);
  const usize len = static_cast<usize>(tend - start) + (neg ? 1u : 0u);
  if ( cap < len ) return 0;
  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  for ( char *q = start; q != tend; ++q ) buf[pos++] = *q;
  return pos;
}

inline constexpr bool
from_chars(u128 &out, const char *p, usize n, u32 base = 10u)
{
  out = static_cast<u128>(0);
  if ( p == nullptr || n == 0 || base < 2u || base > 36u ) return false;
  const char *ptr = p;
  const char *const end = p + n;
  if ( *ptr == '+' ) ++ptr;
  if ( ptr == end ) return false;

  u128 acc = static_cast<u128>(0);
  const u128 b = static_cast<u128>(base);
  // ~0 / base, computed once: the overflow guard
  const u128 umax = static_cast<u128>(0) - static_cast<u128>(1);
  const u128 lim = umax / b;
  const u128 limr = umax - lim * b;
  for ( ; ptr != end; ++ptr ) {
    const int d = __impl::digit_val(*ptr, base);
    if ( d < 0 ) return false;
    const u128 du = static_cast<u128>(static_cast<u64>(d));
    if ( acc > lim || (acc == lim && du > limr) ) return false;
    acc = acc * b + du;
  }
  out = acc;
  return true;
}

inline constexpr bool
from_chars(i128 &out, const char *p, usize n, u32 base = 10u)
{
  out = static_cast<i128>(0);
  if ( p == nullptr || n == 0 || base < 2u || base > 36u ) return false;
  bool neg = false;
  const char *ptr = p;
  if ( *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( *ptr == '+' ) {
    ++ptr;
  }
  if ( ptr == p + n ) return false;
  if ( *ptr == '+' || *ptr == '-' ) return false;

  u128 mag = static_cast<u128>(0);
  if ( !from_chars(mag, ptr, static_cast<usize>((p + n) - ptr), base) ) return false;

  // i128 spans [-2^127, 2^127-1]
  const u128 pos_max = (static_cast<u128>(1) << 127) - static_cast<u128>(1);
  const u128 neg_max = static_cast<u128>(1) << 127;
  if ( neg ) {
    if ( mag > neg_max ) return false;
  } else {
    if ( mag > pos_max ) return false;
  }
#if defined(__micron_arch_width_64)
  out = neg ? static_cast<i128>(static_cast<u128>(0) - mag) : static_cast<i128>(mag);
#else
  out = neg ? i128(i128::__negate_u(mag)) : i128(mag);
#endif
  return true;
}

// %%%%%%%%%%%%%%%%%%%%
// floating point

enum class float_format : u8 {
  shortest,        // d2s / f2s
  fixed,           // %f
  scientific,      // %e
  general,         // %g
  hex              // %a
};

inline constexpr usize f32_shortest_chars_capacity = 16;
inline constexpr usize f64_shortest_chars_capacity = 26;

struct chars4_result {
  usize lengths[4];

  inline constexpr usize
  operator[](usize lane) const noexcept
  {
    return lengths[lane];
  }
};

inline constexpr usize
to_chars(char *buf, usize cap, f64 v, float_format fmt = float_format::shortest, i32 precision = -1)
{
  if ( buf == nullptr ) return 0;
  const bool has_prec = precision >= 0;
  const u32 prec = has_prec ? static_cast<u32>(precision) : 6u;
  switch ( fmt ) {
  case float_format::fixed:
    return __impl::__fpconv::d2f_buffered(v, buf, cap, prec);
  case float_format::scientific:
    return __impl::__fpconv::d2e_buffered(v, buf, cap, prec);
  case float_format::general:
    return __impl::__fpconv::d2g_buffered(v, buf, cap, prec, false, false);
  case float_format::hex:
    return __impl::__fpconv::d2a_buffered(v, buf, cap, prec, has_prec, false);
  default:
    break;
  }
  if ( cap < 26u ) {
    char tmp[26];
    const usize n = __impl::__fpconv::d2s_buffered(v, tmp);
    if ( n > cap ) return 0;
    for ( usize i = 0; i < n; ++i ) buf[i] = tmp[i];
    return n;
  }
  return __impl::__fpconv::d2s_buffered(v, buf);
}

inline usize
to_chars(char *buf, usize cap, f32 v, float_format fmt = float_format::shortest, i32 precision = -1)
{
  if ( buf == nullptr ) return 0;
  if ( fmt != float_format::shortest ) return to_chars(buf, cap, static_cast<f64>(v), fmt, precision);
  if ( cap < 16u ) {
    char tmp[16];
    const usize n = __impl::__fpconv::f2s_buffered(v, tmp);
    if ( n > cap ) return 0;
    for ( usize i = 0; i < n; ++i ) buf[i] = tmp[i];
    return n;
  }
  return __impl::__fpconv::f2s_buffered(v, buf);
}

// four fixed-stride shortest conversions
inline constexpr chars4_result
to_chars4(char *out, usize stride, const f64 (&values)[4]) noexcept
{
  chars4_result result{};
  __impl::__fpconv::d2s_buffered4(values, out, stride, result.lengths);
  return result;
}

inline constexpr chars4_result
to_chars4(char *out, usize stride, const f32 (&values)[4]) noexcept
{
  chars4_result result{};
  __impl::__fpconv::f2s_buffered4(values, out, stride, result.lengths);
  return result;
}

constexpr bool
__tc_leads_space(const char *p, usize n) noexcept
{
  if ( p == nullptr || n == 0 ) return false;
  const char c = p[0];
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr bool
from_chars(f64 &out, const char *p, usize n)
{
  out = 0.0;
  if ( __tc_leads_space(p, n) ) return false;
  return micron::try_parse_double<char>(p, n, out);
}

constexpr bool
from_chars(f32 &out, const char *p, usize n)
{
  out = 0.0f;
  if ( __tc_leads_space(p, n) ) return false;
  return micron::try_parse_float<char>(p, n, out);
}

constexpr usize
to_chars(char *buf, usize cap, bool v)
{
  if ( buf == nullptr ) return 0;
  if ( v ) {
    if ( cap < 4u ) return 0;
    buf[0] = 't';
    buf[1] = 'r';
    buf[2] = 'u';
    buf[3] = 'e';
    return 4;
  }
  if ( cap < 5u ) return 0;
  buf[0] = 'f';
  buf[1] = 'a';
  buf[2] = 'l';
  buf[3] = 's';
  buf[4] = 'e';
  return 5;
}

constexpr bool
from_chars(bool &out, const char *p, usize n)
{
  out = false;
  if ( p == nullptr ) return false;
  if ( n == 1 ) {
    if ( p[0] == '1' ) {
      out = true;
      return true;
    }
    return p[0] == '0';
  }
  if ( n == 4 && p[0] == 't' && p[1] == 'r' && p[2] == 'u' && p[3] == 'e' ) {
    out = true;
    return true;
  }
  if ( n == 5 && p[0] == 'f' && p[1] == 'a' && p[2] == 'l' && p[3] == 's' && p[4] == 'e' ) return true;
  return false;
}

inline usize
to_chars(char *buf, usize cap, const void *v)
{
  if ( buf == nullptr || cap < 3u ) return 0;
  if ( v == nullptr ) {
    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = '0';
    return 3;
  }
  char tmp[24];
  char *tend = tmp + 24;
  char *start = __impl::uint_to_buf_base_backward(tend, reinterpret_cast<u64>(v), 16u, false);
  const usize len = static_cast<usize>(tend - start) + 2u;
  if ( cap < len ) return 0;
  buf[0] = '0';
  buf[1] = 'x';
  usize pos = 2;
  for ( char *q = start; q != tend; ++q ) buf[pos++] = *q;
  return pos;
}

constexpr usize
bytes_to_hex(char *buf, usize cap, const u8 *src, usize n, bool upper = false)
{
  if ( buf == nullptr || src == nullptr ) return 0;
  if ( cap < n * 2u ) return 0;
  const char *tbl = upper ? __impl::__hex_upper : __impl::__hex_lower;
  for ( usize i = 0; i < n; ++i ) {
    buf[2 * i] = tbl[(src[i] >> 4) & 0xFu];
    buf[2 * i + 1] = tbl[src[i] & 0xFu];
  }
  return n * 2u;
}

};      // namespace micron

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "string.hpp"
#include "unitypes.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// unistring & formatting
// for float conversions uses the ryu alg strategy
// for integral conversions uses 10^8 chunked method approach
// gets roughly ~55 cycles per integer (1 to 1 billion)
// ~48 cycles for low numbers (sub 1 mil)

#include "conversions/floating_point.hpp"
#include "conversions/integral.hpp"

namespace micron
{

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// with_capacity

template <typename C = char>
inline micron::hstring<C>
with_capacity(const usize n)
{
  return micron::hstring<C>(n);
};

template <usize N, typename C = char>
inline micron::hstring<C>
with_capacity(void)
{
  return micron::hstring<C>(N);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// invert

template <is_string T>
inline void
invert(T &str)
{
  if ( str.size() <= 1 )
    return;
  typename T::iterator start = str.begin();
  typename T::iterator end = str.end() - 1;
  while ( start < end ) {
    typename T::value_type t = *start;
    *start++ = *end;
    *end-- = t;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// constexpr comptimes

template <typename I, usize N = 24>
  requires(micron::is_integral_v<I> && N >= __impl::max_digits_v<I> + 1)
constexpr micron::sstring<N, char>
constexpr_int_to_string(I n)
{
  micron::sstring<N, char> result;
  char tmp[24];
  usize pos = 0;
  bool neg = false;
  using U = micron::make_unsigned_t<I>;
  U uval;
  if constexpr ( micron::is_signed_v<I> ) {
    if ( n < 0 ) {
      neg = true;
      uval = static_cast<U>(-(n + 1)) + 1u;
    } else {
      uval = static_cast<U>(n);
    }
  } else {
    uval = static_cast<U>(n);
  }
  if ( uval == 0 ) {
    tmp[pos++] = '0';
  } else {
    while ( uval > 0 ) {
      tmp[pos++] = '0' + static_cast<char>(uval % 10);
      uval /= 10;
    }
  }
  char *out = &result[0];
  usize out_pos = 0;
  if ( neg )
    out[out_pos++] = '-';
  for ( usize i = pos; i > 0; --i )
    out[out_pos++] = tmp[i - 1];
  out[out_pos] = '\0';
  result._buf_set_length(out_pos);
  return result;
}

template <typename I, usize N = 24>
  requires(micron::is_integral_v<I> && N >= 19)
constexpr micron::sstring<N, char>
constexpr_hex(I n, bool upper = false)
{
  micron::sstring<N, char> result;
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char tmp[24];
  usize pos = 0;
  if ( uval == 0 ) {
    tmp[pos++] = '0';
  } else {
    while ( uval > 0 ) {
      tmp[pos++] = digits[uval & 0xF];
      uval >>= 4;
    }
  }
  char *out = &result[0];
  for ( usize i = pos; i > 0; --i )
    out[pos - i] = tmp[i - 1];
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

template <typename I, usize N = 68>
  requires(micron::is_integral_v<I> && N >= 67)
constexpr micron::sstring<N, char>
constexpr_bin(I n)
{
  micron::sstring<N, char> result;
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  char tmp[66];
  usize pos = 0;
  if ( uval == 0 ) {
    tmp[pos++] = '0';
  } else {
    while ( uval > 0 ) {
      tmp[pos++] = '0' + static_cast<char>(uval & 1);
      uval >>= 1;
    }
  }
  char *out = &result[0];
  for ( usize i = pos; i > 0; --i )
    out[pos - i] = tmp[i - 1];
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unicode validation

constexpr const char *
u8_check(const char *str, usize n)
{
  const char *end = str + n;
  while ( str < end ) {
    u8 c = static_cast<u8>(*str);
    if ( c < 0x80 ) {
      ++str;
      continue;
    }
    u32 cp = 0;
    usize seq = 0;
    if ( (c & 0xE0) == 0xC0 ) {
      cp = c & 0x1F;
      seq = 2;
    } else if ( (c & 0xF0) == 0xE0 ) {
      cp = c & 0x0F;
      seq = 3;
    } else if ( (c & 0xF8) == 0xF0 ) {
      cp = c & 0x07;
      seq = 4;
    } else
      return nullptr;
    ++str;
    for ( usize j = 1; j < seq; ++j ) {
      if ( str >= end )
        return nullptr;
      u8 cont = static_cast<u8>(*str);
      if ( (cont & 0xC0) != 0x80 )
        return nullptr;
      cp = (cp << 6) | (cont & 0x3F);
      ++str;
    }
    if ( seq == 2 && cp < 0x80 )
      return nullptr;
    if ( seq == 3 && cp < 0x800 )
      return nullptr;
    if ( seq == 4 && cp < 0x10000 )
      return nullptr;
    if ( cp >= 0xD800 && cp <= 0xDFFF )
      return nullptr;
    if ( cp > 0x10FFFF )
      return nullptr;
  }
  return str;
}

constexpr const char16_t *
u16_check(const char16_t *str, usize n)
{
  const char16_t *end = str + n;
  while ( str < end ) {
    u16 c = static_cast<u16>(*str);
    ++str;
    if ( c < 0xD800 || c > 0xDFFF )
      continue;
    if ( c >= 0xD800 && c <= 0xDBFF ) {
      if ( str >= end )
        return nullptr;
      u16 low = static_cast<u16>(*str);
      if ( low < 0xDC00 || low > 0xDFFF )
        return nullptr;
      ++str;
    } else
      return nullptr;
  }
  return str;
}

constexpr const char32_t *
u32_check(const char32_t *str, usize n)
{
  const char32_t *end = str + n;
  while ( str < end ) {
    u32 c = static_cast<u32>(*str);
    ++str;
    if ( c >= 0xD800 && c <= 0xDFFF )
      return nullptr;
    if ( c > 0x10FFFF )
      return nullptr;
  }
  return str;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_strings

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_string(I x)
{
  return int_to_string<I, C>(x);
}

template <typename C>
inline micron::hstring<C>
to_string(const C *str)
{
  return micron::hstring<C>(str);
}

/*
template <usize N, typename C>
inline micron::hstring<C>
to_string(const C (&str)[N])
{
  return micron::hstring<C>(str);
}
*/
template <typename C = char>
inline micron::hstring<C>
to_string_f32(f32 val)
{
  return float_to_string<C>(val);
}

template <typename C = char>
inline micron::hstring<C>
to_string_f64(f64 val)
{
  return double_to_string<C>(val);
}

template <typename C = char>
inline micron::hstring<C>
to_string(f32 val, u32 prec)
{
  return float_to_string<C>(val, prec);
}

template <typename C = char>
inline micron::hstring<C>
to_string(f64 val, u32 prec)
{
  return double_to_string<C>(val, prec);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_string_stack -- stack (sstring) overloads
// Sz controls the buffer size, C controls the character type
// user can write: to_string_stack<i32, 32>(n) or to_string_stack<i32, 32, wchar_t>(n)

template <typename I, usize Sz = 24, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= __impl::max_digits_v<I> + 1)
inline micron::sstring<Sz, C>
to_string_stack(I x)
{
  return int_to_string_stack<I, C, Sz>(x);
}

template <usize Sz, typename C>
inline micron::sstring<Sz, C>
to_string_stack(const C *str)
{
  // copy c-string into stack string
  micron::sstring<Sz, C> result;
  usize len = micron::strlen(str);
  if ( len > Sz - 1 )
    len = Sz - 1;
  C *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = str[i];
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz, usize N, typename C>
  requires(Sz >= N)
inline micron::sstring<Sz, C>
to_string_stack(const C (&str)[N])
{
  micron::sstring<Sz, C> result;
  constexpr usize len = N - 1;
  C *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = str[i];
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <usize Sz = 32, typename C = char>
inline micron::sstring<Sz, C>
to_string_stack(f32 val)
{
  // shortest representation via ryu, then copy to stack
  char tmp[32];
  usize n = __impl::__ryu::d2s_buffered(static_cast<f64>(val), tmp);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 32, typename C = char>
inline micron::sstring<Sz, C>
to_string_stack(f64 val)
{
  // shortest representation via ryu, then copy to stack
  char tmp[32];
  usize n = __impl::__ryu::d2s_buffered(val, tmp);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 48, typename C = char>
inline micron::sstring<Sz, C>
to_string_stack(f32 val, u32 prec)
{
  // fixed-precision via ryu, then copy to stack
  char tmp[48];
  usize n = __impl::__ryu::d2f_buffered(static_cast<f64>(val), tmp, 48, prec);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 64, typename C = char>
inline micron::sstring<Sz, C>
to_string_stack(f64 val, u32 prec)
{
  // fixed-precision via ryu, then copy to stack
  char tmp[64];
  usize n = __impl::__ryu::d2f_buffered(val, tmp, 64, prec);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// float_to_string_stack / double_to_string_stack
// stack overloads for the floating_point.hpp heap functions

template <usize Sz = 32, typename C = char>
inline micron::sstring<Sz, C>
float_to_string_stack(f32 val)
{
  char tmp[32];
  usize n = __impl::__ryu::d2s_buffered(static_cast<f64>(val), tmp);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 32, typename C = char>
inline micron::sstring<Sz, C>
double_to_string_stack(f64 val)
{
  char tmp[32];
  usize n = __impl::__ryu::d2s_buffered(val, tmp);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 48, typename C = char>
inline micron::sstring<Sz, C>
float_to_string_stack(f32 val, u32 prec)
{
  char tmp[48];
  usize n = __impl::__ryu::d2f_buffered(static_cast<f64>(val), tmp, 48, prec);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 64, typename C = char>
inline micron::sstring<Sz, C>
double_to_string_stack(f64 val, u32 prec)
{
  char tmp[64];
  usize n = __impl::__ryu::d2f_buffered(val, tmp, 64, prec);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_fixed_stack / to_scientific_stack / to_general_stack

template <usize Sz = 64, typename C = char>
inline micron::sstring<Sz, C>
to_fixed_stack(f64 val, u32 precision = 6)
{
  char tmp[64];
  usize n = __impl::__ryu::d2f_buffered(val, tmp, 64, precision);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 80, typename C = char>
inline micron::sstring<Sz, C>
to_scientific_stack(f64 val, u32 precision = 6)
{
  char tmp[80];
  usize n = __impl::__ryu::d2e_buffered(val, tmp, 80, precision);
  if ( n > Sz - 1 )
    n = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < n; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[n] = '\0';
  result._buf_set_length(n);
  return result;
}

template <usize Sz = 80, typename C = char>
inline micron::sstring<Sz, C>
to_general_stack(f64 val, u32 precision = 6)
{
  f64 abs_val = val < 0.0 ? -val : val;
  if ( abs_val != 0.0 ) {
    i32 exp = 0;
    f64 tmp = abs_val;
    while ( tmp >= 10.0 ) {
      tmp /= 10.0;
      ++exp;
    }
    while ( tmp < 1.0 ) {
      tmp *= 10.0;
      --exp;
    }
    if ( exp < -4 || exp >= static_cast<i32>(precision) )
      return to_scientific_stack<Sz, C>(val, precision > 0 ? precision - 1 : 0);
  }
  return to_fixed_stack<Sz, C>(val, precision);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_base_stack / uint_to_string_base_stack

template <typename I, usize Sz = 72, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
int_to_string_base_stack(I n, u32 base, bool upper = false)
{
  // convert to tmp via backward-write, copy to stack string
  char tmp[72];
  char *tend = tmp + 72;
  using U = micron::make_unsigned_t<I>;
  U uval;
  bool neg = false;
  if constexpr ( micron::is_signed_v<I> ) {
    if ( n < 0 && base == 10 ) {
      neg = true;
      uval = static_cast<U>(-(n + 1)) + 1u;
    } else {
      uval = static_cast<U>(n);
    }
  } else {
    uval = static_cast<U>(n);
  }
  char *start = __impl::uint_to_buf_base_backward(tend, static_cast<u64>(uval), base, upper);
  if ( neg )
    *--start = '-';
  usize len = static_cast<usize>(tend - start);
  if ( len > Sz - 1 )
    len = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = static_cast<C>(start[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <typename I, usize Sz = 72, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
uint_to_string_base_stack(I n, u32 base, bool upper = false)
{
  using U = micron::make_unsigned_t<I>;
  char tmp[72];
  char *tend = tmp + 72;
  char *start = __impl::uint_to_buf_base_backward(tend, static_cast<u64>(static_cast<U>(n)), base, upper);
  usize len = static_cast<usize>(tend - start);
  if ( len > Sz - 1 )
    len = Sz - 1;
  micron::sstring<Sz, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = static_cast<C>(start[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_hex_stack / to_oct_stack / to_bin_stack

template <typename I, usize Sz = 20, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
to_hex_stack(I n, bool upper = false)
{
  return uint_to_string_base_stack<I, Sz, C>(n, 16, upper);
}

template <typename I, usize Sz = 24, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
to_oct_stack(I n)
{
  return uint_to_string_base_stack<I, Sz, C>(n, 8, false);
}

template <typename I, usize Sz = 68, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
to_bin_stack(I n)
{
  return uint_to_string_base_stack<I, Sz, C>(n, 2, false);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// to_hex_fixed_stack / to_bin_fixed_stack

template <typename I, usize Sz, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
to_hex_fixed_stack(I n, usize digits, bool upper = false)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  usize dlen = digits;
  if ( dlen > Sz - 1 )
    dlen = Sz - 1;
  micron::sstring<Sz, C> buf;
  const char *hex = upper ? __impl::__hex_upper : __impl::__hex_lower;
  C *out = &buf[0];
  for ( usize i = dlen; i > 0; --i ) {
    out[i - 1] = static_cast<C>(hex[uval & 0xF]);
    uval >>= 4;
  }
  out[dlen] = '\0';
  buf._buf_set_length(dlen);
  return buf;
}

template <typename I, usize Sz, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= 2)
inline micron::sstring<Sz, C>
to_bin_fixed_stack(I n, usize digits)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  usize dlen = digits;
  if ( dlen > Sz - 1 )
    dlen = Sz - 1;
  micron::sstring<Sz, C> buf;
  C *out = &buf[0];
  for ( usize i = dlen; i > 0; --i ) {
    out[i - 1] = static_cast<C>('0' + (uval & 1));
    uval >>= 1;
  }
  out[dlen] = '\0';
  buf._buf_set_length(dlen);
  return buf;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_padded_stack

template <typename I, usize Sz, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= __impl::max_digits_v<I> + 1)
inline micron::sstring<Sz, C>
int_to_string_padded_stack(I n, usize width)
{
  // convert to tmp first
  char tmp[24];
  using U = micron::make_unsigned_t<I>;
  U uval;
  bool neg = false;
  if constexpr ( micron::is_signed_v<I> ) {
    if ( n < 0 ) {
      neg = true;
      uval = static_cast<U>(-(n + 1)) + 1u;
    } else {
      uval = static_cast<U>(n);
    }
  } else {
    uval = static_cast<U>(n);
  }
  usize off = 0;
  if ( neg )
    tmp[off++] = '-';
  char dtmp[24];
  char *dend = dtmp + 24;
  char *dstart = __impl::uint_to_buf_backward(dend, static_cast<u64>(uval));
  usize dlen = static_cast<usize>(dend - dstart);
  // raw length = off + dlen
  usize raw_len = off + dlen;

  micron::sstring<Sz, C> result;
  C *out = &result[0];
  usize pos = 0;

  if ( raw_len >= width ) {
    // no padding needed
    if ( neg )
      out[pos++] = '-';
    for ( usize i = 0; i < dlen && pos < Sz - 1; ++i )
      out[pos++] = static_cast<C>(dstart[i]);
  } else {
    usize pad = width - raw_len;
    if ( neg )
      out[pos++] = '-';
    for ( usize i = 0; i < pad && pos < Sz - 1; ++i )
      out[pos++] = '0';
    for ( usize i = 0; i < dlen && pos < Sz - 1; ++i )
      out[pos++] = static_cast<C>(dstart[i]);
  }
  out[pos] = '\0';
  result._buf_set_length(pos);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bytes_to_string / bytes_to_string_stack

template <typename I, typename C = char>
inline micron::hstring<C>
bytes_to_string(I n)
{
  return int_to_string<I, C>(n);
}

template <typename I, usize Sz = 24, typename C = char>
  requires(micron::is_integral_v<I> && Sz >= __impl::max_digits_v<I> + 1)
inline micron::sstring<Sz, C>
bytes_to_string_stack(I n)
{
  return int_to_string_stack<I, C, Sz>(n);
}

};     // namespace micron

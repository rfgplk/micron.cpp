//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"

<<<<<<< HEAD
=======
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>>>>>>> master
// integral conversions
// (uses 10^8 method, minimal divisions)

namespace micron
{
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// string to ints

template <typename T = char>
i64
hex_string_to_int64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  i64 result = 0;
  bool neg = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( ptr != end && *ptr == '+' )
    ++ptr;
  if ( ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') )
    ptr += 2;
  while ( ptr != end ) {
    int dv = __impl::hex_digit_val(*ptr);
    if ( dv < 0 )
      break;
    if ( static_cast<u64>(result) > (0x7FFFFFFFFFFFFFFFull >> 4) )
      break;
    result = (result << 4) | dv;
    ++ptr;
  }
  return neg ? -result : result;
}

template <typename T = char>
u64
hex_string_to_uint64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') )
    ptr += 2;
  while ( ptr != end ) {
    int dv = __impl::hex_digit_val(*ptr);
    if ( dv < 0 )
      break;
    if ( result > (0xFFFFFFFFFFFFFFFFull >> 4) )
      break;
    result = (result << 4) | static_cast<u64>(dv);
    ++ptr;
  }
  return result;
}

template <typename T = char>
u64
oct_string_to_uint64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'o' || *(ptr + 1) == 'O') )
    ptr += 2;
  else if ( ptr != end && *ptr == '0' )
    ++ptr;
  while ( ptr != end ) {
    char c = *ptr;
    if ( c < '0' || c > '7' )
      break;
    if ( result > (0xFFFFFFFFFFFFFFFFull >> 3) )
      break;
    result = (result << 3) | static_cast<u64>(c - '0');
    ++ptr;
  }
  return result;
}

template <typename T = char>
u64
bin_string_to_uint64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'b' || *(ptr + 1) == 'B') )
    ptr += 2;
  while ( ptr != end ) {
    char c = *ptr;
    if ( c != '0' && c != '1' )
      break;
    if ( result > (0xFFFFFFFFFFFFFFFFull >> 1) )
      break;
    result = (result << 1) | static_cast<u64>(c - '0');
    ++ptr;
  }
  return result;
}

template <typename T = char>
i64
string_to_int64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  i64 result = 0;
  bool neg = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( ptr != end && *ptr == '+' )
    ++ptr;
  while ( ptr != end ) {
    char c = *ptr;
    if ( c < '0' || c > '9' )
      break;
    result = result * 10 + (c - '0');
    ++ptr;
  }
  return neg ? -result : result;
}

template <typename T = char>
u64
string_to_uint64(const micron::hstring<T> &buf)
{
  if ( buf.empty() )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '+' )
    ++ptr;
  while ( ptr != end ) {
    char c = *ptr;
    if ( c < '0' || c > '9' )
      break;
    result = result * 10 + static_cast<u64>(c - '0');
    ++ptr;
  }
  return result;
}

template <typename T = char>
i64
string_to_int_base(const micron::hstring<T> &buf, u32 base)
{
  if ( buf.empty() || base < 2 || base > 36 )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  i64 result = 0;
  bool neg = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( ptr != end && *ptr == '+' )
    ++ptr;
  if ( base == 16 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') )
    ptr += 2;
  else if ( base == 8 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'o' || *(ptr + 1) == 'O') )
    ptr += 2;
  else if ( base == 2 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'b' || *(ptr + 1) == 'B') )
    ptr += 2;
  while ( ptr != end ) {
    int dv = __impl::digit_val(*ptr, base);
    if ( dv < 0 )
      break;
    result = result * static_cast<i64>(base) + dv;
    ++ptr;
  }
  return neg ? -result : result;
}

template <typename T = char>
u64
string_to_uint_base(const micron::hstring<T> &buf, u32 base)
{
  if ( buf.empty() || base < 2 || base > 36 )
    return 0;
  const T *ptr = buf.begin(), *end = buf.end();
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '+' )
    ++ptr;
  if ( base == 16 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') )
    ptr += 2;
  else if ( base == 8 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'o' || *(ptr + 1) == 'O') )
    ptr += 2;
  else if ( base == 2 && ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'b' || *(ptr + 1) == 'B') )
    ptr += 2;
  while ( ptr != end ) {
    int dv = __impl::digit_val(*ptr, base);
    if ( dv < 0 )
      break;
    result = result * static_cast<u64>(base) + static_cast<u64>(dv);
    ++ptr;
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// try_* variants

template <typename T = char>
bool
try_string_to_int64(const micron::hstring<T> &buf, i64 &out)
{
  out = 0;
  if ( buf.empty() )
    return false;
  const T *ptr = buf.begin(), *end = buf.end();
  bool neg = false, any = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr == end )
    return false;
  if ( *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( *ptr == '+' )
    ++ptr;
  if ( ptr == end )
    return false;
  i64 acc = 0;
  constexpr i64 lim = -static_cast<i64>(0x7FFFFFFFFFFFFFFF) - 1;
  constexpr i64 lim_div = lim / 10;
  constexpr i64 lim_mod = -(lim % 10);
  while ( ptr != end ) {
    char c = *ptr;
    if ( c < '0' || c > '9' ) {
      while ( ptr != end && *ptr == ' ' )
        ++ptr;
      break;
    }
    int d = c - '0';
    any = true;
    if ( acc < lim_div || (acc == lim_div && d > lim_mod) )
      return false;
    acc = acc * 10 - d;
    ++ptr;
  }
  if ( !any || ptr != end )
    return false;
  if ( neg )
    out = acc;
  else {
    if ( acc == lim )
      return false;
    out = -acc;
  }
  return true;
}

template <typename T = char>
bool
try_string_to_uint64(const micron::hstring<T> &buf, u64 &out)
{
  out = 0;
  if ( buf.empty() )
    return false;
  const T *ptr = buf.begin(), *end = buf.end();
  bool any = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '+' )
    ++ptr;
  if ( ptr == end )
    return false;
  if ( ptr != end && *ptr == '-' )
    return false;
  constexpr u64 mx = 0xFFFFFFFFFFFFFFFFull;
  constexpr u64 mx_d = mx / 10, mx_m = mx % 10;
  u64 acc = 0;
  while ( ptr != end ) {
    char c = *ptr;
    if ( c < '0' || c > '9' ) {
      while ( ptr != end && *ptr == ' ' )
        ++ptr;
      break;
    }
    u64 d = static_cast<u64>(c - '0');
    any = true;
    if ( acc > mx_d || (acc == mx_d && d > mx_m) )
      return false;
    acc = acc * 10 + d;
    ++ptr;
  }
  if ( !any || ptr != end )
    return false;
  out = acc;
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// range variants

template <typename T = char>
i32
string_to_int32(const micron::hstring<T> &buf)
{
  i64 v = string_to_int64(buf);
  return (v < -2147483648LL || v > 2147483647LL) ? 0 : static_cast<i32>(v);
}

template <typename T = char>
u32
string_to_uint32(const micron::hstring<T> &buf)
{
  u64 v = string_to_uint64(buf);
  return (v > 4294967295ULL) ? 0 : static_cast<u32>(v);
}

template <typename T = char>
i16
string_to_int16(const micron::hstring<T> &buf)
{
  i64 v = string_to_int64(buf);
  return (v < -32768LL || v > 32767LL) ? 0 : static_cast<i16>(v);
}

template <typename T = char>
u16
string_to_uint16(const micron::hstring<T> &buf)
{
  u64 v = string_to_uint64(buf);
  return (v > 65535ULL) ? 0 : static_cast<u16>(v);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// offsets

inline i64
parse_int(const char *&ptr, const char *end)
{
  i64 result = 0;
  bool neg = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '-' ) {
    neg = true;
    ++ptr;
  } else if ( ptr != end && *ptr == '+' )
    ++ptr;
  while ( ptr != end && *ptr >= '0' && *ptr <= '9' ) {
    result = result * 10 + (*ptr - '0');
    ++ptr;
  }
  return neg ? -result : result;
}

inline u64
parse_uint(const char *&ptr, const char *end)
{
  u64 result = 0;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '+' )
    ++ptr;
  while ( ptr != end && *ptr >= '0' && *ptr <= '9' ) {
    result = result * 10 + static_cast<u64>(*ptr - '0');
    ++ptr;
  }
  return result;
}

inline i64
parse_hex(const char *&ptr, const char *end)
{
  i64 result = 0;
  bool neg = false;
  while ( ptr != end && *ptr == ' ' )
    ++ptr;
  if ( ptr != end && *ptr == '-' ) {
    neg = true;
    ++ptr;
  }
  if ( ptr + 1 < end && *ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') )
    ptr += 2;
  while ( ptr != end ) {
    int dv = __impl::hex_digit_val(*ptr);
    if ( dv < 0 )
      break;
    result = (result << 4) | dv;
    ++ptr;
  }
  return neg ? -result : result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ints to string
// (uses 10^8 method)

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
int_to_string(I n)
{
  constexpr usize cap = __impl::max_digits_v<I>;
  char tmp[cap];
  char *tend = tmp + cap;
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
  char *start = __impl::uint_to_buf_backward(tend, static_cast<u64>(uval));
  if ( neg )
    *--start = '-';
  usize len = static_cast<usize>(tend - start);
  micron::hstring<T> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
uint_to_string(I n)
{
  using U = micron::make_unsigned_t<I>;
  constexpr usize cap = __impl::max_digits_v<U>;
  char tmp[cap];
  char *tend = tmp + cap;
  char *start = __impl::uint_to_buf_backward(tend, static_cast<u64>(static_cast<U>(n)));
  usize len = static_cast<usize>(tend - start);
  micron::hstring<T> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

template <typename I, typename T = char, usize N>
  requires(micron::is_integral_v<I> && N >= __impl::max_digits_v<I> + 1)
inline micron::sstring<N, T>
int_to_string_stack(I n)
{
  constexpr usize cap = __impl::max_digits_v<I>;
  char tmp[cap];
  char *tend = tmp + cap;
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
  char *start = __impl::uint_to_buf_backward(tend, static_cast<u64>(uval));
  if ( neg )
    *--start = '-';
  usize len = static_cast<usize>(tend - start);
  micron::sstring<N, T> result;
  T *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = static_cast<T>(start[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

template <typename I, typename T = char, usize N>
  requires(micron::is_integral_v<I> && N >= __impl::max_digits_v<micron::make_unsigned_t<I>> + 1)
inline micron::sstring<N, T>
uint_to_string_stack(I n)
{
  using U = micron::make_unsigned_t<I>;
  constexpr usize cap = __impl::max_digits_v<U>;
  char tmp[cap];
  char *tend = tmp + cap;
  char *start = __impl::uint_to_buf_backward(tend, static_cast<u64>(static_cast<U>(n)));
  usize len = static_cast<usize>(tend - start);
  micron::sstring<N, T> result;
  T *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = static_cast<T>(start[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arb base

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
int_to_string_base(I n, u32 base, bool upper = false)
{
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
  micron::hstring<T> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
uint_to_string_base(I n, u32 base, bool upper = false)
{
  using U = micron::make_unsigned_t<I>;
  char tmp[72];
  char *tend = tmp + 72;
  char *start = __impl::uint_to_buf_base_backward(tend, static_cast<u64>(static_cast<U>(n)), base, upper);
  usize len = static_cast<usize>(tend - start);
  micron::hstring<T> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// format/stl compatibility

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_hex(I n, bool upper = false)
{
  return uint_to_string_base<I, T>(n, 16, upper);
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_oct(I n)
{
  return uint_to_string_base<I, T>(n, 8, false);
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_bin(I n)
{
  return uint_to_string_base<I, T>(n, 2, false);
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_hex_fixed(I n, usize digits, bool upper = false)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  micron::hstring<T> buf(digits);
  const char *hex = upper ? __impl::__hex_upper : __impl::__hex_lower;
  for ( usize i = digits; i > 0; --i ) {
    buf[i - 1] = static_cast<T>(hex[uval & 0xF]);
    uval >>= 4;
  }
  buf._buf_set_length(digits);
  return buf;
}

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
to_bin_fixed(I n, usize digits)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  micron::hstring<T> buf(digits);
  for ( usize i = digits; i > 0; --i ) {
    buf[i - 1] = static_cast<T>('0' + (uval & 1));
    uval >>= 1;
  }
  buf._buf_set_length(digits);
  return buf;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// padded width variants (zero)

template <typename I, typename T = char>
  requires micron::is_integral_v<I>
inline micron::hstring<T>
int_to_string_padded(I n, usize width)
{
  micron::hstring<T> raw = int_to_string<I, T>(n);
  if ( raw.size() >= width )
    return raw;
  micron::hstring<T> result(width + 1);
  T *out = &result[0];
  usize content_start = 0;
  bool neg = false;
  if ( raw.size() > 0 && raw[0] == '-' ) {
    neg = true;
    content_start = 1;
  }
  usize pad = width - raw.size();
  usize pos = 0;
  if ( neg )
    out[pos++] = '-';
  for ( usize i = 0; i < pad; ++i )
    out[pos++] = '0';
  for ( usize i = content_start; i < raw.size(); ++i )
    out[pos++] = raw[i];
  result._buf_set_length(pos);
  return result;
}
};     // namespace micron

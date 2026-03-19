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

#include "conversions/bits.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// radix table implementation for int to string conv
// roughly ~52 cycles per int for first 1 billion ints

namespace micron
{
namespace rtable
{

namespace __impl
{

//%%%%%%%%%%%%%%%%%%%%%%%%%
// 40 KB radix lookup table
//
// constexpr-generated at compile time.
// __radix_tbl.d[i] contains the 4-char ASCII representation of i
// for all i in [0, 9999]. total: 10000 * 4 = 40000 bytes.

struct quad {
  char c[4];
};

struct __radix_table {
  quad d[10000];

  constexpr __radix_table() : d{}
  {
    for ( u32 i = 0; i < 10000; ++i ) {
      u32 t = i;
      d[i].c[3] = static_cast<char>('0' + t % 10);
      t /= 10;
      d[i].c[2] = static_cast<char>('0' + t % 10);
      t /= 10;
      d[i].c[1] = static_cast<char>('0' + t % 10);
      t /= 10;
      d[i].c[0] = static_cast<char>('0' + t);
    }
  }
};

inline constexpr __radix_table __radix_tbl{};

//%%%%%%%%%%%%%%%%%%%%%%%%%
// emit exactly 4 digits from table

inline void
emit4(char *buf, u32 v)
{
  // v in [0, 9999]: single indexed load from table
  const char *src = __radix_tbl.d[v].c;
  buf[0] = src[0];
  buf[1] = src[1];
  buf[2] = src[2];
  buf[3] = src[3];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// emit exactly 8 digits via two table lookups

inline void
emit8(char *buf, u32 v)
{
  // v in [0, 99999999]
  u32 hi = static_cast<u32>(micron::__impl::fast_div10000(static_cast<u64>(v)));
  u32 lo = v - hi * 10000;
  emit4(buf, hi);
  emit4(buf + 4, lo);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// emit 1-4 digits (leading block, no leading zeros)

inline usize
emit_lead4(char *buf, u32 v)
{
  const char *src = __radix_tbl.d[v].c;
  if ( v >= 1000 ) {
    buf[0] = src[0];
    buf[1] = src[1];
    buf[2] = src[2];
    buf[3] = src[3];
    return 4;
  }
  if ( v >= 100 ) {
    buf[0] = src[1];
    buf[1] = src[2];
    buf[2] = src[3];
    return 3;
  }
  if ( v >= 10 ) {
    buf[0] = src[2];
    buf[1] = src[3];
    return 2;
  }
  buf[0] = src[3];
  return 1;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// emit 1-8 digits (leading block, no leading zeros)

inline usize
emit_lead8(char *buf, u32 v)
{
  // v in [0, 99999999], nonzero
  if ( v < 10000 )
    return emit_lead4(buf, v);
  u32 hi = static_cast<u32>(micron::__impl::fast_div10000(static_cast<u64>(v)));
  u32 lo = v - hi * 10000;
  usize hlen = emit_lead4(buf, hi);
  emit4(buf + hlen, lo);
  return hlen + 4;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
<<<<<<< HEAD
<<<<<<< HEAD
// full u64 → decimal string via table
//
// splits into 1e8 blocks, uses emit8/emit_lead8.
// uses fast_div1e8 from bits.hpp for block splitting.
// correction step after each division ensures remainder stays in [0, 1e8)
// even if the reciprocal constant underestimates at exact multiples.
// returns number of chars written.
=======
// full u64: decimal string via table
>>>>>>> master
=======
// full u64: decimal string via table
>>>>>>> master

inline usize
rtable_u64(char *buf, u64 val)
{
  if ( val == 0 ) {
    buf[0] = '0';
    return 1;
  }

  constexpr u64 B = 100000000ull;

  if ( val < B ) {
    // single block: 1-8 digits
    return emit_lead8(buf, static_cast<u32>(val));
  }

  if ( val < B * B ) {
    // two blocks: leading (1-8 digits) + fixed (8 digits)
    u64 q = micron::__impl::fast_div1e8(val);
    u32 lo = static_cast<u32>(val - q * B);
<<<<<<< HEAD
<<<<<<< HEAD
    // correction: reciprocal may underestimate at exact multiples
=======
>>>>>>> master
=======
>>>>>>> master
    if ( lo >= B ) {
      ++q;
      lo -= static_cast<u32>(B);
    }
    usize hlen = emit_lead8(buf, static_cast<u32>(q));
    emit8(buf + hlen, lo);
    return hlen + 8;
  }

  // three blocks: leading (1-4 digits) + fixed (8 digits) + fixed (8 digits)
  u64 q1 = micron::__impl::fast_div1e8(val);
  u32 lo = static_cast<u32>(val - q1 * B);
  if ( lo >= B ) {
    ++q1;
    lo -= static_cast<u32>(B);
  }
  u64 q2 = micron::__impl::fast_div1e8(q1);
  u32 mid = static_cast<u32>(q1 - q2 * B);
  if ( mid >= B ) {
    ++q2;
    mid -= static_cast<u32>(B);
  }
  usize hlen = emit_lead8(buf, static_cast<u32>(q2));
  emit8(buf + hlen, mid);
  emit8(buf + hlen + 8, lo);
  return hlen + 16;
}

};     // namespace __impl

<<<<<<< HEAD
<<<<<<< HEAD
//%%%%%%%%%%%%%%%%%%%%%%%%%
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
// int_to_string / uint_to_string

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
int_to_string(I n)
{
<<<<<<< HEAD
<<<<<<< HEAD
  // table-driven decimal conversion
=======
>>>>>>> master
=======
>>>>>>> master
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
  usize dlen = __impl::rtable_u64(tmp + off, static_cast<u64>(uval));
  usize total = off + dlen;

  micron::hstring<C> result(total);
  micron::memcpy(&result[0], tmp, total);
  result._buf_set_length(total);
  return result;
}

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
uint_to_string(I n)
{
  using U = micron::make_unsigned_t<I>;
  char tmp[24];
  usize len = __impl::rtable_u64(tmp, static_cast<u64>(static_cast<U>(n)));
  micron::hstring<C> result(len);
  micron::memcpy(&result[0], tmp, len);
  result._buf_set_length(len);
  return result;
}

<<<<<<< HEAD
<<<<<<< HEAD
//%%%%%%%%%%%%%%%%%%%%%%%%%
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
// int_to_string_stack / uint_to_string_stack

template <typename I, typename C = char, usize N>
  requires(micron::is_integral_v<I> && N >= micron::__impl::max_digits_v<I> + 1)
inline micron::sstring<N, C>
int_to_string_stack(I n)
{
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
  usize dlen = __impl::rtable_u64(tmp + off, static_cast<u64>(uval));
  usize total = off + dlen;

  micron::sstring<N, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < total; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[total] = '\0';
  result._buf_set_length(total);
  return result;
}

template <typename I, typename C = char, usize N>
  requires(micron::is_integral_v<I> && N >= micron::__impl::max_digits_v<micron::make_unsigned_t<I>> + 1)
inline micron::sstring<N, C>
uint_to_string_stack(I n)
{
  using U = micron::make_unsigned_t<I>;
  char tmp[24];
  usize len = __impl::rtable_u64(tmp, static_cast<u64>(static_cast<U>(n)));
  micron::sstring<N, C> result;
  C *out = &result[0];
  for ( usize i = 0; i < len; ++i )
    out[i] = static_cast<C>(tmp[i]);
  out[len] = '\0';
  result._buf_set_length(len);
  return result;
}

<<<<<<< HEAD
<<<<<<< HEAD
//%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_base / uint_to_string_base
// decimal uses radix table, other bases delegate to bits.hpp backward-write
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_base / uint_to_string_base
>>>>>>> master
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_base / uint_to_string_base
>>>>>>> master

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
int_to_string_base(I n, u32 base, bool upper = false)
{
  if ( base == 10 )
    return int_to_string<I, C>(n);

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

  char *start = micron::__impl::uint_to_buf_base_backward(tend, static_cast<u64>(uval), base, upper);
  if ( neg )
    *--start = '-';
  usize len = static_cast<usize>(tend - start);
  micron::hstring<C> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
uint_to_string_base(I n, u32 base, bool upper = false)
{
  if ( base == 10 )
    return uint_to_string<I, C>(n);

  using U = micron::make_unsigned_t<I>;
  char tmp[72];
  char *tend = tmp + 72;
  char *start = micron::__impl::uint_to_buf_base_backward(tend, static_cast<u64>(static_cast<U>(n)), base, upper);
  usize len = static_cast<usize>(tend - start);
  micron::hstring<C> result(len);
  micron::memcpy(&result[0], start, len);
  result._buf_set_length(len);
  return result;
}

<<<<<<< HEAD
<<<<<<< HEAD
//%%%%%%%%%%%%%%%%%%%%%%%%%
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
// to_hex / to_oct / to_bin

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_hex(I n, bool upper = false)
{
  return uint_to_string_base<I, C>(n, 16, upper);
}

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_oct(I n)
{
  return uint_to_string_base<I, C>(n, 8, false);
}

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_bin(I n)
{
  return uint_to_string_base<I, C>(n, 2, false);
}

<<<<<<< HEAD
<<<<<<< HEAD
//%%%%%%%%%%%%%%%%%%%%%%%%%
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
=======
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
>>>>>>> master
// to_hex_fixed / to_bin_fixed

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_hex_fixed(I n, usize digits, bool upper = false)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  micron::hstring<C> buf(digits);
  const char *hex = upper ? micron::__impl::__hex_upper : micron::__impl::__hex_lower;
  for ( usize i = digits; i > 0; --i ) {
    buf[i - 1] = static_cast<C>(hex[uval & 0xF]);
    uval >>= 4;
  }
  buf._buf_set_length(digits);
  return buf;
}

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_bin_fixed(I n, usize digits)
{
  using U = micron::make_unsigned_t<I>;
  U uval = static_cast<U>(n);
  micron::hstring<C> buf(digits);
  for ( usize i = digits; i > 0; --i ) {
    buf[i - 1] = static_cast<C>('0' + (uval & 1));
    uval >>= 1;
  }
  buf._buf_set_length(digits);
  return buf;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// int_to_string_padded

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
int_to_string_padded(I n, usize width)
{
  micron::hstring<C> raw = int_to_string<I, C>(n);
  if ( raw.size() >= width )
    return raw;
  micron::hstring<C> result(width + 1);
  C *out = &result[0];
  usize cstart = 0;
  bool neg = false;
  if ( raw.size() > 0 && raw[0] == '-' ) {
    neg = true;
    cstart = 1;
  }
  usize pad = width - raw.size();
  usize pos = 0;
  if ( neg )
    out[pos++] = '-';
  for ( usize i = 0; i < pad; ++i )
    out[pos++] = '0';
  for ( usize i = cstart; i < raw.size(); ++i )
    out[pos++] = raw[i];
  result._buf_set_length(pos);
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// bytes_to_string

template <typename I, typename C = char>
inline micron::hstring<C>
bytes_to_string(I n)
{
  return int_to_string<I, C>(n);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// to_string

template <typename I, typename C = char>
  requires micron::is_integral_v<I>
inline micron::hstring<C>
to_string(I x)
{
  return int_to_string<I, C>(x);
}

};     // namespace rtable
};     // namespace micron

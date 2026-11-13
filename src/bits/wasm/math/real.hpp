// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../types.hpp"

namespace micron::wasm::math
{
using wide = unsigned __int128;

template<typename F> struct format {
  using uint_type = micron::conditional_t<sizeof(F) == 4, u32, micron::conditional_t<sizeof(F) == 8, u64, wide>>;
  static constexpr unsigned precision = sizeof(F) == 4 ? 24 : sizeof(F) == 8 ? 53 : 113;
  static constexpr unsigned mantissa = precision - 1;
  static constexpr unsigned exponent = sizeof(F) == 4 ? 8 : sizeof(F) == 8 ? 11 : 15;
  static constexpr int bias = (1 << (exponent - 1)) - 1;
  static constexpr unsigned emax = (1u << exponent) - 1;
  static constexpr wide implicit = wide(1) << mantissa;
  static constexpr wide fraction = implicit - 1;
  static constexpr wide sign = wide(1) << (mantissa + exponent);
  static constexpr wide infinity = wide(emax) << mantissa;
  static constexpr wide quiet = implicit >> 1;
};

template<typename F>
inline constexpr wide
bits(F x) noexcept
{
  return __builtin_bit_cast(typename format<F>::uint_type, x);
}

template<typename F>
inline constexpr F
from(wide x) noexcept
{
  return __builtin_bit_cast(F, typename format<F>::uint_type(x));
}

template<typename F>
inline constexpr F
nan() noexcept
{
  return from<F>(format<F>::infinity | format<F>::quiet);
}

template<typename F>
inline constexpr bool
isnan(F x) noexcept
{
  return (bits(x) & ~format<F>::sign) > format<F>::infinity;
}

template<typename F>
inline constexpr bool
isinf(F x) noexcept
{
  return (bits(x) & ~format<F>::sign) == format<F>::infinity;
}

template<typename F>
inline constexpr F
abs(F x) noexcept
{
  return from<F>(bits(x) & ~format<F>::sign);
}

template<typename F>
inline constexpr F
copysign(F x, F y) noexcept
{
  return from<F>((bits(x) & ~format<F>::sign) | (bits(y) & format<F>::sign));
}

inline constexpr int
msb(wide x) noexcept
{
  const u64 hi = u64(x >> 64);
  return hi ? 127 - __builtin_clzll(hi) : u64(x) ? 63 - __builtin_clzll(u64(x)) : -1;
}

inline constexpr wide
jam(wide x, unsigned n) noexcept
{
  return n == 0 ? x : n >= 128 ? wide(x != 0) : (x >> n) | wide((x << (128 - n)) != 0);
}

template<typename F>
inline constexpr F
pack(wide sign, int exponent, wide significand) noexcept
{
  using T = format<F>;
  if ( exponent >= int(T::emax) ) return from<F>(sign | T::infinity);
  if ( exponent <= 0 ) {
    significand = jam(significand, unsigned(1 - exponent));
    exponent = 0;
  }
  const unsigned grs = unsigned(significand & 7);
  wide result = (wide(exponent) << T::mantissa) | ((significand >> 3) & T::fraction);
  if ( grs > 4 || (grs == 4 && ((significand >> 3) & 1)) ) ++result;
  return from<F>(sign | result);
}

struct parts {
  wide significand;
  int exponent;
};

template<typename F>
inline constexpr parts
decompose(F x) noexcept
{
  using T = format<F>;
  const wide b = bits(x) & ~T::sign;
  const int e = int(b >> T::mantissa);
  return { (b & T::fraction) | (e ? T::implicit : 0), (e ? e : 1) - T::bias - int(T::mantissa) };
}

template<typename F>
inline constexpr F
scalbn(F x, int exponent) noexcept
{
  using T = format<F>;
  if ( x == F(0) || isnan(x) || isinf(x) ) return x;
  auto p = decompose(x);
  const int leading = msb(p.significand);
  p.significand <<= T::mantissa - leading;
  if ( exponent > 4 * T::bias ) exponent = 4 * T::bias;
  if ( exponent < -4 * T::bias ) exponent = -4 * T::bias;
  return pack<F>(bits(x) & T::sign, p.exponent + leading + T::bias + exponent, p.significand << 3);
}

template<typename F>
inline constexpr F
frexp(F x, int *exponent) noexcept
{
  using T = format<F>;
  if ( x == F(0) || isnan(x) || isinf(x) ) {
    *exponent = 0;
    return x;
  }
  const auto p = decompose(x);
  const int leading = msb(p.significand);
  *exponent = p.exponent + leading + 1;
  return from<F>((bits(x) & T::sign) | (wide(T::bias - 1) << T::mantissa) | ((p.significand << (T::mantissa - leading)) & T::fraction));
}

template<typename F>
inline constexpr F
trunc(F x) noexcept
{
  using T = format<F>;
  const wide b = bits(x);
  const int e = int((b & ~T::sign) >> T::mantissa) - T::bias;
  if ( e < 0 ) return from<F>(b & T::sign);
  if ( e >= int(T::mantissa) ) return x;
  return from<F>(b & ~((wide(1) << (T::mantissa - e)) - 1));
}

template<typename F>
inline constexpr F
floor(F x) noexcept
{
  F t = trunc(x);
  return t > x ? t - F(1) : t;
}

template<typename F>
inline constexpr F
ceil(F x) noexcept
{
  F t = trunc(x);
  return t < x ? t + F(1) : t;
}

template<typename F>
inline constexpr F
nearest(F x) noexcept
{
  using T = format<F>;
  if ( isnan(x) || isinf(x) || abs(x) >= from<F>(wide(T::bias + T::mantissa) << T::mantissa) ) return x;
  const F a = abs(x), integral = trunc(a), fraction = a - integral;
  const int exponent = int((bits(integral) & ~T::sign) >> T::mantissa) - T::bias;
  const bool odd = exponent >= 0 && ((bits(integral) | T::implicit) >> (T::mantissa - exponent) & 1) != 0;
  return copysign(integral + F(fraction > F(0.5) || (fraction == F(0.5) && odd)), x);
}

template<typename F>
inline constexpr F
round(F x) noexcept
{
  if ( isnan(x) || isinf(x) ) return x;
  F a = abs(x), integral = trunc(a);
  return copysign(integral + F(a - integral >= F(0.5)), x);
}

template<typename F>
inline constexpr F
sqrt(F x) noexcept
{
  using T = format<F>;
  if ( isnan(x) ) return from<F>(bits(x) | T::quiet);
  if ( x == F(0) ) return x;
  if ( x < F(0) ) return nan<F>();
  if ( isinf(x) ) return x;
  auto p = decompose(x);
  int leading = msb(p.significand), exponent = p.exponent + leading;
  p.significand <<= T::mantissa - leading;
  if ( exponent & 1 ) {
    p.significand <<= 1;
    --exponent;
  }
  constexpr unsigned shift = T::mantissa + 4;
  wide root = 0, remainder = 0;
  for ( int pair = int(T::precision + 1); pair >= 0; --pair ) {
    unsigned digits = 0;
    for ( int bit = 1; bit >= 0; --bit ) {
      const int pos = 2 * pair + bit - int(shift);
      digits = (digits << 1) | (pos >= 0 && pos < 128 ? unsigned((p.significand >> pos) & 1) : 0);
    }
    remainder = (remainder << 2) | digits;
    root <<= 1;
    const wide trial = (root << 1) | 1;
    if ( remainder >= trial ) {
      remainder -= trial;
      ++root;
    }
  }
  return pack<F>(0, exponent / 2 + T::bias, (root << 1) | wide(remainder != 0));
}

struct accumulator {
  u64 limb[8]{};
};

inline constexpr accumulator
term(const u64 *words, unsigned count, int offset) noexcept
{
  accumulator out{};
  bool sticky = false;
  for ( unsigned i = 0; i < count * 64; ++i )
    if ( (words[i / 64] >> (i % 64)) & 1 ) {
      const int bit = int(i) + offset;
      if ( bit < 0 )
        sticky = true;
      else if ( bit < 512 )
        out.limb[bit / 64] |= u64(1) << (bit % 64);
    }
  if ( sticky ) out.limb[0] |= 1;
  return out;
}

inline constexpr int
compare(const accumulator &a, const accumulator &b) noexcept
{
  for ( int i = 7; i >= 0; --i )
    if ( a.limb[i] != b.limb[i] ) return a.limb[i] < b.limb[i] ? -1 : 1;
  return 0;
}

inline constexpr accumulator
add(accumulator a, const accumulator &b) noexcept
{
  u64 carry = 0;
  for ( unsigned i = 0; i < 8; ++i ) {
    const wide v = wide(a.limb[i]) + b.limb[i] + carry;
    a.limb[i] = u64(v);
    carry = u64(v >> 64);
  }
  return a;
}

inline constexpr accumulator
subtract(accumulator a, const accumulator &b) noexcept
{
  u64 borrow = 0;
  for ( unsigned i = 0; i < 8; ++i ) {
    const u64 old = a.limb[i], sub = b.limb[i] + borrow;
    borrow = sub < b.limb[i] || old < sub;
    a.limb[i] = old - sub;
  }
  return a;
}

template<typename F>
inline constexpr F
fma(F a, F b, F c) noexcept
{
  using T = format<F>;
  const wide ab = bits(a), bb = bits(b), cb = bits(c), sign = (ab ^ bb) & T::sign;
  if ( isnan(a) ) return from<F>(ab | T::quiet);
  if ( isnan(b) ) return from<F>(bb | T::quiet);
  if ( isnan(c) ) return from<F>(cb | T::quiet);
  if ( isinf(a) || isinf(b) ) {
    if ( a == F(0) || b == F(0) || (isinf(c) && ((cb ^ sign) & T::sign)) ) return nan<F>();
    return from<F>(sign | T::infinity);
  }
  if ( isinf(c) ) return c;
  if ( a == F(0) || b == F(0) ) return c == F(0) ? from<F>(sign & cb & T::sign) : c;
  const auto ap = decompose(a), bp = decompose(b), cp = decompose(c);
  const u64 al = u64(ap.significand), ah = u64(ap.significand >> 64), bl = u64(bp.significand), bh = u64(bp.significand >> 64);
  const wide ll = wide(al) * bl, lh = wide(al) * bh, hl = wide(ah) * bl, hh = wide(ah) * bh;
  const wide mid = (ll >> 64) + u64(lh) + u64(hl), hi = hh + (lh >> 64) + (hl >> 64) + (mid >> 64);
  const u64 product[4]{ u64(ll), u64(mid), u64(hi), u64(hi >> 64) };
  const u64 cwords[2]{ u64(cp.significand), u64(cp.significand >> 64) };
  const int product_msb = hi ? 128 + msb(hi) : 64 + (u64(mid) ? 63 - __builtin_clzll(u64(mid)) : -64 + msb(wide(u64(ll))));
  const int pe = ap.exponent + bp.exponent;
  const int top_p = pe + product_msb, top_c = cp.significand ? cp.exponent + msb(cp.significand) : top_p;
  const int top = top_p > top_c ? top_p : top_c;
  auto p = term(product, 4, 500 + pe - top), q = term(cwords, 2, 500 + cp.exponent - top);
  wide result_sign = sign;
  if ( (cb & T::sign) == sign )
    p = add(p, q);
  else if ( compare(p, q) < 0 ) {
    p = subtract(q, p);
    result_sign = cb & T::sign;
  } else
    p = subtract(p, q);
  int leading = -1;
  for ( int i = 7; i >= 0; --i )
    if ( p.limb[i] ) {
      leading = i * 64 + 63 - __builtin_clzll(p.limb[i]);
      break;
    }
  if ( leading < 0 ) return F(0);
  const int shift = leading - int(T::mantissa) - 3;
  wide significand = 0;
  bool sticky = false;
  for ( int i = 0; i <= leading; ++i )
    if ( (p.limb[i / 64] >> (i % 64)) & 1 ) {
      if ( i < shift )
        sticky = true;
      else
        significand |= wide(1) << (i - shift);
    }
  if ( sticky ) significand |= 1;
  return pack<F>(result_sign, top - 500 + leading + T::bias, significand);
}
};      // namespace micron::wasm::math

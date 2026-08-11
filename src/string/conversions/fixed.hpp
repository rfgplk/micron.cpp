//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits.hpp"
#include "decimal.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// float -> text at a set precision: %f, %e, %a

namespace micron
{
namespace __impl
{
namespace __ryu
{

namespace __fx
{

// %%%%%%%%%%%%%%%%%%%%%%%%%
// decomposition

struct parts {
  u64 m2 = 0;      // full significand, implicit bit restored
  i32 e2 = 0;      // value == m2 * 2^e2
  bool neg = false;
  bool is_zero = false;
  bool is_inf = false;
  bool is_nan = false;
};

inline constexpr parts
__decompose(f64 v) noexcept
{
  const u64 bits = micron::math::ieee::to_bits<f64>(v);
  const u32 be = static_cast<u32>((bits >> 52) & 0x7FFu);
  const u64 mf = bits & ((1ull << 52) - 1);

  parts p{};
  p.neg = (bits >> 63) != 0;
  if ( be == 0x7FFu ) {
    p.is_inf = (mf == 0);
    p.is_nan = (mf != 0);
    return p;
  }
  if ( be == 0 ) {
    if ( mf == 0 ) {
      p.is_zero = true;
      return p;
    }
    p.m2 = mf;      // subnormal: no implicit bit
    p.e2 = 1 - 1023 - 52;
    return p;
  }
  p.m2 = mf | (1ull << 52);
  p.e2 = static_cast<i32>(be) - 1023 - 52;
  return p;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// exact u64 kernel
//
// v = m2 * 2^e2 splits into an integer and a fractional half by pure bit operations, both exact:
//     e2 >= 0   ip = m2 << e2                      frac = 0
//     e2 <  0   ip = m2 >> t, frac = m2 & (2^t-1)  with t = -e2

constexpr static const u64 __pow5_u64[20]
    = { 1ull,          5ull,           25ull,           125ull,          625ull,           3125ull,          15625ull,
        78125ull,      390625ull,      1953125ull,      9765625ull,      48828125ull,      244140625ull,     1220703125ull,
        6103515625ull, 30517578125ull, 152587890625ull, 762939453125ull, 3814697265625ull, 19073486328125ull };

constexpr static const u64 __pow10_u64[20] = { 1ull,
                                               10ull,
                                               100ull,
                                               1000ull,
                                               10000ull,
                                               100000ull,
                                               1000000ull,
                                               10000000ull,
                                               100000000ull,
                                               1000000000ull,
                                               10000000000ull,
                                               100000000000ull,
                                               1000000000000ull,
                                               10000000000000ull,
                                               100000000000000ull,
                                               1000000000000000ull,
                                               10000000000000000ull,
                                               100000000000000000ull,
                                               1000000000000000000ull,
                                               10000000000000000000ull };

constexpr static const u32 __max_u64_prec = 19;

struct fixed64 {
  u64 ip = 0;
  u64 fp = 0;
  bool ok = false;
};

inline constexpr fixed64
__fixed_u64(u64 m2, i32 e2, u32 p) noexcept
{
  fixed64 r{};
  if ( p > __max_u64_prec ) return r;

  u64 ip = 0, frac = 0;
  u32 t = 0;
  if ( e2 >= 0 ) {
    if ( e2 >= 64 ) return r;      // |v| >= 2^64
    if ( e2 > 0 && (m2 >> (64 - static_cast<u32>(e2))) != 0 ) return r;
    ip = m2 << e2;
  } else {
    t = static_cast<u32>(-e2);
    if ( t >= 64 ) {
      ip = 0;
      frac = m2;      // the whole significand is fractional
    } else {
      ip = m2 >> t;
      frac = m2 & ((1ull << t) - 1);
    }
  }

  u64 fp = 0;
  bool up = false;
  if ( frac != 0 && p > 0 ) {
    if ( t <= p ) {
      // fp = frac * 5^p * 2^(p-t)
      fp = frac * __pow5_u64[p];
      fp <<= (p - t);
    } else {
      const u32 u = t - p;
      if ( u >= 99 ) {
        fp = 0;      // product < 2^98 <= 2^(u-1): strictly below half, rounds down
      } else {
        const __fmt_uint128_t prod = umul128(frac, __pow5_u64[p]);
        const u64 lo = prod.lo;      // a plain {lo,hi} struct on every arch, not a builtin
        const u64 hi = prod.hi;
        u64 rem_hi = 0, rem_lo = 0;
        if ( u < 64 ) {
          fp = (u == 0) ? lo : ((hi << (64 - u)) | (lo >> u));
          rem_lo = (u == 0) ? 0ull : (lo & ((1ull << u) - 1));
        } else {
          const u32 s = u - 64;
          fp = (s == 0) ? hi : (hi >> s);
          rem_hi = (s == 0) ? 0ull : (hi & ((1ull << s) - 1));
          rem_lo = lo;
        }
        // half == 2^(u-1); compare the remainder against it
        if ( u < 64 ) {
          const u64 half = 1ull << (u - 1);
          up = (rem_lo > half) || (rem_lo == half && ((p == 0 ? ip : fp) & 1) != 0);
        } else {
          const u32 s = u - 64;
          // half sits at bit (u-1): in rem_hi when s > 0, else the top bit of rem_lo
          if ( s == 0 ) {
            const u64 half = 1ull << 63;
            up = (rem_lo > half) || (rem_lo == half && ((p == 0 ? ip : fp) & 1) != 0);
          } else {
            const u64 half_hi = 1ull << (s - 1);
            up = (rem_hi > half_hi) || (rem_hi == half_hi && rem_lo != 0)
                 || (rem_hi == half_hi && rem_lo == 0 && ((p == 0 ? ip : fp) & 1) != 0);
          }
        }
      }
    }
  } else if ( frac != 0 && p == 0 ) {
    // rounding the fraction away entirely: compare frac against half of 2^t
    if ( t < 64 ) {
      const u64 half = 1ull << (t - 1);
      up = (frac > half) || (frac == half && (ip & 1) != 0);
    } else {
      // t >= 64 means |v| < 1 and frac == m2 < 2^53 < 2^(t-1): below half
      up = false;
    }
  }

  if ( up ) ++fp;
  if ( p > 0 && fp == __pow10_u64[p] ) {      // the fraction carried into the integer
    fp = 0;
    if ( ip == ~0ull ) return r;
    ++ip;
  } else if ( p == 0 && up ) {
    if ( ip == ~0ull ) return r;
    ++ip;
  }

  r.ip = ip;
  r.fp = fp;
  r.ok = true;
  return r;
}

inline constexpr usize
__emit_fixed64(char *buf, usize buf_sz, bool neg, u64 ip, u64 fp, u32 p) noexcept
{
  char itmp[24];
  char *iend = itmp + 24;
  char *ist = uint_to_buf_backward(iend, ip);
  const usize ilen = static_cast<usize>(iend - ist);

  const usize need = (neg ? 1u : 0u) + ilen + (p > 0 ? 1u + static_cast<usize>(p) : 0u);
  if ( buf_sz < need ) return 0;

  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  for ( usize i = 0; i < ilen; ++i ) buf[pos++] = ist[i];
  if ( p > 0 ) {
    buf[pos++] = '.';
    char ftmp[24];
    char *fend = ftmp + 24;
    char *fst = uint_to_buf_backward(fend, fp);
    const usize flen = static_cast<usize>(fend - fst);
    for ( usize i = flen; i < p; ++i ) buf[pos++] = '0';      // left-pad to exactly p places
    for ( usize i = 0; i < flen; ++i ) buf[pos++] = fst[i];
  }
  return pos;
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// exact digits

constexpr static const u32 __dig_cap = micron::__impl::__dec::__dec_cap + 2;

enum class cut : u8 { significant, fraction };

[[gnu::noinline, gnu::cold]] inline constexpr void
__exact_round(u64 m2, i32 e2, cut mode, i32 arg, char *dig, u32 &n, i32 &pt) noexcept
{
  micron::__impl::__dec::__decimal v;
  micron::__impl::__dec::__dec_load_u64(v, m2);
  micron::__impl::__dec::__dec_shift(v, e2);

  pt = v.point;
  n = 0;
  if ( v.ndigits == 0 ) return;      // exact zero

  const i32 keep = (mode == cut::significant) ? arg : v.point + arg;
  if ( keep < 0 ) return;      // every digit is past the cut: rounds to zero

  u32 k = static_cast<u32>(keep);
  if ( k >= v.ndigits ) {      // nothing is cut, so nothing rounds
    for ( u32 i = 0; i < v.ndigits; ++i ) dig[i] = static_cast<char>('0' + v.d[i]);
    n = v.ndigits;
    return;
  }

  const bool up = micron::__impl::__dec::__dec_round_up(v, k);
  for ( u32 i = 0; i < k; ++i ) dig[i] = static_cast<char>('0' + v.d[i]);
  n = k;
  if ( !up ) return;

  for ( u32 i = k; i-- > 0; ) {
    if ( dig[i] != '9' ) {
      ++dig[i];
      return;
    }
    dig[i] = '0';
  }
  // the carry escaped the leading digit: 0.999.. -> 0.100.. * 10^(pt+1)
  dig[0] = '1';
  if ( n == 0 ) n = 1;
  ++pt;
}

// %%%%%%%%%%%%%%%%%%%%%%
// emitters

inline constexpr usize
__emit_special(const parts &p, char *buf, usize buf_sz) noexcept
{
  if ( p.is_nan ) {
    if ( buf_sz < 3 ) return 0;
    buf[0] = 'N';
    buf[1] = 'a';
    buf[2] = 'N';
    return 3;
  }
  // inf
  const usize need = p.neg ? 4u : 3u;
  if ( buf_sz < need ) return 0;
  usize pos = 0;
  if ( p.neg ) buf[pos++] = '-';
  buf[pos++] = 'I';
  buf[pos++] = 'n';
  buf[pos++] = 'f';
  return pos;
}

inline constexpr usize
__emit_fixed(char *buf, usize buf_sz, bool neg, const char *dig, u32 n, i32 pt, u32 prec) noexcept
{
  const usize idig = (pt > 0) ? static_cast<usize>(pt) : 1u;
  const usize need = (neg ? 1u : 0u) + idig + (prec > 0 ? 1u + static_cast<usize>(prec) : 0u);
  if ( buf_sz < need ) return 0;

  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  if ( pt <= 0 )
    buf[pos++] = '0';
  else
    for ( i32 i = 0; i < pt; ++i ) buf[pos++] = (static_cast<u32>(i) < n) ? dig[i] : '0';

  if ( prec > 0 ) {
    buf[pos++] = '.';
    for ( u32 j = 0; j < prec; ++j ) {
      const i32 idx = pt + static_cast<i32>(j);
      buf[pos++] = (idx >= 0 && static_cast<u32>(idx) < n) ? dig[idx] : '0';
    }
  }
  return pos;
}

inline constexpr usize
__emit_sci(char *buf, usize buf_sz, bool neg, const char *dig, u32 n, i32 sci_exp, u32 prec) noexcept
{
  u32 edig = 2;
  u32 ae = static_cast<u32>(sci_exp < 0 ? -sci_exp : sci_exp);
  if ( ae >= 100 ) edig = 3;
  const usize need = (neg ? 1u : 0u) + 1u + (prec > 0 ? 1u + static_cast<usize>(prec) : 0u) + 2u + edig;
  if ( buf_sz < need ) return 0;

  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  buf[pos++] = (n > 0) ? dig[0] : '0';
  if ( prec > 0 ) {
    buf[pos++] = '.';
    for ( u32 j = 1; j <= prec; ++j ) buf[pos++] = (j < n) ? dig[j] : '0';
  }
  buf[pos++] = 'e';
  buf[pos++] = (sci_exp < 0) ? '-' : '+';
  if ( edig == 3 ) {
    buf[pos++] = static_cast<char>('0' + ae / 100);
    ae %= 100;
  }
  buf[pos++] = __digit_tbl[ae].d[0];
  buf[pos++] = __digit_tbl[ae].d[1];
  return pos;
}

};      // namespace __fx

// buffer sizing
constexpr static const usize __fmt_fixed_max = 1100;

inline constexpr usize
d2f_size(f64 val, u32 precision) noexcept
{
  const __fx::parts p = __fx::__decompose(val);
  if ( p.is_nan ) return 3;
  if ( p.is_inf ) return p.neg ? 4u : 3u;

  // integer digits, from the values binary exponent
  usize idig = 1;
  if ( !p.is_zero ) {
    const i32 bexp = p.e2 + static_cast<i32>(64u - micron::__impl::clz64(p.m2));
    if ( bexp > 0 ) idig = static_cast<usize>(log10_pow2(bexp)) + 1u;
  }
  return (p.neg ? 1u : 0u) + idig + (precision > 0 ? 1u + static_cast<usize>(precision) : 0u);
}

inline constexpr usize
d2e_size(f64 val, u32 precision) noexcept
{
  const __fx::parts p = __fx::__decompose(val);
  if ( p.is_nan ) return 3;
  if ( p.is_inf ) return p.neg ? 4u : 3u;
  return (p.neg ? 1u : 0u) + 1u + (precision > 0 ? 1u + static_cast<usize>(precision) : 0u) + 5u;
}

inline constexpr usize
d2f_buffered(f64 val, char *buf, usize buf_sz, u32 precision)
{
  const __fx::parts p = __fx::__decompose(val);
  if ( p.is_nan || p.is_inf ) return __fx::__emit_special(p, buf, buf_sz);
  if ( p.is_zero ) return __fx::__emit_fixed(buf, buf_sz, p.neg, nullptr, 0, 0, precision);

  const __fx::fixed64 k = __fx::__fixed_u64(p.m2, p.e2, precision);
  if ( k.ok ) return __fx::__emit_fixed64(buf, buf_sz, p.neg, k.ip, k.fp, precision);

  char dig[__fx::__dig_cap];
  u32 n = 0;
  i32 pt = 0;
  __fx::__exact_round(p.m2, p.e2, __fx::cut::fraction, static_cast<i32>(precision), dig, n, pt);
  return __fx::__emit_fixed(buf, buf_sz, p.neg, dig, n, pt, precision);
}

inline constexpr usize
d2e_buffered(f64 val, char *buf, usize buf_sz, u32 precision)
{
  const __fx::parts p = __fx::__decompose(val);
  if ( p.is_nan || p.is_inf ) return __fx::__emit_special(p, buf, buf_sz);
  if ( p.is_zero ) return __fx::__emit_sci(buf, buf_sz, p.neg, nullptr, 0, 0, precision);

  char dig[__fx::__dig_cap];
  u32 n = 0;
  i32 pt = 0;
  // %e keeps precision+1 significant digits
  __fx::__exact_round(p.m2, p.e2, __fx::cut::significant, static_cast<i32>(precision) + 1, dig, n, pt);
  return __fx::__emit_sci(buf, buf_sz, p.neg, dig, n, pt - 1, precision);
}

// %%%%%%%%%%%%%%
// %g
inline constexpr usize
__d2g_core(f64 val, char *buf, usize buf_sz, u32 precision, bool alt, bool upper)
{
  const __fx::parts p = __fx::__decompose(val);
  if ( p.is_nan || p.is_inf ) return __fx::__emit_special(p, buf, buf_sz);

  const u32 P = (precision == 0) ? 1u : precision;

  char dig[__fx::__dig_cap];
  u32 n = 0;
  i32 pt = 0;
  if ( !p.is_zero ) __fx::__exact_round(p.m2, p.e2, __fx::cut::significant, static_cast<i32>(P), dig, n, pt);
  const i32 X = p.is_zero ? 0 : (pt - 1);

  usize len = 0;
  if ( X >= -4 && X < static_cast<i32>(P) )
    len = __fx::__emit_fixed(buf, buf_sz, p.neg, dig, n, pt, static_cast<u32>(static_cast<i32>(P) - 1 - X));
  else
    len = __fx::__emit_sci(buf, buf_sz, p.neg, dig, n, X, P - 1u);
  if ( len == 0 ) return 0;

  if ( !alt ) {
    usize dot = len, epos = len;
    for ( usize i = 0; i < len; ++i ) {
      if ( buf[i] == '.' )
        dot = i;
      else if ( buf[i] == 'e' ) {
        epos = i;
        break;
      }
    }
    if ( dot != len ) {
      usize end = epos;
      while ( end > dot + 1 && buf[end - 1] == '0' ) --end;
      if ( end == dot + 1 ) end = dot;
      if ( end != epos ) {
        usize w = end;
        for ( usize i = epos; i < len; ++i ) buf[w++] = buf[i];      // pull the exponent down
        len = w;
      }
    }
  }
  if ( upper )
    for ( usize i = 0; i < len; ++i )
      if ( buf[i] == 'e' ) buf[i] = 'E';
  return len;
}

inline constexpr usize
d2g_size(f64 val, u32 precision, bool alt) noexcept
{
  char scratch[__fmt_fixed_max];
  return __d2g_core(val, scratch, __fmt_fixed_max, precision, alt, false);
}

inline constexpr usize
d2g_buffered(f64 val, char *buf, usize buf_sz, u32 precision, bool alt, bool upper)
{
  char scratch[__fmt_fixed_max];
  const usize len = __d2g_core(val, scratch, __fmt_fixed_max, precision, alt, upper);
  if ( len == 0 || len > buf_sz ) return 0;
  for ( usize i = 0; i < len; ++i ) buf[i] = scratch[i];
  return len;
}

// %%%%%%%%%%%%%%%%%%%
// %a
// (exact hex float)
//
// glibc-compatible
//   1.0 -> 0x1p+0    pi -> 0x1.921fb54442d11p+1    -0.0 -> -0x0p+0    0.0 -> 0x0p+0
inline constexpr usize
d2a_buffered(f64 val, char *buf, usize buf_sz, u32 precision, bool has_prec, bool upper)
{
  const u64 bits = micron::math::ieee::to_bits<f64>(val);
  const u32 be = static_cast<u32>((bits >> 52) & 0x7FFu);
  const u64 mf = bits & ((1ull << 52) - 1);
  const bool neg = (bits >> 63) != 0;

  if ( be == 0x7FFu ) {
    __fx::parts sp{};
    sp.neg = neg;
    sp.is_nan = (mf != 0);
    sp.is_inf = (mf == 0);
    return __fx::__emit_special(sp, buf, buf_sz);
  }

  u32 lead = 0;
  i32 exp2 = 0;
  if ( be == 0 ) {
    lead = 0;
    exp2 = (mf == 0) ? 0 : -1022;
  } else {
    lead = 1;
    exp2 = static_cast<i32>(be) - 1023;
  }

  u8 nib[13] = {};
  for ( u32 i = 0; i < 13; ++i ) nib[i] = static_cast<u8>((mf >> (48u - 4u * i)) & 0xFull);

  u32 ndig = 13;
  if ( has_prec ) {
    if ( precision < 13u ) {
      const u32 shift = 52u - 4u * precision;
      const u64 disc = mf & ((1ull << shift) - 1);
      const u64 half = 1ull << (shift - 1);
      const u32 last = (precision == 0) ? lead : nib[precision - 1];
      if ( disc > half || (disc == half && (last & 1u) != 0u) ) {
        u32 i = precision;
        bool carry = true;
        while ( carry && i-- > 0 ) {
          if ( nib[i] == 0xFu )
            nib[i] = 0;
          else {
            ++nib[i];
            carry = false;
          }
        }
        if ( carry ) ++lead;      // 0x1.fff.. -> 0x2.000..
      }
    }
    ndig = precision;
  } else {
    while ( ndig > 0 && nib[ndig - 1] == 0 ) --ndig;      // trim, and drop a bare '.'
  }

  const char *tbl = upper ? __hex_upper : __hex_lower;
  u32 ae = static_cast<u32>(exp2 < 0 ? -exp2 : exp2);
  u32 edig = 1;
  for ( u32 t = ae; t >= 10u; t /= 10u ) ++edig;

  const usize need = (neg ? 1u : 0u) + 2u + 1u + (ndig > 0 ? 1u + static_cast<usize>(ndig) : 0u) + 1u + 1u + edig;
  if ( buf_sz < need ) return 0;

  usize pos = 0;
  if ( neg ) buf[pos++] = '-';
  buf[pos++] = '0';
  buf[pos++] = upper ? 'X' : 'x';
  buf[pos++] = static_cast<char>('0' + lead);
  if ( ndig > 0 ) {
    buf[pos++] = '.';
    for ( u32 i = 0; i < ndig; ++i ) buf[pos++] = (i < 13u) ? tbl[nib[i]] : '0';
  }
  buf[pos++] = upper ? 'P' : 'p';
  buf[pos++] = (exp2 < 0) ? '-' : '+';
  {
    char tmp[8];
    u32 t = edig;
    u32 v = ae;
    while ( t-- > 0 ) {
      tmp[t] = static_cast<char>('0' + v % 10u);
      v /= 10u;
    }
    for ( u32 i = 0; i < edig; ++i ) buf[pos++] = tmp[i];
  }
  return pos;
}

#if defined(__micron_has_wide_float)
template<typename F>
inline usize
x2a_buffered(F val, char *buf, usize buf_sz)
{
  if constexpr ( sizeof(F) <= 8 ) {
    return d2a_buffered(static_cast<f64>(val), buf, buf_sz, 0, false, false);
  } else {
    unsigned char raw[sizeof(F)] = {};
    __builtin_memcpy(raw, &val, sizeof(F));      // every arch micron targets is little-endian

    u64 lo = 0, hi = 0;
    for ( u32 i = 0; i < 8; ++i ) lo |= static_cast<u64>(raw[i]) << (8u * i);

    u32 se = 0;
    i32 shift = 0;
    bool nan_bit = false;
#if defined(__micron_ldbl_x87_80)
    if constexpr ( micron::is_same_v<F, long double> ) {
      se = static_cast<u32>(raw[8]) | (static_cast<u32>(raw[9]) << 8);
      shift = 63;
      nan_bit = (lo & 0x7FFFFFFFFFFFFFFFull) != 0;
    } else
#endif
    {
      for ( u32 i = 8; i < 16 && i < sizeof(F); ++i ) hi |= static_cast<u64>(raw[i]) << (8u * (i - 8u));
      se = static_cast<u32>(hi >> 48);
      hi &= 0x0000FFFFFFFFFFFFull;
      shift = 112;
      nan_bit = (hi | lo) != 0;
      if ( (se & 0x7FFFu) != 0 && (se & 0x7FFFu) != 0x7FFFu ) hi |= (1ull << 48);      // implicit bit
    }

    const bool neg = (se & 0x8000u) != 0;
    const u32 e = se & 0x7FFFu;

    if ( e == 0x7FFFu ) {
      __fx::parts sp{};
      sp.neg = neg;
      sp.is_nan = nan_bit;
      sp.is_inf = !nan_bit;
      return __fx::__emit_special(sp, buf, buf_sz);
    }

    i32 exp2 = (e == 0 ? 1 : static_cast<i32>(e)) - 16383 - shift;

    if ( (hi | lo) == 0 ) {
      const usize need = (neg ? 1u : 0u) + 6u;
      if ( buf_sz < need ) return 0;
      usize z = 0;
      if ( neg ) buf[z++] = '-';
      buf[z++] = '0';
      buf[z++] = 'x';
      buf[z++] = '0';
      buf[z++] = 'p';
      buf[z++] = '+';
      buf[z++] = '0';
      return z;
    }

    while ( (lo & 0xFull) == 0 ) {      // strip trailing zero nibbles, exact
      lo = (lo >> 4) | (hi << 60);
      hi >>= 4;
      exp2 += 4;
    }

    char tmp[40];
    char *tend = tmp + 40;
    char *start = micron::__impl::uint128_to_buf_base_backward(tend, hi, lo, 16u, false);
    const usize hlen = static_cast<usize>(tend - start);

    u32 ae = static_cast<u32>(exp2 < 0 ? -exp2 : exp2);
    u32 edig = 1;
    for ( u32 t = ae; t >= 10u; t /= 10u ) ++edig;

    const usize need = (neg ? 1u : 0u) + 2u + hlen + 2u + edig;
    if ( buf_sz < need ) return 0;

    usize pos = 0;
    if ( neg ) buf[pos++] = '-';
    buf[pos++] = '0';
    buf[pos++] = 'x';
    for ( char *q = start; q != tend; ++q ) buf[pos++] = *q;
    buf[pos++] = 'p';
    buf[pos++] = (exp2 < 0) ? '-' : '+';
    {
      char et[8];
      u32 t = edig;
      u32 v = ae;
      while ( t-- > 0 ) {
        et[t] = static_cast<char>('0' + v % 10u);
        v /= 10u;
      }
      for ( u32 i = 0; i < edig; ++i ) buf[pos++] = et[i];
    }
    return pos;
  }
}
#endif

inline constexpr usize
d2f_trim_buffered(f64 val, char *buf, usize buf_sz, u32 precision)
{
  usize n = d2f_buffered(val, buf, buf_sz, precision);
  usize dot = n;
  for ( usize i = 0; i < n; ++i ) {
    if ( buf[i] == '.' ) {
      dot = i;
      break;
    }
  }
  usize end = n;
  if ( dot != n ) {                                            // has a fractional part -> trim it
    while ( end > dot + 1 && buf[end - 1] == '0' ) --end;      // strip trailing zeros
    if ( end == dot + 1 ) end = dot;                           // drop a bare trailing '.'
  }
  if ( end >= 1 && buf[0] == '-' ) {
    bool __all_zero = true;
    for ( usize i = 1; i < end; ++i )
      if ( buf[i] != '0' && buf[i] != '.' ) {
        __all_zero = false;
        break;
      }
    if ( __all_zero ) {
      for ( usize i = 1; i < end; ++i ) buf[i - 1] = buf[i];      // shift left over the leading '-'
      --end;
    }
  }
  return end;
}

};      // namespace __ryu
};      // namespace __impl
};      // namespace micron

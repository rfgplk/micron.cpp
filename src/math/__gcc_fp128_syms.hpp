//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// FREESTANDING LIBGCC BINARY128 SOFT-FLOAT SYMBOLS
//
// WARNING: NOT FOR EXTERNAL USE. THE COMPILERS LOWERS THESE OPS TO LIBGCC SOFT FP CALLS
// ON AARCH64 LONG DOUBLE IS BINARY128; AMD64 ITS __FLOAT128
//
// ADD/SUB/MUL/DIV ARE TRUE BINARY128 SOFT FP WITH ROUND TO NEAREST EVEN, EXCEPT THAT SUBNORMAL RESULTS FLUSH TO SIGNED ZERO (FTZ)

#include "../types.hpp"

#if defined(__SIZEOF_INT128__) && defined(__FLT128_MANT_DIG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

// IEEE-754 binary128 soft float runtime helpers
namespace
{
using __tf_rep = unsigned __int128;

inline constexpr int __tf_mantbits = 112;      // stored mantissa bits
inline constexpr int __tf_bias = 16383;
inline constexpr unsigned __tf_einf = 0x7FFFu;                                            // all-ones exponent field
inline constexpr __tf_rep __tf_implicit = static_cast<__tf_rep>(1) << __tf_mantbits;      // 2^112
inline constexpr __tf_rep __tf_mantmask = __tf_implicit - 1;                              // low 112 bits
inline constexpr __tf_rep __tf_signbit = static_cast<__tf_rep>(1) << 127;
inline constexpr __tf_rep __tf_absmask = ~__tf_signbit;
inline constexpr __tf_rep __tf_infrep = static_cast<__tf_rep>(__tf_einf) << __tf_mantbits;
inline constexpr __tf_rep __tf_qnanbit = static_cast<__tf_rep>(1) << (__tf_mantbits - 1);
inline constexpr __tf_rep __tf_qnan = __tf_infrep | __tf_qnanbit;

[[gnu::always_inline]] inline __tf_rep
__tf_bits(_Float128 x) noexcept
{
  return __builtin_bit_cast(__tf_rep, x);
}

[[gnu::always_inline]] inline _Float128
__tf_from(__tf_rep b) noexcept
{
  return __builtin_bit_cast(_Float128, b);
}

[[gnu::always_inline]] inline int
__tf_clz(__tf_rep x) noexcept      // precondition: x != 0
{
  const unsigned long long hi = static_cast<unsigned long long>(x >> 64);
  if ( hi ) return __builtin_clzll(hi);
  return 64 + __builtin_clzll(static_cast<unsigned long long>(x));
}

// shift a (sub)normal significand so its leading 1 lands at bit __tf_mantbits
[[gnu::always_inline]] inline int
__tf_normalize(__tf_rep &sig) noexcept
{
  const int shift = __tf_clz(sig) - (127 - __tf_mantbits);
  sig <<= shift;
  return 1 - shift;
}

inline _Float128
__tf_addsub(__tf_rep a, __tf_rep b) noexcept
{
  __tf_rep aAbs = a & __tf_absmask;
  __tf_rep bAbs = b & __tf_absmask;

  if ( aAbs >= __tf_infrep || bAbs >= __tf_infrep || aAbs == 0 || bAbs == 0 ) {
    if ( aAbs > __tf_infrep ) return __tf_from(a | __tf_qnanbit);      // a is NaN -> quiet
    if ( bAbs > __tf_infrep ) return __tf_from(b | __tf_qnanbit);
    if ( aAbs == __tf_infrep ) {
      if ( bAbs == __tf_infrep && ((a ^ b) >> 127) ) return __tf_from(__tf_qnan);      // inf + -inf
      return __tf_from(a);
    }
    if ( bAbs == __tf_infrep ) return __tf_from(b);
    if ( aAbs == 0 ) return __tf_from(bAbs == 0 ? (a & b & __tf_signbit) : b);      // -0 + -0 = -0
    return __tf_from(a);                                                            // b == 0
  }

  if ( bAbs > aAbs ) {      // keep |a| >= |b|
    __tf_rep t = a;
    a = b;
    b = t;
    t = aAbs;
    aAbs = bAbs;
    bAbs = t;
  }

  int aExp = static_cast<int>(aAbs >> __tf_mantbits);
  int bExp = static_cast<int>(bAbs >> __tf_mantbits);
  __tf_rep aSig = aAbs & __tf_mantmask;
  __tf_rep bSig = bAbs & __tf_mantmask;
  if ( aExp == 0 )
    aExp = __tf_normalize(aSig);
  else
    aSig |= __tf_implicit;
  if ( bExp == 0 )
    bExp = __tf_normalize(bSig);
  else
    bSig |= __tf_implicit;

  const bool subtraction = ((a ^ b) >> 127) != 0;
  const __tf_rep resultSign = a & __tf_signbit;

  aSig <<= 3;      // make room for guard/round/sticky in the low 3 bits
  bSig <<= 3;
  const int align = aExp - bExp;
  if ( align ) {
    if ( align < 128 ) {
      const __tf_rep sticky = (bSig << (128 - align)) != 0 ? static_cast<__tf_rep>(1) : 0;
      bSig = (bSig >> align) | sticky;
    } else {
      bSig = 1;      // wholly below the sticky bit
    }
  }

  if ( subtraction ) {
    aSig -= bSig;
    if ( aSig == 0 ) return __tf_from(0);      // exact cancellation -> +0
    if ( aSig < (__tf_implicit << 3) ) {       // renormalize after cancellation
      const int shift = __tf_clz(aSig) - (127 - (__tf_mantbits + 3));
      aSig <<= shift;
      aExp -= shift;
    }
  } else {
    aSig += bSig;
    if ( aSig & (__tf_implicit << 4) ) {      // carry out of the top
      const __tf_rep sticky = aSig & 1;
      aSig = (aSig >> 1) | sticky;
      ++aExp;
    }
  }

  if ( aExp >= static_cast<int>(__tf_einf) ) return __tf_from(resultSign | __tf_infrep);
  if ( aExp <= 0 ) return __tf_from(resultSign);      // subnormal/underflow -> signed zero (FTZ)

  const unsigned grs = static_cast<unsigned>(aSig & 7u);
  const __tf_rep frac = (aSig >> 3) & __tf_mantmask;
  __tf_rep result = resultSign | (static_cast<__tf_rep>(aExp) << __tf_mantbits) | frac;
  if ( grs > 4u || (grs == 4u && ((aSig >> 3) & 1u)) ) result += 1;      // RNE; carry flows into exp/inf
  return __tf_from(result);
}

[[gnu::always_inline]] inline void
__tf_wide_mul(__tf_rep a, __tf_rep b, __tf_rep &hi, __tf_rep &lo) noexcept
{
  const unsigned long long al = static_cast<unsigned long long>(a), ah = static_cast<unsigned long long>(a >> 64);
  const unsigned long long bl = static_cast<unsigned long long>(b), bh = static_cast<unsigned long long>(b >> 64);
  const __tf_rep ll = static_cast<__tf_rep>(al) * bl;
  const __tf_rep lh = static_cast<__tf_rep>(al) * bh;
  const __tf_rep hl = static_cast<__tf_rep>(ah) * bl;
  const __tf_rep hh = static_cast<__tf_rep>(ah) * bh;
  const __tf_rep mid = (ll >> 64) + static_cast<unsigned long long>(lh) + static_cast<unsigned long long>(hl);
  lo = static_cast<unsigned long long>(ll) | (mid << 64);
  hi = hh + (lh >> 64) + (hl >> 64) + (mid >> 64);
}

// three-way ordered compare (callers handle NaN): -1/0/+1; +-0 compare equal
[[gnu::always_inline]] inline int
__tf_cmp3(__tf_rep a, __tf_rep b) noexcept
{
  const __tf_rep aAbs = a & __tf_absmask, bAbs = b & __tf_absmask;
  if ( (aAbs | bAbs) == 0 ) return 0;
  const bool aNeg = (a >> 127) != 0, bNeg = (b >> 127) != 0;
  if ( aNeg != bNeg ) return aNeg ? -1 : 1;
  if ( aAbs == bAbs ) return 0;
  return ((aAbs < bAbs) != aNeg) ? -1 : 1;
}

[[gnu::always_inline]] inline bool
__tf_either_nan(__tf_rep a, __tf_rep b) noexcept
{
  return (a & __tf_absmask) > __tf_infrep || (b & __tf_absmask) > __tf_infrep;
}

// round a binary128 to a narrower binary format
template<typename U, int MBITS, int EBITS, int BIAS>
[[gnu::always_inline]] inline U
__tf_trunc_to(__tf_rep b) noexcept
{
  const U sign = static_cast<U>(b >> 127) << (MBITS + EBITS);
  const int e = static_cast<int>((b >> __tf_mantbits) & __tf_einf);
  const __tf_rep m = b & __tf_mantmask;
  const U emax = (static_cast<U>(1) << EBITS) - 1;
  if ( e == static_cast<int>(__tf_einf) ) {
    if ( m == 0 ) return sign | (emax << MBITS);      // inf
    U pay = static_cast<U>(m >> (__tf_mantbits - MBITS));
    pay |= static_cast<U>(1) << (MBITS - 1);
    return sign | (emax << MBITS) | pay;
  }
  int ue;
  __tf_rep sig;
  if ( e == 0 ) {
    if ( m == 0 ) return sign;      // +-0
    ue = 1 - __tf_bias;             // f128 subnormal: far below any double/float subnormal
    sig = m;
  } else {
    ue = e - __tf_bias;
    sig = __tf_implicit | m;
  }
  const int te = ue + BIAS;
  if ( te >= static_cast<int>(emax) ) return sign | (emax << MBITS);      // overflow -> inf
  int rshift = __tf_mantbits - MBITS;
  if ( te <= 0 ) rshift += 1 - te;
  if ( rshift > 113 ) return sign;
  U kept = static_cast<U>(sig >> rshift);
  const __tf_rep rem = sig & ((static_cast<__tf_rep>(1) << rshift) - 1);
  const __tf_rep half = static_cast<__tf_rep>(1) << (rshift - 1);
  if ( rem > half || (rem == half && (kept & 1)) ) ++kept;
  const U base = te > 0 ? static_cast<U>(te - 1) << MBITS : 0;
  return sign | (base + kept);
}
}      // namespace

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arithmetic

extern "C" __attribute__((weak)) _Float128
__addtf3(_Float128 fa, _Float128 fb) noexcept
{
  return __tf_addsub(__tf_bits(fa), __tf_bits(fb));
}

extern "C" __attribute__((weak)) _Float128
__subtf3(_Float128 fa, _Float128 fb) noexcept
{
  return __tf_addsub(__tf_bits(fa), __tf_bits(fb) ^ __tf_signbit);
}

extern "C" __attribute__((weak)) _Float128
__multf3(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  const __tf_rep aAbs = a & __tf_absmask, bAbs = b & __tf_absmask;
  const __tf_rep sign = (a ^ b) & __tf_signbit;

  if ( aAbs > __tf_infrep ) return __tf_from(a | __tf_qnanbit);      // NaN
  if ( bAbs > __tf_infrep ) return __tf_from(b | __tf_qnanbit);
  if ( aAbs == __tf_infrep || bAbs == __tf_infrep ) {
    if ( aAbs == 0 || bAbs == 0 ) return __tf_from(__tf_qnan);      // inf * 0
    return __tf_from(sign | __tf_infrep);
  }
  if ( aAbs == 0 || bAbs == 0 ) return __tf_from(sign);      // signed zero

  int aExp = static_cast<int>(aAbs >> __tf_mantbits);
  int bExp = static_cast<int>(bAbs >> __tf_mantbits);
  __tf_rep aSig = aAbs & __tf_mantmask;
  __tf_rep bSig = bAbs & __tf_mantmask;
  if ( aExp == 0 )
    aExp = __tf_normalize(aSig);
  else
    aSig |= __tf_implicit;
  if ( bExp == 0 )
    bExp = __tf_normalize(bSig);
  else
    bSig |= __tf_implicit;

  __tf_rep hi, lo;
  __tf_wide_mul(aSig, bSig, hi, lo);      // 226-bit product in (hi:lo)

  int exp = aExp + bExp - __tf_bias;
  int shift;                                          // bits below the kept 113-bit significand
  if ( hi & (static_cast<__tf_rep>(1) << 97) ) {      // leading bit at 225
    shift = 113;
    exp += 1;
  } else {      // leading bit at 224
    shift = 112;
  }

  const __tf_rep sig = (lo >> shift) | (hi << (128 - shift));      // P >> shift, a 113-bit significand
  const unsigned guard = static_cast<unsigned>((lo >> (shift - 1)) & 1);
  const bool sticky = (lo & ((static_cast<__tf_rep>(1) << (shift - 1)) - 1)) != 0;

  if ( exp >= static_cast<int>(__tf_einf) ) return __tf_from(sign | __tf_infrep);
  if ( exp <= 0 ) return __tf_from(sign);      // subnormal/underflow -> signed zero (FTZ)

  __tf_rep result = sign | (static_cast<__tf_rep>(exp) << __tf_mantbits) | (sig & __tf_mantmask);
  if ( guard && (sticky || (sig & 1)) ) result += 1;      // RNE; carry flows into exp/inf
  return __tf_from(result);
}

extern "C" __attribute__((weak)) _Float128
__divtf3(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  const __tf_rep aAbs = a & __tf_absmask, bAbs = b & __tf_absmask;
  const __tf_rep sign = (a ^ b) & __tf_signbit;

  if ( aAbs > __tf_infrep ) return __tf_from(a | __tf_qnanbit);      // NaN propagates
  if ( bAbs > __tf_infrep ) return __tf_from(b | __tf_qnanbit);
  if ( aAbs == __tf_infrep ) {
    if ( bAbs == __tf_infrep ) return __tf_from(__tf_qnan);      // inf / inf
    return __tf_from(sign | __tf_infrep);
  }
  if ( bAbs == __tf_infrep ) return __tf_from(sign);      // finite / inf -> signed zero
  if ( bAbs == 0 ) {
    if ( aAbs == 0 ) return __tf_from(__tf_qnan);      // 0 / 0
    return __tf_from(sign | __tf_infrep);              // x / 0 -> signed inf
  }
  if ( aAbs == 0 ) return __tf_from(sign);      // 0 / x -> signed zero

  int aExp = static_cast<int>(aAbs >> __tf_mantbits);
  int bExp = static_cast<int>(bAbs >> __tf_mantbits);
  __tf_rep aSig = aAbs & __tf_mantmask;
  __tf_rep bSig = bAbs & __tf_mantmask;
  if ( aExp == 0 )
    aExp = __tf_normalize(aSig);
  else
    aSig |= __tf_implicit;
  if ( bExp == 0 )
    bExp = __tf_normalize(bSig);
  else
    bSig |= __tf_implicit;

  int exp = aExp - bExp + __tf_bias;
  if ( aSig < bSig ) {      // keep the quotient in [1, 2)
    aSig <<= 1;
    --exp;
  }

  // restoring long division: implicit leading 1, then 112 fraction bits + one guard bit
  __tf_rep rem = aSig - bSig;
  __tf_rep q = 1;
  for ( int i = 0; i < __tf_mantbits + 1; ++i ) {
    rem <<= 1;
    q <<= 1;
    if ( rem >= bSig ) {
      rem -= bSig;
      q |= 1;
    }
  }
  const unsigned guard = static_cast<unsigned>(q & 1);
  const bool sticky = rem != 0;
  const __tf_rep sig = q >> 1;      // 113-bit significand

  if ( exp >= static_cast<int>(__tf_einf) ) return __tf_from(sign | __tf_infrep);
  if ( exp <= 0 ) return __tf_from(sign);      // subnormal/underflow -> signed zero (FTZ)

  __tf_rep result = sign | (static_cast<__tf_rep>(exp) << __tf_mantbits) | (sig & __tf_mantmask);
  if ( guard && (sticky || (sig & 1)) ) result += 1;      // RNE; carry flows into exp/inf
  return __tf_from(result);
}

extern "C" __attribute__((weak)) _Float128
__negtf2(_Float128 x) noexcept
{
  return __tf_from(__tf_bits(x) ^ __tf_signbit);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// integer -> binary128

extern "C" __attribute__((weak)) _Float128
__floatditf(long long i) noexcept      // signed 64-bit int -> binary128 (exact)
{
  if ( i == 0 ) return __tf_from(0);
  const bool neg = i < 0;
  const unsigned long long mag = neg ? (0ull - static_cast<unsigned long long>(i)) : static_cast<unsigned long long>(i);
  const int msb = 63 - __builtin_clzll(mag);
  __tf_rep sig = static_cast<__tf_rep>(mag) << (__tf_mantbits - msb);      // leading 1 -> bit 112; exact (64 <= 113)
  const __tf_rep exp = static_cast<__tf_rep>(msb + __tf_bias);
  return __tf_from((neg ? __tf_signbit : 0) | (exp << __tf_mantbits) | (sig & __tf_mantmask));
}

extern "C" __attribute__((weak)) _Float128
__floatunditf(unsigned long long u) noexcept      // unsigned 64-bit int -> binary128 (exact)
{
  if ( u == 0 ) return __tf_from(0);
  const int msb = 63 - __builtin_clzll(u);
  const __tf_rep sig = static_cast<__tf_rep>(u) << (__tf_mantbits - msb);
  const __tf_rep exp = static_cast<__tf_rep>(msb + __tf_bias);
  return __tf_from((exp << __tf_mantbits) | (sig & __tf_mantmask));
}

extern "C" __attribute__((weak)) _Float128
__floatsitf(int i) noexcept
{
  return __floatditf(i);
}

extern "C" __attribute__((weak)) _Float128
__floatunsitf(unsigned int u) noexcept
{
  return __floatunditf(u);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// binary128 -> integer
// sign-selected extreme, matching __fixtfdi's original convention)

extern "C" __attribute__((weak)) long long
__fixtfdi(_Float128 fa) noexcept      // binary128 -> signed 64-bit int (truncate toward zero)
{
  const __tf_rep a = __tf_bits(fa);
  const bool neg = (a >> 127) != 0;
  const unsigned exp = static_cast<unsigned>((a >> __tf_mantbits) & __tf_einf);
  const __tf_rep mant = a & __tf_mantmask;
  constexpr long long imin = -9223372036854775807LL - 1;
  constexpr long long imax = 9223372036854775807LL;
  if ( exp < static_cast<unsigned>(__tf_bias) ) return 0;      // |x| < 1 (incl. zero/subnormal)
  if ( exp == __tf_einf ) return neg ? imin : imax;            // inf/NaN saturate
  const int unbiased = static_cast<int>(exp) - __tf_bias;      // >= 0
  if ( unbiased >= 63 ) {
    if ( neg && unbiased == 63 && mant == 0 ) return imin;      // exactly -2^63
    return neg ? imin : imax;                                   // out of range -> saturate
  }
  const __tf_rep sig = __tf_implicit | mant;      // 113-bit
  const unsigned long long m = static_cast<unsigned long long>(sig >> (__tf_mantbits - unbiased));
  return neg ? -static_cast<long long>(m) : static_cast<long long>(m);
}

extern "C" __attribute__((weak)) unsigned long long
__fixunstfdi(_Float128 fa) noexcept      // binary128 -> unsigned 64-bit int (truncate toward zero)
{
  const __tf_rep a = __tf_bits(fa);
  const bool neg = (a >> 127) != 0;
  const unsigned exp = static_cast<unsigned>((a >> __tf_mantbits) & __tf_einf);
  if ( neg ) return 0;                                         // negatives (incl. -inf/-NaN) saturate low
  if ( exp == __tf_einf ) return ~0ull;                        // +inf/+NaN saturate high
  if ( exp < static_cast<unsigned>(__tf_bias) ) return 0;      // |x| < 1
  const int unbiased = static_cast<int>(exp) - __tf_bias;
  if ( unbiased >= 64 ) return ~0ull;
  const __tf_rep sig = __tf_implicit | (a & __tf_mantmask);
  return static_cast<unsigned long long>(sig >> (__tf_mantbits - unbiased));
}

extern "C" __attribute__((weak)) int
__fixtfsi(_Float128 fa) noexcept
{
  const long long v = __fixtfdi(fa);
  if ( v > 2147483647ll ) return 2147483647;
  if ( v < -2147483648ll ) return (-2147483647 - 1);
  return static_cast<int>(v);
}

extern "C" __attribute__((weak)) unsigned int
__fixunstfsi(_Float128 fa) noexcept
{
  const unsigned long long v = __fixunstfdi(fa);
  return v > 0xffffffffull ? 0xffffffffu : static_cast<unsigned int>(v);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// widening conversions

extern "C" __attribute__((weak)) _Float128
__extenddftf2(double x) noexcept
{
  const unsigned long long db = __builtin_bit_cast(unsigned long long, x);
  const __tf_rep s = static_cast<__tf_rep>(db >> 63) << 127;
  const int de = static_cast<int>((db >> 52) & 0x7ff);
  const unsigned long long dm = db & 0xfffffffffffffull;
  if ( de == 0x7ff )      // inf/nan: shift the payload up, quietness rides along in the top bit
    return __tf_from(s | __tf_infrep | (static_cast<__tf_rep>(dm) << 60));
  if ( de == 0 ) {
    if ( dm == 0 ) return __tf_from(s);      // +-0
    // subnormal double (value = dm * 2^-1074)
    const int p = 63 - __builtin_clzll(dm);
    const __tf_rep m = (static_cast<__tf_rep>(dm) << (__tf_mantbits - p)) & __tf_mantmask;
    return __tf_from(s | (static_cast<__tf_rep>((p - 1074) + __tf_bias) << __tf_mantbits) | m);
  }
  return __tf_from(s | (static_cast<__tf_rep>(de - 1023 + __tf_bias) << __tf_mantbits) | (static_cast<__tf_rep>(dm) << 60));
}

extern "C" __attribute__((weak)) _Float128
__extendsftf2(float x) noexcept
{
  const unsigned int fb = __builtin_bit_cast(unsigned int, x);
  const __tf_rep s = static_cast<__tf_rep>(fb >> 31) << 127;
  const int fe = static_cast<int>((fb >> 23) & 0xff);
  const unsigned int fm = fb & 0x7fffffu;
  if ( fe == 0xff ) return __tf_from(s | __tf_infrep | (static_cast<__tf_rep>(fm) << 89));
  if ( fe == 0 ) {
    if ( fm == 0 ) return __tf_from(s);
    // subnormal float (value = fm * 2^-149)
    const int p = 31 - __builtin_clz(fm);
    const __tf_rep m = (static_cast<__tf_rep>(fm) << (__tf_mantbits - p)) & __tf_mantmask;
    return __tf_from(s | (static_cast<__tf_rep>((p - 149) + __tf_bias) << __tf_mantbits) | m);
  }
  return __tf_from(s | (static_cast<__tf_rep>(fe - 127 + __tf_bias) << __tf_mantbits) | (static_cast<__tf_rep>(fm) << 89));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// narrowing conversions

extern "C" __attribute__((weak)) double
__trunctfdf2(_Float128 x) noexcept
{
  return __builtin_bit_cast(double, __tf_trunc_to<unsigned long long, 52, 11, 1023>(__tf_bits(x)));
}

extern "C" __attribute__((weak)) float
__trunctfsf2(_Float128 x) noexcept
{
  return __builtin_bit_cast(float, __tf_trunc_to<unsigned int, 23, 8, 127>(__tf_bits(x)));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// comparisons; libgcc style

extern "C" __attribute__((weak)) int
__lttf2(_Float128 fa, _Float128 fb) noexcept      // <0 if a<b, 0 if a==b, >0 otherwise; unordered -> >0
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return 1;
  return __tf_cmp3(a, b);
}

extern "C" __attribute__((weak)) int
__letf2(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return 1;
  return __tf_cmp3(a, b);
}

extern "C" __attribute__((weak)) int
__gttf2(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return -1;
  return __tf_cmp3(a, b);
}

extern "C" __attribute__((weak)) int
__getf2(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return -1;
  return __tf_cmp3(a, b);
}

extern "C" __attribute__((weak)) int
__eqtf2(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return 1;
  return __tf_cmp3(a, b) != 0;
}

extern "C" __attribute__((weak)) int
__netf2(_Float128 fa, _Float128 fb) noexcept
{
  const __tf_rep a = __tf_bits(fa), b = __tf_bits(fb);
  if ( __tf_either_nan(a, b) ) return 1;
  return __tf_cmp3(a, b) != 0;
}

extern "C" __attribute__((weak)) int
__unordtf2(_Float128 fa, _Float128 fb) noexcept
{
  return __tf_either_nan(__tf_bits(fa), __tf_bits(fb)) ? 1 : 0;
}

#pragma GCC diagnostic pop
#endif

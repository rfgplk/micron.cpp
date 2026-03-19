//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"

#include "../string.hpp"
#include "../unitypes.hpp"

namespace micron
{
namespace __impl
{
#if defined(__SIZEOF_INT128__)
struct __fmt_uint128_t {
  u64 lo;
  u64 hi;
};

inline __fmt_uint128_t
umul128(u64 a, u64 b)
{
  unsigned __int128 r = static_cast<unsigned __int128>(a) * b;
  return { static_cast<u64>(r), static_cast<u64>(r >> 64) };
}
#else
// no 128-bit
inline __fmt_uint128_t
umul128(u64 a, u64 b)
{
  u64 aHi = a >> 32, aLo = a & 0xFFFFFFFF;
  u64 bHi = b >> 32, bLo = b & 0xFFFFFFFF;

  u64 p00 = aLo * bLo;
  u64 p01 = aLo * bHi;
  u64 p10 = aHi * bLo;
  u64 p11 = aHi * bHi;

  u64 mid = (p00 >> 32) + (p01 & 0xFFFFFFFF) + (p10 & 0xFFFFFFFF);

  return { (p00 & 0xFFFFFFFF) | (mid << 32), p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32) };
}
#endif

inline u64
shiftright128(u64 lo, u64 hi, u32 dist)
{
  if ( dist == 0 )
    return lo;
  if ( dist >= 64 )
    return hi >> (dist - 64);
  return (lo >> dist) | (hi << (64 - dist));
}

inline u64
shiftleft128(u64 lo, u64 hi, u32 dist)
{
  if ( dist == 0 )
    return hi;
  if ( dist >= 64 )
    return lo << (dist - 64);
  return (hi << dist) | (lo >> (64 - dist));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tables

struct digit_pair {
  char d[2];
};

constexpr static const digit_pair __digit_tbl[100] = {
  { '0', '0' }, { '0', '1' }, { '0', '2' }, { '0', '3' }, { '0', '4' }, { '0', '5' }, { '0', '6' }, { '0', '7' }, { '0', '8' },
  { '0', '9' }, { '1', '0' }, { '1', '1' }, { '1', '2' }, { '1', '3' }, { '1', '4' }, { '1', '5' }, { '1', '6' }, { '1', '7' },
  { '1', '8' }, { '1', '9' }, { '2', '0' }, { '2', '1' }, { '2', '2' }, { '2', '3' }, { '2', '4' }, { '2', '5' }, { '2', '6' },
  { '2', '7' }, { '2', '8' }, { '2', '9' }, { '3', '0' }, { '3', '1' }, { '3', '2' }, { '3', '3' }, { '3', '4' }, { '3', '5' },
  { '3', '6' }, { '3', '7' }, { '3', '8' }, { '3', '9' }, { '4', '0' }, { '4', '1' }, { '4', '2' }, { '4', '3' }, { '4', '4' },
  { '4', '5' }, { '4', '6' }, { '4', '7' }, { '4', '8' }, { '4', '9' }, { '5', '0' }, { '5', '1' }, { '5', '2' }, { '5', '3' },
  { '5', '4' }, { '5', '5' }, { '5', '6' }, { '5', '7' }, { '5', '8' }, { '5', '9' }, { '6', '0' }, { '6', '1' }, { '6', '2' },
  { '6', '3' }, { '6', '4' }, { '6', '5' }, { '6', '6' }, { '6', '7' }, { '6', '8' }, { '6', '9' }, { '7', '0' }, { '7', '1' },
  { '7', '2' }, { '7', '3' }, { '7', '4' }, { '7', '5' }, { '7', '6' }, { '7', '7' }, { '7', '8' }, { '7', '9' }, { '8', '0' },
  { '8', '1' }, { '8', '2' }, { '8', '3' }, { '8', '4' }, { '8', '5' }, { '8', '6' }, { '8', '7' }, { '8', '8' }, { '8', '9' },
  { '9', '0' }, { '9', '1' }, { '9', '2' }, { '9', '3' }, { '9', '4' }, { '9', '5' }, { '9', '6' }, { '9', '7' }, { '9', '8' },
  { '9', '9' },
};

constexpr static const char __hex_lower[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

constexpr static const char __hex_upper[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

// floor(x/10) = mulhi(x, 0xCCCCCCCCCCCCCCCD) >> 3
inline u64
fast_div10(u64 x)
{
  return umul128(x, 0xCCCCCCCCCCCCCCCDULL).hi >> 3;
}

// floor(x/100) granlund-montgomery
inline u64
fast_div100(u64 x)
{
  u64 hi = umul128(x, 5165088340638674453ull).hi;
  return (hi + ((x - hi) >> 1)) >> 6;
}

// floor(x/1e4) via reciprocal
inline u64
fast_div10000(u64 x)
{
  return umul128(x, 0xD1B71758E219652CULL).hi >> 13;
}

// floor(x/1e8) via reciprocal
inline u64
fast_div1e8(u64 x)
{
  return umul128(x, 0xABCC77118461CEFCULL).hi >> 26;
}

inline u32
fast_mod100(u64 x, u64 q)
{
  return static_cast<u32>(x - q * 100);
}

inline u32
fast_mod10000(u64 x, u64 q)
{
  return static_cast<u32>(x - q * 10000);
}

inline u32
fast_mod1e8(u64 x, u64 q)
{
  return static_cast<u32>(x - q * 100000000ull);
}

template <typename I> struct max_digits;

template <> struct max_digits<i8> {
  static constexpr usize value = 5;
};

template <> struct max_digits<u8> {
  static constexpr usize value = 4;
};

template <> struct max_digits<i16> {
  static constexpr usize value = 7;
};

template <> struct max_digits<u16> {
  static constexpr usize value = 6;
};

template <> struct max_digits<i32> {
  static constexpr usize value = 12;
};

template <> struct max_digits<u32> {
  static constexpr usize value = 11;
};

template <> struct max_digits<i64> {
  static constexpr usize value = 21;
};

template <> struct max_digits<u64> {
  static constexpr usize value = 21;
};

template <typename I> constexpr static const usize max_digits_v = max_digits<I>::value;

inline __attribute__((always_inline)) u32
clz64(u64 v)
{
  return v ? static_cast<u32>(__builtin_clzll(v)) : 64;
}

inline __attribute__((always_inline)) void
emit8_backward(char *buf, u32 v)
{
  // v < 100000000
  u32 hi4 = v / 10000;
  u32 lo4 = v - hi4 * 10000;

  u32 d0 = lo4 / 100;
  u32 d1 = lo4 - d0 * 100;
  buf[7] = __digit_tbl[d1].d[1];
  buf[6] = __digit_tbl[d1].d[0];
  buf[5] = __digit_tbl[d0].d[1];
  buf[4] = __digit_tbl[d0].d[0];

  u32 d2 = hi4 / 100;
  u32 d3 = hi4 - d2 * 100;
  buf[3] = __digit_tbl[d3].d[1];
  buf[2] = __digit_tbl[d3].d[0];
  buf[1] = __digit_tbl[d2].d[1];
  buf[0] = __digit_tbl[d2].d[0];
}

inline __attribute__((always_inline)) char *
emit_block_backward(char *end, u32 v)
{
  // not leading block emit exactly 8 digits
  // leading block emit without leading zeros
  if ( v == 0 ) {
    *--end = '0';
    return end;
  }

  while ( v >= 100 ) {
    u32 r = v % 100;
    v /= 100;
    *--end = __digit_tbl[r].d[1];
    *--end = __digit_tbl[r].d[0];
  }
  if ( v >= 10 ) {
    *--end = __digit_tbl[v].d[1];
    *--end = __digit_tbl[v].d[0];
  } else {
    *--end = static_cast<char>('0' + v);
  }
  return end;
}

// chunked 10^8 method

inline char *
uint_to_buf_backward(char *end, u64 val)
{
  if ( val == 0 ) {
    *--end = '0';
    return end;
  }

  if ( val < 100000000ull ) {
    return emit_block_backward(end, static_cast<u32>(val));
  }

  if ( val < 10000000000000000ull ) {
    u64 q = fast_div1e8(val);
    u32 lo = fast_mod1e8(val, q);

    emit8_backward(end - 8, lo);
    end -= 8;

    return emit_block_backward(end, static_cast<u32>(q));
  }

  u64 q1 = fast_div1e8(val);
  u32 lo = fast_mod1e8(val, q1);

  u64 q2 = fast_div1e8(q1);
  u32 mid = fast_mod1e8(q1, q2);

  emit8_backward(end - 8, lo);
  end -= 8;

  emit8_backward(end - 8, mid);
  end -= 8;

  return emit_block_backward(end, static_cast<u32>(q2));
}

inline char *
uint_to_buf_base_backward(char *end, u64 val, u32 base, bool upper)
{
  if ( base < 2 || base > 36 )
    base = 10;
  if ( val == 0 ) {
    *--end = '0';
    return end;
  }

  const char *digits = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";

  if ( base == 16 ) {
    const char *hex = upper ? __hex_upper : __hex_lower;
    while ( val > 0 ) {
      *--end = hex[val & 0xF];
      val >>= 4;
    }
    return end;
  }

  if ( base == 8 ) {
    while ( val > 0 ) {
      *--end = static_cast<char>('0' + (val & 7));
      val >>= 3;
    }
    return end;
  }

  if ( base == 2 ) {
    while ( val > 0 ) {
      *--end = static_cast<char>('0' + (val & 1));
      val >>= 1;
    }
    return end;
  }

  if ( base == 4 ) {
    while ( val > 0 ) {
      *--end = digits[val & 3];
      val >>= 2;
    }
    return end;
  }

  if ( base == 32 ) {
    while ( val > 0 ) {
      *--end = digits[val & 31];
      val >>= 5;
    }
    return end;
  }

  while ( val > 0 ) {
    *--end = digits[val % base];
    val /= base;
  }
  return end;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ryu algorithm: https://github.com/ulfjack/ryu Ulf Adams

namespace __ryu
{

constexpr static const u32 __mantissa_bits = 52;
constexpr static const u32 __exponent_bits = 11;
constexpr static const i32 __bias = 1023;
constexpr static const u32 __pow5_inv_bitcount = 122;
constexpr static const u32 __pow5_bitcount = 121;

constexpr static const u64 __pow5_table[26] = { 1ull,
                                                5ull,
                                                25ull,
                                                125ull,
                                                625ull,
                                                3125ull,
                                                15625ull,
                                                78125ull,
                                                390625ull,
                                                1953125ull,
                                                9765625ull,
                                                48828125ull,
                                                244140625ull,
                                                1220703125ull,
                                                6103515625ull,
                                                30517578125ull,
                                                152587890625ull,
                                                762939453125ull,
                                                3814697265625ull,
                                                19073486328125ull,
                                                95367431640625ull,
                                                476837158203125ull,
                                                2384185791015625ull,
                                                11920928955078125ull,
                                                59604644775390625ull,
                                                298023223876953125ull };

constexpr static const u64 __pow5_split2[13][2] = { { 0ull, 1152921504606846976ull },
                                                    { 0ull, 1490116119384765625ull },
                                                    { 1032610780636961552ull, 1925929944387235853ull },
                                                    { 7910200175544436838ull, 1244603055572228341ull },
                                                    { 16941905809032713930ull, 1608611746553235488ull },
                                                    { 13024893955298202172ull, 2079081953128979843ull },
                                                    { 6607496772837067824ull, 1343575832879272604ull },
                                                    { 17332926989895652603ull, 1736530273035216783ull },
                                                    { 13037379183483547984ull, 2244412773384604712ull },
                                                    { 1605989338741628675ull, 1450417759929778918ull },
                                                    { 9630225068416591280ull, 1874621017369538693ull },
                                                    { 665883850346957067ull, 1211445438634777304ull },
                                                    { 14931890668723713708ull, 1565756531257009982ull } };

constexpr static const u64 __pow5_inv_split2[15][2] = { { 1ull, 2305843009213693952ull },
                                                        { 5955668970331000884ull, 1784059615882449851ull },
                                                        { 8982663654677661702ull, 1380349269358112757ull },
                                                        { 7286864317269821294ull, 2135987035920910082ull },
                                                        { 7005857020398200553ull, 1652639921975621497ull },
                                                        { 17965325103354776697ull, 1278668206209430417ull },
                                                        { 8928596168509315048ull, 1978643211784836272ull },
                                                        { 10075671573058298858ull, 1530901034580419511ull },
                                                        { 597001226353042382ull, 1184477304306571148ull },
                                                        { 1527430471115325346ull, 1832889850782397517ull },
                                                        { 12533209867169019542ull, 1418129833677084982ull },
                                                        { 5577825024675947042ull, 2194449627517475473ull },
                                                        { 11006974540203867551ull, 1697873161311732311ull },
                                                        { 10313493231639821582ull, 1313665730009899186ull },
                                                        { 12701016819766672773ull, 2032799256770390445ull } };

constexpr static const u32 __pow5_offsets[21]
    = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000000, 0x59695995, 0x55455555, 0x41545555, 0x54040455, 0x15545455, 0x44455045,
        0x44554554, 0x55555054, 0x45555555, 0x05511504, 0x45515555, 0x05054514, 0x45050004, 0x55555555, 0x55050A04, 0x00505515 };

constexpr static const u32 __pow5_inv_offsets[19]
    = { 0x54544554, 0x04055545, 0x10041000, 0x00400414, 0x40010000, 0x41155555, 0x00000454, 0x00010044, 0x40000000, 0x44000041,
        0x50454450, 0x55550054, 0x51655554, 0x40004000, 0x01000001, 0x00010500, 0x51515411, 0x05555554, 0x00000000 };

inline constexpr u32
pow5bits(u32 e)
{
  return static_cast<u32>((static_cast<u64>(e) * 1217359ull) >> 19) + 1;
}

inline constexpr i32
log10_pow2(i32 e)
{
  return static_cast<i32>((static_cast<u64>(static_cast<u32>(e)) * 78913ull) >> 18);
}

inline constexpr i32
log10_pow5(i32 e)
{
  return static_cast<i32>((static_cast<u64>(static_cast<u32>(e)) * 732923ull) >> 20);
}

// divisibility by 5 == v * 0xCCCCCCCCCCCCCCCD <= floor(2^64 / 5)
inline u32
pow5_factor(u64 v)
{
  // lots of magic here
  // 2^64 / 5 = 3689348814741910323
  constexpr u64 inv5 = 0xCCCCCCCCCCCCCCCDull;
  constexpr u64 thresh5 = 0x3333333333333333ull;     // floor(2^64 / 5)
  u32 count = 0;
  while ( true ) {
    u64 q = v * inv5;
    if ( q > thresh5 )
      break;
    v = q;
    ++count;
  }
  return count;
}

inline bool
pow5_multiple(u64 v, u32 p)
{
  return pow5_factor(v) >= p;
}

inline bool
pow2_multiple(u64 v, u32 p)
{
  return (v & ((1ull << p) - 1)) == 0;
}

inline void
pow5_compute(u32 i, u64 result[2])
{
  const u32 base = i / 26;
  const u32 base2 = base * 26;
  const u64 *mul = __pow5_split2[base];

  if ( i == base2 ) {
    result[0] = mul[0];
    result[1] = mul[1];
    return;
  }

  u32 offset = i - base2;
  u64 m = __pow5_table[offset];

  __fmt_uint128_t b0 = umul128(m, mul[0]);
  __fmt_uint128_t b2 = umul128(m, mul[1]);

  u64 mid = b0.hi + b2.lo;
  u64 high = b2.hi + (mid < b0.hi ? 1ull : 0ull);

  u32 delta = pow5bits(i) - pow5bits(base2);
  u32 corr = (__pow5_offsets[i / 16] >> ((i % 16) << 1)) & 3;

  result[0] = shiftleft128(b0.lo, mid, delta) + corr;
  result[1] = shiftleft128(mid, high, delta);
}

inline void
pow5_compute_inv(u32 i, u64 result[2])
{
  const u32 base = (i + 25) / 26;
  const u32 base2 = base * 26;
  const u64 *mul = __pow5_inv_split2[base];

  if ( i == base2 ) {
    result[0] = mul[0] + 1;
    result[1] = mul[1];
    return;
  }

  u32 offset = base2 - i;
  u64 m = __pow5_table[offset];

  __fmt_uint128_t b0 = umul128(m, mul[0]);
  __fmt_uint128_t b2 = umul128(m, mul[1]);

  u64 mid = b0.hi + b2.lo;
  u64 high = b2.hi + (mid < b0.hi ? 1ull : 0ull);

  u32 delta = pow5bits(base2) - pow5bits(i);
  u32 corr = (__pow5_inv_offsets[i / 16] >> ((i % 16) << 1)) & 3;

  result[0] = shiftleft128(b0.lo, mid, delta) + corr + 1;
  result[1] = shiftleft128(mid, high, delta);
}

inline u64
shift_mul64(u64 m, const u64 mul[2], i32 j)
{
  __fmt_uint128_t b0 = umul128(m, mul[0]);
  __fmt_uint128_t b2 = umul128(m, mul[1]);

  u64 mid = b0.hi + b2.lo;
  u64 hi = b2.hi + (mid < b0.hi ? 1ull : 0ull);

  return shiftright128(mid, hi, static_cast<u32>(j) - 64);
}

struct decimal64 {
  u64 mantissa;
  i32 exponent;
};

inline decimal64
d2d(u64 ieeeMantissa, u32 ieeeExponent)
{
  const int __mantissa_bits = 52;
  const int __bias = 1023;

  i32 e2;
  u64 m2;

  if ( ieeeExponent == 0 ) {
    // subnormal
    e2 = 1 - __bias - __mantissa_bits;
    m2 = ieeeMantissa;
  } else {
    e2 = static_cast<i32>(ieeeExponent) - __bias - __mantissa_bits;
    m2 = (1ull << __mantissa_bits) | ieeeMantissa;
  }

  bool even = (m2 & 1) == 0;
  bool acceptBounds = even;

  __uint128_t mv = static_cast<__uint128_t>(m2) << 2;
  u32 mmShift = (ieeeMantissa != 0 || ieeeExponent <= 1) ? 1 : 0;

  __uint128_t vr, vp, vm;
  i32 e10;
  bool trailingVr = false, trailingVm = false;

  if ( e2 >= 0 ) {
    // positive exponent
    i32 q = log10_pow2(e2) - (e2 > 3 ? 1 : 0);
    e10 = q;
    i32 k = static_cast<i32>(__pow5_inv_bitcount + pow5bits(static_cast<u32>(q))) - 1;
    i32 i = -e2 + q + k;

    u64 pow5[2];
    pow5_compute_inv(static_cast<u32>(q), pow5);

    vr = shift_mul64(mv, pow5, i);
    vp = shift_mul64(mv + 2, pow5, i);
    vm = shift_mul64(mv - 1 - mmShift, pow5, i);

    if ( q <= 21 ) {
      if ( mv % 5 == 0 )
        trailingVr = pow5_multiple(mv, static_cast<u32>(q));
      else if ( acceptBounds )
        trailingVm = pow5_multiple(mv - 1 - mmShift, static_cast<u32>(q));
      else
        vp -= pow5_multiple(mv + 2, static_cast<u32>(q)) ? 1 : 0;
    }
  } else {
    // negative exponent
    i32 q = log10_pow5(-e2) - (-e2 > 1 ? 1 : 0);
    e10 = q + e2;
    i32 i = -e2 - q;
    i32 k = static_cast<i32>(pow5bits(static_cast<u32>(i))) - static_cast<i32>(__pow5_bitcount);
    i32 j = q - k;

    u64 pow5[2];
    pow5_compute(static_cast<u32>(i), pow5);

    vr = shift_mul64(mv, pow5, j);
    vp = shift_mul64(mv + 2, pow5, j);
    vm = shift_mul64(mv - 1 - mmShift, pow5, j);

    if ( q <= 1 ) {
      trailingVr = true;
      trailingVm = acceptBounds && (mmShift == 1);
    } else if ( q < 63 ) {
      trailingVr = pow2_multiple(mv, static_cast<u32>(q));
    }
  }

  // 3. Remove trailing digits & round-to-even
  i32 removed = 0;
  u8 lastRemoved = 0;

  if ( trailingVm || trailingVr ) {
    for ( ;; ) {
      __uint128_t vpD = vp / 10, vmD = vm / 10;
      if ( vpD <= vmD )
        break;
      __uint128_t vrD = vr / 10;
      u32 vrM = static_cast<u32>(vr - 10 * vrD);
      trailingVm &= (vm - 10 * vmD == 0);
      trailingVr &= (lastRemoved == 0);
      lastRemoved = static_cast<u8>(vrM);
      vr = vrD;
      vp = vpD;
      vm = vmD;
      ++removed;
    }
    if ( trailingVm ) {
      for ( ;; ) {
        __uint128_t vmD = vm / 10;
        if ( vm - 10 * vmD != 0 )
          break;
        __uint128_t vpD = vp / 10, vrD = vr / 10;
        u32 vrM = static_cast<u32>(vr - 10 * vrD);
        trailingVr &= (lastRemoved == 0);
        lastRemoved = static_cast<u8>(vrM);
        vr = vrD;
        vp = vpD;
        vm = vmD;
        ++removed;
      }
    }
    if ( trailingVr && lastRemoved == 5 && (vr & 1) == 0 )
      lastRemoved = 4;

    u64 out = static_cast<u64>(vr + ((vr == vm && (!acceptBounds || !trailingVm)) || lastRemoved >= 5 ? 1ull : 0ull));
    return { out, e10 + removed };
  } else {
    bool roundUp = false;
    for ( ;; ) {
      __uint128_t vpD = vp / 10, vmD = vm / 10;
      if ( vpD <= vmD )
        break;
      __uint128_t vrD = vr / 10;
      u32 vrM = static_cast<u32>(vr - 10 * vrD);
      roundUp = (vrM >= 5);
      vr = vrD;
      vp = vpD;
      vm = vmD;
      ++removed;
    }
    u64 out = static_cast<u64>(vr + ((vr == vm || roundUp) ? 1ull : 0ull));
    return { out, e10 + removed };
  }
}

inline u32
decimalLength(u64 v)
{
  // floor(log10(2^(63-clz))) + correction
  // powers of 10 for boundary checks
  constexpr static const u64 __pow10[20] = { 1ull,
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
  // log10 approximation from bit count floor(bits * 77/ 256) = floor(log10(2^bits))
  // may be off by one
  u32 bits = 64 - clz64(v);
  u32 approx = (bits * 77) >> 8;
  return approx + (v >= __pow10[approx] ? 1 : 0);
}

// ryu shortest representation
inline usize
d2s_buffered(f64 value, char *buf)
{
  u64 bits;
  memcpy(&bits, &value, sizeof(u64));
  // const char *src = reinterpret_cast<const char *>(&value);
  // char *dst = reinterpret_cast<char *>(&bits);
  // for ( int k = 0; k < 8; ++k )
  //   dst[k] = src[k];

  bool sign = (bits >> 63) != 0;
  u64 ieeeMantissa = bits & ((1ull << 52) - 1);
  u32 ieeeExponent = static_cast<u32>((bits >> 52) & 0x7FF);

  usize pos = 0;

  if ( ieeeExponent == 0x7FF ) {
    if ( ieeeMantissa ) {
      buf[0] = 'n';
      buf[1] = 'a';
      buf[2] = 'n';
      return 3;
    }
    if ( sign )
      buf[pos++] = '-';
    buf[pos] = 'i';
    buf[pos + 1] = 'n';
    buf[pos + 2] = 'f';
    return pos + 3;
  }

  if ( ieeeExponent == 0 && ieeeMantissa == 0 ) {
    if ( sign )
      buf[pos++] = '-';
    buf[pos] = '0';
    buf[pos + 1] = '.';
    buf[pos + 2] = '0';
    return pos + 3;
  }

  if ( sign )
    buf[pos++] = '-';

  decimal64 dec = d2d(ieeeMantissa, ieeeExponent);
  u64 output = dec.mantissa;
  i32 exp = dec.exponent;
  u32 olength = decimalLength(output);

  i32 sciExp = exp + static_cast<i32>(olength) - 1;

  char digits[20];
  char *dend = digits + 20;
  char *dstart = uint_to_buf_backward(dend, output);
  usize dlen = static_cast<usize>(dend - dstart);

  if ( sciExp >= -3 && sciExp <= 7 ) {
    if ( exp >= 0 ) {
      for ( usize i = 0; i < dlen; ++i )
        buf[pos++] = dstart[i];
      for ( i32 i = 0; i < exp; ++i )
        buf[pos++] = '0';
      buf[pos++] = '.';
      buf[pos++] = '0';
    } else if ( exp + static_cast<i32>(olength) > 0 ) {
      i32 intDigits = static_cast<i32>(olength) + exp;
      for ( i32 i = 0; i < intDigits; ++i )
        buf[pos++] = dstart[i];
      buf[pos++] = '.';
      for ( usize i = static_cast<usize>(intDigits); i < dlen; ++i )
        buf[pos++] = dstart[i];
    } else {
      buf[pos++] = '0';
      buf[pos++] = '.';
      i32 leadingZeros = -(exp + static_cast<i32>(olength));
      for ( i32 i = 0; i < leadingZeros; ++i )
        buf[pos++] = '0';
      for ( usize i = 0; i < dlen; ++i )
        buf[pos++] = dstart[i];
    }
  } else {
    buf[pos++] = dstart[0];
    if ( dlen > 1 ) {
      buf[pos++] = '.';
      for ( usize i = 1; i < dlen; ++i )
        buf[pos++] = dstart[i];
    }
    buf[pos++] = 'e';
    if ( sciExp >= 0 ) {
      buf[pos++] = '+';
    } else {
      buf[pos++] = '-';
      sciExp = -sciExp;
    }
    if ( sciExp >= 100 ) {
      buf[pos++] = static_cast<char>('0' + sciExp / 100);
      sciExp %= 100;
      buf[pos++] = __digit_tbl[sciExp].d[0];
      buf[pos++] = __digit_tbl[sciExp].d[1];
    } else if ( sciExp >= 10 ) {
      buf[pos++] = __digit_tbl[sciExp].d[0];
      buf[pos++] = __digit_tbl[sciExp].d[1];
    } else {
      buf[pos++] = static_cast<char>('0' + sciExp);
    }
  }

  return pos;
}

inline usize
d2f_buffered(f64 val, char *buf, usize buf_sz, u32 precision)
{
  if ( buf_sz < 4 )
    return 0;
  usize pos = 0;

  // bool negative = false;
  if ( val < 0.0 ) {
    buf[pos++] = '-';
    val = -val;
    // negative = true;
  }

  u64 bits;
  {
    const char *src = reinterpret_cast<const char *>(&val);
    char *dst = reinterpret_cast<char *>(&bits);
    for ( int k = 0; k < 8; ++k )
      dst[k] = src[k];
  }

  u64 ieeeMantissa = bits & ((1ull << 52) - 1);
  u32 ieeeExponent = static_cast<u32>((bits >> 52) & 0x7FF);

  // zero case
  if ( ieeeExponent == 0 && ieeeMantissa == 0 ) {
    buf[pos++] = '0';
    if ( precision > 0 ) {
      buf[pos++] = '.';
      for ( u32 d = 0; d < precision && pos < buf_sz; ++d )
        buf[pos++] = '0';
    }
    return pos;
  }

  // inf nan case
  if ( ieeeExponent == 0x7FF ) {
    if ( ieeeMantissa ) {
      buf[0] = 'n';
      buf[1] = 'a';
      buf[2] = 'n';
      return 3;
    }
    buf[pos] = 'i';
    buf[pos + 1] = 'n';
    buf[pos + 2] = 'f';
    return pos + 3;
  }

  decimal64 dec = d2d(ieeeMantissa, ieeeExponent);
  u64 mantissa = dec.mantissa;
  i32 ryuExp = dec.exponent;
  u32 ryuLen = decimalLength(mantissa);

  // mantissa * 10^ryuExp == d.ddd × 10^sciExp
  // sciExp = ryuExp + ryuLen - 1
  i32 sciExp = ryuExp + static_cast<i32>(ryuLen) - 1;

  i32 intDigits = sciExp + 1;

  char ryuBuf[20];
  char *rend = ryuBuf + 20;
  char *rstart = uint_to_buf_backward(rend, mantissa);
  // rstart[0..ryuLen-1] are the significant digits

  if ( intDigits <= 0 ) {
    buf[pos++] = '0';
    if ( precision > 0 && pos < buf_sz ) {
      buf[pos++] = '.';
      u32 leadZeros = static_cast<u32>(-intDigits);
      for ( u32 d = 0; d < leadZeros && d < precision && pos < buf_sz; ++d )
        buf[pos++] = '0';
      u32 emitted = leadZeros;
      for ( u32 d = 0; emitted < precision && d < ryuLen && pos < buf_sz; ++d, ++emitted )
        buf[pos++] = rstart[d];
      while ( emitted < precision && pos < buf_sz ) {
        buf[pos++] = '0';
        ++emitted;
      }
    }
  } else {
    u32 idig = static_cast<u32>(intDigits);

    for ( u32 d = 0; d < idig && pos < buf_sz; ++d ) {
      if ( d < ryuLen )
        buf[pos++] = rstart[d];
      else
        buf[pos++] = '0';     // trailing zeros in integer part
    }

    if ( precision > 0 && pos < buf_sz ) {
      buf[pos++] = '.';
      u32 emitted = 0;
      for ( u32 d = idig; emitted < precision && pos < buf_sz; ++d, ++emitted ) {
        if ( d < ryuLen )
          buf[pos++] = rstart[d];
        else
          buf[pos++] = '0';
      }
    }
  }

  return pos;
}

inline usize
d2e_buffered(f64 val, char *buf, usize buf_sz, u32 precision)
{
  if ( buf_sz < 8 )
    return 0;
  usize pos = 0;

  if ( val < 0.0 ) {
    buf[pos++] = '-';
    val = -val;
  }

  u64 bits;
  {
    const char *src = reinterpret_cast<const char *>(&val);
    char *dst = reinterpret_cast<char *>(&bits);
    for ( int k = 0; k < 8; ++k )
      dst[k] = src[k];
  }

  u64 ieeeMantissa = bits & ((1ull << 52) - 1);
  u32 ieeeExponent = static_cast<u32>((bits >> 52) & 0x7FF);

  if ( ieeeExponent == 0 && ieeeMantissa == 0 ) {
    buf[pos++] = '0';
    if ( precision > 0 ) {
      buf[pos++] = '.';
      for ( u32 d = 0; d < precision && pos < buf_sz - 5; ++d )
        buf[pos++] = '0';
    }
    buf[pos++] = 'e';
    buf[pos++] = '+';
    buf[pos++] = '0';
    buf[pos++] = '0';
    return pos;
  }

  if ( ieeeExponent == 0x7FF ) {
    if ( ieeeMantissa ) {
      buf[0] = 'n';
      buf[1] = 'a';
      buf[2] = 'n';
      return 3;
    }
    buf[pos] = 'i';
    buf[pos + 1] = 'n';
    buf[pos + 2] = 'f';
    return pos + 3;
  }

  decimal64 dec = d2d(ieeeMantissa, ieeeExponent);
  u64 mantissa = dec.mantissa;
  i32 ryuExp = dec.exponent;
  u32 ryuLen = decimalLength(mantissa);
  i32 sciExp = ryuExp + static_cast<i32>(ryuLen) - 1;

  char ryuBuf[20];
  char *rend = ryuBuf + 20;
  char *rstart = uint_to_buf_backward(rend, mantissa);

  buf[pos++] = rstart[0];

  if ( precision > 0 && pos < buf_sz - 5 ) {
    buf[pos++] = '.';
    u32 emitted = 0;
    for ( u32 d = 1; emitted < precision && pos < buf_sz - 5; ++d, ++emitted ) {
      if ( d < ryuLen )
        buf[pos++] = rstart[d];
      else
        buf[pos++] = '0';
    }
  }

  buf[pos++] = 'e';
  if ( sciExp >= 0 )
    buf[pos++] = '+';
  else {
    buf[pos++] = '-';
    sciExp = -sciExp;
  }
  if ( sciExp >= 100 ) {
    buf[pos++] = static_cast<char>('0' + sciExp / 100);
    sciExp %= 100;
  }
  if ( sciExp >= 10 ) {
    buf[pos++] = __digit_tbl[sciExp].d[0];
    buf[pos++] = __digit_tbl[sciExp].d[1];
  } else {
    buf[pos++] = '0';
    buf[pos++] = static_cast<char>('0' + sciExp);
  }
  return pos;
}

};     // namespace __ryu

inline constexpr int
hex_digit_val(char c)
{
  if ( c >= '0' && c <= '9' )
    return c - '0';
  if ( c >= 'a' && c <= 'f' )
    return c - 'a' + 10;
  if ( c >= 'A' && c <= 'F' )
    return c - 'A' + 10;
  return -1;
}

inline constexpr int
digit_val(char c, u32 base)
{
  int v = -1;
  if ( c >= '0' && c <= '9' )
    v = c - '0';
  else if ( c >= 'a' && c <= 'z' )
    v = c - 'a' + 10;
  else if ( c >= 'A' && c <= 'Z' )
    v = c - 'A' + 10;
  return (v >= 0 && static_cast<u32>(v) < base) ? v : -1;
}

};     // namespace __impl

};     // namespace micron

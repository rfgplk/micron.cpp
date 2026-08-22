//  A double-to-string conversion library: https://github.com/vitaut/zmij/
//
//  Copyright (c) 2025-present, Victor Zverovich
//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "fixed.hpp"

#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse2)
#include "../../simd/aliases/sse.hpp"
#if defined(__micron_x86_avx2)
#include "../../simd/aliases/avx2.hpp"
#endif
#elif (defined(__micron_arch_arm32) || defined(__micron_arch_arm64)) && defined(__micron_arm_neon)
#include "../../simd/aliases/neon.hpp"
#endif

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Schubfach/zmij float -> decimal

namespace micron
{
namespace __impl
{
namespace __zmij
{

struct uint128 {
  u64 hi;
  u64 lo;

  constexpr uint128(u64 h = 0, u64 l = 0) noexcept : hi(h), lo(l) { }
};

inline constexpr uint128
__umul192_hi128(u64 x_hi, u64 x_lo, u64 y) noexcept
{
  const __fmt_uint128_t p = umul128(x_hi, y);
  const u64 lo = p.lo + umul128(x_lo, y).hi;
  return { p.hi + (lo < p.lo), lo };
}

inline constexpr i32
__compute_dec_exp(i32 bin_exp, bool regular = true) noexcept
{
  constexpr i32 log10_3_over_4_sig = 131072;
  constexpr i32 log10_2_sig = 315653;
  constexpr i32 log10_2_exp = 20;
  return (bin_exp * log10_2_sig - static_cast<i32>(!regular) * log10_3_over_4_sig) >> log10_2_exp;
}

inline constexpr i8
__compute_exp_shift(i32 bin_exp, i32 dec_exp) noexcept
{
  constexpr i32 log2_pow10_sig = 217707;
  constexpr i32 log2_pow10_exp = 16;
  const i32 pow10_bin_exp = -dec_exp * log2_pow10_sig >> log2_pow10_exp;
  return static_cast<i8>(bin_exp + pow10_bin_exp + 1);
}

inline constexpr u64 __pow10s[19] = { 1ull,
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
                                      1000000000000000000ull };

inline constexpr u64 __pow10_minor[28]
    = { 0x8000000000000000ull, 0xa000000000000000ull, 0xc800000000000000ull, 0xfa00000000000000ull, 0x9c40000000000000ull,
        0xc350000000000000ull, 0xf424000000000000ull, 0x9896800000000000ull, 0xbebc200000000000ull, 0xee6b280000000000ull,
        0x9502f90000000000ull, 0xba43b74000000000ull, 0xe8d4a51000000000ull, 0x9184e72a00000000ull, 0xb5e620f480000000ull,
        0xe35fa931a0000000ull, 0x8e1bc9bf04000000ull, 0xb1a2bc2ec5000000ull, 0xde0b6b3a76400000ull, 0x8ac7230489e80000ull,
        0xad78ebc5ac620000ull, 0xd8d726b7177a8000ull, 0x878678326eac9000ull, 0xa968163f0a57b400ull, 0xd3c21bcecceda100ull,
        0x84595161401484a0ull, 0xa56fa5b99019a5c8ull, 0xcecb8f27f4200f3aull };

inline constexpr uint128 __pow10_major[25]
    = { { 0xaddcb9e83c6b1793ull, 0xdf4abe242a1bbf3eull }, { 0xaf8e5410288e1b6full, 0x07ecf0ae5ee44ddaull },
        { 0xb1442798f49ffb4aull, 0x99cd11cfdf41779dull }, { 0xb2fe3f0b8599ef07ull, 0x861fa7e6dcb4aa15ull },
        { 0xb4bca50b065abe63ull, 0x0fed077a756b53aaull }, { 0xb67f6455292cbf08ull, 0x1a3bc84c17b1d543ull },
        { 0xb84687c269ef3bfbull, 0x3d5d514f40eea742ull }, { 0xba121a4650e4ddebull, 0x92f34d62616ce413ull },
        { 0xbbe226efb628afeaull, 0x890489f70a55368cull }, { 0xbdb6b8e905cb600full, 0x5400e987bbc1c921ull },
        { 0xbf8fdb78849a5f96ull, 0xde98520472bdd034ull }, { 0xc16d9a0095928a27ull, 0x75b7053c0f178294ull },
        { 0xc350000000000000ull, 0x0000000000000000ull }, { 0xc5371912364ce305ull, 0x6c28000000000000ull },
        { 0xc722f0ef9d80aad6ull, 0x424d3ad2b7b97ef6ull }, { 0xc913936dd571c84cull, 0x03bc3a19cd1e38eaull },
        { 0xcb090c8001ab551cull, 0x5cadf5bfd3072cc6ull }, { 0xcd036837130890a1ull, 0x36dba887c37a8c10ull },
        { 0xcf02b2c21207ef2eull, 0x94f967e45e03f4bcull }, { 0xd106f86e69d785c7ull, 0xe13336d701beba52ull },
        { 0xd31045a8341ca07cull, 0x1ede48111209a051ull }, { 0xd51ea6fa85785631ull, 0x552a74227f3ea566ull },
        { 0xd732290fbacaf133ull, 0xa97c177947ad4096ull }, { 0xd94ad8b1c7380874ull, 0x18375281ae7822bdull },
        { 0xdb68c2ca82ed2a05ull, 0xa67398db9f6820e1ull } };

inline constexpr u32 __pow10_fixups[21] = { 0x8d8fc810u, 0x06100293u, 0x19000000u, 0x00100000u, 0x00000908u, 0x00000000u, 0x04e00300u,
                                            0x3807e0b2u, 0x3d83d793u, 0x0006f5ccu, 0x00000000u, 0xffff0000u, 0x8076337du, 0x4ff45ba0u,
                                            0x09405033u, 0x034376d9u, 0x09000000u, 0x4e100501u, 0x076d14dcu, 0xf964f45eu, 0x0000003du };

struct pow10_significand_table {
#if defined(__OPTIMIZE_SIZE__)
  static constexpr bool compressed = true;
#else
  static constexpr bool compressed = false;
#endif
  static constexpr i32 count = 649;
  uint128 table[compressed ? 1 : count]{};

  static constexpr uint128
  compute(u32 i) noexcept
  {
    constexpr u32 stride = 28;
    const u64 m = __pow10_minor[(i + 24) % stride];
    const uint128 h = __pow10_major[(i + 24) / stride];
    const u64 h1 = umul128(h.lo, m).hi;
    const u64 c0 = h.lo * m;
    const u64 c1 = h1 + h.hi * m;
    const u64 c2 = (c1 < h1) + umul128(h.hi, m).hi;
    uint128 r = (c2 >> 63) != 0 ? uint128{ c2, c1 } : uint128{ c2 << 1 | c1 >> 63, c1 << 1 | c0 >> 63 };
    r.lo -= (__pow10_fixups[i >> 5] >> (i & 31)) & 1u;
    return r;
  }

  constexpr pow10_significand_table() noexcept
  {
    if constexpr ( !compressed )
      for ( i32 i = 0; i < count; ++i ) table[i] = compute(static_cast<u32>(i));
  }

  constexpr uint128
  operator[](i32 dec_exp) const noexcept
  {
    const u32 i = static_cast<u32>(dec_exp + 307);
    if constexpr ( compressed ) return compute(i);
    return table[i];
  }
};

struct exp_shift_table {
#if defined(__OPTIMIZE_SIZE__)
  static constexpr bool enabled = false;
#else
  static constexpr bool enabled = true;
#endif
  static constexpr i32 extra_shift = 9;
  u8 table[enabled ? 2048 : 1]{};

  constexpr exp_shift_table() noexcept
  {
    if constexpr ( enabled ) {
      for ( i32 raw_exp = 0; raw_exp < 2048; ++raw_exp ) {
        i32 bin_exp = raw_exp - 1075;
        if ( raw_exp == 0 ) ++bin_exp;
        const i32 dec_exp = __compute_dec_exp(bin_exp);
        table[raw_exp] = static_cast<u8>(__compute_exp_shift(bin_exp, dec_exp + 1) + extra_shift);
      }
    }
  }
};

struct data {
  u64 threshold = 1000000000000000ull;
  u64 biased_half = (1ull << 63) + 6;
  exp_shift_table exp_shifts{};
  alignas(64) pow10_significand_table pow10_significands{};
};

inline constexpr data __data{};

template<typename F> struct traits;

template<> struct traits<f32> {
  using sig_type = u32;
  static constexpr i32 num_sig_bits = 23;
  static constexpr i32 num_bits = 32;
  static constexpr i32 exp_mask = 255;
  static constexpr i32 exp_offset = 150;
  static constexpr u32 implicit_bit = 1u << 23;
  static constexpr i32 max_digits10 = 9;
};

template<> struct traits<f64> {
  using sig_type = u64;
  static constexpr i32 num_sig_bits = 52;
  static constexpr i32 num_bits = 64;
  static constexpr i32 exp_mask = 2047;
  static constexpr i32 exp_offset = 1075;
  static constexpr u64 implicit_bit = 1ull << 52;
  static constexpr i32 max_digits10 = 17;
};

struct shortest_decimal {
  u64 sig;
  i32 exp;
  i32 last_digit;
  bool has_last_digit;
};

template<typename F, typename U>
inline constexpr shortest_decimal
__to_decimal(U bin_sig, i32 raw_exp, bool regular) noexcept
{
  using T = traits<F>;
  i32 bin_exp = raw_exp - T::exp_offset;
  constexpr i32 extra_shift = exp_shift_table::extra_shift;

  if ( !regular ) {
    const i32 dec_exp = __compute_dec_exp(bin_exp, false);
    const u8 shift = static_cast<u8>(__compute_exp_shift(bin_exp, dec_exp + 1) + extra_shift);
    const uint128 pow10 = __data.pow10_significands[-dec_exp - 1];
    const uint128 p = __umul192_hi128(pow10.hi, pow10.lo, static_cast<u64>(bin_sig) << shift);
    u64 integral = p.hi >> extra_shift;
    const u64 fractional = p.hi << (64 - extra_shift) | p.lo >> extra_shift;
    const u64 half_ulp = pow10.hi >> (extra_shift + 1 - shift);
    const bool round_up = half_ulp > ~u64(0) - fractional;
    const bool round_down = (half_ulp >> 1) > fractional;
    integral += round_up;
    i32 digit = static_cast<i32>(umul128_add_hi64(fractional, 10, (1ull << 63) - 1));
    const i32 lo = static_cast<i32>(umul128_add_hi64(fractional - (half_ulp >> 1), 10, ~u64(0)));
    if ( digit < lo ) digit = lo;
    return { integral, dec_exp, digit, (round_up + round_down) == 0 };
  }

  const i32 dec_exp = __compute_dec_exp(bin_exp);
  u8 shift = exp_shift_table::enabled ? __data.exp_shifts.table[bin_exp + traits<f64>::exp_offset]
                                      : static_cast<u8>(__compute_exp_shift(bin_exp, dec_exp + 1) + extra_shift);
  const u64 even = 1 - (static_cast<u64>(bin_sig) & 1);

  if constexpr ( T::num_bits == 32 ) {
    constexpr i32 float_shift = 34;
    shift = static_cast<u8>(shift + float_shift - extra_shift);
    const u64 pow10_hi = __data.pow10_significands[-dec_exp - 1].hi;
    const u64 p = umul128(pow10_hi + 1, static_cast<u64>(bin_sig) << shift).hi;
    u64 integral = p >> float_shift;
    const u64 fractional = p & ((1ull << float_shift) - 1);
    const u64 half_ulp = (pow10_hi >> (65 - shift)) + even;
    const bool round_up = (fractional + half_ulp) >> float_shift;
    const bool round_down = half_ulp > fractional;
    integral += round_up;
    i32 digit = static_cast<i32>((fractional * 10 + (1ull << (float_shift - 1))) >> float_shift);
    if ( fractional == (1ull << (float_shift - 2)) ) digit = 2;
    return { integral, dec_exp, digit, (round_up + round_down) == 0 };
  } else {
    const uint128 pow10 = __data.pow10_significands[-dec_exp - 1];
    const uint128 p = __umul192_hi128(pow10.hi, pow10.lo, static_cast<u64>(bin_sig) << shift);
    u64 integral = p.hi >> extra_shift;
    const u64 fractional = p.hi << (64 - extra_shift) | p.lo >> extra_shift;
    const u64 half_ulp = (pow10.hi >> (extra_shift + 1 - shift)) + even;
    const bool round_up = fractional + half_ulp < fractional;
    const bool round_down = half_ulp > fractional;
    integral += round_up;
    i32 digit = static_cast<i32>(umul128_add_hi64(fractional, 10, __data.biased_half));
    if ( fractional == (1ull << 62) ) digit = 2;
    return { integral, dec_exp, digit, (round_up + round_down) == 0 };
  }
}

struct decimal_fp {
  u64 sig;
  i32 exp;
  bool negative;
  bool nonfinite;
  bool nan;
};

template<typename F, typename U>
inline constexpr decimal_fp
__to_decimal_untrimmed_bits(U bits, i32 &last_digit, bool &has_last_digit) noexcept
{
  using T = traits<F>;
  i32 bin_exp = static_cast<i32>((bits >> T::num_sig_bits) & static_cast<U>(T::exp_mask));
  U bin_sig = bits & (T::implicit_bit - 1);
  const bool negative = (bits >> (T::num_bits - 1)) != 0;
  if ( bin_exp == 0 || bin_exp == T::exp_mask ) {
    if ( bin_exp != 0 ) return { static_cast<u64>(bin_sig), 0, negative, true, bin_sig != 0 };
    if ( bin_sig == 0 ) return { 0, 0, negative, false, false };
    bin_exp = 1;
    bin_sig |= T::implicit_bit;
  }
  const shortest_decimal dec = __to_decimal<F>(bin_sig ^ T::implicit_bit, bin_exp, bin_sig != 0);
  last_digit = dec.last_digit;
  has_last_digit = dec.has_last_digit;
  return { dec.sig, dec.exp, negative, false, false };
}

template<typename F>
inline constexpr decimal_fp
__to_decimal_untrimmed(F value, i32 &last_digit, bool &has_last_digit) noexcept
{
  using U = typename traits<F>::sig_type;
  return __to_decimal_untrimmed_bits<F, U>(micron::math::ieee::to_bits<F>(value), last_digit, has_last_digit);
}

#if defined(__micron_x86_avx2)
struct __avx_uint128 {
  __m256i hi;
  __m256i lo;
};

inline __m256i
__avx_set_u64(u64 x0, u64 x1, u64 x2, u64 x3) noexcept
{
  return micron::simd::avx2::set_i64(static_cast<i64>(x3), static_cast<i64>(x2), static_cast<i64>(x1), static_cast<i64>(x0));
}

template<int Lane>
inline u64
__avx_extract_u64(__m256i value) noexcept
{
  return static_cast<u64>(micron::simd::avx2::extract_i64<Lane>(value));
}

inline __m256i
__avx_lt_u64(__m256i a, __m256i b) noexcept
{
  const __m256i sign = micron::simd::avx2::splat_i64(-9223372036854775807ll - 1);
  return micron::simd::avx2::gt_i64(micron::simd::avx2::xor_i256(b, sign), micron::simd::avx2::xor_i256(a, sign));
}

inline __avx_uint128
__avx_umul128(__m256i x, __m256i y) noexcept
{
  namespace av = micron::simd::avx2;
  const __m256i mask32 = av::splat_i64(0xffffffffull);
  const __m256i x_hi = av::shr_i64(x, 32);
  const __m256i y_hi = av::shr_i64(y, 32);
  const __m256i p00 = av::mul_2x32_to_2x64_u(x, y);
  const __m256i p10 = av::mul_2x32_to_2x64_u(x_hi, y);
  const __m256i p01 = av::mul_2x32_to_2x64_u(x, y_hi);
  const __m256i p11 = av::mul_2x32_to_2x64_u(x_hi, y_hi);
  const __m256i middle = av::add_i64(av::shr_i64(p00, 32), av::add_i64(av::and_i256(p10, mask32), av::and_i256(p01, mask32)));
  const __m256i lo = av::or_i256(av::and_i256(p00, mask32), av::shl_i64(middle, 32));
  const __m256i hi = av::add_i64(p11, av::add_i64(av::shr_i64(p10, 32), av::add_i64(av::shr_i64(p01, 32), av::shr_i64(middle, 32))));
  return { hi, lo };
}

inline __avx_uint128
__avx_umul192_hi128(__m256i x_hi, __m256i x_lo, __m256i y) noexcept
{
  namespace av = micron::simd::avx2;
  const __m256i one = av::splat_i64(1);
  const __avx_uint128 p = __avx_umul128(x_hi, y);
  const __m256i lo2 = __avx_umul128(x_lo, y).hi;
  const __m256i lo = av::add_i64(p.lo, lo2);
  const __m256i carry = av::and_i256(__avx_lt_u64(lo, p.lo), one);
  return { av::add_i64(p.hi, carry), lo };
}

inline bool
__to_decimal4_avx2(const f64 *values, decimal_fp *dec, i32 *last_digit, bool *has_last_digit) noexcept
{
  namespace av = micron::simd::avx2;
  if constexpr ( !exp_shift_table::enabled ) return false;
  const u64 bits0 = micron::math::ieee::to_bits<f64>(values[0]);
  const u64 bits1 = micron::math::ieee::to_bits<f64>(values[1]);
  const u64 bits2 = micron::math::ieee::to_bits<f64>(values[2]);
  const u64 bits3 = micron::math::ieee::to_bits<f64>(values[3]);
  const u32 raw0 = static_cast<u32>(bits0 >> 52 & 0x7ffu);
  const u32 raw1 = static_cast<u32>(bits1 >> 52 & 0x7ffu);
  const u32 raw2 = static_cast<u32>(bits2 >> 52 & 0x7ffu);
  const u32 raw3 = static_cast<u32>(bits3 >> 52 & 0x7ffu);
  constexpr u64 sig_mask = (1ull << 52) - 1;
  const u64 sig0 = bits0 & sig_mask;
  const u64 sig1 = bits1 & sig_mask;
  const u64 sig2 = bits2 & sig_mask;
  const u64 sig3 = bits3 & sig_mask;
  const bool exceptional = ((raw0 - 1u) >= 2046u) | ((raw1 - 1u) >= 2046u) | ((raw2 - 1u) >= 2046u) | ((raw3 - 1u) >= 2046u) | (sig0 == 0)
                           | (sig1 == 0) | (sig2 == 0) | (sig3 == 0);
  if ( exceptional ) return false;

  const i32 bin0 = static_cast<i32>(raw0) - 1075;
  const i32 bin1 = static_cast<i32>(raw1) - 1075;
  const i32 bin2 = static_cast<i32>(raw2) - 1075;
  const i32 bin3 = static_cast<i32>(raw3) - 1075;
  const i32 exp0 = __compute_dec_exp(bin0);
  const i32 exp1 = __compute_dec_exp(bin1);
  const i32 exp2 = __compute_dec_exp(bin2);
  const i32 exp3 = __compute_dec_exp(bin3);
  const u8 shift0 = __data.exp_shifts.table[raw0];
  const u8 shift1 = __data.exp_shifts.table[raw1];
  const u8 shift2 = __data.exp_shifts.table[raw2];
  const u8 shift3 = __data.exp_shifts.table[raw3];
  const uint128 pow0 = __data.pow10_significands[-exp0 - 1];
  const uint128 pow1 = __data.pow10_significands[-exp1 - 1];
  const uint128 pow2 = __data.pow10_significands[-exp2 - 1];
  const uint128 pow3 = __data.pow10_significands[-exp3 - 1];

  const __m256i sig = __avx_set_u64(sig0 | 1ull << 52, sig1 | 1ull << 52, sig2 | 1ull << 52, sig3 | 1ull << 52);
  const __m256i shifts = __avx_set_u64(shift0, shift1, shift2, shift3);
  const __m256i scaled_sig = av::shl_per_i64(sig, shifts);
  const __avx_uint128 product = __avx_umul192_hi128(__avx_set_u64(pow0.hi, pow1.hi, pow2.hi, pow3.hi),
                                                    __avx_set_u64(pow0.lo, pow1.lo, pow2.lo, pow3.lo), scaled_sig);
  const __m256i integral0 = av::shr_i64(product.hi, 9);
  const __m256i fractional = av::or_i256(av::shl_i64(product.hi, 55), av::shr_i64(product.lo, 9));
  const __m256i half_shift = av::sub_i64(av::splat_i64(10), shifts);
  const __m256i even = av::sub_i64(av::splat_i64(1), av::and_i256(sig, av::splat_i64(1)));
  const __m256i half = av::add_i64(av::shr_per_i64(__avx_set_u64(pow0.hi, pow1.hi, pow2.hi, pow3.hi), half_shift), even);
  const __m256i round_up_mask = __avx_lt_u64(av::add_i64(fractional, half), fractional);
  const __m256i round_down_mask = __avx_lt_u64(fractional, half);
  const __m256i integral = av::add_i64(integral0, av::and_i256(round_up_mask, av::splat_i64(1)));

  const __avx_uint128 digit_product = __avx_umul128(fractional, av::splat_i64(10));
  const __m256i digit_lo = av::add_i64(digit_product.lo, av::splat_i64(static_cast<i64>(__data.biased_half)));
  __m256i digit = av::add_i64(digit_product.hi, av::and_i256(__avx_lt_u64(digit_lo, digit_product.lo), av::splat_i64(1)));
  const __m256i quarter = av::splat_i64(1ull << 62);
  digit = av::blendv_i8(digit, av::splat_i64(2), av::eq_i64(fractional, quarter));

  const __m256i rounded = av::or_i256(round_up_mask, round_down_mask);
  dec[0] = { __avx_extract_u64<0>(integral), exp0, (bits0 >> 63) != 0, false, false };
  dec[1] = { __avx_extract_u64<1>(integral), exp1, (bits1 >> 63) != 0, false, false };
  dec[2] = { __avx_extract_u64<2>(integral), exp2, (bits2 >> 63) != 0, false, false };
  dec[3] = { __avx_extract_u64<3>(integral), exp3, (bits3 >> 63) != 0, false, false };
  last_digit[0] = static_cast<i32>(__avx_extract_u64<0>(digit));
  last_digit[1] = static_cast<i32>(__avx_extract_u64<1>(digit));
  last_digit[2] = static_cast<i32>(__avx_extract_u64<2>(digit));
  last_digit[3] = static_cast<i32>(__avx_extract_u64<3>(digit));
  has_last_digit[0] = __avx_extract_u64<0>(rounded) == 0;
  has_last_digit[1] = __avx_extract_u64<1>(rounded) == 0;
  has_last_digit[2] = __avx_extract_u64<2>(rounded) == 0;
  has_last_digit[3] = __avx_extract_u64<3>(rounded) == 0;
  return true;
}

inline bool
__to_decimal4_avx2(const f32 *values, decimal_fp *dec, i32 *last_digit, bool *has_last_digit) noexcept
{
  namespace av = micron::simd::avx2;
  if constexpr ( !exp_shift_table::enabled ) return false;
  const u32 bits0 = micron::math::ieee::to_bits<f32>(values[0]);
  const u32 bits1 = micron::math::ieee::to_bits<f32>(values[1]);
  const u32 bits2 = micron::math::ieee::to_bits<f32>(values[2]);
  const u32 bits3 = micron::math::ieee::to_bits<f32>(values[3]);
  const u32 raw0 = bits0 >> 23 & 0xffu;
  const u32 raw1 = bits1 >> 23 & 0xffu;
  const u32 raw2 = bits2 >> 23 & 0xffu;
  const u32 raw3 = bits3 >> 23 & 0xffu;
  constexpr u32 sig_mask = (1u << 23) - 1;
  const u32 sig0 = bits0 & sig_mask;
  const u32 sig1 = bits1 & sig_mask;
  const u32 sig2 = bits2 & sig_mask;
  const u32 sig3 = bits3 & sig_mask;
  const bool exceptional = ((raw0 - 1u) >= 254u) | ((raw1 - 1u) >= 254u) | ((raw2 - 1u) >= 254u) | ((raw3 - 1u) >= 254u) | (sig0 == 0)
                           | (sig1 == 0) | (sig2 == 0) | (sig3 == 0);
  if ( exceptional ) return false;

  const i32 bin0 = static_cast<i32>(raw0) - 150;
  const i32 bin1 = static_cast<i32>(raw1) - 150;
  const i32 bin2 = static_cast<i32>(raw2) - 150;
  const i32 bin3 = static_cast<i32>(raw3) - 150;
  const i32 exp0 = __compute_dec_exp(bin0);
  const i32 exp1 = __compute_dec_exp(bin1);
  const i32 exp2 = __compute_dec_exp(bin2);
  const i32 exp3 = __compute_dec_exp(bin3);
  const u8 shift0 = static_cast<u8>(__data.exp_shifts.table[raw0 + 925] + 25);
  const u8 shift1 = static_cast<u8>(__data.exp_shifts.table[raw1 + 925] + 25);
  const u8 shift2 = static_cast<u8>(__data.exp_shifts.table[raw2 + 925] + 25);
  const u8 shift3 = static_cast<u8>(__data.exp_shifts.table[raw3 + 925] + 25);
  const u64 pow0 = __data.pow10_significands[-exp0 - 1].hi + 1;
  const u64 pow1 = __data.pow10_significands[-exp1 - 1].hi + 1;
  const u64 pow2 = __data.pow10_significands[-exp2 - 1].hi + 1;
  const u64 pow3 = __data.pow10_significands[-exp3 - 1].hi + 1;

  const __m256i sig = __avx_set_u64(sig0 | 1u << 23, sig1 | 1u << 23, sig2 | 1u << 23, sig3 | 1u << 23);
  const __m256i shifts = __avx_set_u64(shift0, shift1, shift2, shift3);
  const __m256i product = __avx_umul128(__avx_set_u64(pow0, pow1, pow2, pow3), av::shl_per_i64(sig, shifts)).hi;
  const __m256i fractional = av::and_i256(product, av::splat_i64((1ull << 34) - 1));
  const __m256i even = av::sub_i64(av::splat_i64(1), av::and_i256(sig, av::splat_i64(1)));
  const __m256i half
      = av::add_i64(av::shr_per_i64(__avx_set_u64(pow0 - 1, pow1 - 1, pow2 - 1, pow3 - 1), av::sub_i64(av::splat_i64(65), shifts)), even);
  const __m256i round_up = av::shr_i64(av::add_i64(fractional, half), 34);
  const __m256i round_down_mask = __avx_lt_u64(fractional, half);
  const __m256i integral = av::add_i64(av::shr_i64(product, 34), round_up);
  __m256i digit = av::shr_i64(av::add_i64(__avx_umul128(fractional, av::splat_i64(10)).lo, av::splat_i64(1ull << 33)), 34);
  digit = av::blendv_i8(digit, av::splat_i64(2), av::eq_i64(fractional, av::splat_i64(1ull << 32)));

  dec[0] = { __avx_extract_u64<0>(integral), exp0, (bits0 >> 31) != 0, false, false };
  dec[1] = { __avx_extract_u64<1>(integral), exp1, (bits1 >> 31) != 0, false, false };
  dec[2] = { __avx_extract_u64<2>(integral), exp2, (bits2 >> 31) != 0, false, false };
  dec[3] = { __avx_extract_u64<3>(integral), exp3, (bits3 >> 31) != 0, false, false };
  last_digit[0] = static_cast<i32>(__avx_extract_u64<0>(digit));
  last_digit[1] = static_cast<i32>(__avx_extract_u64<1>(digit));
  last_digit[2] = static_cast<i32>(__avx_extract_u64<2>(digit));
  last_digit[3] = static_cast<i32>(__avx_extract_u64<3>(digit));
  has_last_digit[0] = (__avx_extract_u64<0>(round_up) | __avx_extract_u64<0>(round_down_mask)) == 0;
  has_last_digit[1] = (__avx_extract_u64<1>(round_up) | __avx_extract_u64<1>(round_down_mask)) == 0;
  has_last_digit[2] = (__avx_extract_u64<2>(round_up) | __avx_extract_u64<2>(round_down_mask)) == 0;
  has_last_digit[3] = (__avx_extract_u64<3>(round_up) | __avx_extract_u64<3>(round_down_mask)) == 0;
  return true;
}
#endif

#if ( defined(__micron_arch_arm32) || defined(__micron_arch_arm64) ) && defined(__micron_arm_neon)
inline void
__to_decimal4_neon(const f32 *values, decimal_fp *dec, i32 *last_digit, bool *has_last_digit) noexcept
{
  u32 bits[4];
  const uint32x4_t packed = micron::simd::neon::reinterpret_u32_from_f32(micron::simd::neon::load_f32(values));
  micron::simd::neon::store_u32(bits, packed);
  dec[0] = __to_decimal_untrimmed_bits<f32, u32>(bits[0], last_digit[0], has_last_digit[0]);
  dec[1] = __to_decimal_untrimmed_bits<f32, u32>(bits[1], last_digit[1], has_last_digit[1]);
  dec[2] = __to_decimal_untrimmed_bits<f32, u32>(bits[2], last_digit[2], has_last_digit[2]);
  dec[3] = __to_decimal_untrimmed_bits<f32, u32>(bits[3], last_digit[3], has_last_digit[3]);
}

#if defined(__micron_arch_arm64)
inline void
__to_decimal4_neon(const f64 *values, decimal_fp *dec, i32 *last_digit, bool *has_last_digit) noexcept
{
  u64 bits[4];
  micron::simd::neon::store_u64(bits, micron::simd::neon::reinterpret_u64_from_f64(micron::simd::neon::load_f64(values)));
  micron::simd::neon::store_u64(bits + 2, micron::simd::neon::reinterpret_u64_from_f64(micron::simd::neon::load_f64(values + 2)));
  dec[0] = __to_decimal_untrimmed_bits<f64, u64>(bits[0], last_digit[0], has_last_digit[0]);
  dec[1] = __to_decimal_untrimmed_bits<f64, u64>(bits[1], last_digit[1], has_last_digit[1]);
  dec[2] = __to_decimal_untrimmed_bits<f64, u64>(bits[2], last_digit[2], has_last_digit[2]);
  dec[3] = __to_decimal_untrimmed_bits<f64, u64>(bits[3], last_digit[3], has_last_digit[3]);
}
#endif
#endif

template<typename F>
inline constexpr decimal_fp
to_decimal(F value) noexcept
{
  i32 last_digit = 0;
  bool has_last_digit = false;
  decimal_fp result = __to_decimal_untrimmed(value, last_digit, has_last_digit);
  if ( result.nonfinite || (result.sig == 0 && !has_last_digit) ) return result;
  u64 sig = result.sig * 10 + static_cast<u64>(has_last_digit ? last_digit : 0);
  i32 exp = result.exp;
  while ( sig % 10 == 0 ) {
    sig /= 10;
    ++exp;
  }
  result.sig = sig;
  result.exp = exp;
  return result;
}

inline constexpr usize
__emit_special(const decimal_fp &v, char *buf) noexcept
{
  if ( v.nan ) {
    buf[0] = 'N';
    buf[1] = 'a';
    buf[2] = 'N';
    return 3;
  }
  usize pos = 0;
  if ( v.negative ) buf[pos++] = '-';
  buf[pos++] = 'I';
  buf[pos++] = 'n';
  buf[pos++] = 'f';
  return pos;
}

inline constexpr void __digits18(u64 value, char (&digits)[18]) noexcept;
inline constexpr char *__digits_n(u64 value, u32 count, char (&digits)[18]) noexcept;

inline constexpr usize
__emit_exp_digits(char *buf, usize pos, u32 exp) noexcept
{
  if ( exp >= 100 ) {
    buf[pos++] = static_cast<char>('0' + exp / 100);
    exp %= 100;
    buf[pos++] = static_cast<char>('0' + exp / 10);
  } else if ( exp >= 10 ) {
    buf[pos++] = static_cast<char>('0' + exp / 10);
  }
  buf[pos++] = static_cast<char>('0' + exp % 10);
  return pos;
}

inline constexpr usize
__emit_d2s(decimal_fp dec, i32 last_digit, bool has_last_digit, char *buf) noexcept
{
  if ( dec.nonfinite ) return __emit_special(dec, buf);
  usize pos = 0;
  if ( dec.negative ) buf[pos++] = '-';
  if ( dec.sig == 0 && !has_last_digit ) {
    buf[pos++] = '0';
    buf[pos++] = '.';
    buf[pos++] = '0';
    return pos;
  }

  if ( has_last_digit )
    dec.sig = dec.sig * 10 + static_cast<u64>(last_digit);
  else
    ++dec.exp;
  char digits[18];
  __digits18(dec.sig, digits);
  usize len = __ryu::decimalLength(dec.sig);
  char *start = digits + 18 - len;
  while ( len > 1 && start[len - 1] == '0' ) {
    --len;
    ++dec.exp;
  }
  i32 exp = dec.exp;
  i32 sci_exp = exp + static_cast<i32>(len) - 1;

  if ( sci_exp >= -3 && sci_exp <= 7 ) {
    if ( exp >= 0 ) {
      for ( usize i = 0; i < len; ++i ) buf[pos++] = start[i];
      for ( i32 i = 0; i < exp; ++i ) buf[pos++] = '0';
      buf[pos++] = '.';
      buf[pos++] = '0';
    } else if ( exp + static_cast<i32>(len) > 0 ) {
      const i32 whole = static_cast<i32>(len) + exp;
      for ( i32 i = 0; i < whole; ++i ) buf[pos++] = start[i];
      buf[pos++] = '.';
      for ( usize i = static_cast<usize>(whole); i < len; ++i ) buf[pos++] = start[i];
    } else {
      buf[pos++] = '0';
      buf[pos++] = '.';
      for ( i32 i = 0; i < -(exp + static_cast<i32>(len)); ++i ) buf[pos++] = '0';
      for ( usize i = 0; i < len; ++i ) buf[pos++] = start[i];
    }
    return pos;
  }

  buf[pos++] = start[0];
  if ( len > 1 ) {
    buf[pos++] = '.';
    for ( usize i = 1; i < len; ++i ) buf[pos++] = start[i];
  }
  buf[pos++] = 'e';
  if ( sci_exp >= 0 )
    buf[pos++] = '+';
  else {
    buf[pos++] = '-';
    sci_exp = -sci_exp;
  }
  return __emit_exp_digits(buf, pos, static_cast<u32>(sci_exp));
}

inline constexpr usize
d2s_buffered(f64 value, char *buf) noexcept
{
  i32 last_digit = 0;
  bool has_last_digit = false;
  const decimal_fp dec = __to_decimal_untrimmed(value, last_digit, has_last_digit);
  return __emit_d2s(dec, last_digit, has_last_digit, buf);
}

inline constexpr usize
__emit_f2s(decimal_fp dec, i32 last_digit, bool has_last_digit, char *buf) noexcept
{
  if ( dec.nonfinite ) return __emit_special(dec, buf);
  usize pos = 0;
  if ( dec.negative ) buf[pos++] = '-';
  if ( dec.sig == 0 && !has_last_digit ) {
    buf[pos++] = '0';
    buf[pos++] = 'E';
    buf[pos++] = '0';
    return pos;
  }
  if ( has_last_digit )
    dec.sig = dec.sig * 10 + static_cast<u64>(last_digit);
  else
    ++dec.exp;
  char digits[18];
  usize len = __ryu::decimalLength(dec.sig);
  char *start = __digits_n(dec.sig, static_cast<u32>(len), digits);
  while ( len > 1 && start[len - 1] == '0' ) {
    --len;
    ++dec.exp;
  }
  i32 sci_exp = dec.exp + static_cast<i32>(len) - 1;
  buf[pos++] = start[0];
  if ( len > 1 ) {
    buf[pos++] = '.';
    for ( usize i = 1; i < len; ++i ) buf[pos++] = start[i];
  }
  buf[pos++] = 'E';
  if ( sci_exp < 0 ) {
    buf[pos++] = '-';
    sci_exp = -sci_exp;
  }
  return __emit_exp_digits(buf, pos, static_cast<u32>(sci_exp));
}

inline constexpr usize
f2s_buffered(f32 value, char *buf) noexcept
{
  i32 last_digit = 0;
  bool has_last_digit = false;
  const decimal_fp dec = __to_decimal_untrimmed(value, last_digit, has_last_digit);
  return __emit_f2s(dec, last_digit, has_last_digit, buf);
}

inline constexpr void
d2s_buffered4(const f64 *values, char *out, usize stride, usize *lengths) noexcept
{
#if !defined(__micron_x86_avx2) && !(defined(__micron_arch_arm64) && defined(__micron_arm_neon))
  lengths[0] = d2s_buffered(values[0], out);
  lengths[1] = d2s_buffered(values[1], out + stride);
  lengths[2] = d2s_buffered(values[2], out + stride * 2);
  lengths[3] = d2s_buffered(values[3], out + stride * 3);
  return;
#endif
  decimal_fp dec[4];
  i32 last[4] = {};
  bool has_last[4] = {};
#if defined(__micron_x86_avx2)
  if !consteval {
    if ( !__to_decimal4_avx2(values, dec, last, has_last) ) {
      dec[0] = __to_decimal_untrimmed(values[0], last[0], has_last[0]);
      dec[1] = __to_decimal_untrimmed(values[1], last[1], has_last[1]);
      dec[2] = __to_decimal_untrimmed(values[2], last[2], has_last[2]);
      dec[3] = __to_decimal_untrimmed(values[3], last[3], has_last[3]);
    }
  } else
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if !consteval {
    __to_decimal4_neon(values, dec, last, has_last);
  } else
#endif
  {
    dec[0] = __to_decimal_untrimmed(values[0], last[0], has_last[0]);
    dec[1] = __to_decimal_untrimmed(values[1], last[1], has_last[1]);
    dec[2] = __to_decimal_untrimmed(values[2], last[2], has_last[2]);
    dec[3] = __to_decimal_untrimmed(values[3], last[3], has_last[3]);
  }
  lengths[0] = __emit_d2s(dec[0], last[0], has_last[0], out);
  lengths[1] = __emit_d2s(dec[1], last[1], has_last[1], out + stride);
  lengths[2] = __emit_d2s(dec[2], last[2], has_last[2], out + stride * 2);
  lengths[3] = __emit_d2s(dec[3], last[3], has_last[3], out + stride * 3);
}

inline constexpr void
f2s_buffered4(const f32 *values, char *out, usize stride, usize *lengths) noexcept
{
#if !defined(__micron_x86_avx2) && !((defined(__micron_arch_arm32) || defined(__micron_arch_arm64)) && defined(__micron_arm_neon))
  lengths[0] = f2s_buffered(values[0], out);
  lengths[1] = f2s_buffered(values[1], out + stride);
  lengths[2] = f2s_buffered(values[2], out + stride * 2);
  lengths[3] = f2s_buffered(values[3], out + stride * 3);
  return;
#endif
  decimal_fp dec[4];
  i32 last[4] = {};
  bool has_last[4] = {};
#if defined(__micron_x86_avx2)
  if !consteval {
    if ( !__to_decimal4_avx2(values, dec, last, has_last) ) {
      dec[0] = __to_decimal_untrimmed(values[0], last[0], has_last[0]);
      dec[1] = __to_decimal_untrimmed(values[1], last[1], has_last[1]);
      dec[2] = __to_decimal_untrimmed(values[2], last[2], has_last[2]);
      dec[3] = __to_decimal_untrimmed(values[3], last[3], has_last[3]);
    }
  } else
#elif (defined(__micron_arch_arm32) || defined(__micron_arch_arm64)) && defined(__micron_arm_neon)
  if !consteval {
    __to_decimal4_neon(values, dec, last, has_last);
  } else
#endif
  {
    dec[0] = __to_decimal_untrimmed(values[0], last[0], has_last[0]);
    dec[1] = __to_decimal_untrimmed(values[1], last[1], has_last[1]);
    dec[2] = __to_decimal_untrimmed(values[2], last[2], has_last[2]);
    dec[3] = __to_decimal_untrimmed(values[3], last[3], has_last[3]);
  }
  lengths[0] = __emit_f2s(dec[0], last[0], has_last[0], out);
  lengths[1] = __emit_f2s(dec[1], last[1], has_last[1], out + stride);
  lengths[2] = __emit_f2s(dec[2], last[2], has_last[2], out + stride * 2);
  lengths[3] = __emit_f2s(dec[3], last[3], has_last[3], out + stride * 3);
}

struct precision_decimal {
  u64 sig;
  i32 lead_exp;
};

inline constexpr u64
__scale(u64 bin_sig, i32 bin_exp, i32 dec_exp) noexcept
{
  constexpr i32 shift = 11;
  const i32 point_shift = shift - __compute_exp_shift(bin_exp, dec_exp);
  const uint128 pow10 = __data.pow10_significands[-dec_exp];
  const uint128 p = __umul192_hi128(pow10.hi, pow10.lo + (dec_exp < -55 || dec_exp > 0), bin_sig << shift);
  const u64 integral = p.hi >> point_shift;
  const u64 half = p.hi >> (point_shift - 1) & 1;
  const u64 tail = (p.hi & ((1ull << (point_shift - 1)) - 1)) | p.lo;
  return integral << 2 | half << 1 | (tail != 0);
}

inline constexpr u64
__round_even(u64 x) noexcept
{
  return (x + 1 + ((x >> 2) & 1)) >> 2;
}

inline constexpr precision_decimal
__to_precision(u64 bin_sig, i32 bin_exp, i32 precision) noexcept
{
  i32 dec_exp = __compute_dec_exp(bin_exp + 52) - (precision - 1);
  const u64 scaled = __scale(bin_sig, bin_exp, dec_exp);
  u64 dec_sig = __round_even(scaled);
  if ( dec_sig >= __pow10s[precision] ) {
    dec_sig = __round_even(scaled / 10 | (scaled & 1) | (scaled % 10 != 0));
    ++dec_exp;
  }
  return { dec_sig * __pow10s[18 - precision], dec_exp + precision - 1 };
}

inline constexpr void
__digits18(u64 value, char (&digits)[18]) noexcept
{
  if !consteval {
#if defined(__micron_arch_x86_any) && defined(__micron_x86_sse2)
    const u64 hi = value / 10000000000000000ull;
    digits[0] = static_cast<char>('0' + hi / 10);
    digits[1] = static_cast<char>('0' + hi % 10);
    micron::simd::sse::decimal_digits_16(digits + 2, value - hi * 10000000000000000ull);
    return;
#elif (defined(__micron_arch_arm32) || defined(__micron_arch_arm64)) && defined(__micron_arm_neon)
    const u64 hi = value / 10000000000000000ull;
    digits[0] = static_cast<char>('0' + hi / 10);
    digits[1] = static_cast<char>('0' + hi % 10);
    micron::simd::neon::decimal_digits_16(digits + 2, value - hi * 10000000000000000ull);
    return;
#endif
  }
  for ( i32 i = 18; i-- > 0; ) {
    digits[i] = static_cast<char>('0' + value % 10);
    value /= 10;
  }
}

inline constexpr char *
__digits_n(u64 value, u32 count, char (&digits)[18]) noexcept
{
  char *const end = digits + 18;
  if ( count <= 8 ) {
    micron::__impl::emit8_backward(end - 8, static_cast<u32>(value));
  } else if ( count <= 16 ) {
    const u64 hi = micron::__impl::fast_div1e8(value);
    micron::__impl::emit8_backward(end - 16, static_cast<u32>(hi));
    micron::__impl::emit8_backward(end - 8, micron::__impl::fast_mod1e8(value, hi));
  } else {
    __digits18(value, digits);
  }
  return end - count;
}

inline constexpr void
__normalize(u64 &sig, i32 &exp) noexcept
{
  const i32 shift = static_cast<i32>(micron::__impl::clz64(sig)) - 11;
  if ( shift > 0 ) {
    sig <<= shift;
    exp -= shift;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exact base-2^32 precision path

struct bigint {
  static constexpr i32 capacity = 96;
  u32 limbs[capacity];
  i32 size = 0;

  constexpr explicit bigint(u64 value) noexcept
  {
    limbs[0] = static_cast<u32>(value);
    limbs[1] = static_cast<u32>(value >> 32);
    size = limbs[1] != 0 ? 2 : limbs[0] != 0 ? 1 : 0;
  }

  constexpr void
  trim() noexcept
  {
    while ( size > 0 && limbs[size - 1] == 0 ) --size;
  }

  constexpr void
  shl(i32 bits) noexcept
  {
    if ( size == 0 ) return;
    const i32 words = bits >> 5;
    const i32 shift = bits & 31;
    if ( shift == 0 ) {
      for ( i32 i = size; i-- > 0; ) limbs[i + words] = limbs[i];
      size += words;
    } else {
      limbs[size + words] = limbs[size - 1] >> (32 - shift);
      for ( i32 i = size - 1; i > 0; --i ) limbs[i + words] = limbs[i] << shift | limbs[i - 1] >> (32 - shift);
      limbs[words] = limbs[0] << shift;
      size += words + 1;
    }
    for ( i32 i = 0; i < words; ++i ) limbs[i] = 0;
    trim();
  }

  constexpr void
  mul(u32 factor) noexcept
  {
    u64 carry = 0;
    for ( i32 i = 0; i < size; ++i ) {
      const u64 product = static_cast<u64>(limbs[i]) * factor + carry;
      limbs[i] = static_cast<u32>(product);
      carry = product >> 32;
    }
    if ( carry != 0 ) limbs[size++] = static_cast<u32>(carry);
  }

  constexpr void
  mul_pow5(i32 exp) noexcept
  {
    constexpr u32 pow5[13] = { 1u, 5u, 25u, 125u, 625u, 3125u, 15625u, 78125u, 390625u, 1953125u, 9765625u, 48828125u, 244140625u };
    while ( exp >= 13 ) {
      mul(1220703125u);
      exp -= 13;
    }
    if ( exp != 0 ) mul(pow5[exp]);
  }

  constexpr u32
  divmod_1e9() noexcept
  {
    u64 rem = 0;
    for ( i32 i = size; i-- > 0; ) {
      const u64 value = rem << 32 | limbs[i];
      limbs[i] = static_cast<u32>(value / 1000000000u);
      rem = value % 1000000000u;
    }
    trim();
    return static_cast<u32>(rem);
  }
};

[[gnu::noinline]] inline constexpr char *
__write_big_digits(bigint &value, char *end) noexcept
{
  char *out = end;
  u32 group = value.divmod_1e9();
  while ( value.size != 0 ) {
    out -= 9;
    const u32 hi = group / 100000000u;
    out[0] = static_cast<char>('0' + hi);
    micron::__impl::emit8_backward(out + 1, group - hi * 100000000u);
    group = value.divmod_1e9();
  }
  return micron::__impl::uint_to_buf_backward(out, group);
}

enum class cut : u8 { significant, fraction };

[[gnu::noinline]] inline constexpr void
__exact_round(u64 sig, i32 exp, cut mode, i32 argument, char *digits, u32 &count, i32 &point) noexcept
{
  bigint value(sig);
  i32 base_exp = 0;
  if ( exp >= 0 )
    value.shl(exp);
  else {
    value.mul_pow5(-exp);
    base_exp = exp;
  }

  char exact[__ryu::__fx::__dig_cap];
  char *start = __write_big_digits(value, exact + sizeof(exact));
  const u32 exact_count = static_cast<u32>(exact + sizeof(exact) - start);
  point = static_cast<i32>(exact_count) + base_exp;
  count = 0;

  const i32 keep = mode == cut::significant ? argument : point + argument;
  if ( keep < 0 ) return;
  const u32 kept = static_cast<u32>(keep);
  if ( kept >= exact_count ) {
    for ( u32 i = 0; i < exact_count; ++i ) digits[i] = start[i];
    count = exact_count;
    return;
  }

  bool tail = false;
  for ( u32 i = kept + 1; i < exact_count; ++i ) tail |= start[i] != '0';
  const char dropped = start[kept];
  const bool odd = kept != 0 && ((start[kept - 1] - '0') & 1) != 0;
  const bool up = dropped > '5' || (dropped == '5' && (tail || odd));
  for ( u32 i = 0; i < kept; ++i ) digits[i] = start[i];
  count = kept;
  if ( !up ) return;

  for ( u32 i = kept; i-- > 0; ) {
    if ( digits[i] != '9' ) {
      ++digits[i];
      return;
    }
    digits[i] = '0';
  }
  digits[0] = '1';
  if ( count == 0 ) count = 1;
  ++point;
}

inline constexpr usize
__d2f_exact(const __ryu::__fx::parts &p, char *buf, usize cap, u32 precision) noexcept
{
  char digits[__ryu::__fx::__dig_cap];
  u32 count = 0;
  i32 point = 0;
  __exact_round(p.m2, p.e2, cut::fraction, static_cast<i32>(precision), digits, count, point);
  return __ryu::__fx::__emit_fixed(buf, cap, p.neg, digits, count, point, precision);
}

inline constexpr usize
__d2e_exact(const __ryu::__fx::parts &p, char *buf, usize cap, u32 precision) noexcept
{
  char digits[__ryu::__fx::__dig_cap];
  u32 count = 0;
  i32 point = 0;
  __exact_round(p.m2, p.e2, cut::significant, static_cast<i32>(precision) + 1, digits, count, point);
  return __ryu::__fx::__emit_sci(buf, cap, p.neg, digits, count, point - 1, precision);
}

inline constexpr usize
d2f_buffered(f64 value, char *buf, usize cap, u32 precision) noexcept
{
  const __ryu::__fx::parts p = __ryu::__fx::__decompose(value);
  if ( p.is_nan || p.is_inf ) return __ryu::__fx::__emit_special(p, buf, cap);
  if ( p.is_zero ) return __ryu::__fx::__emit_fixed(buf, cap, p.neg, nullptr, 0, 0, precision);
  if ( precision > 18 ) return __d2f_exact(p, buf, cap, precision);

  const i32 dec_exp = __compute_dec_exp(p.e2 + 52) - 17;
  u64 scaled = __scale(p.m2, p.e2, dec_exp);
  const u64 integral = scaled >> 2;
  const i32 scaled_digits = 18 + static_cast<i32>(integral >= __pow10s[18]);
  i32 lead_exp = dec_exp + scaled_digits - 1;
  const i32 num_digits = lead_exp + 1 + static_cast<i32>(precision);
  if ( num_digits <= 0 ) {
    if ( num_digits < 0 ) return __ryu::__fx::__emit_fixed(buf, cap, p.neg, nullptr, 0, 0, precision);
    const u64 half = 5 * __pow10s[scaled_digits - 1];
    const bool up = integral > half || (integral == half && (scaled & 3) != 0);
    const char one = '1';
    return __ryu::__fx::__emit_fixed(buf, cap, p.neg, up ? &one : nullptr, static_cast<u32>(up), up ? 1 - static_cast<i32>(precision) : 0,
                                     precision);
  }
  if ( num_digits > 18 ) return __d2f_exact(p, buf, cap, precision);
  if ( num_digits < scaled_digits ) {
    const u64 pow = __pow10s[scaled_digits - num_digits];
    const u64 rem = integral % pow;
    const u64 half = rem >= pow / 2;
    const u64 sticky = (rem != pow / 2) || ((scaled & 3) != 0);
    scaled = (integral / pow) << 2 | half << 1 | sticky;
  }
  u64 dec_sig = __round_even(scaled);
  if ( dec_sig >= __pow10s[num_digits] ) {
    ++lead_exp;
    dec_sig /= 10;
  }
  char digits[18];
  char *const start = __digits_n(dec_sig, static_cast<u32>(num_digits), digits);
  return __ryu::__fx::__emit_fixed(buf, cap, p.neg, start, static_cast<u32>(num_digits), lead_exp + 1, precision);
}

inline constexpr usize
d2e_buffered(f64 value, char *buf, usize cap, u32 precision) noexcept
{
  const __ryu::__fx::parts p = __ryu::__fx::__decompose(value);
  if ( p.is_nan || p.is_inf ) return __ryu::__fx::__emit_special(p, buf, cap);
  if ( p.is_zero ) return __ryu::__fx::__emit_sci(buf, cap, p.neg, nullptr, 0, 0, precision);
  const u32 significant = precision + 1;
  if ( significant > 18 ) return __d2e_exact(p, buf, cap, precision);
  u64 sig = p.m2;
  i32 exp = p.e2;
  __normalize(sig, exp);
  const precision_decimal dec = __to_precision(sig, exp, static_cast<i32>(significant));
  char digits[18];
  __digits18(dec.sig, digits);
  return __ryu::__fx::__emit_sci(buf, cap, p.neg, digits, 18, dec.lead_exp, precision);
}

inline constexpr usize
__d2g_core(f64 value, char *buf, usize cap, u32 precision, bool alt, bool upper) noexcept
{
  const __ryu::__fx::parts p = __ryu::__fx::__decompose(value);
  if ( p.is_nan || p.is_inf ) return __ryu::__fx::__emit_special(p, buf, cap);
  const u32 significant = precision == 0 ? 1u : precision;
  if ( significant > 18 ) {
    char digits[__ryu::__fx::__dig_cap];
    u32 count = 0;
    i32 point = 0;
    __exact_round(p.m2, p.e2, cut::significant, static_cast<i32>(significant), digits, count, point);
    const i32 lead_exp = point - 1;
    usize len = 0;
    if ( lead_exp >= -4 && lead_exp < static_cast<i32>(significant) )
      len = __ryu::__fx::__emit_fixed(buf, cap, p.neg, digits, count, point,
                                      static_cast<u32>(static_cast<i32>(significant) - 1 - lead_exp));
    else
      len = __ryu::__fx::__emit_sci(buf, cap, p.neg, digits, count, lead_exp, significant - 1);
    if ( len == 0 ) return 0;
    if ( !alt ) {
      usize dot = len;
      usize exponent = len;
      for ( usize i = 0; i < len; ++i ) {
        if ( buf[i] == '.' )
          dot = i;
        else if ( buf[i] == 'e' ) {
          exponent = i;
          break;
        }
      }
      if ( dot != len ) {
        usize end = exponent;
        while ( end > dot + 1 && buf[end - 1] == '0' ) --end;
        if ( end == dot + 1 ) end = dot;
        if ( end != exponent ) {
          usize out = end;
          for ( usize i = exponent; i < len; ++i ) buf[out++] = buf[i];
          len = out;
        }
      }
    }
    if ( upper )
      for ( usize i = 0; i < len; ++i )
        if ( buf[i] == 'e' ) buf[i] = 'E';
    return len;
  }
  char digits[18];
  for ( char &digit : digits ) digit = '0';
  i32 lead_exp = 0;
  if ( !p.is_zero ) {
    u64 sig = p.m2;
    i32 exp = p.e2;
    __normalize(sig, exp);
    const precision_decimal dec = __to_precision(sig, exp, static_cast<i32>(significant));
    __digits18(dec.sig, digits);
    lead_exp = dec.lead_exp;
  }
  usize len = 0;
  if ( lead_exp >= -4 && lead_exp < static_cast<i32>(significant) )
    len = __ryu::__fx::__emit_fixed(buf, cap, p.neg, digits, 18, lead_exp + 1,
                                    static_cast<u32>(static_cast<i32>(significant) - 1 - lead_exp));
  else
    len = __ryu::__fx::__emit_sci(buf, cap, p.neg, digits, 18, lead_exp, significant - 1);
  if ( len == 0 ) return 0;
  if ( !alt ) {
    usize dot = len;
    usize exponent = len;
    for ( usize i = 0; i < len; ++i ) {
      if ( buf[i] == '.' )
        dot = i;
      else if ( buf[i] == 'e' ) {
        exponent = i;
        break;
      }
    }
    if ( dot != len ) {
      usize end = exponent;
      while ( end > dot + 1 && buf[end - 1] == '0' ) --end;
      if ( end == dot + 1 ) end = dot;
      if ( end != exponent ) {
        usize out = end;
        for ( usize i = exponent; i < len; ++i ) buf[out++] = buf[i];
        len = out;
      }
    }
  }
  if ( upper )
    for ( usize i = 0; i < len; ++i )
      if ( buf[i] == 'e' ) buf[i] = 'E';
  return len;
}

inline constexpr usize
d2g_buffered(f64 value, char *buf, usize cap, u32 precision, bool alt, bool upper) noexcept
{
  if ( precision <= cap && cap - precision >= 8 ) return __d2g_core(value, buf, cap, precision, alt, upper);
  char scratch[__ryu::__fmt_fixed_max];
  const usize len = __d2g_core(value, scratch, sizeof(scratch), precision, alt, upper);
  if ( len == 0 || len > cap ) return 0;
  for ( usize i = 0; i < len; ++i ) buf[i] = scratch[i];
  return len;
}

inline constexpr usize
d2f_trim_buffered(f64 value, char *buf, usize cap, u32 precision) noexcept
{
  usize len = d2f_buffered(value, buf, cap, precision);
  usize dot = len;
  for ( usize i = 0; i < len; ++i )
    if ( buf[i] == '.' ) {
      dot = i;
      break;
    }
  usize end = len;
  if ( dot != len ) {
    while ( end > dot + 1 && buf[end - 1] == '0' ) --end;
    if ( end == dot + 1 ) end = dot;
  }
  if ( end > 0 && buf[0] == '-' ) {
    bool zero = true;
    for ( usize i = 1; i < end; ++i )
      if ( buf[i] != '0' && buf[i] != '.' ) zero = false;
    if ( zero ) {
      for ( usize i = 1; i < end; ++i ) buf[i - 1] = buf[i];
      --end;
    }
  }
  return end;
}

};      // namespace __zmij

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// selected decimal backend

namespace __fpconv
{

inline constexpr usize __fmt_fixed_max = __ryu::__fmt_fixed_max;

inline constexpr usize
d2f_size(f64 value, u32 precision) noexcept
{
  return __ryu::d2f_size(value, precision);
}

inline constexpr usize
d2e_size(f64 value, u32 precision) noexcept
{
  return __ryu::d2e_size(value, precision);
}

inline constexpr usize
d2g_size(f64 value, u32 precision, bool alt) noexcept
{
  return __ryu::d2g_size(value, precision, alt);
}

#if defined(MICRON_USE_RYU)
inline constexpr usize
d2s_buffered(f64 v, char *b) noexcept
{
  return __ryu::d2s_buffered(v, b);
}

inline constexpr usize
f2s_buffered(f32 v, char *b) noexcept
{
  return __ryu::__f32::f2s_buffered(v, b);
}

inline constexpr void
d2s_buffered4(const f64 *v, char *b, usize stride, usize *lengths) noexcept
{
  for ( usize i = 0; i < 4; ++i ) lengths[i] = __ryu::d2s_buffered(v[i], b + i * stride);
}

inline constexpr void
f2s_buffered4(const f32 *v, char *b, usize stride, usize *lengths) noexcept
{
  for ( usize i = 0; i < 4; ++i ) lengths[i] = __ryu::__f32::f2s_buffered(v[i], b + i * stride);
}

inline constexpr usize
d2f_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __ryu::d2f_buffered(v, b, c, p);
}

inline constexpr usize
d2f_trim_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __ryu::d2f_trim_buffered(v, b, c, p);
}

inline constexpr usize
d2e_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __ryu::d2e_buffered(v, b, c, p);
}

inline constexpr usize
d2g_buffered(f64 v, char *b, usize c, u32 p, bool a, bool u) noexcept
{
  return __ryu::d2g_buffered(v, b, c, p, a, u);
}
#else
inline constexpr usize
d2s_buffered(f64 v, char *b) noexcept
{
  return __zmij::d2s_buffered(v, b);
}

inline constexpr usize
f2s_buffered(f32 v, char *b) noexcept
{
  return __zmij::f2s_buffered(v, b);
}

inline constexpr void
d2s_buffered4(const f64 *v, char *b, usize stride, usize *lengths) noexcept
{
  __zmij::d2s_buffered4(v, b, stride, lengths);
}

inline constexpr void
f2s_buffered4(const f32 *v, char *b, usize stride, usize *lengths) noexcept
{
  __zmij::f2s_buffered4(v, b, stride, lengths);
}

inline constexpr usize
d2f_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __zmij::d2f_buffered(v, b, c, p);
}

inline constexpr usize
d2f_trim_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __zmij::d2f_trim_buffered(v, b, c, p);
}

inline constexpr usize
d2e_buffered(f64 v, char *b, usize c, u32 p) noexcept
{
  return __zmij::d2e_buffered(v, b, c, p);
}

inline constexpr usize
d2g_buffered(f64 v, char *b, usize c, u32 p, bool a, bool u) noexcept
{
  return __zmij::d2g_buffered(v, b, c, p, a, u);
}
#endif

inline constexpr usize
d2a_buffered(f64 v, char *b, usize c, u32 p, bool h, bool u) noexcept
{
  return __ryu::d2a_buffered(v, b, c, p, h, u);
}

};      // namespace __fpconv
};      // namespace __impl
};      // namespace micron

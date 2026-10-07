// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
#pragma once

#include "../float16.hpp"
#include "../ieee.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Explicit mixed storage and widened products; caller owns every input/output buffer

namespace micron::math::blas::mixed
{
enum class status { ok, bad_dimension, bad_zero_point, accumulator_overflow };

template<class T>
concept accumulator = ieee754_floating<T> && (sizeof(T) == 4 || sizeof(T) == 8);

template<class T>
concept floating_input = accumulator<T> || micron::is_same_v<T, float16> || micron::is_same_v<T, bfloat16>;

namespace __impl
{
constexpr bool
__layout(usize rows, usize columns, usize ld) noexcept
{
  return !rows || !columns || (ld >= columns && rows - 1 <= (usize(-1) - columns) / ld);
}

template<accumulator Acc, floating_input T>
constexpr Acc
__read(const T &value) noexcept
{
  if constexpr ( accumulator<T> )
    return Acc(value);
  else if constexpr ( sizeof(Acc) == 8 )
    return Acc(value.to_double());
  else
    return Acc(value.to_float());
}

template<accumulator F> struct __fraction {
  F mantissa;
  i32 exponent;
};

// Normalize via bits so FTZ/DAZ cannot turn a valid subnormal scale into zero.
template<accumulator F>
constexpr __fraction<F>
__normalize(typename ieee::traits<F>::uint_type raw) noexcept
{
  using traits = ieee::traits<F>;
  using U = typename traits::uint_type;
  const U exponent = (raw & traits::exp_mask) >> traits::mant_bits;
  U fraction = raw & traits::mant_mask;
  i32 e = i32(exponent ? exponent : 1) - traits::exp_bias;
  if ( !exponent )
    while ( !(fraction & traits::implicit_one) ) {
      fraction <<= 1;
      --e;
    }
  return { __builtin_bit_cast(F, (U(traits::exp_bias) << traits::mant_bits) | (fraction & traits::mant_mask)), e };
}
};      // namespace __impl

// Each operand is converted to Acc before multiplication. Output stays in Acc;
// narrowing is a separate explicit conversion. beta==0 never reads C. No packing,
// runtime ISA selection or implicit heap allocation; compiler vectorization is allowed.
template<accumulator Acc, floating_input A, floating_input B>
[[nodiscard]] constexpr status
gemm_row(bool ta, bool tb, usize m, usize n, usize k, Acc alpha, const A *a, usize lda, const B *b, usize ldb, Acc beta, Acc *c,
         usize ldc) noexcept
{
  if ( !m || !n ) return status::ok;
  if ( !c || !__impl::__layout(m, n, ldc)
       || (k && alpha != Acc(0)
           && (!a || !b || !__impl::__layout(ta ? k : m, ta ? m : k, lda) || !__impl::__layout(tb ? n : k, tb ? k : n, ldb))) )
    return status::bad_dimension;
  for ( usize i = 0; i < m; ++i )
    for ( usize j = 0; j < n; ++j ) {
      Acc sum{};
      if ( alpha != Acc(0) )
        for ( usize z = 0; z < k; ++z )
          sum += __impl::__read<Acc>(a[ta ? z * lda + i : i * lda + z]) * __impl::__read<Acc>(b[tb ? j * ldb + z : z * ldb + j]);
      c[i * ldc + j] = alpha * sum + (beta == Acc(0) ? Acc(0) : beta * c[i * ldc + j]);
    }
  return status::ok;
}

template<class T>
concept integer_accumulator = micron::is_same_v<T, i32> || micron::is_same_v<T, i64>;

template<integer_accumulator Acc> inline constexpr u64 max_i8_terms = (sizeof(Acc) == 4 ? 2147483647ull : 9223372036854775807ull) / 65025;

// Exact sum((A-zero_a)*(B-zero_b)) in i32/i64, with a conservative worst-case bound
// checked before writing. Both zero points are signed int8 values. Scaling and bias
// follow in floating point: output=Acc_sum*scale_a*scale_b[column]+bias[column].
template<integer_accumulator Acc>
[[nodiscard]] constexpr status
gemm_i8_row(bool ta, bool tb, usize m, usize n, usize k, const i8 *a, usize lda, i32 zero_a, const i8 *b, usize ldb, i32 zero_b, Acc *c,
            usize ldc) noexcept
{
  if ( zero_a < -128 || zero_a > 127 || zero_b < -128 || zero_b > 127 ) return status::bad_zero_point;
  if ( u64(k) > max_i8_terms<Acc> ) return status::accumulator_overflow;
  if ( !m || !n ) return status::ok;
  if ( !c || !__impl::__layout(m, n, ldc)
       || (k && (!a || !b || !__impl::__layout(ta ? k : m, ta ? m : k, lda) || !__impl::__layout(tb ? n : k, tb ? k : n, ldb))) )
    return status::bad_dimension;
  for ( usize i = 0; i < m; ++i )
    for ( usize j = 0; j < n; ++j ) {
      Acc sum{};
      for ( usize z = 0; z < k; ++z )
        sum += Acc(i32(a[ta ? z * lda + i : i * lda + z]) - zero_a) * Acc(i32(b[tb ? j * ldb + z : z * ldb + j]) - zero_b);
      c[i * ldc + j] = sum;
    }
  return status::ok;
}

// Positive finite scale and signed-int8 zero point are caller preconditions.
// RNE is applied before adding the zero point; saturation follows. NaN maps to
// zero_point and infinities saturate. Model interfaces may reject nonfinite input.
template<accumulator F>
[[nodiscard]] constexpr i8
quantize_i8(const F &value, F scale, i32 zero_point = 0) noexcept
{
  using traits = ieee::traits<F>;
  const auto raw = __builtin_bit_cast(typename traits::uint_type, value);
  if ( (raw & traits::exp_mask) == traits::exp_mask )
    return raw & traits::mant_mask ? i8(zero_point) : (raw & traits::sign_mask ? i8(-128) : i8(127));
  if ( (raw & ~traits::sign_mask) == 0 ) return i8(zero_point);
  const auto x = __impl::__normalize<F>(raw), s = __impl::__normalize<F>(__builtin_bit_cast(typename traits::uint_type, scale));
  const i32 exponent = x.exponent - s.exponent;
  if ( exponent >= 9 ) return raw & traits::sign_mask ? i8(-128) : i8(127);
  if ( exponent < -1 ) return i8(zero_point);
  using U = typename traits::uint_type;
  const F power = __builtin_bit_cast(F, U(exponent + traits::exp_bias) << traits::mant_bits);
  const F magnitude = (x.mantissa / s.mantissa) * power;
  const F scaled = raw & traits::sign_mask ? -magnitude : magnitude;
  if ( scaled >= F(127 - zero_point) ) return 127;
  if ( scaled <= F(-128 - zero_point) ) return -128;
  i32 integral = i32(scaled);
  const F remainder = scaled - F(integral), absolute = remainder < F(0) ? -remainder : remainder;
  if ( absolute > F(.5) || (absolute == F(.5) && (integral % 2)) ) integral += scaled < F(0) ? -1 : 1;
  return i8(integral + zero_point);
}
};      // namespace micron::math::blas::mixed

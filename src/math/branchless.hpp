//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
namespace math
{
namespace branchless
{

[[nodiscard, gnu::always_inline]] inline constexpr i8
abs8(i8 x) noexcept
{
  const i8 m = i8(x >> 7);
  return i8((x ^ m) - m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
abs16(i16 x) noexcept
{
  const i16 m = i16(x >> 15);
  return i16((x ^ m) - m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
abs32(i32 x) noexcept
{
  const i32 m = x >> 31;
  return (x ^ m) - m;
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
abs64(i64 x) noexcept
{
  const i64 m = x >> 63;
  return (x ^ m) - m;
}

[[nodiscard, gnu::always_inline]] inline constexpr i8
sign8(i8 x) noexcept
{
  return i8(i8(x >> 7) | i8(u8(-u8(x)) >> 7));
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
sign16(i16 x) noexcept
{
  return i16(i16(x >> 15) | i16(u16(-u16(x)) >> 15));
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
sign32(i32 x) noexcept
{
  return i32(x >> 31) | i32(u32(-u32(x)) >> 31);
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
sign64(i64 x) noexcept
{
  return i64(x >> 63) | i64(u64(-u64(x)) >> 63);
}

[[nodiscard, gnu::always_inline]] inline constexpr i8
min8(i8 a, i8 b) noexcept
{
  const i32 d = i32(a) - i32(b);
  return i8(b + (d & (d >> 31)));
}

[[nodiscard, gnu::always_inline]] inline constexpr i8
max8(i8 a, i8 b) noexcept
{
  const i32 d = i32(a) - i32(b);
  return i8(a - (d & (d >> 31)));
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
min16(i16 a, i16 b) noexcept
{
  const i32 d = i32(a) - i32(b);
  return i16(b + (d & (d >> 31)));
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
max16(i16 a, i16 b) noexcept
{
  const i32 d = i32(a) - i32(b);
  return i16(a - (d & (d >> 31)));
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
min32(i32 a, i32 b) noexcept
{
  const i64 d = i64(a) - i64(b);
  return i32(b + (d & (d >> 63)));
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
max32(i32 a, i32 b) noexcept
{
  const i64 d = i64(a) - i64(b);
  return i32(a - (d & (d >> 63)));
}

// a - b overflows only when the operands differ in sign
[[nodiscard, gnu::always_inline]] inline constexpr i64
__lt_mask64(i64 a, i64 b) noexcept
{
  const i64 x = a ^ b;
  return ((x & a) | (~x & i64(u64(a) - u64(b)))) >> 63;
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
min64(i64 a, i64 b) noexcept
{
  const i64 m = __lt_mask64(a, b);
  return (a & m) | (b & ~m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
max64(i64 a, i64 b) noexcept
{
  const i64 m = __lt_mask64(a, b);
  return (b & m) | (a & ~m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i8
clamp8(i8 x, i8 lo, i8 hi) noexcept
{
  return min8(max8(x, lo), hi);
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
clamp16(i16 x, i16 lo, i16 hi) noexcept
{
  return min16(max16(x, lo), hi);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
clamp32(i32 x, i32 lo, i32 hi) noexcept
{
  return min32(max32(x, lo), hi);
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
clamp64(i64 x, i64 lo, i64 hi) noexcept
{
  return min64(max64(x, lo), hi);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
lt8(i8 a, i8 b) noexcept
{
  return i32(u32(i32(a) - i32(b)) >> 31);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
lt16(i16 a, i16 b) noexcept
{
  return i32(u32(i32(a) - i32(b)) >> 31);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
lt32(i32 a, i32 b) noexcept
{
  return i32(u64(i64(a) - i64(b)) >> 63);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
lt64(i64 a, i64 b) noexcept
{
  return i32(__lt_mask64(a, b) & 1);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
gt8(i8 a, i8 b) noexcept
{
  return lt8(b, a);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
gt16(i16 a, i16 b) noexcept
{
  return lt16(b, a);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
gt32(i32 a, i32 b) noexcept
{
  return lt32(b, a);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
gt64(i64 a, i64 b) noexcept
{
  return lt64(b, a);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
le8(i8 a, i8 b) noexcept
{
  return 1 - gt8(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
le16(i16 a, i16 b) noexcept
{
  return 1 - gt16(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
le32(i32 a, i32 b) noexcept
{
  return 1 - gt32(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
le64(i64 a, i64 b) noexcept
{
  return 1 - gt64(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
ge8(i8 a, i8 b) noexcept
{
  return 1 - lt8(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
ge16(i16 a, i16 b) noexcept
{
  return 1 - lt16(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
ge32(i32 a, i32 b) noexcept
{
  return 1 - lt32(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
ge64(i64 a, i64 b) noexcept
{
  return 1 - lt64(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
eq8(i8 a, i8 b) noexcept
{
  return !(a ^ b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
eq16(i16 a, i16 b) noexcept
{
  return !(a ^ b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
eq32(i32 a, i32 b) noexcept
{
  return !(a ^ b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
eq64(i64 a, i64 b) noexcept
{
  return !(a ^ b);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
bit8(u8 x, int n) noexcept
{
  return i32((x >> n) & 1);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
bit16(u16 x, int n) noexcept
{
  return i32((x >> n) & 1);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
bit32(u32 x, int n) noexcept
{
  return i32((x >> n) & 1);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
bit64(u64 x, int n) noexcept
{
  return i32((x >> n) & 1);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mask8(u8 x, u8 m) noexcept
{
  return !!(x & m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mask16(u16 x, u16 m) noexcept
{
  return !!(x & m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mask32(u32 x, u32 m) noexcept
{
  return !!(x & m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mask64(u64 x, u64 m) noexcept
{
  return !!(x & m);
}

[[nodiscard, gnu::always_inline]] inline constexpr i8
mod_pow2_8(i8 x, int p) noexcept
{
  return i8(x & i8((1 << p) - 1));
}

[[nodiscard, gnu::always_inline]] inline constexpr i16
mod_pow2_16(i16 x, int p) noexcept
{
  return i16(x & i16((1 << p) - 1));
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mod_pow2_32(i32 x, int p) noexcept
{
  return i32(u32(x) & ~(~u32(0) << p));
}

[[nodiscard, gnu::always_inline]] inline constexpr i64
mod_pow2_64(i64 x, int p) noexcept
{
  return i64(u64(x) & ~(~u64(0) << p));
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
mod3(u32 a) noexcept
{
  constexpr u32 m = 0xAAAAAAABu;
  const u32 q = u32((u64(a) * m) >> 33);
  return a - q * 3u;
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
mod5(u32 a) noexcept
{
  constexpr u32 m = 0xCCCCCCCDu;
  const u32 q = u32((u64(a) * m) >> 34);
  return a - q * 5u;
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
mod7(u32 a) noexcept
{
  constexpr u32 m = 0x24924925u;
  u32 q = u32((u64(a) * m) >> 32);
  q += (a - q) >> 1;
  q >>= 2;
  return a - q * 7u;
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
div_const(u32 a, u32 d) noexcept
{
  const u64 m = (u64(1) << 32) / d;
  const u32 q = u32((u64(a) * m) >> 32);
  return q + u32((a - q * d) >= d);
}

[[nodiscard, gnu::always_inline]] inline constexpr i32
mul_const(i32 x, i32 k) noexcept
{
  u32 r = 0;
  for ( u32 i = 0, uk = u32(k); uk; ++i, uk >>= 1 ) {
    if ( uk & 1 ) r += u32(x) << i;
  }
  return i32(r);
}

[[nodiscard, gnu::always_inline]] inline constexpr u32
swap_bits32(u32 x, u32 mask, int shift) noexcept
{
  const u32 t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}

[[nodiscard, gnu::always_inline]] inline constexpr u64
swap_bits64(u64 x, u64 mask, int shift) noexcept
{
  const u64 t = ((x >> shift) ^ x) & mask;
  return x ^ t ^ (t << shift);
}

};      // namespace branchless
};      // namespace math
};      // namespace micron

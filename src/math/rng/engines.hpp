//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// main rng engines
//   splitmix64       = 8 byte state, period 2^64
//   xoshiro256ss     = 32 byte state, period 2^256-1
//   xoshiro128ss     = 16 byte state, period 2^128-1
//   pcg64            = 32 byte state, period 2^128
//   mt19937          = 2.5 KiB state, period 2^19937-1
//   mwc64            = 16 byte state
//   lcg64            = 8 byte state, period 2^64

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"

namespace micron
{
namespace math
{
namespace rng
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SplitMix64
struct splitmix64 {
  using result_type = u64;
  static constexpr u32 result_bits = 64;
  static constexpr u64 increment = 0x9E3779B97F4A7C15ULL;

  u64 s;

  constexpr splitmix64() noexcept : s(0) { }

  constexpr explicit splitmix64(u64 seed) noexcept : s(seed) { }

  [[nodiscard]] static constexpr splitmix64
  from_seed(u64 seed) noexcept
  {
    return splitmix64(seed);
  }

  [[nodiscard, gnu::always_inline]] static constexpr u64
  __mix(u64 z) noexcept
  {
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    s += increment;
    return __mix(s);
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    return next();
  }

  inline constexpr void
  generate(u64 *__restrict__ out, usize n) noexcept
  {
    u64 state = s;
    constexpr u64 i2 = increment * 2ULL;
    constexpr u64 i3 = increment * 3ULL;
    constexpr u64 i4 = increment * 4ULL;
    constexpr u64 i5 = increment * 5ULL;
    constexpr u64 i6 = increment * 6ULL;
    constexpr u64 i7 = increment * 7ULL;
    constexpr u64 i8 = increment * 8ULL;
    while ( n >= 8 ) {
      out[0] = __mix(state + increment);
      out[1] = __mix(state + i2);
      out[2] = __mix(state + i3);
      out[3] = __mix(state + i4);
      out[4] = __mix(state + i5);
      out[5] = __mix(state + i6);
      out[6] = __mix(state + i7);
      out[7] = __mix(state + i8);
      state += i8;
      out += 8;
      n -= 8;
    }
    while ( n-- != 0 ) {
      state += increment;
      *out++ = __mix(state);
    }
    s = state;
  }
};

static_assert(sizeof(splitmix64) == 8, "splitmix64 state must be 8 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// xor-shift-rotate engines
struct xoshiro256ss {
  using result_type = u64;
  static constexpr u32 result_bits = 64;

  u64 s[4];

  constexpr xoshiro256ss() noexcept : s{ 0xE220A8397B1DCDAFULL, 0x6E789E6AA1B965F4ULL, 0x06C45D188009454FULL, 0xF88BB8A8724C81ECULL } { }

  constexpr xoshiro256ss(u64 a, u64 b, u64 c, u64 d) noexcept : s{ a, b, c, d }
  {
    if ( (a | b | c | d) == 0 ) s[0] = 0x9E3779B97F4A7C15ULL;
  }

  [[nodiscard]] static constexpr xoshiro256ss
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    return xoshiro256ss{ sm.next(), sm.next(), sm.next(), sm.next() };
  }

  [[nodiscard, gnu::always_inline]] static constexpr u64
  __step(u64 &s0, u64 &s1, u64 &s2, u64 &s3) noexcept
  {
    const u64 result = bits::rol64(s1 * 5ULL, 7) * 9ULL;
    const u64 t = s1 << 17;
    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;
    s2 ^= t;
    s3 = bits::rol64(s3, 45);
    return result;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    return __step(s[0], s[1], s[2], s[3]);
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    return next();
  }

  inline constexpr void
  generate(u64 *__restrict__ out, usize n) noexcept
  {
    u64 s0 = s[0], s1 = s[1], s2 = s[2], s3 = s[3];
    while ( n >= 4 ) {
      out[0] = __step(s0, s1, s2, s3);
      out[1] = __step(s0, s1, s2, s3);
      out[2] = __step(s0, s1, s2, s3);
      out[3] = __step(s0, s1, s2, s3);
      out += 4;
      n -= 4;
    }
    while ( n-- != 0 ) *out++ = __step(s0, s1, s2, s3);
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }

  constexpr void
  jump() noexcept
  {
    static constexpr u64 J[] = { 0x180EC6D33CFD0ABAULL, 0xD5A61266F0C9392CULL, 0xA9582618E03FC9AAULL, 0x39ABDC4529B1661CULL };
    u64 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 64; ++b ) {
        const u64 mask = u64(0) - u64((J[i] >> b) & 1ULL);
        s0 ^= s[0] & mask;
        s1 ^= s[1] & mask;
        s2 ^= s[2] & mask;
        s3 ^= s[3] & mask;
        (void)next();
      }
    }
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }

  constexpr void
  long_jump() noexcept
  {
    static constexpr u64 J[] = { 0x76E15D3EFEFDCBBFULL, 0xC5004E441C522FB3ULL, 0x77710069854EE241ULL, 0x39109BB02ACBE635ULL };
    u64 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 64; ++b ) {
        const u64 mask = u64(0) - u64((J[i] >> b) & 1ULL);
        s0 ^= s[0] & mask;
        s1 ^= s[1] & mask;
        s2 ^= s[2] & mask;
        s3 ^= s[3] & mask;
        (void)next();
      }
    }
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }
};

static_assert(sizeof(xoshiro256ss) == 32, "xoshiro256ss state must be 32 bytes");

struct xoshiro128ss {
  using result_type = u32;
  static constexpr u32 result_bits = 32;

  u32 s[4];

  constexpr xoshiro128ss() noexcept : s{ 0x7B1DCDAFu, 0xE220A839u, 0xA1B965F4u, 0x6E789E6Au } { }

  constexpr xoshiro128ss(u32 a, u32 b, u32 c, u32 d) noexcept : s{ a, b, c, d }
  {
    if ( (a | b | c | d) == 0 ) s[0] = 0x9E3779B9u;
  }

  [[nodiscard]] static constexpr xoshiro128ss
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    const u64 a = sm.next();
    const u64 b = sm.next();
    return xoshiro128ss{ u32(a), u32(a >> 32), u32(b), u32(b >> 32) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr u32
  __step(u32 &s0, u32 &s1, u32 &s2, u32 &s3) noexcept
  {
    const u32 result = bits::rol32(s1 * 5u, 7) * 9u;
    const u32 t = s1 << 9;
    s2 ^= s0;
    s3 ^= s1;
    s1 ^= s2;
    s0 ^= s3;
    s2 ^= t;
    s3 = bits::rol32(s3, 11);
    return result;
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next() noexcept
  {
    return __step(s[0], s[1], s[2], s[3]);
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next32() noexcept
  {
    return next();
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    const u64 lo = next();
    const u64 hi = next();
    return lo | (hi << 32);
  }

  inline constexpr void
  generate(u32 *__restrict__ out, usize n) noexcept
  {
    u32 s0 = s[0], s1 = s[1], s2 = s[2], s3 = s[3];
    while ( n >= 4 ) {
      out[0] = __step(s0, s1, s2, s3);
      out[1] = __step(s0, s1, s2, s3);
      out[2] = __step(s0, s1, s2, s3);
      out[3] = __step(s0, s1, s2, s3);
      out += 4;
      n -= 4;
    }
    while ( n-- != 0 ) *out++ = __step(s0, s1, s2, s3);
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }

  constexpr void
  jump() noexcept
  {
    static constexpr u32 J[] = { 0x8764000Bu, 0xF542D2D3u, 0x6FA035C3u, 0x77F2DB5Bu };
    u32 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 32; ++b ) {
        const u32 mask = u32(0) - u32((J[i] >> b) & 1u);
        s0 ^= s[0] & mask;
        s1 ^= s[1] & mask;
        s2 ^= s[2] & mask;
        s3 ^= s[3] & mask;
        (void)next();
      }
    }
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }

  constexpr void
  long_jump() noexcept
  {
    static constexpr u32 J[] = { 0xB523952Eu, 0x0B6F099Fu, 0xCCF5A0EFu, 0x1C580662u };
    u32 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 32; ++b ) {
        const u32 mask = u32(0) - u32((J[i] >> b) & 1u);
        s0 ^= s[0] & mask;
        s1 ^= s[1] & mask;
        s2 ^= s[2] & mask;
        s3 ^= s[3] & mask;
        (void)next();
      }
    }
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }
};

static_assert(sizeof(xoshiro128ss) == 16, "xoshiro128ss state must be 16 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// PCG XSL-RR 128/64
struct pcg64 {
  using result_type = u64;
  static constexpr u32 result_bits = 64;

  u128 state;
  u128 inc;

  [[nodiscard, gnu::always_inline]] static constexpr u128
  __multiplier() noexcept
  {
    return (u128(0x2360ED051FC65DA4ULL) << 64) | u128(0x4385DF649FCCF645ULL);
  }

  [[nodiscard, gnu::always_inline]] static constexpr u64
  __output(u128 old) noexcept
  {
    const u64 xsl = u64(old >> 64) ^ u64(old);
    const u32 rot = u32(u64(old >> 122));
    return bits::ror64(xsl, int(rot));
  }

  constexpr pcg64() noexcept : state(0), inc(3)
  {
    state = state * __multiplier() + inc;
    state = state * __multiplier() + inc;
  }

  constexpr pcg64(u128 s, u128 i) noexcept : state(s), inc(i | u128(1)) { }

  [[nodiscard]] static constexpr pcg64
  make(u64 seed, u64 stream = 1) noexcept
  {
    pcg64 r{ u128(0), (u128(stream) << 1) | u128(1) };
    r.state = r.state * __multiplier() + r.inc;
    r.state += u128(seed);
    r.state = r.state * __multiplier() + r.inc;
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    const u128 old = state;
    state = old * __multiplier() + inc;
    return __output(old);
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    return next();
  }

  inline constexpr void
  generate(u64 *__restrict__ out, usize n) noexcept
  {
    u128 st = state;
    const u128 increment_value = inc;
    const u128 a1 = __multiplier();
    const u128 a2 = a1 * a1;
    const u128 a3 = a2 * a1;
    const u128 a4 = a2 * a2;
    const u128 c2 = increment_value * (a1 + u128(1));
    const u128 c3 = increment_value * (a2 + a1 + u128(1));
    const u128 c4 = increment_value * (a3 + a2 + a1 + u128(1));
    while ( n >= 4 ) {
      const u128 s1 = st * a1 + increment_value;
      const u128 s2 = st * a2 + c2;
      const u128 s3 = st * a3 + c3;
      out[0] = __output(st);
      out[1] = __output(s1);
      out[2] = __output(s2);
      out[3] = __output(s3);
      st = st * a4 + c4;
      out += 4;
      n -= 4;
    }
    while ( n-- != 0 ) {
      const u128 old = st;
      st = old * a1 + increment_value;
      *out++ = __output(old);
    }
    state = st;
  }
};

static_assert(sizeof(pcg64) == 32, "pcg64 state must be 32 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MT19937
struct mt19937 {
  using result_type = u32;
  static constexpr u32 result_bits = 32;
  static constexpr usize N = 624;
  static constexpr usize M = 397;
  static constexpr u32 MATRIX_A = 0x9908B0DFu;
  static constexpr u32 UPPER_MASK = 0x80000000u;
  static constexpr u32 LOWER_MASK = 0x7FFFFFFFu;

  u32 mt[N];
  u32 idx;

  constexpr mt19937() noexcept : mt{}, idx(0) { seed_with(5489u); }

  constexpr explicit mt19937(u32 seed) noexcept : mt{}, idx(0) { seed_with(seed); }

  [[nodiscard]] static constexpr mt19937
  from_seed(u64 seed) noexcept
  {
    return mt19937(u32(seed));
  }

  constexpr void
  seed_with(u32 seed) noexcept
  {
    mt[0] = seed;
    for ( usize i = 1; i < N; ++i ) mt[i] = u32(1812433253u * (mt[i - 1] ^ (mt[i - 1] >> 30)) + u32(i));
    idx = N;
  }

  [[nodiscard, gnu::always_inline]] static constexpr u32
  __twist_word(u32 upper, u32 lower, u32 mix) noexcept
  {
    const u32 y = (upper & UPPER_MASK) | (lower & LOWER_MASK);
    return mix ^ (y >> 1) ^ ((u32(0) - (y & 1u)) & MATRIX_A);
  }

  constexpr void
  twist() noexcept
  {
    for ( usize i = 0; i < N - M; ++i ) mt[i] = __twist_word(mt[i], mt[i + 1], mt[i + M]);
    for ( usize i = N - M; i < N - 1; ++i ) mt[i] = __twist_word(mt[i], mt[i + 1], mt[i + (M - N)]);
    mt[N - 1] = __twist_word(mt[N - 1], mt[0], mt[M - 1]);
    idx = 0;
  }

  constexpr void
  generate() noexcept
  {
    twist();
  }

  [[nodiscard, gnu::always_inline]] static constexpr u32
  __temper(u32 y) noexcept
  {
    y ^= y >> 11;
    y ^= (y << 7) & 0x9D2C5680u;
    y ^= (y << 15) & 0xEFC60000u;
    return y ^ (y >> 18);
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next() noexcept
  {
    if ( idx >= N ) [[unlikely]]
      twist();
    return __temper(mt[idx++]);
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next32() noexcept
  {
    return next();
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    const u64 lo = next();
    const u64 hi = next();
    return lo | (hi << 32);
  }

  inline constexpr void
  generate(u32 *__restrict__ out, usize n) noexcept
  {
    while ( n != 0 ) {
      if ( idx >= N ) twist();
      const usize available = N - idx;
      const usize count = n < available ? n : available;
      const u32 *input = mt + idx;
      for ( usize i = 0; i < count; ++i ) out[i] = __temper(input[i]);
      idx += u32(count);
      out += count;
      n -= count;
    }
  }
};

static_assert(sizeof(mt19937) >= 2500 && sizeof(mt19937) <= 2512, "mt19937 state size out of expected range");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// multiply-with-carry
struct mwc64 {
  using result_type = u64;
  static constexpr u32 result_bits = 64;
  static constexpr u64 A_HI = 4294957665ULL;
  static constexpr u64 A_LO = 4294963023ULL;

  u64 hi;
  u64 lo;

  constexpr mwc64() noexcept : hi(0xCAFEBABEDEADBEEFULL), lo(0xFEEDFACEC0FFEE00ULL) { }

  constexpr mwc64(u64 a, u64 b) noexcept : hi(a), lo(b) { }

  [[nodiscard, gnu::always_inline]] static constexpr u64
  __normalize(u64 state, u64 multiplier) noexcept
  {
    u32 value = u32(state);
    const u32 carry = u32(state >> 32) % u32(multiplier);
    if ( value == 0 && carry == 0 ) value = 1;
    return (u64(carry) << 32) | value;
  }

  [[nodiscard]] static constexpr mwc64
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    return mwc64{ __normalize(sm.next(), A_HI), __normalize(sm.next(), A_LO) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr u64
  __step(u64 &high, u64 &low) noexcept
  {
    high = A_HI * u64(u32(high)) + (high >> 32);
    low = A_LO * u64(u32(low)) + (low >> 32);
    return (u64(u32(high)) << 32) | u64(u32(low));
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    return __step(hi, lo);
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    return next();
  }

  inline constexpr void
  generate(u64 *__restrict__ out, usize n) noexcept
  {
    u64 high = hi, low = lo;
    while ( n >= 4 ) {
      out[0] = __step(high, low);
      out[1] = __step(high, low);
      out[2] = __step(high, low);
      out[3] = __step(high, low);
      out += 4;
      n -= 4;
    }
    while ( n-- != 0 ) *out++ = __step(high, low);
    hi = high;
    lo = low;
  }
};

static_assert(sizeof(mwc64) == 16, "mwc64 state must be 16 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// full-period 64-bit LCG
struct lcg64 {
  using result_type = u64;
  static constexpr u32 result_bits = 64;
  static constexpr u64 A = 6364136223846793005ULL;
  static constexpr u64 C = 1442695040888963407ULL;
  static constexpr u64 A2 = A * A;
  static constexpr u64 C2 = A * C + C;
  static constexpr u64 A3 = A2 * A;
  static constexpr u64 C3 = A * C2 + C;
  static constexpr u64 A4 = A2 * A2;
  static constexpr u64 C4 = A * C3 + C;

  u64 s;

  constexpr lcg64() noexcept : s(0) { }

  constexpr explicit lcg64(u64 seed) noexcept : s(seed) { }

  [[nodiscard]] static constexpr lcg64
  from_seed(u64 seed) noexcept
  {
    return lcg64{ seed };
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    s = A * s + C;
    return s;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    return next();
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next32() noexcept
  {
    return u32(next() >> 32);
  }

  inline constexpr void
  generate(u64 *__restrict__ out, usize n) noexcept
  {
    u64 state = s;
    while ( n >= 4 ) {
      out[0] = A * state + C;
      out[1] = A2 * state + C2;
      out[2] = A3 * state + C3;
      out[3] = A4 * state + C4;
      state = out[3];
      out += 4;
      n -= 4;
    }
    while ( n-- != 0 ) {
      state = A * state + C;
      *out++ = state;
    }
    s = state;
  }
};

static_assert(sizeof(lcg64) == 8, "lcg64 state must be 8 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generic width-aware draws
template<typename T>
concept rng_concept = requires(T t) {
  { t.next() } -> micron::convertible_to<u64>;
};

namespace __impl
{

template<rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline constexpr u32
next32(Rng &g) noexcept
{
  if constexpr ( requires { g.next32(); } )
    return u32(g.next32());
  else
    return u32(g.next());
}

template<rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline constexpr u64
next64(Rng &g) noexcept
{
  if constexpr ( requires { g.next64(); } ) {
    return u64(g.next64());
  } else if constexpr ( sizeof(decltype(g.next())) <= sizeof(u32) ) {
    const u64 lo = u32(g.next());
    const u64 hi = u32(g.next());
    return lo | (hi << 32);
  } else {
    return u64(g.next());
  }
}

};      // namespace __impl

};      // namespace rng
};      // namespace math
};      // namespace micron

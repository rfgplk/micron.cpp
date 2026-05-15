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
//   hardware (v asm)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"

namespace micron
{
namespace math
{
namespace rng
{

// Vigna's seed mixer.
struct splitmix64 {
  u64 s;

  constexpr splitmix64() noexcept : s(0) { }

  constexpr explicit splitmix64(u64 seed) noexcept : s(seed) { }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    u64 z = (s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }
};

static_assert(sizeof(splitmix64) == 8, "splitmix64 state must be 8 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// xor shift rotates
struct xoshiro256ss {
  u64 s[4];

  constexpr xoshiro256ss() noexcept : s{ 0, 0, 0, 0 } { }

  constexpr xoshiro256ss(u64 a, u64 b, u64 c, u64 d) noexcept : s{ a, b, c, d } { }

  // seed via splitmix64
  [[nodiscard]] static constexpr xoshiro256ss
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    return xoshiro256ss{ sm.next(), sm.next(), sm.next(), sm.next() };
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    const u64 result = bits::rol64(s[1] * 5ULL, 7) * 9ULL;
    const u64 t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = bits::rol64(s[3], 45);
    return result;
  }

  // advance by 2^128 steps for parallel stream splitting
  constexpr void
  jump() noexcept
  {
    static constexpr u64 J[] = { 0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL, 0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL };
    u64 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 64; ++b ) {
        if ( J[i] & (1ULL << b) ) {
          s0 ^= s[0];
          s1 ^= s[1];
          s2 ^= s[2];
          s3 ^= s[3];
        }
        (void)next();
      }
    }
    s[0] = s0;
    s[1] = s1;
    s[2] = s2;
    s[3] = s3;
  }

  // advance by 2^192 steps
  constexpr void
  long_jump() noexcept
  {
    static constexpr u64 LJ[] = { 0x76e15d3efefdcbbfULL, 0xc5004e441c522fb3ULL, 0x77710069854ee241ULL, 0x39109bb02acbe635ULL };
    u64 s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for ( int i = 0; i < 4; ++i ) {
      for ( int b = 0; b < 64; ++b ) {
        if ( LJ[i] & (1ULL << b) ) {
          s0 ^= s[0];
          s1 ^= s[1];
          s2 ^= s[2];
          s3 ^= s[3];
        }
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
  u32 s[4];

  constexpr xoshiro128ss() noexcept : s{ 0, 0, 0, 0 } { }

  constexpr xoshiro128ss(u32 a, u32 b, u32 c, u32 d) noexcept : s{ a, b, c, d } { }

  [[nodiscard]] static constexpr xoshiro128ss
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    u64 a = sm.next();
    u64 b = sm.next();
    return xoshiro128ss{ u32(a), u32(a >> 32), u32(b), u32(b >> 32) };
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next() noexcept
  {
    const u32 result = bits::rol32(s[1] * 5u, 7) * 9u;
    const u32 t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = bits::rol32(s[3], 11);
    return result;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next64() noexcept
  {
    u64 lo = next();
    u64 hi = next();
    return lo | (hi << 32);
  }
};

static_assert(sizeof(xoshiro128ss) == 16, "xoshiro128ss state must be 16 bytes");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// permuted congruential generator
struct pcg64 {
  u128 state;
  u128 inc;      // must be odd

  constexpr pcg64() noexcept : state(0), inc(1) { }

  constexpr pcg64(u128 s, u128 i) noexcept : state(s), inc(i | u128(1)) { }

  [[nodiscard]] static constexpr pcg64
  make(u64 seed, u64 stream = 1) noexcept
  {
    pcg64 r{};
    r.inc = (u128(stream) << 1) | u128(1);
    r.state = u128(0);
    r.state = r.state + r.inc;
    // mix the seed in via one step
    r.state = r.state * u128(0x2360ED051FC65DA4ULL) * u128(0x4385DF649FCCF645ULL >> 0) + r.inc;
    splitmix64 sm{ seed };
#if defined(__SIZEOF_INT128__) && defined(__micron_arch_width_64)
    u64 hi = sm.next();
    u64 lo = sm.next();
    r.state = (static_cast<u128>(hi) << 64) | u128(lo);
    r.state = r.state | u128(1);
    // ensure step
    (void)r.next();
#else
    (void)sm;
#endif
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    u128 old = state;
#if defined(__SIZEOF_INT128__) && defined(__micron_arch_width_64)
    state = old * (u128(0x2360ED051FC65DA4ULL) << 64 | u128(0x4385DF649FCCF645ULL)) + inc;
    u64 xsl = u64(u64(old >> 64) ^ u64(old));
    u32 rot = u32(u64(old >> 122) & 0x3F);
#else
    state = old * u128(0x4385DF649FCCF645ULL) + inc;
    u64 xsl = u64(old) ^ u64(old >> 64);
    u32 rot = u32(u64(old >> 122) & 0x3F);
#endif
    return bits::ror64(xsl, int(rot));
  }
};

static_assert(sizeof(pcg64) == 32, "pcg64 state must be 32 bytes");

// standard Mersenne Twister, STL like
struct mt19937 {
  static constexpr usize N = 624;
  static constexpr usize M = 397;
  static constexpr u32 MATRIX_A = 0x9908b0dfu;
  static constexpr u32 UPPER_MASK = 0x80000000u;
  static constexpr u32 LOWER_MASK = 0x7fffffffu;

  u32 mt[N];
  u32 idx;

  constexpr mt19937() noexcept : mt{}, idx(N + 1) { }

  constexpr explicit mt19937(u32 seed) noexcept : mt{}, idx(0) { seed_with(seed); }

  [[nodiscard]] static constexpr mt19937
  from_seed(u64 seed) noexcept
  {
    mt19937 r;
    r.seed_with(u32(seed));
    return r;
  }

  constexpr void
  seed_with(u32 seed) noexcept
  {
    mt[0] = seed;
    for ( usize i = 1; i < N; ++i ) {
      mt[i] = u32(1812433253u * (mt[i - 1] ^ (mt[i - 1] >> 30)) + u32(i));
    }
    idx = N;
  }

  constexpr void
  generate() noexcept
  {
    for ( usize i = 0; i < N - M; ++i ) {
      const u32 y = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
      mt[i] = mt[i + M] ^ (y >> 1) ^ ((y & 1u) ? MATRIX_A : 0u);
    }
    for ( usize i = N - M; i < N - 1; ++i ) {
      const u32 y = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
      mt[i] = mt[i + (M - N)] ^ (y >> 1) ^ ((y & 1u) ? MATRIX_A : 0u);
    }
    const u32 y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ ((y & 1u) ? MATRIX_A : 0u);
    idx = 0;
  }

  [[nodiscard, gnu::always_inline]] constexpr u32
  next32() noexcept
  {
    if ( idx >= N ) generate();
    u32 y = mt[idx++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    y ^= y >> 18;
    return y;
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    const u64 lo = next32();
    const u64 hi = next32();
    return (hi << 32) | lo;
  }
};

static_assert(sizeof(mt19937) >= 2500 && sizeof(mt19937) <= 2512, "mt19937 state size out of expected range");

// Marsaglias branchless multiply-with-carry.
struct mwc64 {
  static constexpr u64 A_HI = 4294957665ULL;
  static constexpr u64 A_LO = 4294963023ULL;

  u64 hi;
  u64 lo;

  constexpr mwc64() noexcept : hi(0xCAFEBABEDEADBEEFULL), lo(0xFEEDFACEC0FFEE00ULL) { }

  constexpr mwc64(u64 a, u64 b) noexcept : hi(a), lo(b) { }

  [[nodiscard]] static constexpr mwc64
  from_seed(u64 seed) noexcept
  {
    splitmix64 sm{ seed };
    u64 a = sm.next();
    u64 b = sm.next();
    if ( u32(a) == 0 ) a |= 1ULL;
    if ( u32(b) == 0 ) b |= 1ULL;
    return mwc64{ a, b };
  }

  [[nodiscard, gnu::always_inline]] constexpr u64
  next() noexcept
  {
    hi = A_HI * (hi & 0xFFFFFFFFULL) + (hi >> 32);
    lo = A_LO * (lo & 0xFFFFFFFFULL) + (lo >> 32);
    return (u64(u32(hi)) << 32) | u64(u32(lo));
  }
};

static_assert(sizeof(mwc64) == 16, "mwc64 state must be 16 bytes");

// Knuths linear congruential mixer
struct lcg64 {
  static constexpr u64 A = 6364136223846793005ULL;
  static constexpr u64 C = 1442695040888963407ULL;

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
};

static_assert(sizeof(lcg64) == 8, "lcg64 state must be 8 bytes");

template<typename T>
concept rng_concept = requires(T t) {
  { t.next() } -> micron::convertible_to<u64>;
};

};      // namespace rng
};      // namespace math
};      // namespace micron

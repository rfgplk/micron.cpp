//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits.hpp"
#include "../except.hpp"
#include "../memory/memory.hpp"
#include "../tuple.hpp"
#include "../types.hpp"
#include "__load.hpp"

namespace micron
{

// port of the xx64 hash
namespace hashes
{

constexpr u32 xxprime32a = 0x9E3779B1U; /*!< 0b10011110001101110111100110110001 */
constexpr u32 xxprime32b = 0x85EBCA77U; /*!< 0b10000101111010111100101001110111 */
constexpr u32 xxprime32c = 0xC2B2AE3DU; /*!< 0b11000010101100101010111000111101 */
constexpr u32 xxprime32d = 0x27D4EB2FU; /*!< 0b00100111110101001110101100101111 */
constexpr u32 xxprime32e = 0x165667B1U; /*!< 0b00010110010101100110011110110001 */

constexpr u64 xxprime64a = 0x9E3779B185EBCA87ULL;
constexpr u64 xxprime64b = 0xC2B2AE3D27D4EB4FULL;
constexpr u64 xxprime64c = 0x165667B19E3779F9ULL;
constexpr u64 xxprime64d = 0x85EBCA77C2B2AE63ULL;
constexpr u64 xxprime64e = 0x27D4EB2F165667C5ULL;

// Bounded incremental XXH64. The byte tail also makes the state usable during
// constant evaluation; no pointer punning or caller-provided padding is needed.
struct xxhash64_stream {
  u64 total = 0, seed = 0, lanes[4]{};
  byte tail[32]{};
  usize buffered = 0;

  constexpr explicit xxhash64_stream(u64 initial = 0) noexcept { reset(initial); }

  static constexpr u64
  rotate(u64 x, u32 n) noexcept
  {
    return (x << n) | (x >> (64 - n));
  }

  static constexpr u64
  round(u64 a, u64 x) noexcept
  {
    return rotate(a + x * xxprime64b, 31) * xxprime64a;
  }

  static constexpr u64
  load(const byte *p, usize n) noexcept
  {
    u64 x = 0;
    for ( usize i = 0; i < n; ++i ) x |= u64(p[i]) << (8 * i);
    return x;
  }

  constexpr void
  reset(u64 initial = 0) noexcept
  {
    total = 0;
    seed = initial;
    buffered = 0;
    lanes[0] = seed + xxprime64a + xxprime64b;
    lanes[1] = seed + xxprime64b;
    lanes[2] = seed;
    lanes[3] = seed - xxprime64a;
  }

  constexpr void
  stripe(const byte *p) noexcept
  {
    for ( usize i = 0; i < 4; ++i ) lanes[i] = round(lanes[i], load(p + 8 * i, 8));
  }

  constexpr void
  update(const byte *p, usize n) noexcept
  {
    total += n;
    usize i = 0;
    if ( buffered ) {
      while ( buffered < 32 && i < n ) tail[buffered++] = p[i++];
      if ( buffered < 32 ) return;
      stripe(tail);
      buffered = 0;
    }
    for ( ; n - i >= 32; i += 32 ) stripe(p + i);
    while ( i < n ) tail[buffered++] = p[i++];
  }

  constexpr u64
  digest() const noexcept
  {
    u64 h = seed + xxprime64e;
    if ( total >= 32 ) {
      h = rotate(lanes[0], 1) + rotate(lanes[1], 7) + rotate(lanes[2], 12) + rotate(lanes[3], 18);
      for ( auto v : lanes ) h = (h ^ round(0, v)) * xxprime64a + xxprime64d;
    }
    h += total;
    usize i = 0;
    for ( ; buffered - i >= 8; i += 8 ) h = rotate(h ^ round(0, load(tail + i, 8)), 27) * xxprime64a + xxprime64d;
    if ( buffered - i >= 4 ) {
      h = rotate(h ^ (load(tail + i, 4) * xxprime64a), 23) * xxprime64b + xxprime64c;
      i += 4;
    }
    while ( i < buffered ) h = rotate(h ^ (u64(tail[i++]) * xxprime64e), 11) * xxprime64a;
    h ^= h >> 33;
    h *= xxprime64b;
    h ^= h >> 29;
    h *= xxprime64c;
    return h ^ (h >> 32);
  }
};

constexpr u64
xxhash64_bytes(const byte *p, usize n, u64 seed = 0) noexcept
{
  xxhash64_stream s(seed);
  s.update(p, n);
  return s.digest();
}

struct xxhash64_state {
  u64 len;
  u64 v[4];
  u64 mem64[4];
  u32 memsize;
  u32 pad32;
  u64 pad64;
};

typedef xxhash64_state xstate;

template<u64 seed>
inline xstate
init_seed()
{
  struct xxhash64_state s;
  s.v[0] = seed + xxprime64a + xxprime64b;
  s.v[1] = seed + xxprime64b;
  s.v[2] = seed;
  s.v[3] = seed - xxprime64a;
  return s;
}

/*
inline __attribute__((always_inline)) u64
rotl64(const u64 x, const i8 r)
{
  return (x << r) | (x >> (64 - r));
}
*/

constexpr u32
__xxrotl32(u32 x, i32 r) noexcept
{
  return (x << r) | (x >> (32 - r));
}

constexpr u32
xxround_32(u32 acc, u32 input) noexcept
{
  acc += input * xxprime32b;
  acc = __xxrotl32(acc, 13);
  acc *= xxprime32a;
  return acc;
}

inline __attribute__((always_inline)) u64
xxround(u64 acc, u64 input)
{
  acc += input * xxprime64b;
  acc = rotl64(acc, 31);
  acc *= xxprime64a;
  return acc;
}

inline __attribute__((always_inline)) u64
xxmergeround(u64 (&v)[4])
{
  u64 xxh = rotl64(v[0], 1) + rotl64(v[1], 7) + rotl64(v[2], 12) + rotl64(v[3], 18);
  v[0] = xxround(0, v[0]);
  xxh ^= v[0];
  xxh = xxh * xxprime64a + xxprime64d;

  v[1] = xxround(0, v[1]);
  xxh ^= v[1];
  xxh = xxh * xxprime64a + xxprime64d;

  v[2] = xxround(0, v[2]);
  xxh ^= v[2];
  xxh = xxh * xxprime64a + xxprime64d;

  v[3] = xxround(0, v[3]);
  xxh ^= v[3];
  xxh = xxh * xxprime64a + xxprime64d;
  return xxh;
}

inline u64
xxhash_final(u64 xxhash, const byte *ptr, usize len)
{
  len &= 31;
  while ( len >= 8 ) {
    const u64 k1 = xxround(0, __load64(ptr));
    ptr += 8;
    xxhash ^= k1;
    xxhash = rotl64(xxhash, 27) * xxprime64a + xxprime64d;
    len -= 8;
  }
  if ( len >= 4 ) {
    xxhash ^= u64(__load32(ptr)) * xxprime64a;
    ptr += 4;
    xxhash = rotl64(xxhash, 23) * xxprime64b + xxprime64c;
    len -= 4;
  }
  while ( len > 0 ) {
    xxhash ^= (*ptr++) * xxprime64e;
    xxhash = rotl64(xxhash, 11) * xxprime64a;
    --len;
  }

  xxhash ^= xxhash >> 33;
  xxhash *= xxprime64b;
  xxhash ^= xxhash >> 29;
  xxhash *= xxprime64c;
  xxhash ^= xxhash >> 32;

  return xxhash;
}

// for lz4 czlib code (spliced in from there)
constexpr u32
xxhash32(const byte *src, usize len, u32 seed = 0) noexcept
{
  usize i = 0;
  u32 h = 0;
  if ( len >= 16 ) {
    u32 v1 = seed + xxprime32a + xxprime32b;
    u32 v2 = seed + xxprime32b;
    u32 v3 = seed;
    u32 v4 = seed - xxprime32a;
    for ( ; i + 16 <= len; i += 16 ) {
      v1 = xxround_32(v1, __load32(src + i));
      v2 = xxround_32(v2, __load32(src + i + 4));
      v3 = xxround_32(v3, __load32(src + i + 8));
      v4 = xxround_32(v4, __load32(src + i + 12));
    }
    h = __xxrotl32(v1, 1) + __xxrotl32(v2, 7) + __xxrotl32(v3, 12) + __xxrotl32(v4, 18);
  } else {
    h = seed + xxprime32e;
  }
  h += static_cast<u32>(len);
  for ( ; i + 4 <= len; i += 4 ) h = __xxrotl32(h + __load32(src + i) * xxprime32c, 17) * xxprime32d;
  for ( ; i < len; ++i ) h = __xxrotl32(h + u32(src[i]) * xxprime32e, 11) * xxprime32a;
  h ^= h >> 15;
  h *= xxprime32b;
  h ^= h >> 13;
  h *= xxprime32c;
  h ^= h >> 16;
  return h;
}

template<u64 seed>
inline u64
xxhash64(const byte *src, usize len)
{
  // xstate state = init_seed<seed>();
  u64 xxhash = 0;
  if ( len >= 32 ) [[likely]] {
    const byte *const end = src + len;
    const byte *const limit = end - 31;
    u64 v[4];
    v[0] = seed + xxprime64a + xxprime64b;
    v[1] = seed + xxprime64b;
    v[2] = seed;
    v[3] = seed - xxprime64a;

    do {
      v[0] = xxround(v[0], __load64(src));
      v[1] = xxround(v[1], __load64(src + 8));
      v[2] = xxround(v[2], __load64(src + 16));
      v[3] = xxround(v[3], __load64(src + 24));
      src += 32;
    } while ( src < limit );

    xxhash = xxmergeround(v);
  } else {
    xxhash = seed + xxprime64e;
  }
  xxhash += len;
  return xxhash_final(xxhash, src, len);
}

inline u64
xxhash64_rtseed(const byte *src, u64 seed, usize len)
{
  // xstate state = init_seed<seed>();
  u64 xxhash = 0;
  if ( len >= 32 ) [[likely]] {
    const byte *const end = src + len;
    const byte *const limit = end - 31;
    u64 v[4];
    v[0] = seed + xxprime64a + xxprime64b;
    v[1] = seed + xxprime64b;
    v[2] = seed;
    v[3] = seed - xxprime64a;

    do {
      v[0] = xxround(v[0], __load64(src));
      v[1] = xxround(v[1], __load64(src + 8));
      v[2] = xxround(v[2], __load64(src + 16));
      v[3] = xxround(v[3], __load64(src + 24));
      src += 32;
    } while ( src < limit );

    xxhash = xxmergeround(v);
  } else {
    xxhash = seed + xxprime64e;
  }
  xxhash += len;
  return xxhash_final(xxhash, src, len);
}
};      // namespace hashes

};      // namespace micron

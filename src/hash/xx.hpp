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

struct xxhash64_state {
  u64 len;
  u64 v[4];
  u64 mem64[4];
  u32 memsize;
  u32 pad32;
  u64 pad64;
};

typedef xxhash64_state xstate;

template <u64 seed>
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

inline __attribute__((always_inline)) u32
xxround_32(u32 acc, u32 input)
{
  acc += input * xxprime32b;
  acc = rotl32(acc, 13);
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
  u64 xxh = rotl64(v[0], 1) + rotl64(v[1], 7) + rotl64(v[3], 12) + rotl64(v[3], 18);
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
    const u64 k1 = xxround(0, *reinterpret_cast<const u64 *>(ptr));
    ptr += 8;
    xxhash ^= k1;
    xxhash = rotl64(xxhash, 27) * xxprime64a + xxprime64d;
    len -= 8;
  }
  if ( len >= 4 ) {
    xxhash ^= (*reinterpret_cast<const u32 *>(ptr)) * xxprime64a;     // ??
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

/*
template <u32 seed>
inline u32
xxhash32(const byte *src, usize len)
{
  if ((((usize)src) & 7) != 0 ) [[unlikely]]
    exc<except::library_error>("micron::hash::xxhash src isn't aligned");
  //xstate state = init_seed<seed>();
  u64 xxhash = 0;
  if ( len >= 32 ) [[likely]] {
    const byte *const end = src + len;
    const byte *const limit = end - 15;
    u32 v[4];
    v[0] = seed + xxprime32a + xxprime32b;
    v[1] = seed + xxprime32b;
    v[2] = seed;
    v[3] = seed - xxprime32a;

    do {
      v[0] = xxround32(v[0], *reinterpret_cast<const u64 *>(src));
      v[1] = xxround32(v[1], *reinterpret_cast<const u64 *>(src + 4));
      v[2] = xxround32(v[2], *reinterpret_cast<const u64 *>(src + 8));
      v[3] = xxround32(v[3], *reinterpret_cast<const u64 *>(src + 12));
      src += 16;
    } while ( src < limit );

    xxhash = rotl32(v[0], 1) + rotl32(v[1], 7) + rotl32(v[2], 12) + rotl32(v[3], 18);
  } else {
    xxhash = seed + xxprime64e;
  }
  xxhash += static_cast<u32>(len);
  return xxhash_final_32(xxhash, src, len);
}
*/
template <u64 seed>
inline u64
xxhash64(const byte *src, usize len)
{
  if ( (((usize)src) & 7) != 0 ) [[unlikely]]
    exc<except::library_error>("micron::hash::xxhash src isn't aligned");
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
      v[0] = xxround(v[0], *reinterpret_cast<const u64 *>(src));
      v[1] = xxround(v[1], *reinterpret_cast<const u64 *>(src + 8));
      v[2] = xxround(v[2], *reinterpret_cast<const u64 *>(src + 16));
      v[3] = xxround(v[3], *reinterpret_cast<const u64 *>(src + 24));
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
  if ( (((usize)src) & 7) != 0 ) [[unlikely]]
    exc<except::library_error>("micron::hash::xxhash src isn't aligned");
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
      v[0] = xxround(v[0], *reinterpret_cast<const u64 *>(src));
      v[1] = xxround(v[1], *reinterpret_cast<const u64 *>(src + 8));
      v[2] = xxround(v[2], *reinterpret_cast<const u64 *>(src + 16));
      v[3] = xxround(v[3], *reinterpret_cast<const u64 *>(src + 24));
      src += 32;
    } while ( src < limit );

    xxhash = xxmergeround(v);
  } else {
    xxhash = seed + xxprime64e;
  }
  xxhash += len;
  return xxhash_final(xxhash, src, len);
}
};     // namespace hashes

};     // namespace micron

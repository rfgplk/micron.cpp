//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/cmemory.hpp"
#include "../types.hpp"

// ============================================================================
//  This file embeds a port of rapidhash V3 into micron.
//
//  rapidhash V3 - Very fast, high quality, platform-independent hashing algorithm.
//
//  Based on 'wyhash', by Wang Yi <godspeed_china@yeah.net>
//
//  Copyright (C) 2025 Nicolas De Carli
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//
//  You can contact the author at:
//    - rapidhash source repository: https://github.com/Nicoshev/rapidhash
//
//  NOTE: adapted to the micron native types
// ============================================================================

namespace micron
{
namespace hashes
{

#define __rapid_likely(x) __builtin_expect(!!(x), 1)
#define __rapid_unlikely(x) __builtin_expect(!!(x), 0)

constexpr static const u64 rapid_secret[8] = { 0x2d358dccaa6c78a5ull, 0x8bb84b93962eacc9ull, 0x4b33a62ed433d4a3ull, 0x4d5a2da51de1aa47ull,
                                               0xa0761d6478bd642full, 0xe7037ed1a0b428dbull, 0x90ed1765281c388cull, 0xaaaaaaaaaaaaaaaaull };

inline __attribute__((always_inline)) void
rapid_mum(u64 *A, u64 *B) noexcept
{
  u128 r = *A;
  r *= *B;
  *A = (u64)r;
  *B = (u64)(r >> 64);
}

inline __attribute__((always_inline)) u64
rapid_mix(u64 A, u64 B) noexcept
{
  rapid_mum(&A, &B);
  return A ^ B;
}

inline __attribute__((always_inline)) u64
rapid_read64(const u8 *p) noexcept
{
  u64 v;
  micron::cbytecpy<sizeof(u64)>(&v, p);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  v = __builtin_bswap64(v);
#endif
  return v;
}

inline __attribute__((always_inline)) u64
rapid_read32(const u8 *p) noexcept
{
  u32 v;
  micron::cbytecpy<sizeof(u32)>(&v, p);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  v = __builtin_bswap32(v);
#endif
  return (u64)v;
}

inline u64
rapidhash_internal(const void *key, usize len, u64 seed, const u64 *secret) noexcept
{
  const u8 *p = (const u8 *)key;
  seed ^= rapid_mix(seed ^ secret[2], secret[1]);
  u64 a = 0, b = 0;
  usize i = len;
  if ( __rapid_likely(len <= 16) ) {
    if ( len >= 4 ) {
      seed ^= len;
      if ( len >= 8 ) {
        const u8 *plast = p + len - 8;
        a = rapid_read64(p);
        b = rapid_read64(plast);
      } else {
        const u8 *plast = p + len - 4;
        a = rapid_read32(p);
        b = rapid_read32(plast);
      }
    } else if ( len > 0 ) {
      a = (((u64)p[0]) << 45) | p[len - 1];
      b = p[len >> 1];
    } else
      a = b = 0;
  } else {
    if ( len > 112 ) {
      u64 see1 = seed, see2 = seed;
      u64 see3 = seed, see4 = seed;
      u64 see5 = seed, see6 = seed;
      do {
        seed = rapid_mix(rapid_read64(p) ^ secret[0], rapid_read64(p + 8) ^ seed);
        see1 = rapid_mix(rapid_read64(p + 16) ^ secret[1], rapid_read64(p + 24) ^ see1);
        see2 = rapid_mix(rapid_read64(p + 32) ^ secret[2], rapid_read64(p + 40) ^ see2);
        see3 = rapid_mix(rapid_read64(p + 48) ^ secret[3], rapid_read64(p + 56) ^ see3);
        see4 = rapid_mix(rapid_read64(p + 64) ^ secret[4], rapid_read64(p + 72) ^ see4);
        see5 = rapid_mix(rapid_read64(p + 80) ^ secret[5], rapid_read64(p + 88) ^ see5);
        see6 = rapid_mix(rapid_read64(p + 96) ^ secret[6], rapid_read64(p + 104) ^ see6);
        p += 112;
        i -= 112;
      } while ( i > 112 );
      seed ^= see1;
      see2 ^= see3;
      see4 ^= see5;
      seed ^= see6;
      see2 ^= see4;
      seed ^= see2;
    }
    if ( i > 16 ) {
      seed = rapid_mix(rapid_read64(p) ^ secret[2], rapid_read64(p + 8) ^ seed);
      if ( i > 32 ) {
        seed = rapid_mix(rapid_read64(p + 16) ^ secret[2], rapid_read64(p + 24) ^ seed);
        if ( i > 48 ) {
          seed = rapid_mix(rapid_read64(p + 32) ^ secret[1], rapid_read64(p + 40) ^ seed);
          if ( i > 64 ) {
            seed = rapid_mix(rapid_read64(p + 48) ^ secret[1], rapid_read64(p + 56) ^ seed);
            if ( i > 80 ) {
              seed = rapid_mix(rapid_read64(p + 64) ^ secret[2], rapid_read64(p + 72) ^ seed);
              if ( i > 96 ) {
                seed = rapid_mix(rapid_read64(p + 80) ^ secret[1], rapid_read64(p + 88) ^ seed);
              }
            }
          }
        }
      }
    }
    a = rapid_read64(p + i - 16) ^ i;
    b = rapid_read64(p + i - 8);
  }
  a ^= secret[1];
  b ^= seed;
  rapid_mum(&a, &b);
  return rapid_mix(a ^ secret[7], b ^ secret[1] ^ i);
}

inline u64
rapidhash_micro_internal(const void *key, usize len, u64 seed, const u64 *secret) noexcept
{
  const u8 *p = (const u8 *)key;
  seed ^= rapid_mix(seed ^ secret[2], secret[1]);
  u64 a = 0, b = 0;
  usize i = len;
  if ( __rapid_likely(len <= 16) ) {
    if ( len >= 4 ) {
      seed ^= len;
      if ( len >= 8 ) {
        const u8 *plast = p + len - 8;
        a = rapid_read64(p);
        b = rapid_read64(plast);
      } else {
        const u8 *plast = p + len - 4;
        a = rapid_read32(p);
        b = rapid_read32(plast);
      }
    } else if ( len > 0 ) {
      a = (((u64)p[0]) << 45) | p[len - 1];
      b = p[len >> 1];
    } else
      a = b = 0;
  } else {
    if ( i > 80 ) {
      u64 see1 = seed, see2 = seed;
      u64 see3 = seed, see4 = seed;
      do {
        seed = rapid_mix(rapid_read64(p) ^ secret[0], rapid_read64(p + 8) ^ seed);
        see1 = rapid_mix(rapid_read64(p + 16) ^ secret[1], rapid_read64(p + 24) ^ see1);
        see2 = rapid_mix(rapid_read64(p + 32) ^ secret[2], rapid_read64(p + 40) ^ see2);
        see3 = rapid_mix(rapid_read64(p + 48) ^ secret[3], rapid_read64(p + 56) ^ see3);
        see4 = rapid_mix(rapid_read64(p + 64) ^ secret[4], rapid_read64(p + 72) ^ see4);
        p += 80;
        i -= 80;
      } while ( i > 80 );
      seed ^= see1;
      see2 ^= see3;
      seed ^= see4;
      seed ^= see2;
    }
    if ( i > 16 ) {
      seed = rapid_mix(rapid_read64(p) ^ secret[2], rapid_read64(p + 8) ^ seed);
      if ( i > 32 ) {
        seed = rapid_mix(rapid_read64(p + 16) ^ secret[2], rapid_read64(p + 24) ^ seed);
        if ( i > 48 ) {
          seed = rapid_mix(rapid_read64(p + 32) ^ secret[1], rapid_read64(p + 40) ^ seed);
          if ( i > 64 ) {
            seed = rapid_mix(rapid_read64(p + 48) ^ secret[1], rapid_read64(p + 56) ^ seed);
          }
        }
      }
    }
    a = rapid_read64(p + i - 16) ^ i;
    b = rapid_read64(p + i - 8);
  }
  a ^= secret[1];
  b ^= seed;
  rapid_mum(&a, &b);
  return rapid_mix(a ^ secret[7], b ^ secret[1] ^ i);
}

inline u64
rapidhash_nano_internal(const void *key, usize len, u64 seed, const u64 *secret) noexcept
{
  const u8 *p = (const u8 *)key;
  seed ^= rapid_mix(seed ^ secret[2], secret[1]);
  u64 a = 0, b = 0;
  usize i = len;
  if ( __rapid_likely(len <= 16) ) {
    if ( len >= 4 ) {
      seed ^= len;
      if ( len >= 8 ) {
        const u8 *plast = p + len - 8;
        a = rapid_read64(p);
        b = rapid_read64(plast);
      } else {
        const u8 *plast = p + len - 4;
        a = rapid_read32(p);
        b = rapid_read32(plast);
      }
    } else if ( len > 0 ) {
      a = (((u64)p[0]) << 45) | p[len - 1];
      b = p[len >> 1];
    } else
      a = b = 0;
  } else {
    if ( i > 48 ) {
      u64 see1 = seed, see2 = seed;
      do {
        seed = rapid_mix(rapid_read64(p) ^ secret[0], rapid_read64(p + 8) ^ seed);
        see1 = rapid_mix(rapid_read64(p + 16) ^ secret[1], rapid_read64(p + 24) ^ see1);
        see2 = rapid_mix(rapid_read64(p + 32) ^ secret[2], rapid_read64(p + 40) ^ see2);
        p += 48;
        i -= 48;
      } while ( i > 48 );
      seed ^= see1;
      seed ^= see2;
    }
    if ( i > 16 ) {
      seed = rapid_mix(rapid_read64(p) ^ secret[2], rapid_read64(p + 8) ^ seed);
      if ( i > 32 ) {
        seed = rapid_mix(rapid_read64(p + 16) ^ secret[2], rapid_read64(p + 24) ^ seed);
      }
    }
    a = rapid_read64(p + i - 16) ^ i;
    b = rapid_read64(p + i - 8);
  }
  a ^= secret[1];
  b ^= seed;
  rapid_mum(&a, &b);
  return rapid_mix(a ^ secret[7], b ^ secret[1] ^ i);
}

#undef __rapid_likely
#undef __rapid_unlikely

template<u64 Seed = 0>
inline u64
rapidhash(const byte *key, usize len) noexcept
{
  return rapidhash_internal(key, len, Seed, rapid_secret);
}

inline u64
rapidhash(const byte *key, u64 seed, usize len) noexcept
{
  return rapidhash_internal(key, len, seed, rapid_secret);
}

template<u64 Seed = 0>
inline u64
rapidhash_micro(const byte *key, usize len) noexcept
{
  return rapidhash_micro_internal(key, len, Seed, rapid_secret);
}

inline u64
rapidhash_micro(const byte *key, u64 seed, usize len) noexcept
{
  return rapidhash_micro_internal(key, len, seed, rapid_secret);
}

template<u64 Seed = 0>
inline u64
rapidhash_nano(const byte *key, usize len) noexcept
{
  return rapidhash_nano_internal(key, len, Seed, rapid_secret);
}

inline u64
rapidhash_nano(const byte *key, u64 seed, usize len) noexcept
{
  return rapidhash_nano_internal(key, len, seed, rapid_secret);
}

};      // namespace hashes
};      // namespace micron

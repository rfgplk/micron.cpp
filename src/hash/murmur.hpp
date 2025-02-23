#pragma once

#include "../tuple.hpp"
#include "../types.hpp"

namespace micron
{
namespace hashes
{

inline __attribute__((always_inline)) u64
rotl64(const u64 x, const i8 r)
{
  return (x << r) | (x >> (64 - r));
}

// NOTE: this is an idiomatic port of murmur3 (x64), due to this library being designed for exclusive x64 use, the 32-bit
// variants of this hash are being omitted. credits to smhasher
template <typename T = byte>
micron::pair<u64, u64>
murmur(const T *key, const size_t len, const u32 seed)
{
  const size_t nblocks = len / 16;
  u64 s1 = seed;
  u64 s2 = seed;

  const u64 c1 = 0x87c37b91114253d5;
  const u64 c2 = 0x4cf5ad432745937f;

  const u64 *blcks = reinterpret_cast<const u64 *>(key);

  for ( size_t i = 0; i < nblocks; i++ ) {
    u64 k1 = blcks[i * 2];
    u64 k2 = blcks[i * 2 + 1];

    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    s1 ^= k1;

    s1 = rotl64(s1, 27);
    s1 += s2;
    s1 = s1 * 5 + 0x52dce729;

    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    s2 ^= k2;

    k2 = rotl64(k2, 31);
    k2 += k1;
    k2 = k2 * 5 + 0x38495ab5;
  }
  const uint8_t *tail = (const u8 *)(key + nblocks * 16);

  u64 k1 = 0;
  u64 k2 = 0;

  switch ( len & 15 ) {
  case 15:
    k2 ^= ((u64)tail[14]) << 48;
  case 14:
    k2 ^= ((u64)tail[13]) << 40;
  case 13:
    k2 ^= ((u64)tail[12]) << 32;
  case 12:
    k2 ^= ((u64)tail[11]) << 24;
  case 11:
    k2 ^= ((u64)tail[10]) << 16;
  case 10:
    k2 ^= ((u64)tail[9]) << 8;
  case 9:
    k2 ^= ((u64)tail[8]) << 0;
    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    s2 ^= k2;

  case 8:
    k1 ^= ((u64)tail[7]) << 56;
  case 7:
    k1 ^= ((u64)tail[6]) << 48;
  case 6:
    k1 ^= ((u64)tail[5]) << 40;
  case 5:
    k1 ^= ((u64)tail[4]) << 32;
  case 4:
    k1 ^= ((u64)tail[3]) << 24;
  case 3:
    k1 ^= ((u64)tail[2]) << 16;
  case 2:
    k1 ^= ((u64)tail[1]) << 8;
  case 1:
    k1 ^= ((u64)tail[0]) << 0;
    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    s1 ^= k1;
  };
  s1 ^= len;
  s2 ^= len;

  s1 += s2;
  s2 += s1;

  s1 ^= s1 >> 33;
  s1 *= 0xff51afd7ed558ccd;
  s1 ^= s1 >> 33;
  s1 *= 0xc4ceb9fe1a85ec53;
  s1 ^= s1 >> 33;

  s2 ^= s2 >> 33;
  s2 *= 0xff51afd7ed558ccd;
  s2 ^= s2 >> 33;
  s2 *= 0xc4ceb9fe1a85ec53;
  s2 ^= s2 >> 33;

  s1 += s2;
  s2 += s1;
  return { s1, s2 };
}

template <u32 seed, typename T = byte>
micron::pair<u64, u64>
murmur(const T *key, const size_t len)
{
  const size_t nblocks = len / 16;
  u64 s1 = seed;
  u64 s2 = seed;

  const u64 c1 = 0x87c37b91114253d5;
  const u64 c2 = 0x4cf5ad432745937f;

  const u64 *blcks = reinterpret_cast<const u64 *>(key);

  for ( size_t i = 0; i < nblocks; i++ ) {
    u64 k1 = blcks[i * 2];
    u64 k2 = blcks[i * 2 + 1];

    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    s1 ^= k1;

    s1 = rotl64(s1, 27);
    s1 += s2;
    s1 = s1 * 5 + 0x52dce729;

    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    s2 ^= k2;

    k2 = rotl64(k2, 31);
    k2 += k1;
    k2 = k2 * 5 + 0x38495ab5;
  }
  const uint8_t *tail = (const u8 *)(key + nblocks * 16);

  u64 k1 = 0;
  u64 k2 = 0;

  switch ( len & 15 ) {
  case 15:
    k2 ^= ((u64)tail[14]) << 48;
  case 14:
    k2 ^= ((u64)tail[13]) << 40;
  case 13:
    k2 ^= ((u64)tail[12]) << 32;
  case 12:
    k2 ^= ((u64)tail[11]) << 24;
  case 11:
    k2 ^= ((u64)tail[10]) << 16;
  case 10:
    k2 ^= ((u64)tail[9]) << 8;
  case 9:
    k2 ^= ((u64)tail[8]) << 0;
    k2 *= c2;
    k2 = rotl64(k2, 33);
    k2 *= c1;
    s2 ^= k2;

  case 8:
    k1 ^= ((u64)tail[7]) << 56;
  case 7:
    k1 ^= ((u64)tail[6]) << 48;
  case 6:
    k1 ^= ((u64)tail[5]) << 40;
  case 5:
    k1 ^= ((u64)tail[4]) << 32;
  case 4:
    k1 ^= ((u64)tail[3]) << 24;
  case 3:
    k1 ^= ((u64)tail[2]) << 16;
  case 2:
    k1 ^= ((u64)tail[1]) << 8;
  case 1:
    k1 ^= ((u64)tail[0]) << 0;
    k1 *= c1;
    k1 = rotl64(k1, 31);
    k1 *= c2;
    s1 ^= k1;
  };
  s1 ^= len;
  s2 ^= len;

  s1 += s2;
  s2 += s1;

  s1 ^= s1 >> 33;
  s1 *= 0xff51afd7ed558ccd;
  s1 ^= s1 >> 33;
  s1 *= 0xc4ceb9fe1a85ec53;
  s1 ^= s1 >> 33;

  s2 ^= s2 >> 33;
  s2 *= 0xff51afd7ed558ccd;
  s2 ^= s2 >> 33;
  s2 *= 0xc4ceb9fe1a85ec53;
  s2 ^= s2 >> 33;

  s1 += s2;
  s2 += s1;
  return { s1, s2 };
}

};

};

//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/hash/hash.hpp"

#include "../snowball/snowball.hpp"

using ::sb::print;
using ::sb::require_true;

static u64
next_rand(u64 &s)
{
  u64 z = (s += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

static u32
popcount64(u64 v)
{
  return (u32)__builtin_popcountll(v);
}

typedef u64 (*hfn)(const byte *, i64, usize);

static u64
h_zzzf(const byte *p, i64 s, usize n)
{
  return micron::hashes::zzzf64(p, s, n);
}

static u64
h_murmur(const byte *p, i64 s, usize n)
{
  auto h = micron::hashes::murmur(p, n, (u32)s);
  return h.a ^ h.b;
}

static u64
h_xxh(const byte *p, i64 s, usize n)
{
  return micron::hashes::xxhash64_rtseed(p, (u64)s, n);
}

static u64
h_rapid(const byte *p, i64 s, usize n)
{
  return micron::hashes::rapidhash(p, (u64)s, n);
}
#if defined(__micron_arch_x86_any)
static u64
h_meow(const byte *p, i64 s, usize n)
{
  return micron::hashes::meowhash64(p, (u64)s, n);
}
#endif

struct hentry {
  const char *name;
  hfn fn;
};

int
main()
{
  print("=== MICRON HASH AVALANCHE ===");
  static const hentry hs[] = {
    { "zzzf64", h_zzzf },     { "murmur", h_murmur }, { "xxhash64", h_xxh }, { "rapidhash", h_rapid },
#if defined(__micron_arch_x86_any)
    { "meowhash64", h_meow },
#endif
  };

  constexpr usize sizes[] = { 8, 32, 33, 96 };
  constexpr u64 trials = 100;

  for ( const auto &H : hs ) {
    sb::test_case(H.name);
    u64 rng = 0x1234567ull;
    for ( usize nbytes : sizes ) {
      const u64 nbits = nbytes * 8;
      for ( u64 bit = 0; bit < nbits; ++bit ) {
        u64 changed = 0;
        u64 flips = 0;
        for ( u64 t = 0; t < trials; ++t ) {
          alignas(64) byte a[96], b[96];
          for ( usize i = 0; i < 96; i += 8 ) {
            u64 r = next_rand(rng);
            for ( usize j = 0; j < 8; ++j ) a[i + j] = (byte)(r >> (j * 8));
          }
          for ( usize i = 0; i < 96; ++i ) b[i] = a[i];
          b[bit >> 3] ^= (byte)(1u << (bit & 7));
          u64 ha = H.fn(a, 99, nbytes);
          u64 hb = H.fn(b, 99, nbytes);
          if ( ha != hb ) ++changed;
          flips += popcount64(ha ^ hb);
        }
        require_true(changed == trials);
        require_true(flips >= 28 * trials && flips <= 36 * trials);
      }
    }
    sb::end_test_case();
    print("  avalanche ok: ", H.name);
  }

  print("[HASH AVALANCHE OK]");
  return 1;
}

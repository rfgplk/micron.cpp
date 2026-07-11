//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../snowball/snowball.hpp"

#include "../../src/slice.hpp"

#include "../../src/hash/hash.hpp"
#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

#include "../../src/sort/quick.hpp"

using ::sb::print;
using ::sb::require_true;

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

static u64
count_pairs(mc::slice<u64> &v)
{
  mc::sort::quick<mc::slice<u64>>(v.begin(), v.end());
  u64 pairs = 0, run = 1;
  for ( usize i = 1; i < v.size(); ++i ) {
    if ( v[i] == v[i - 1] )
      ++run;
    else {
      pairs += run * (run - 1) / 2;
      run = 1;
    }
  }
  pairs += run * (run - 1) / 2;
  return pairs;
}

struct hentry {
  const char *name;
  hfn fn;
};

int
main()
{
  print("=== MICRON HASH COLLISIONS ===");
  static const hentry hs[] = {
    { "zzzf64", h_zzzf },     { "murmur", h_murmur }, { "xxhash64", h_xxh }, { "rapidhash", h_rapid },
#if defined(__micron_arch_x86_any)
    { "meowhash64", h_meow },
#endif
  };

  constexpr u64 N = 100'000;

  constexpr u64 FULL64_MAX = 2;
  constexpr u64 LOW32_MAX = 12;

  for ( const auto &H : hs ) {
    sb::test_case(H.name);
    mc::slice<u64> h64(N);
    mc::slice<u64> h32(N);

    for ( u64 i = 0; i < N; ++i ) {
      alignas(16) byte key[16] = {};
      for ( int j = 0; j < 8; ++j ) key[j] = (byte)((i >> (j * 8)) & 0xFF);
      u64 h = H.fn(key, 0x5eed, 16);
      h64[i] = h;
      h32[i] = h & 0xFFFFFFFFull;
    }
    h64.mark(N);
    h32.mark(N);
    u64 c64 = count_pairs(h64);
    u64 c32 = count_pairs(h32);
    print("  ", H.name, " sequential: full64=", c64, " low32=", c32);
    require_true(c64 <= FULL64_MAX);
    require_true(c32 <= LOW32_MAX);

    for ( u64 i = 0; i < N; ++i ) {
      alignas(32) byte key[32];
      for ( int j = 0; j < 32; ++j ) key[j] = (byte)(((i * 2654435761u) >> ((j & 7) * 8)) & 0xFF);
      u64 h = H.fn(key, 0x5eed, 32);
      h64[i] = h;
    }
    h64.mark(N);
    u64 r64 = count_pairs(h64);
    print("  ", H.name, " replicated: full64=", r64);
    require_true(r64 <= FULL64_MAX);

    sb::end_test_case();
  }

  sb::test_case("zzzf length + dead-byte regressions");
  {
    alignas(32) byte buf[64] = {};
    buf[0] = 'X';
    require_true(micron::hashes::zzzf64(buf, 42, 1) != micron::hashes::zzzf64(buf, 42, 2));
    require_true(micron::hashes::zzzf64(buf, 42, 1) != micron::hashes::zzzf64(buf, 42, 32));
    require_true(micron::hashes::zzzf64(buf, 1, 0) != micron::hashes::zzzf64(buf, 2, 0));

    alignas(32) byte a[32] = {}, b[32] = {};
    for ( u64 i = 0; i < 32; ++i ) a[i] = b[i] = (byte)(i * 37 + 11);
    b[7] ^= 0x80;
    require_true(micron::hashes::zzzf64(a, 7, 32) != micron::hashes::zzzf64(b, 7, 32));
    b[7] ^= 0x80;
    b[31] ^= 0x80;
    require_true(micron::hashes::zzzf64(a, 7, 32) != micron::hashes::zzzf64(b, 7, 32));
  }
  sb::end_test_case();

  print("[HASH COLLISIONS OK]");
  return 1;
}

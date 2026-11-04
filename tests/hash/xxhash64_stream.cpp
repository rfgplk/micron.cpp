// Copyright (c) 2026 David Lucius Severus
// Distributed under the Boost Software License, Version 1.0 (see LICENSE).
#include "../../src/hash/xx.hpp"
#include "../snowball/snowball.hpp"
constexpr byte sample[] = { 'a', 'b', 'c' };
static_assert(micron::hashes::xxhash64_bytes(nullptr, 0) == 0xef46db3751d8e999ULL);
static_assert(micron::hashes::xxhash64_bytes(sample, 3) == 0x44bc2cf5ad770999ULL);
static_assert([] {
  micron::hashes::xxhash64_stream h;
  h.update(sample, 1);
  h.update(sample + 1, 2);
  return h.digest() == 0x44bc2cf5ad770999ULL;
}());

int
main()
{
  sb::test_case("incremental XXH64: all tails, alignments, chunk splits and seeds");
  byte input[4128]{};
  u32 r = 1;
  for ( usize i = 0; i < sizeof(input); ++i ) {
    r = r * 1664525 + 1013904223;
    input[i] = byte(r >> 24);
  }
  for ( usize offset = 0; offset < 16; ++offset )
    for ( usize n = 0; n <= 4096; n += n < 96 ? 1 : 127 )
      for ( u64 seed : { u64(0), u64(1), ~u64(0) } ) {
        const u64 expected = micron::hashes::xxhash64_rtseed(input + offset, seed, n);
        sb::require(micron::hashes::xxhash64_bytes(input + offset, n, seed) == expected);
        for ( usize chunk : { usize(1), usize(7), usize(31), usize(32), usize(33), usize(129) } ) {
          micron::hashes::xxhash64_stream h(seed);
          for ( usize pos = 0; pos < n; ) {
            const usize k = n - pos < chunk ? n - pos : chunk;
            h.update(input + offset + pos, k);
            pos += k;
          }
          sb::require(h.digest() == expected);
          sb::require(h.digest() == expected);
          h.reset(seed);
          h.update(input + offset, n);
          sb::require(h.digest() == expected);
        }
      }
  return 1;
}

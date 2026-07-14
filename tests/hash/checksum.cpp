//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/hash/checksum.hpp"

#include "../snowball/snowball.hpp"

using ::sb::print;
using ::sb::require_true;

namespace
{

u64 rng_state = 0x9E3779B97F4A7C15ull;

u8
next_byte()
{
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 7;
  rng_state ^= rng_state << 17;
  return (u8)rng_state;
}

constexpr usize k_big = 70016;
u8 g_buf[k_big];

u32
ref_crc32(const u8 *p, usize len) noexcept
{
  return micron::crc::__crc32_refl_bytewise(p, len, 0xFFFFFFFFu) ^ 0xFFFFFFFFu;
}

};      // namespace

static constexpr u8 k_ascii_digits[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
static constexpr u8 k_wikipedia[9] = { 'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a' };

static_assert(micron::crc32_gzip_refl(0u, k_ascii_digits, 9) == 0xCBF43926u);
static_assert(micron::crc32_gzip_refl(0u, k_ascii_digits, 0) == 0u);
static_assert(micron::adler32(1u, k_wikipedia, 9) == 0x11E60398u);
static_assert(micron::adler32(1u, k_wikipedia, 0) == 1u);
static_assert(micron::hashes::xxhash32(k_ascii_digits, 9) == 0x937BAD67u);
static_assert(micron::hashes::xxhash32(k_ascii_digits, 0) == 0x02CC5D05u);

int
main()
{
  print("=== MICRON CHECKSUM DIFFERENTIAL ===");

#if defined(__micron_crc32_clmul)
  print("crc32   tier: carry-less fold (clmul/pmull) + slice-by-8 tail");
#else
  print("crc32   tier: slice-by-8 ONLY -- no carry-less fold on this build (fold NOT covered by this run)");
#endif
#if defined(__micron_adler32_simd)
  print("adler32 tier: SIMD kernel (avx2/ssse3/neon) + scalar tail");
#else
  print("adler32 tier: scalar ONLY -- no SIMD on this build (kernels NOT covered by this run)");
#endif

  for ( usize i = 0; i < k_big; ++i ) g_buf[i] = next_byte();

  sb::test_case("crc32 known vectors");
  {
    require_true(micron::crc32_gzip_refl(0u, k_ascii_digits, 9) == 0xCBF43926u);
    require_true(micron::crc32_gzip_refl(0u, k_ascii_digits, 0) == 0u);
  }
  sb::end_test_case();

  sb::test_case("crc32 fast paths == bytewise oracle, lengths 0..8192 x offsets {0,7}");
  {
    for ( usize off = 0; off < 8; off += 7 ) {
      for ( usize len = 0; len <= 8192; ++len ) {
        const u32 ours = micron::crc32_gzip_refl(0u, g_buf + off, len);
        const u32 ref = ref_crc32(g_buf + off, len);
        if ( ours != ref ) {
          print("crc32 mismatch at len ", len, " off ", off);
          require_true(false);
        }
      }
    }
  }
  sb::end_test_case();

  sb::test_case("crc32 fast paths == bytewise oracle at large sizes and odd offsets");
  {
    const usize sizes[] = { 4096, 4097, 4111, 16384, 16385, 65535, 65536, 69999 };
    for ( usize sz : sizes )
      for ( usize off = 0; off < 3; ++off )
        require_true(micron::crc32_gzip_refl(0u, g_buf + off, sz - off) == ref_crc32(g_buf + off, sz - off));
  }
  sb::end_test_case();

  sb::test_case("crc32 chaining: split == whole, across fold-group boundaries");
  {

    const usize cuts[] = { 1, 15, 16, 17, 48, 63, 64, 65, 112, 127, 128, 129, 2048, 4095 };
    const u32 whole = micron::crc32_gzip_refl(0u, g_buf, 4096);
    for ( usize cut : cuts ) {
      const u32 a = micron::crc32_gzip_refl(0u, g_buf, cut);
      const u32 b = micron::crc32_gzip_refl(a, g_buf + cut, 4096 - cut);
      require_true(b == whole);
    }
  }
  sb::end_test_case();

  sb::test_case("adler32 known vectors");
  {
    require_true(micron::adler32(1u, k_wikipedia, 9) == 0x11E60398u);
    require_true(micron::adler32(1u, k_wikipedia, 0) == 1u);
  }
  sb::end_test_case();

  sb::test_case("adler32 SIMD kernels == byte-serial oracle, lengths 0..8192 x offsets {0,7}");
  {
    for ( usize off = 0; off < 8; off += 7 ) {
      u32 s1 = 1, s2 = 0;
      for ( usize len = 0; len <= 8192; ++len ) {
        const u32 ref = (s2 << 16) | s1;
        const u32 ours = micron::adler32(1u, g_buf + off, len);
        if ( ours != ref ) {
          print("adler32 mismatch at len ", len, " off ", off);
          require_true(false);
        }

        if ( len < 8192 ) {
          s1 = (s1 + g_buf[off + len]) % 65521;
          s2 = (s2 + s1) % 65521;
        }
      }
    }
  }
  sb::end_test_case();

  sb::test_case("adler32 NMAX boundaries, worst-case (all-0xFF) bytes");
  {

    constexpr usize big = 5552 * 3 + 7;
    static u8 ff[big];
    for ( usize i = 0; i < big; ++i ) ff[i] = 0xFF;
    const usize sizes[] = { 5551, 5552, 5553, 11104, 11105, big };
    for ( usize sz : sizes ) {
      u32 s1 = 1, s2 = 0;
      for ( usize i = 0; i < sz; ++i ) {
        s1 = (s1 + ff[i]) % 65521;
        s2 = (s2 + s1) % 65521;
      }
      require_true(micron::adler32(1u, ff, sz) == ((s2 << 16) | s1));
    }
  }
  sb::end_test_case();

  sb::test_case("adler32 chaining: split == whole, across SIMD-block and NMAX-window boundaries");
  {
    const u32 whole = micron::adler32(1u, g_buf, 8192);
    const usize cuts[] = { 1, 3, 4, 16, 31, 32, 33, 63, 64, 65, 5535, 5536, 5537, 5551, 5552, 5553 };
    for ( usize cut : cuts ) {
      const u32 a = micron::adler32(1u, g_buf, cut);
      const u32 b = micron::adler32(a, g_buf + cut, 8192 - cut);
      require_true(b == whole);
    }
  }
  sb::end_test_case();

  sb::test_case("xxhash32 canonical vectors");
  {
    require_true(micron::hashes::xxhash32(k_ascii_digits, 9) == 0x937BAD67u);
    require_true(micron::hashes::xxhash32(k_ascii_digits, 0) == 0x02CC5D05u);

    require_true(micron::hashes::xxhash32(k_ascii_digits, 9, 0x9E3779B1u) == micron::hashes::xxhash32(k_ascii_digits, 9, 0x9E3779B1u));
    for ( usize len = 0; len <= 64; ++len ) {
      const u32 a = micron::hashes::xxhash32(g_buf, len);
      const u32 b = micron::hashes::xxhash32(g_buf, len, 0);
      require_true(a == b);
    }
  }
  sb::end_test_case();

  sb::test_case("xxhash32 reads unaligned (LZ4 frames checksum at arbitrary offsets)");
  {

    static u8 pad[600];
    for ( usize off = 0; off < 8; ++off ) {
      for ( usize i = 0; i < 512; ++i ) pad[off + i] = g_buf[i];
      require_true(micron::hashes::xxhash32(pad + off, 512) == micron::hashes::xxhash32(g_buf, 512));
    }
  }
  sb::end_test_case();

  print("[CHECKSUM DIFFERENTIAL OK]");
  return 1;
}

//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE GENERIC (SCALAR) SIMD TIER, AGAINST A HAND-WRITTEN ORACLE.
//
// simd/arch/*_generic.hpp is the backend micron uses where no vector unit is available or where one
// exists but is not ours to touch: the generic arch tier, x86 below SSE2, ARM without NEON, and
// MICRON_NO_SIMD (a Linux kernel module on amd64 -- AVX-512 is right there, and using it outside
// kernel_fpu_begin() corrupts user FP state).
//
// This file is deliberately built BOTH WAYS and must pass identically:
//
//     duck test tests/rigor/simd_generic.cpp -o bin/t                      # the ISA backend
//     duck test tests/rigor/simd_generic.cpp --def MICRON_NO_SIMD -o bin/t # the scalar backend
//
// The oracles below are byte loops written out longhand. They are the specification; both backends
// are the thing under test. Comparing the two backends to each other would only prove they agree,
// which is worth nothing if they agree on being wrong -- and the SIMD backend is not the reference
// here, it is a second implementation.
//
// Coverage is chosen to land on the seams rather than to be large:
//   - every length 0..300, which walks the whole small-copy ladder and both sides of every tier
//     boundary in cmemory/bits.hpp, then a set of large lengths for the bulk/NT paths
//   - every src/dst misalignment pair in 0..15, because the block loops peel on alignment
//   - memmove at every overlap delta in [-64, 64], forward and backward, which is the only place
//     copy direction is observable
//   - memcmp/memchr with the difference planted at each byte of the final partial word, where an
//     off-by-one in the word-at-a-time tail would hide
//
// Seeds are fixed hex literals. Never time-based: a fuzz test that cannot be replayed is a bug
// report you cannot act on.

#include "../../src/memory/cmemory.hpp"
#include "../../src/memory/cstring.hpp"
#include "../../src/simd/bitwise.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deterministic source of bytes -- splitmix64, fixed seed

struct rng {
  u64 s;
  constexpr explicit rng(u64 seed) noexcept : s(seed) { }

  u64
  next() noexcept
  {
    u64 z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }

  u8
  byte() noexcept
  {
    return static_cast<u8>(next() >> 24);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the oracles. plain byte loops, no cleverness, no micron calls

void
o_copy(u8 *d, const u8 *s, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) d[i] = s[i];
}

void
o_move(u8 *d, const u8 *s, usize n) noexcept
{
  if ( d == s || n == 0 ) return;
  if ( d < s )
    for ( usize i = 0; i < n; ++i ) d[i] = s[i];
  else
    for ( usize i = n; i-- > 0; ) d[i] = s[i];
}

void
o_set(u8 *d, u8 v, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) d[i] = v;
}

i64
o_cmp(const u8 *a, const u8 *b, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( a[i] != b[i] ) return static_cast<i64>(static_cast<unsigned>(a[i])) - static_cast<i64>(static_cast<unsigned>(b[i]));
  return 0;
}

const u8 *
o_chr(const u8 *p, u8 c, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == c ) return p + i;
  return nullptr;
}

const u8 *
o_rchr(const u8 *p, u8 c, usize n) noexcept
{
  for ( usize i = n; i-- > 0; )
    if ( p[i] == c ) return p + i;
  return nullptr;
}

const u8 *
o_mem(const u8 *h, usize hn, const u8 *ne, usize nn) noexcept
{
  if ( nn == 0 ) return h;
  if ( nn > hn ) return nullptr;
  for ( usize i = 0; i + nn <= hn; ++i ) {
    usize j = 0;
    for ( ; j < nn; ++j )
      if ( h[i + j] != ne[j] ) break;
    if ( j == nn ) return h + i;
  }
  return nullptr;
}

usize
o_ffs(const u8 *p, usize n, u8 c) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == c ) return i;
  return n;
}

usize
o_count(const u8 *p, usize n, u8 c) noexcept
{
  usize k = 0;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == c ) ++k;
  return k;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch. one over-aligned slab so every misalignment is reachable by offsetting into it

constexpr usize kSlab = 1u << 21;      // 2 MiB, past every NT/rep threshold
alignas(64) u8 g_src[kSlab];
alignas(64) u8 g_dst[kSlab];
alignas(64) u8 g_ref[kSlab];

void
fill(u8 *p, usize n, u64 seed) noexcept
{
  rng r{ seed };
  for ( usize i = 0; i < n; ++i ) p[i] = r.byte();
}

// the length set: every seam in the small ladder, then the bulk tiers
constexpr usize kBig[] = { 512, 1024, 2048, 4095, 4096, 16384, 65536, 262144, 1048576 };

}      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("memcpy: lengths 0..300 x misalignment 0..15");
  {
    fill(g_src, kSlab, 0xA5C3F1D2E7B90461ull);
    bool ok = true;
    for ( usize so = 0; so < 16 && ok; ++so )
      for ( usize dofs = 0; dofs < 16 && ok; ++dofs )
        for ( usize n = 0; n <= 300 && ok; ++n ) {
          u8 *d = g_dst + dofs;
          const u8 *s = g_src + so;
          o_set(g_dst, 0xCC, 400);
          o_set(g_ref, 0xCC, 400);
          o_copy(g_ref + dofs, s, n);
          micron::memcpy(reinterpret_cast<byte *>(d), reinterpret_cast<const byte *>(s), n);
          // the copy itself, and that it wrote not one byte outside [dofs, dofs+n)
          if ( o_cmp(g_dst, g_ref, 400) != 0 ) ok = false;
        }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("memcpy: bulk lengths through the NT / rep-movsb tiers");
  {
    fill(g_src, kSlab, 0x1F2E3D4C5B6A7988ull);
    bool ok = true;
    for ( usize n : kBig ) {
      for ( usize ofs : { usize{ 0 }, usize{ 1 }, usize{ 7 }, usize{ 31 } } ) {
        o_set(g_dst, 0x00, n + 64);
        o_set(g_ref, 0x00, n + 64);
        o_copy(g_ref + ofs, g_src + ofs, n);
        micron::memcpy(reinterpret_cast<byte *>(g_dst + ofs), reinterpret_cast<const byte *>(g_src + ofs), n);
        if ( o_cmp(g_dst, g_ref, n + 64) != 0 ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("memmove: every overlap delta in [-64, 64]");
  {
    bool ok = true;
    constexpr usize base = 4096;
    for ( i64 delta = -64; delta <= 64 && ok; ++delta )
      for ( usize n = 0; n <= 200 && ok; ++n ) {
        fill(g_src, base * 2, 0x77AA33CC1188EE55ull);
        o_copy(g_dst, g_src, base * 2);
        o_copy(g_ref, g_src, base * 2);

        u8 *dp = g_dst + base + delta;
        const u8 *sp = g_dst + base;
        u8 *rp = g_ref + base + delta;
        const u8 *rs = g_ref + base;

        o_move(rp, rs, n);
        micron::memmove(reinterpret_cast<byte *>(dp), reinterpret_cast<const byte *>(sp), n);
        if ( o_cmp(g_dst, g_ref, base * 2) != 0 ) ok = false;
      }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("memset / byteset: lengths 0..300 x misalignment, plus bulk");
  {
    bool ok = true;
    for ( usize dofs = 0; dofs < 16 && ok; ++dofs )
      for ( usize n = 0; n <= 300 && ok; ++n ) {
        o_set(g_dst, 0xCC, 400);
        o_set(g_ref, 0xCC, 400);
        o_set(g_ref + dofs, 0x5A, n);
        micron::memset(reinterpret_cast<byte *>(g_dst + dofs), static_cast<byte>(0x5A), n);
        if ( o_cmp(g_dst, g_ref, 400) != 0 ) ok = false;
      }
    for ( usize n : kBig ) {
      o_set(g_dst, 0x11, n + 64);
      o_set(g_ref, 0x11, n + 64);
      o_set(g_ref + 3, 0xA7, n);
      micron::memset(reinterpret_cast<byte *>(g_dst + 3), static_cast<byte>(0xA7), n);
      if ( o_cmp(g_dst, g_ref, n + 64) != 0 ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // the sign of the result is contractual (>0 / <0 / ==0), the magnitude is not
  sb::test_case("memcmp: equal, and difference planted at every byte");
  {
    bool ok = true;
    fill(g_src, 4096, 0x0BADC0DE12345678ull);
    for ( usize n = 0; n <= 300 && ok; ++n )
      for ( usize ofs = 0; ofs < 16 && ok; ++ofs ) {
        o_copy(g_dst, g_src, 4096);
        const i64 e = micron::memcmp<byte>(reinterpret_cast<const byte *>(g_src + ofs), reinterpret_cast<const byte *>(g_dst + ofs), n);
        if ( e != 0 ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          const u8 save = g_dst[ofs + k];
          g_dst[ofs + k] = static_cast<u8>(save ^ 0xFFu);
          const i64 got
              = micron::memcmp<byte>(reinterpret_cast<const byte *>(g_src + ofs), reinterpret_cast<const byte *>(g_dst + ofs), n);
          const i64 want = o_cmp(g_src + ofs, g_dst + ofs, n);
          if ( (got < 0) != (want < 0) || (got > 0) != (want > 0) || (got == 0) != (want == 0) ) ok = false;
          g_dst[ofs + k] = save;
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("simd::memchr128 / memrchr128: hit at every position, and miss");
  {
    bool ok = true;
    for ( usize n = 0; n <= 200 && ok; ++n )
      for ( usize ofs = 0; ofs < 16 && ok; ++ofs ) {
        o_set(g_dst, 0x41, n + ofs + 8);
        const u8 *p = g_dst + ofs;

        // miss: the needle appears nowhere
        if ( micron::simd::memchr128<u8>(p, 0x7E, n) != nullptr ) ok = false;
        if ( micron::simd::memrchr128<u8>(p, 0x7E, n) != nullptr ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          g_dst[ofs + k] = 0x7E;
          if ( micron::simd::memchr128<u8>(p, 0x7E, n) != o_chr(p, 0x7E, n) ) ok = false;
          if ( micron::simd::memrchr128<u8>(p, 0x7E, n) != o_rchr(p, 0x7E, n) ) ok = false;
          g_dst[ofs + k] = 0x41;
        }

        // two hits: memchr must find the first, memrchr the last
        if ( n >= 2 ) {
          g_dst[ofs] = 0x7E;
          g_dst[ofs + n - 1] = 0x7E;
          if ( micron::simd::memchr128<u8>(p, 0x7E, n) != p ) ok = false;
          if ( micron::simd::memrchr128<u8>(p, 0x7E, n) != p + n - 1 ) ok = false;
          g_dst[ofs] = 0x41;
          g_dst[ofs + n - 1] = 0x41;
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("simd::memmem128: needle at every offset, empty needle, oversized needle");
  {
    bool ok = true;
    fill(g_src, 1024, 0x5150494E4E454421ull);
    // force a dense alphabet so first-byte candidates fire constantly
    for ( usize i = 0; i < 1024; ++i ) g_src[i] = static_cast<u8>('a' + (g_src[i] % 4));

    for ( usize nn = 1; nn <= 20 && ok; ++nn )
      for ( usize at = 0; at + nn <= 300 && ok; at += 7 ) {
        o_copy(g_dst, g_src, 1024);
        u8 needle[24];
        o_copy(needle, g_dst + at, nn);
        const u8 *want = o_mem(g_dst, 300, needle, nn);
        const u8 *got = micron::simd::memmem128<u8>(g_dst, 300, needle, nn);
        if ( got != want ) ok = false;
      }

    // empty needle returns the haystack; a needle longer than the haystack misses
    if ( micron::simd::memmem128<u8>(g_dst, 16, g_src, 0) != g_dst ) ok = false;
    if ( micron::simd::memmem128<u8>(g_dst, 4, g_src, 8) != nullptr ) ok = false;
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("simd::wordset128: pattern phase is preserved at every length");
  {
    bool ok = true;
    const u64 pat = 0x0123456789ABCDEFull;
    u8 pb[8];
    __builtin_memcpy(pb, &pat, 8);
    for ( usize n = 0; n <= 300 && ok; ++n )
      for ( usize ofs = 0; ofs < 16 && ok; ++ofs ) {
        o_set(g_dst, 0xCC, 400);
        micron::simd::wordset128(g_dst + ofs, pat, n);
        for ( usize i = 0; i < n; ++i )
          if ( g_dst[ofs + i] != pb[i % 8] ) ok = false;
        for ( usize i = 0; i < ofs; ++i )
          if ( g_dst[i] != 0xCC ) ok = false;
        if ( g_dst[ofs + n] != 0xCC ) ok = false;
      }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("simd::mempcpy128 returns dest + bytes");
  {
    bool ok = true;
    for ( usize n = 0; n <= 200 && ok; ++n ) {
      u8 *e = micron::simd::mempcpy128<u8>(g_dst, g_src, n);
      if ( e != g_dst + n ) ok = false;
      if ( o_cmp(g_dst, g_src, n) != 0 ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("simd byte predicates: find_first_set / count_set / any / all / none");
  {
    bool ok = true;
    for ( usize n = 0; n <= 200 && ok; ++n )
      for ( usize ofs = 0; ofs < 16 && ok; ++ofs ) {
        o_set(g_dst, 0x30, n + ofs + 8);
        const u8 *p = g_dst + ofs;

        // uniform buffer: all_set true, none/any consistent
        if ( micron::simd::find_first_set_128(p, n, '0') != o_ffs(p, n, '0') ) ok = false;
        if ( micron::simd::count_set_128(p, n, '0') != o_count(p, n, '0') ) ok = false;
        if ( micron::simd::all_set_128(p, n, '0') != true ) ok = false;
        if ( micron::simd::any_set_128(p, n, '9') != false ) ok = false;
        if ( micron::simd::none_set_128(p, n, '9') != true ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          g_dst[ofs + k] = '9';
          if ( micron::simd::find_first_set_128(p, n, '9') != o_ffs(p, n, '9') ) ok = false;
          if ( micron::simd::count_set_128(p, n, '9') != o_count(p, n, '9') ) ok = false;
          if ( micron::simd::any_set_128(p, n, '9') != true ) ok = false;
          if ( micron::simd::none_set_128(p, n, '9') != false ) ok = false;
          if ( n > 1 && micron::simd::all_set_128(p, n, '9') != false ) ok = false;
          g_dst[ofs + k] = '0';
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  sb::test_case("micron::strlen agrees with the oracle at every alignment");
  {
    bool ok = true;
    for ( usize n = 0; n <= 300 && ok; ++n )
      for ( usize ofs = 0; ofs < 16 && ok; ++ofs ) {
        char *c = reinterpret_cast<char *>(g_dst + ofs);
        for ( usize i = 0; i < n; ++i ) c[i] = 'x';
        c[n] = '\0';
        usize want = 0;
        while ( c[want] ) ++want;
        if ( micron::strlen(c) != want ) ok = false;
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// THE simd BYTE-PREDICATE FAMILY, AGAINST A SCALAR ORACLE.
//
// find_first_set_* / count_set_* / any_set_* / all_set_* / none_set_* are the whole of
// simd/bitwise.hpp. Only any_set_128 has an in-tree caller (io.hpp's flush check), so the rest were
// never exercised by anything -- which is how all_set_128 stayed wrong on x86 for every buffer of
// 16 bytes or more without a single test noticing.
//
// INVERTED POLARITY: every assertion below states the property that SHOULD hold, so this file FAILS
// on a tree carrying the defect and that observed failure is the finding. On an unpatched tree the
// "all_set: uniform buffer" case fails at n = 16.
//
// The oracles are byte loops written out longhand. Comparing the 128 path to the 256 path would
// only prove they agree, which is worth nothing when both can be wrong the same way.
//
// Build: duck test tests/rigor/simd_bitwise.cpp -o bin/t
//        duck test tests/rigor/simd_bitwise.cpp --isa base -o bin/t     (SSE2 leg)
//        duck test tests/rigor/simd_bitwise.cpp --isa v3   -o bin/t     (AVX2 leg)

#include "../../src/simd/bitwise.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;

namespace
{

// scratch is over-aligned so every misalignment in 0..15 is reachable by offsetting into it; the
// block loops peel on alignment, and the tail is where an off-by-one hides
alignas(64) unsigned char g_buf[1024];

constexpr char kFill = '0';
constexpr char kNeedle = '9';

usize
o_first(const unsigned char *p, usize n, char c) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == static_cast<unsigned char>(c) ) return i;
  return n;
}

usize
o_count(const unsigned char *p, usize n, char c) noexcept
{
  usize k = 0;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == static_cast<unsigned char>(c) ) ++k;
  return k;
}

bool
o_all(const unsigned char *p, usize n, char c) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] != static_cast<unsigned char>(c) ) return false;
  return true;
}

// bitwise_arm32.hpp and bitwise_arm64.hpp carry the 128 set only; the _256 forms are amd64-only.
// Every check runs on each width the target actually has, so one file covers all three backends.
struct chk {
  static bool
  all(const unsigned char *p, usize n, char c, bool want) noexcept
  {
    if ( micron::simd::all_set_128(p, n, c) != want ) return false;
#if defined(__micron_arch_x86_any)
    if ( micron::simd::all_set_256(p, n, c) != want ) return false;
#endif
    return true;
  }

  static bool
  first(const unsigned char *p, usize n, char c, usize want) noexcept
  {
    if ( micron::simd::find_first_set_128(p, n, c) != want ) return false;
#if defined(__micron_arch_x86_any)
    if ( micron::simd::find_first_set_256(p, n, c) != want ) return false;
#endif
    return true;
  }

  static bool
  count(const unsigned char *p, usize n, char c, usize want) noexcept
  {
    if ( micron::simd::count_set_128(p, n, c) != want ) return false;
#if defined(__micron_arch_x86_any)
    if ( micron::simd::count_set_256(p, n, c) != want ) return false;
#endif
    return true;
  }

  static bool
  presence(const unsigned char *p, usize n, char c, bool want) noexcept
  {
    if ( micron::simd::any_set_128(p, n, c) != want ) return false;
    if ( micron::simd::none_set_128(p, n, c) != !want ) return false;
#if defined(__micron_arch_x86_any)
    if ( micron::simd::any_set_256(p, n, c) != want ) return false;
    if ( micron::simd::none_set_256(p, n, c) != !want ) return false;
#endif
    return true;
  }
};

}      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // THE ONE THAT CATCHES IT. A buffer whose every byte matches must answer all_set == true.
  // _mm_movemask_epi8 fills the LOW 16 bits and zeroes the rest, so a full 16-byte match is
  // 0x0000FFFF -- never -1. Any length below 16 takes the scalar tail and passes either way, which
  // is why a small smoke test would have missed this entirely.
  sb::test_case("all_set: a uniform buffer is all-set at every length and misalignment");
  {
    bool ok = true;
    for ( usize ofs = 0; ofs < 16 && ok; ++ofs )
      for ( usize n = 0; n <= 300 && ok; ++n ) {
        unsigned char *p = g_buf + ofs;
        for ( usize i = 0; i < n; ++i ) p[i] = static_cast<unsigned char>(kFill);
        if ( !chk::all(p, n, kFill, true) ) ok = false;
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("all_set: one wrong byte at any position makes it false");
  {
    bool ok = true;
    for ( usize ofs = 0; ofs < 16 && ok; ++ofs )
      for ( usize n = 1; n <= 200 && ok; ++n ) {
        unsigned char *p = g_buf + ofs;
        for ( usize i = 0; i < n; ++i ) p[i] = static_cast<unsigned char>(kFill);
        for ( usize k = 0; k < n && ok; ++k ) {
          p[k] = static_cast<unsigned char>(kNeedle);
          if ( !chk::all(p, n, kFill, o_all(p, n, kFill)) ) ok = false;
          p[k] = static_cast<unsigned char>(kFill);
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("find_first_set: hit at every position, and miss");
  {
    bool ok = true;
    for ( usize ofs = 0; ofs < 16 && ok; ++ofs )
      for ( usize n = 0; n <= 200 && ok; ++n ) {
        unsigned char *p = g_buf + ofs;
        for ( usize i = 0; i < n; ++i ) p[i] = static_cast<unsigned char>(kFill);

        // a miss must answer exactly len, not some sentinel
        if ( !chk::first(p, n, kNeedle, n) ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          p[k] = static_cast<unsigned char>(kNeedle);
          if ( !chk::first(p, n, kNeedle, o_first(p, n, kNeedle)) ) ok = false;
          p[k] = static_cast<unsigned char>(kFill);
        }

        // two hits: it must find the FIRST
        if ( n >= 2 ) {
          p[0] = static_cast<unsigned char>(kNeedle);
          p[n - 1] = static_cast<unsigned char>(kNeedle);
          if ( !chk::first(p, n, kNeedle, 0) ) ok = false;
          p[0] = static_cast<unsigned char>(kFill);
          p[n - 1] = static_cast<unsigned char>(kFill);
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("count_set: 0, 1 and n matches at every length");
  {
    bool ok = true;
    for ( usize ofs = 0; ofs < 16 && ok; ++ofs )
      for ( usize n = 0; n <= 200 && ok; ++n ) {
        unsigned char *p = g_buf + ofs;
        for ( usize i = 0; i < n; ++i ) p[i] = static_cast<unsigned char>(kFill);

        if ( !chk::count(p, n, kNeedle, 0) ) ok = false;
        // every byte matches: the count is the length
        if ( !chk::count(p, n, kFill, n) ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          p[k] = static_cast<unsigned char>(kNeedle);
          if ( !chk::count(p, n, kNeedle, o_count(p, n, kNeedle)) ) ok = false;
          p[k] = static_cast<unsigned char>(kFill);
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("any_set / none_set are exact complements of find_first_set");
  {
    bool ok = true;
    for ( usize ofs = 0; ofs < 16 && ok; ++ofs )
      for ( usize n = 0; n <= 200 && ok; ++n ) {
        unsigned char *p = g_buf + ofs;
        for ( usize i = 0; i < n; ++i ) p[i] = static_cast<unsigned char>(kFill);

        if ( !chk::presence(p, n, kNeedle, false) ) ok = false;

        for ( usize k = 0; k < n && ok; ++k ) {
          p[k] = static_cast<unsigned char>(kNeedle);
          if ( !chk::presence(p, n, kNeedle, o_first(p, n, kNeedle) != n) ) ok = false;
          p[k] = static_cast<unsigned char>(kFill);
        }
      }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

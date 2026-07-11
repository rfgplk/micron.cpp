//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Exhaustive small-size memcpy/memmove/memset validation:
//   n = 0..300 x dst_off 0..63 x src_off 0..63, byte-exact vs a naive
//   reference, with a FULL-storage sentinel scan (any stray wide store,
//   before or after the window, fails).
// Sparse larger sizes cross every dispatch boundary up to 1 MiB + 1.
//
// Build with --def MEM_RIGOR_LITE for qemu runs (offset loops step 3).

#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

namespace
{

#if defined(MEM_RIGOR_LITE)
constexpr u64 OFF_STEP = 3;
#else
constexpr u64 OFF_STEP = 1;
#endif

constexpr byte SENT = 0xEE;
constexpr u64 GUARD = 64;
constexpr u64 MAXN = 300;

// [GUARD sentinel][offset region 0..63][payload MAXN][slack][GUARD sentinel]
alignas(64) byte dst_store[GUARD + 64 + MAXN + 64 + GUARD];
alignas(64) byte src_store[GUARD + 64 + MAXN + 64 + GUARD];

[[gnu::always_inline]] inline byte
src_pat(u64 i) noexcept
{
  return static_cast<byte>(i * 31 + 17);
}

bool
check_copy(u64 n, u64 d_off, u64 s_off, bool move) noexcept
{
  byte *d = dst_store + GUARD + d_off;
  const byte *s = src_store + GUARD + s_off;
  for ( u64 i = 0; i < sizeof(dst_store); ++i ) dst_store[i] = SENT;
  if ( move )
    mc::memmove(d, const_cast<byte *>(s), n);
  else
    mc::memcpy(d, s, n);
  for ( u64 i = 0; i < sizeof(dst_store); ++i ) {
    const byte *p = dst_store + i;
    if ( p >= d && p < d + n ) {
      if ( *p != s[static_cast<u64>(p - d)] ) return false;
    } else if ( *p != SENT )
      return false;
  }
  return true;
}

bool
check_set(u64 n, u64 d_off, byte v) noexcept
{
  byte *d = dst_store + GUARD + d_off;
  for ( u64 i = 0; i < sizeof(dst_store); ++i ) dst_store[i] = SENT;
  mc::memset(d, v, n);
  for ( u64 i = 0; i < sizeof(dst_store); ++i ) {
    const byte *p = dst_store + i;
    if ( p >= d && p < d + n ) {
      if ( *p != v ) return false;
    } else if ( *p != SENT )
      return false;
  }
  return true;
}

// sparse large sizes: window verify + 512-byte aprons + outer guards
constexpr u64 LMAX = (1ull << 20) + 1;
constexpr u64 LGUARD = 512;
alignas(64) byte ldst[LGUARD + 64 + LMAX + 64 + LGUARD];
alignas(64) byte lsrc[LGUARD + 64 + LMAX + 64 + LGUARD];

bool
check_large(u64 n, u64 d_off, u64 s_off, u32 which) noexcept
{
  byte *d = ldst + LGUARD + d_off;
  const byte *s = lsrc + LGUARD + s_off;
  const u64 lo = static_cast<u64>(d - ldst) - LGUARD;
  const u64 hi = static_cast<u64>(d - ldst) + n + LGUARD;
  for ( u64 i = lo; i < hi; ++i ) ldst[i] = SENT;
  if ( which == 0 )
    mc::memcpy(d, s, n);
  else if ( which == 1 )
    mc::memmove(d, const_cast<byte *>(s), n);
  else
    mc::memset(d, static_cast<byte>(0x5A), n);
  for ( u64 i = lo; i < hi; ++i ) {
    const byte *p = ldst + i;
    if ( p >= d && p < d + n ) {
      if ( which == 2 ) {
        if ( *p != static_cast<byte>(0x5A) ) return false;
      } else if ( *p != s[static_cast<u64>(p - d)] )
        return false;
    } else if ( *p != SENT )
      return false;
  }
  return true;
}

}      // namespace

int
main()
{
  for ( u64 i = 0; i < sizeof(src_store); ++i ) src_store[i] = src_pat(i);
  for ( u64 i = 0; i < sizeof(lsrc); ++i ) lsrc[i] = src_pat(i * 7 + 3);

  sb::test_case("memcpy/memmove exhaustive 0..300 x 64x64 offsets");
  for ( u64 n = 0; n <= MAXN; ++n )
    for ( u64 d_off = 0; d_off < 64; d_off += OFF_STEP )
      for ( u64 s_off = 0; s_off < 64; s_off += OFF_STEP ) {
        if ( !check_copy(n, d_off, s_off, false) ) {
          sb::print("memcpy failed: n=", n, " d_off=", d_off, " s_off=", s_off);
          sb::require(false);
        }
        if ( !check_copy(n, d_off, s_off, true) ) {
          sb::print("memmove failed: n=", n, " d_off=", d_off, " s_off=", s_off);
          sb::require(false);
        }
      }
  sb::end_test_case();

  sb::test_case("memset exhaustive 0..300 x 64 offsets x fills");
  const byte fills[] = { static_cast<byte>(0x00), static_cast<byte>(0xFF), static_cast<byte>(0x5A) };
  for ( u64 n = 0; n <= MAXN; ++n )
    for ( u64 d_off = 0; d_off < 64; d_off += OFF_STEP )
      for ( byte v : fills )
        if ( !check_set(n, d_off, v) ) {
          sb::print("memset failed: n=", n, " d_off=", d_off, " v=", (u64)v);
          sb::require(false);
        }
  sb::end_test_case();

  sb::test_case("sparse sizes crossing dispatch boundaries up to 1MiB+1");
  const u64 sparse[] = { 301,  511,   512,   513,   1023,  1024,  1025,   2047,       2048,
                         2049, 2111,  2112,  2113,  4095,  4096,  4097,   8191,       8192,
                         8193, 16384, 32768, 65535, 65536, 65537, 262144, 1ull << 20, (1ull << 20) + 1 };
  const u64 loffs[] = { 0, 1, 63 };
  for ( u64 n : sparse )
    for ( u64 d_off : loffs )
      for ( u64 s_off : loffs )
        for ( u32 which = 0; which < 3; ++which )
          if ( !check_large(n, d_off, s_off, which) ) {
            sb::print("large op ", (u64)which, " failed: n=", n, " d_off=", d_off, " s_off=", s_off);
            sb::require(false);
          }
  sb::end_test_case();

  sb::print("mem_exhaustive_small: all cases passed");
  return 1;
}

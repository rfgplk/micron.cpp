//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Exhaustive memmove overlap validation: every delta in [-130, 130] against
// a shadow-simulated expected buffer, comparing the ENTIRE storage (catches
// out-of-bounds writes in any direction and wrong-direction copies), plus
// large (1 MiB) overlaps at the deltas that matter for the bulk loops.

#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

namespace
{

constexpr u64 BUF = 700;
constexpr i64 A = 260;      // anchor: src window start

byte buf[BUF];
byte shadow[BUF];
byte tmp[256];

bool
check_overlap(i64 delta, u64 n) noexcept
{
  for ( u64 i = 0; i < BUF; ++i ) {
    buf[i] = static_cast<byte>(i * 31 + 17);
    shadow[i] = buf[i];
  }
  // expected: copy src window [A, A+n) to [A+delta, A+delta+n) via a temp
  for ( u64 i = 0; i < n; ++i ) tmp[i] = shadow[static_cast<u64>(A) + i];
  for ( u64 i = 0; i < n; ++i ) shadow[static_cast<u64>(A + delta) + i] = tmp[i];
  mc::memmove(buf + A + delta, buf + A, n);
  for ( u64 i = 0; i < BUF; ++i )
    if ( buf[i] != shadow[i] ) return false;
  return true;
}

constexpr u64 LN = 1ull << 20;
constexpr u64 LBUF = 3ull << 20;
byte lbuf[LBUF];
byte lshadow[LBUF];
byte ltmp[LN];

bool
check_overlap_large(i64 delta) noexcept
{
  const u64 anchor = 1ull << 20;
  for ( u64 i = 0; i < LBUF; ++i ) {
    lbuf[i] = static_cast<byte>(i * 131 + 5);
    lshadow[i] = lbuf[i];
  }
  for ( u64 i = 0; i < LN; ++i ) ltmp[i] = lshadow[anchor + i];
  for ( u64 i = 0; i < LN; ++i ) lshadow[static_cast<u64>(static_cast<i64>(anchor) + delta) + i] = ltmp[i];
  mc::memmove(lbuf + static_cast<i64>(anchor) + delta, lbuf + anchor, LN);
  for ( u64 i = 0; i < LBUF; ++i )
    if ( lbuf[i] != lshadow[i] ) return false;
  return true;
}

}      // namespace

int
main()
{
  sb::test_case("memmove all deltas [-130,130] x sizes");
  const u64 sizes_tail[] = { 127, 128, 129, 192, 255, 256 };
  for ( i64 delta = -130; delta <= 130; ++delta ) {
    for ( u64 n = 0; n <= 96; ++n )
      if ( !check_overlap(delta, n) ) {
        sb::print("memmove overlap failed: delta=", delta, " n=", n);
        sb::require(false);
      }
    for ( u64 n : sizes_tail )
      if ( !check_overlap(delta, n) ) {
        sb::print("memmove overlap failed: delta=", delta, " n=", n);
        sb::require(false);
      }
  }
  sb::end_test_case();

  sb::test_case("memmove large overlaps (1 MiB)");
  const i64 ldeltas[] = { 1, -1, 63, -63, 64, -64, 4096, -4096, (i64)(LN / 2), -(i64)(LN / 2) };
  for ( i64 delta : ldeltas )
    if ( !check_overlap_large(delta) ) {
      sb::print("memmove large overlap failed: delta=", delta);
      sb::require(false);
    }
  sb::end_test_case();

  sb::print("mem_overlap_exhaustive: all cases passed");
  return 1;
}

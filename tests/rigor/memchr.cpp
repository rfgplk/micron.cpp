//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Helpers
// ============================================================

template <u64 N>
void
fill_seq(byte (&buf)[N])
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = static_cast<byte>(i);
}

template <u64 N>
void
fill_value(byte (&buf)[N], byte v)
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = v;
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== MEMCHR TESTS ===");

  // ----------------------------------------------------------
  //  Basic find
  // ----------------------------------------------------------

  sb::test_case("memchr - find first byte");
  {
    byte buf[16];
    fill_seq(buf);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x00), 16);
    sb::require(r == buf);
  }
  sb::end_test_case();

  sb::test_case("memchr - find at end");
  {
    byte buf[16];
    fill_seq(buf);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x0F), 16);
    sb::require(r == buf + 15);
  }
  sb::end_test_case();

  sb::test_case("memchr - find at middle");
  {
    byte buf[32];
    fill_seq(buf);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x0A), 32);
    sb::require(r == buf + 0x0A);
  }
  sb::end_test_case();

  sb::test_case("memchr - find at exact length boundary");
  {
    byte buf[32];
    fill_seq(buf);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x10), 17);     // 0x10 sits at index 16
    sb::require(r == buf + 16);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Not found
  // ----------------------------------------------------------

  sb::test_case("memchr - not found returns null");
  {
    byte buf[32];
    fill_value(buf, static_cast<byte>(0x42));
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x99), 32);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("memchr - target byte just past length");
  {
    byte buf[16];
    fill_seq(buf);
    // 0x10 would be at index 16 if buf were larger; with n=16, must miss
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x10), 16);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Empty / zero length
  // ----------------------------------------------------------

  sb::test_case("memchr - n=0 returns null");
  {
    byte buf[16];
    fill_seq(buf);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x00), 0);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Multiple matches: must return first
  // ----------------------------------------------------------

  sb::test_case("memchr - multiple matches returns first");
  {
    byte buf[32];
    fill_value(buf, static_cast<byte>(0x42));
    buf[5] = static_cast<byte>(0x77);
    buf[10] = static_cast<byte>(0x77);
    buf[20] = static_cast<byte>(0x77);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x77), 32);
    sb::require(r == buf + 5);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Various sizes
  // ----------------------------------------------------------

  sb::test_case("memchr - n=1 hit");
  {
    byte buf[1] = { static_cast<byte>(0x55) };
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x55), 1);
    sb::require(r == buf);
  }
  sb::end_test_case();

  sb::test_case("memchr - n=1 miss");
  {
    byte buf[1] = { static_cast<byte>(0x55) };
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0xAA), 1);
    sb::require(r == nullptr);
  }
  sb::end_test_case();

  sb::test_case("memchr - large buffer hit at end");
  {
    byte buf[256];
    fill_value(buf, static_cast<byte>(0x00));
    buf[255] = static_cast<byte>(0xFF);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0xFF), 256);
    sb::require(r == buf + 255);
  }
  sb::end_test_case();

  sb::test_case("memchr - large buffer hit at start");
  {
    byte buf[256];
    fill_value(buf, static_cast<byte>(0x00));
    buf[0] = static_cast<byte>(0x33);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x33), 256);
    sb::require(r == buf);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Sentinel sanity (0x00 byte, classic strchr edge)
  // ----------------------------------------------------------

  sb::test_case("memchr - find 0x00 in mostly-0xFF buffer");
  {
    byte buf[64];
    fill_value(buf, static_cast<byte>(0xFF));
    buf[42] = static_cast<byte>(0x00);
    byte *r = mc::memchr<byte>(&buf[0], static_cast<byte>(0x00), 64);
    sb::require(r == buf + 42);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Const-correctness: const input + non-const return
  // ----------------------------------------------------------

  sb::test_case("memchr - const-input returns mutable pointer");
  {
    byte buf[16];
    fill_seq(buf);
    const byte *cbuf = buf;
    byte *r = mc::memchr<byte>(&cbuf[0], static_cast<byte>(0x05), 16);
    sb::require(r == buf + 5);
    *r = static_cast<byte>(0xAA);     // proves r is writable
    sb::require(buf[5] == 0xAA);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Dispatch boundary sweep — exercises ≤32 scalar path and
  //  the asm-backed simd path on either side of the threshold.
  // ----------------------------------------------------------
  sb::test_case("memchr - dispatch boundary sweep");
  {
    byte buf[256];
    for ( u64 sz : { (u64)0, (u64)1, (u64)15, (u64)16, (u64)17, (u64)31, (u64)32, (u64)33, (u64)47, (u64)48, (u64)49, (u64)63, (u64)64,
                     (u64)65, (u64)128, (u64)256 } ) {
      fill_value(buf, static_cast<byte>(0xAA));
      sb::require(mc::memchr<byte>(buf, static_cast<byte>(0xCC), sz) == nullptr);
      if ( sz == 0 ) continue;
      buf[0] = 0xCC;
      sb::require(mc::memchr<byte>(buf, static_cast<byte>(0xCC), sz) == &buf[0]);
      buf[0] = 0xAA;
      buf[sz - 1] = 0xCC;
      sb::require(mc::memchr<byte>(buf, static_cast<byte>(0xCC), sz) == &buf[sz - 1]);
    }
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  memrchr - reverse byte search
  // ----------------------------------------------------------
  sb::test_case("memrchr - last hit semantics");
  {
    byte buf[128];
    fill_value(buf, static_cast<byte>(0xAA));
    buf[5] = 0xCC;
    buf[40] = 0xCC;
    buf[120] = 0xCC;
    sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), 128) == &buf[120]);
    sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), 100) == &buf[40]);
    sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), 30) == &buf[5]);
    sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), 5) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("memrchr - dispatch boundary sweep");
  {
    byte buf[256];
    for ( u64 sz : { (u64)1, (u64)15, (u64)16, (u64)17, (u64)31, (u64)32, (u64)33, (u64)47, (u64)48, (u64)49, (u64)63, (u64)64, (u64)65,
                     (u64)128, (u64)256 } ) {
      fill_value(buf, static_cast<byte>(0xAA));
      sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), sz) == nullptr);
      buf[0] = 0xCC;
      buf[sz - 1] = 0xCC;
      sb::require(mc::memrchr<byte>(buf, static_cast<byte>(0xCC), sz) == &buf[sz - 1]);
    }
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  memmem - substring search
  // ----------------------------------------------------------
  sb::test_case("memmem - empty needle returns haystack");
  {
    byte hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte nee[1] = { 0 };
    sb::require(mc::memmem<byte>(hay, (u64)8, nee, (u64)0) == hay);
  }
  sb::end_test_case();

  sb::test_case("memmem - needle larger than haystack");
  {
    byte hay[4] = { 1, 2, 3, 4 };
    byte nee[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    sb::require(mc::memmem<byte>(hay, (u64)4, nee, (u64)8) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("memmem - first match across multiple occurrences");
  {
    byte hay[256];
    fill_value(hay, static_cast<byte>(0xAA));
    hay[10] = 'A';
    hay[11] = 'B';
    hay[12] = 'C';
    hay[100] = 'A';
    hay[101] = 'B';
    hay[102] = 'C';
    byte nee[3] = { 'A', 'B', 'C' };
    sb::require(mc::memmem<byte>(hay, (u64)256, nee, (u64)3) == &hay[10]);
  }
  sb::end_test_case();

  sb::test_case("memmem - needle not found");
  {
    byte hay[256];
    fill_value(hay, static_cast<byte>(0xAA));
    byte nee[3] = { 'X', 'Y', 'Z' };
    sb::require(mc::memmem<byte>(hay, (u64)256, nee, (u64)3) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("memmem - needle == haystack");
  {
    byte buf[64];
    fill_seq(buf);
    sb::require(mc::memmem<byte>(buf, (u64)64, buf, (u64)64) == buf);
  }
  sb::end_test_case();

  sb::test_case("memmem - prefix-only false match");
  {
    byte hay[64];
    fill_value(hay, static_cast<byte>(0xAA));
    hay[20] = 'X';
    hay[21] = 'Y';
    hay[22] = 'A';
    byte nee[3] = { 'X', 'Y', 'Z' };
    sb::require(mc::memmem<byte>(hay, (u64)64, nee, (u64)3) == nullptr);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 0;
}

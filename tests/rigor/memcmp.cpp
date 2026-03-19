//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../../src/slice.hpp"

#include "../snowball/snowball.hpp"

#include <limits>

// ============================================================
//  Notes on return-value semantics
//
//  All compare functions in this library return:
//    0                              — buffers are equal for the given count
//    non-zero (pointer difference)  — first differing element found;
//                                     value is &src[i] - &dest[i] in bytes
//    numeric_limits<i64>::min()     — safe-variant precondition failure
//                                     (nullptr, misalignment, invalid address)
//
//  Because the non-zero return is a raw pointer difference between two
//  independent stack arrays, its sign is platform/layout-dependent.
//  Tests therefore only assert:
//    • == 0  for equal regions
//    • != 0  for unequal regions
//    • == i64::min()  for error conditions
//
//  For "early-stop" correctness we test that a difference BEYOND the
//  count window does NOT produce a non-zero result (off-by-one guard).
// ============================================================

static constexpr i64 ERR = std::numeric_limits<i64>::min();

// ============================================================
//  Helpers
// ============================================================

template <typename T, u64 N>
void
fill(T (&buf)[N], T val)
{
  for ( u64 i = 0; i < N; i++ )
    buf[i] = val;
}

template <typename T>
void
fill(T *buf, u64 n, T val)
{
  for ( u64 i = 0; i < n; i++ )
    buf[i] = val;
}

// Build two identical buffers, then corrupt exactly one position in src
template <typename T, u64 N>
void
make_differ_at(T (&src)[N], T (&dst)[N], T base, u64 pos, T corrupt)
{
  fill(src, base);
  fill(dst, base);
  src[pos] = corrupt;
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== MEMCMP TESTS ===");

  // ============================================================
  //  Section 1: mc::memcmp<T,F>
  // ============================================================

  sb::test_case("memcmp<byte,byte>: equal buffers return 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xAB));
    fill(b, static_cast<byte>(0xAB));
    sb::require(mc::memcmp<byte>(a, b, 32) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: first byte differs returns non-zero");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[0] = 0xFF;
    sb::require(mc::memcmp<byte>(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: last byte differs returns non-zero");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x55));
    fill(b, static_cast<byte>(0x55));
    a[31] = 0xAA;
    sb::require(mc::memcmp<byte>(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: middle byte differs returns non-zero");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x11));
    fill(b, static_cast<byte>(0x11));
    a[16] = 0x22;
    sb::require(mc::memcmp<byte>(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: difference beyond count window is ignored (off-by-one)");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xCC));
    fill(b, static_cast<byte>(0xCC));
    a[31] = 0xFF;                                     // only byte 31 differs
    sb::require(mc::memcmp<byte>(a, b, 31) == 0);     // count=31, byte 31 outside window
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: count = 1 equal");
  {
    byte a[4] = { 0x42, 0xFF, 0xFF, 0xFF };
    byte b[4] = { 0x42, 0x00, 0x00, 0x00 };
    sb::require(mc::memcmp<byte>(a, b, 1) == 0);     // only first byte compared
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: count = 1 unequal");
  {
    byte a[4] = { 0x01, 0x00, 0x00, 0x00 };
    byte b[4] = { 0x02, 0x00, 0x00, 0x00 };
    sb::require(mc::memcmp<byte>(a, b, 1) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: count = 3 (non-multiple-of-4) equal");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0xAA));
    fill(b, static_cast<byte>(0xAA));
    sb::require(mc::memcmp<byte>(a, b, 3) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<byte,byte>: count = 3 difference at position 2 (last in window)");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0xAA));
    fill(b, static_cast<byte>(0xAA));
    a[2] = 0xBB;
    sb::require(mc::memcmp<byte>(a, b, 3) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<u32,u32>: equal u32 buffers return 0");
  {
    u32 a[8], b[8];
    fill(a, 0xDEADBEEFu);
    fill(b, 0xDEADBEEFu);
    sb::require(mc::memcmp<u32>(a, b, 8) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<u32,u32>: one u32 element differs");
  {
    u32 a[8], b[8];
    fill(a, 0xDEADBEEFu);
    fill(b, 0xDEADBEEFu);
    a[5] = 0xCAFEBABEu;
    sb::require(mc::memcmp<u32>(a, b, 8) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp<u32,u32>: difference beyond count window ignored");
  {
    u32 a[8], b[8];
    fill(a, 0x11223344u);
    fill(b, 0x11223344u);
    a[7] = 0xFFFFFFFFu;
    sb::require(mc::memcmp<u32>(a, b, 7) == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: mc::rmemcmp<T,F>
  // ============================================================

  sb::test_case("rmemcmp<byte,byte>: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x5A));
    fill(b, static_cast<byte>(0x5A));
    sb::require(mc::rmemcmp<byte>(*a, *b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("rmemcmp<byte,byte>: last byte differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x5A));
    fill(b, static_cast<byte>(0x5A));
    a[15] = 0xFF;
    sb::require(mc::rmemcmp<byte>(*a, *b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("rmemcmp<byte,byte>: off-by-one — difference past count ignored");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x77));
    fill(b, static_cast<byte>(0x77));
    a[15] = 0x00;
    sb::require(mc::rmemcmp<byte>(*a, *b, 15) == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: mc::constexpr_memcmp<F>
  // ============================================================

  sb::test_case("constexpr_memcmp: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x3C));
    fill(b, static_cast<byte>(0x3C));
    sb::require(mc::constexpr_memcmp(a, b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_memcmp: first byte differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[0] = 0x01;
    sb::require(mc::constexpr_memcmp(a, b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("constexpr_memcmp: difference outside window ignored");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xFF));
    fill(b, static_cast<byte>(0xFF));
    a[15] = 0x00;
    sb::require(mc::constexpr_memcmp(a, b, 15) == 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: mc::cmemcmp<M,T,F> — compile-time count
  // ============================================================

  sb::test_case("cmemcmp<32,byte,byte>: equal (divisible-by-4 fast path)");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x88));
    fill(b, static_cast<byte>(0x88));
    sb::require(mc::cmemcmp<32, byte>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<32,byte,byte>: first element differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x88));
    fill(b, static_cast<byte>(0x88));
    a[0] = 0x99;
    sb::require(mc::cmemcmp<32, byte>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<32,byte,byte>: last element differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x88));
    fill(b, static_cast<byte>(0x88));
    a[31] = 0x00;
    sb::require(mc::cmemcmp<32, byte>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<7,byte,byte>: equal (non-multiple-of-4 slow path)");
  {
    byte a[7], b[7];
    fill(a, static_cast<byte>(0xA5));
    fill(b, static_cast<byte>(0xA5));
    sb::require(mc::cmemcmp<7, byte>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<7,byte,byte>: last element (pos 6) differs");
  {
    byte a[7], b[7];
    fill(a, static_cast<byte>(0xA5));
    fill(b, static_cast<byte>(0xA5));
    a[6] = 0x00;
    sb::require(mc::cmemcmp<7, byte>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<1,byte,byte>: single element equal");
  {
    byte a[1] = { 0x42 };
    byte b[1] = { 0x42 };
    sb::require(mc::cmemcmp<1, byte>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cmemcmp<1,byte,byte>: single element differs");
  {
    byte a[1] = { 0x01 };
    byte b[1] = { 0x02 };
    sb::require(mc::cmemcmp<1, byte>(a, b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: mc::memcmp_Nb — unrolled fixed-size variants
  // ============================================================

  sb::test_case("memcmp_8b: equal 8 bytes returns 0");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0x11));
    fill(b, static_cast<byte>(0x11));
    sb::require(mc::memcmp_8b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_8b: first byte differs");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[0] = 0xFF;
    sb::require(mc::memcmp_8b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_8b: last byte differs");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[7] = 0xFF;
    sb::require(mc::memcmp_8b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_16b: equal 16 bytes returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x22));
    fill(b, static_cast<byte>(0x22));
    sb::require(mc::memcmp_16b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_16b: byte 8 differs (second u64 word)");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x22));
    fill(b, static_cast<byte>(0x22));
    a[8] = 0xFF;
    sb::require(mc::memcmp_16b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_16b: last byte differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x22));
    fill(b, static_cast<byte>(0x22));
    a[15] = 0x00;
    sb::require(mc::memcmp_16b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_32b: equal 32 bytes returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x33));
    fill(b, static_cast<byte>(0x33));
    sb::require(mc::memcmp_32b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_32b: first byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x33));
    fill(b, static_cast<byte>(0x33));
    a[0] = 0x00;
    sb::require(mc::memcmp_32b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_32b: last byte (pos 31) differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x33));
    fill(b, static_cast<byte>(0x33));
    a[31] = 0xEE;
    sb::require(mc::memcmp_32b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_64b: equal 64 bytes returns 0");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x44));
    fill(b, static_cast<byte>(0x44));
    sb::require(mc::memcmp_64b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_64b: byte 32 differs (5th u64 word)");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x44));
    fill(b, static_cast<byte>(0x44));
    a[32] = 0x00;
    sb::require(mc::memcmp_64b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("memcmp_64b: last byte (pos 63) differs");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x44));
    fill(b, static_cast<byte>(0x44));
    a[63] = 0xFF;
    sb::require(mc::memcmp_64b(a, b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: mc::smemcmp — safe variant
  // ============================================================

  sb::test_case("smemcmp<byte,byte>: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xBB));
    fill(b, static_cast<byte>(0xBB));
    sb::require(mc::smemcmp<byte>(a, b, 32) == 0);
  }
  sb::end_test_case();

  sb::test_case("smemcmp<byte,byte>: unequal returns non-zero");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xBB));
    fill(b, static_cast<byte>(0xBB));
    a[10] = 0x00;
    sb::require(mc::smemcmp<byte>(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("smemcmp<byte,byte>: nullptr src returns i64::min");
  {
    byte b[16];
    fill(b, static_cast<byte>(0x11));
    i64 result = mc::smemcmp<byte, byte>(static_cast<byte *>(nullptr), b, 16);
    sb::require(result == ERR);
  }
  sb::end_test_case();

  sb::test_case("smemcmp<byte,byte>: nullptr dest returns i64::min");
  {
    byte a[16];
    fill(a, static_cast<byte>(0x11));
    i64 result = mc::smemcmp<byte, byte>(a, static_cast<byte *>(nullptr), 16);
    sb::require(result == ERR);
  }
  sb::end_test_case();

  sb::test_case("smemcmp<byte,byte>: misaligned src returns i64::min");
  {
    alignas(16) byte storage_a[32] = {};
    alignas(16) byte storage_b[32] = {};
    fill(storage_a, static_cast<byte>(0x55));
    fill(storage_b, static_cast<byte>(0x55));
    byte *misaligned = storage_a + 1;
    i64 result = mc::smemcmp<byte, byte, 16>(misaligned, storage_b, 15);
    sb::require(result == ERR);
  }
  sb::end_test_case();

  sb::test_case("smemcmp<byte,byte>: off-by-one — difference past count ignored");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x66));
    fill(b, static_cast<byte>(0x66));
    a[31] = 0xFF;
    sb::require(mc::smemcmp<byte>(a, b, 31) == 0);
  }
  sb::end_test_case();

  sb::test_case("scmemcmp_safe<32,byte,byte>: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x9F));
    fill(b, static_cast<byte>(0x9F));
    sb::require(mc::scmemcmp_safe<32, byte>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("scmemcmp_safe<32,byte,byte>: last byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x9F));
    fill(b, static_cast<byte>(0x9F));
    a[31] = 0x00;
    sb::require(mc::scmemcmp_safe<32, byte>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("scmemcmp_safe<16,byte,byte>: nullptr returns i64::min");
  {
    byte b[16];
    fill(b, static_cast<byte>(0x33));
    i64 result = mc::scmemcmp_safe<16, byte>(static_cast<byte *>(nullptr), b);
    sb::require(result == ERR);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: mc::bytecmp / mc::bcmp
  // ============================================================

  sb::test_case("bytecmp: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xAA));
    fill(b, static_cast<byte>(0xAA));
    sb::require(mc::bytecmp(a, b, 32) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp: first byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[0] = 0x01;
    sb::require(mc::bytecmp(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp: last byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x55));
    fill(b, static_cast<byte>(0x55));
    a[31] = 0x00;
    sb::require(mc::bytecmp(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp: difference outside window returns 0 (off-by-one)");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x77));
    fill(b, static_cast<byte>(0x77));
    a[31] = 0xFF;
    sb::require(mc::bytecmp(a, b, 31) == 0);     // pos 31 outside window
  }
  sb::end_test_case();

  sb::test_case("bytecmp: count = 1 equal");
  {
    byte a[4] = { 0xAB, 0xFF, 0xFF, 0xFF };
    byte b[4] = { 0xAB, 0x00, 0x00, 0x00 };
    sb::require(mc::bytecmp(a, b, 1) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp: count = 1 unequal");
  {
    byte a[1] = { 0x01 };
    byte b[1] = { 0x02 };
    sb::require(mc::bytecmp(a, b, 1) != 0);
  }
  sb::end_test_case();

  sb::test_case("bcmp: is consistent alias of bytecmp");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xF0));
    fill(b, static_cast<byte>(0xF0));
    sb::require(mc::bcmp(a, b, 16) == mc::bytecmp(a, b, 16));
    a[8] = 0x0F;
    sb::require(mc::bcmp(a, b, 16) == mc::bytecmp(a, b, 16));
  }
  sb::end_test_case();

  sb::test_case("rbytecmp: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xC3));
    fill(b, static_cast<byte>(0xC3));
    sb::require(mc::rbytecmp(*a, *b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("rbytecmp: last byte differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xC3));
    fill(b, static_cast<byte>(0xC3));
    a[15] = 0x00;
    sb::require(mc::rbytecmp(*a, *b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("rbytecmp: off-by-one — difference at position n is ignored");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xC3));
    fill(b, static_cast<byte>(0xC3));
    a[15] = 0xFF;
    sb::require(mc::rbytecmp(*a, *b, 15) == 0);
  }
  sb::end_test_case();

  sb::test_case("rbcmp: alias consistent with rbytecmp");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x1A));
    fill(b, static_cast<byte>(0x1A));
    sb::require(mc::rbcmp(*a, *b, 16) == mc::rbytecmp(*a, *b, 16));
    a[7] = 0xFF;
    sb::require(mc::rbcmp(*a, *b, 16) == mc::rbytecmp(*a, *b, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: cbytecmp<N> / rcbytecmp<N>
  // ============================================================

  sb::test_case("cbytecmp<32>: equal (fast path, divisible by 4)");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xE1));
    fill(b, static_cast<byte>(0xE1));
    sb::require(mc::cbytecmp<32>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cbytecmp<32>: first byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xE1));
    fill(b, static_cast<byte>(0xE1));
    a[0] = 0x00;
    sb::require(mc::cbytecmp<32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cbytecmp<32>: last byte (pos 31) differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xE1));
    fill(b, static_cast<byte>(0xE1));
    a[31] = 0xFF;
    sb::require(mc::cbytecmp<32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cbytecmp<7>: equal (slow path, not divisible by 4)");
  {
    byte a[7], b[7];
    fill(a, static_cast<byte>(0xD2));
    fill(b, static_cast<byte>(0xD2));
    sb::require(mc::cbytecmp<7>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cbytecmp<7>: last element (pos 6) differs");
  {
    byte a[7], b[7];
    fill(a, static_cast<byte>(0xD2));
    fill(b, static_cast<byte>(0xD2));
    a[6] = 0x00;
    sb::require(mc::cbytecmp<7>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cbcmp<16>: alias consistent with cbytecmp<16>");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x4C));
    fill(b, static_cast<byte>(0x4C));
    sb::require(mc::cbcmp<16>(a, b) == mc::cbytecmp<16>(a, b));
    a[8] = 0xFF;
    sb::require(mc::cbcmp<16>(a, b) == mc::cbytecmp<16>(a, b));
  }
  sb::end_test_case();

  sb::test_case("rcbytecmp<16>: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x7B));
    fill(b, static_cast<byte>(0x7B));
    sb::require(mc::rcbytecmp<16>(*a, *b) == 0);
  }
  sb::end_test_case();

  sb::test_case("rcbytecmp<16>: last byte differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x7B));
    fill(b, static_cast<byte>(0x7B));
    a[15] = 0x00;
    sb::require(mc::rcbytecmp<16>(*a, *b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: bytecmp_Nb unrolled fixed-size
  // ============================================================

  sb::test_case("bytecmp_8b: equal returns 0");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0xF1));
    fill(b, static_cast<byte>(0xF1));
    sb::require(mc::bytecmp_8b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_8b: byte 0 differs");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[0] = 0xFF;
    sb::require(mc::bytecmp_8b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_8b: byte 7 (last) differs");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0x00));
    fill(b, static_cast<byte>(0x00));
    a[7] = 0xFF;
    sb::require(mc::bytecmp_8b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bcmp_8b: consistent alias");
  {
    byte a[8], b[8];
    fill(a, static_cast<byte>(0xC1));
    fill(b, static_cast<byte>(0xC1));
    sb::require(mc::bcmp_8b(a, b) == mc::bytecmp_8b(a, b));
    a[3] = 0x00;
    sb::require(mc::bcmp_8b(a, b) == mc::bytecmp_8b(a, b));
  }
  sb::end_test_case();

  sb::test_case("bytecmp_16b: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xA3));
    fill(b, static_cast<byte>(0xA3));
    sb::require(mc::bytecmp_16b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_16b: byte 15 (last) differs");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xA3));
    fill(b, static_cast<byte>(0xA3));
    a[15] = 0x00;
    sb::require(mc::bytecmp_16b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_16b: byte 8 differs (boundary of second u64)");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xA3));
    fill(b, static_cast<byte>(0xA3));
    a[8] = 0x00;
    sb::require(mc::bytecmp_16b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_32b: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x6E));
    fill(b, static_cast<byte>(0x6E));
    sb::require(mc::bytecmp_32b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_32b: byte 31 (last) differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x6E));
    fill(b, static_cast<byte>(0x6E));
    a[31] = 0x00;
    sb::require(mc::bytecmp_32b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_64b: equal returns 0");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x2D));
    fill(b, static_cast<byte>(0x2D));
    sb::require(mc::bytecmp_64b(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_64b: byte 63 (last) differs");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x2D));
    fill(b, static_cast<byte>(0x2D));
    a[63] = 0xFF;
    sb::require(mc::bytecmp_64b(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("bytecmp_64b: byte 56 (boundary of 8th u64) differs");
  {
    byte a[64], b[64];
    fill(a, static_cast<byte>(0x2D));
    fill(b, static_cast<byte>(0x2D));
    a[56] = 0x00;
    sb::require(mc::bytecmp_64b(a, b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 10: sbytecmp / rsbytecmp — safe byte variants
  // ============================================================

  sb::test_case("sbytecmp: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xD4));
    fill(b, static_cast<byte>(0xD4));
    sb::require(mc::sbytecmp(a, b, 32) == 0);
  }
  sb::end_test_case();

  sb::test_case("sbytecmp: unequal returns non-zero");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xD4));
    fill(b, static_cast<byte>(0xD4));
    a[16] = 0x00;
    sb::require(mc::sbytecmp(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("sbytecmp: nullptr src returns i64::min");
  {
    byte b[16];
    fill(b, static_cast<byte>(0x11));
    sb::require(mc::sbytecmp(static_cast<byte *>(nullptr), b, 16) == ERR);
  }
  sb::end_test_case();

  sb::test_case("sbytecmp: nullptr dest returns i64::min");
  {
    byte a[16];
    fill(a, static_cast<byte>(0x11));
    sb::require(mc::sbytecmp(a, static_cast<byte *>(nullptr), 16) == ERR);
  }
  sb::end_test_case();

  sb::test_case("sbytecmp: misaligned src returns i64::min");
  {
    alignas(16) byte storage_a[32] = {};
    alignas(16) byte storage_b[32] = {};
    fill(storage_a, static_cast<byte>(0xAA));
    fill(storage_b, static_cast<byte>(0xAA));
    sb::require(mc::sbytecmp<16>(storage_a + 1, storage_b, 15) == ERR);
  }
  sb::end_test_case();

  sb::test_case("sbytecmp: off-by-one — difference at n ignored");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0x88));
    fill(b, static_cast<byte>(0x88));
    a[31] = 0x00;
    sb::require(mc::sbytecmp(a, b, 31) == 0);
  }
  sb::end_test_case();

  sb::test_case("sbcmp: consistent alias of sbytecmp");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x3B));
    fill(b, static_cast<byte>(0x3B));
    sb::require(mc::sbcmp(a, b, 16) == mc::sbytecmp(a, b, 16));
    a[8] = 0xFF;
    sb::require(mc::sbcmp(a, b, 16) == mc::sbytecmp(a, b, 16));
  }
  sb::end_test_case();

  sb::test_case("rsbytecmp: equal returns 0");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xE7));
    fill(b, static_cast<byte>(0xE7));
    sb::require(mc::rsbytecmp(*a, *b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("rsbytecmp: unequal returns non-zero");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0xE7));
    fill(b, static_cast<byte>(0xE7));
    a[8] = 0x00;
    sb::require(mc::rsbytecmp(*a, *b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("scbytecmp_safe<32>: equal returns 0");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xB5));
    fill(b, static_cast<byte>(0xB5));
    sb::require(mc::scbytecmp_safe<32>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("scbytecmp_safe<32>: last byte differs");
  {
    byte a[32], b[32];
    fill(a, static_cast<byte>(0xB5));
    fill(b, static_cast<byte>(0xB5));
    a[31] = 0x00;
    sb::require(mc::scbytecmp_safe<32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("scbytecmp_safe<16>: nullptr returns i64::min");
  {
    byte b[16];
    fill(b, static_cast<byte>(0x44));
    sb::require(mc::scbytecmp_safe<16>(static_cast<byte *>(nullptr), b) == ERR);
  }
  sb::end_test_case();

  sb::test_case("scbcmp_safe<16>: consistent alias");
  {
    byte a[16], b[16];
    fill(a, static_cast<byte>(0x2F));
    fill(b, static_cast<byte>(0x2F));
    sb::require(mc::scbcmp_safe<16>(a, b) == mc::scbytecmp_safe<16>(a, b));
    a[7] = 0xFF;
    sb::require(mc::scbcmp_safe<16>(a, b) == mc::scbytecmp_safe<16>(a, b));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 11: typecmp / rtypecmp / ctypecmp
  // ============================================================

  sb::test_case("typecmp<u32,u32>: equal returns 0");
  {
    u32 a[8], b[8];
    fill(a, 0xABCD1234u);
    fill(b, 0xABCD1234u);
    sb::require(mc::typecmp<u32>(a, b, 8) == 0);
  }
  sb::end_test_case();

  sb::test_case("typecmp<u32,u32>: first element differs");
  {
    u32 a[8], b[8];
    fill(a, 0xABCD1234u);
    fill(b, 0xABCD1234u);
    a[0] = 0x00000000u;
    sb::require(mc::typecmp<u32>(a, b, 8) != 0);
  }
  sb::end_test_case();

  sb::test_case("typecmp<u32,u32>: last element differs");
  {
    u32 a[8], b[8];
    fill(a, 0xABCD1234u);
    fill(b, 0xABCD1234u);
    a[7] = 0xFFFFFFFFu;
    sb::require(mc::typecmp<u32>(a, b, 8) != 0);
  }
  sb::end_test_case();

  sb::test_case("typecmp<u32,u32>: difference past window ignored (off-by-one)");
  {
    u32 a[8], b[8];
    fill(a, 0x12345678u);
    fill(b, 0x12345678u);
    a[7] = 0x00000000u;
    sb::require(mc::typecmp<u32>(a, b, 7) == 0);
  }
  sb::end_test_case();

  sb::test_case("rtypecmp<u32,u32>: equal returns 0");
  {
    u32 a[8], b[8];
    fill(a, 0xFEDCBA98u);
    fill(b, 0xFEDCBA98u);
    sb::require(mc::rtypecmp<u32>(*a, *b, 8) == 0);
  }
  sb::end_test_case();

  sb::test_case("rtypecmp<u32,u32>: middle element differs");
  {
    u32 a[8], b[8];
    fill(a, 0xFEDCBA98u);
    fill(b, 0xFEDCBA98u);
    a[4] = 0x00000000u;
    sb::require(mc::rtypecmp<u32>(*a, *b, 8) != 0);
  }
  sb::end_test_case();

  sb::test_case("ctypecmp<8,u32,u32>: equal (divisible by 4, fast path)");
  {
    u32 a[8], b[8];
    fill(a, 0x11223344u);
    fill(b, 0x11223344u);
    sb::require(mc::ctypecmp<8, u32>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("ctypecmp<8,u32,u32>: last element differs");
  {
    u32 a[8], b[8];
    fill(a, 0x11223344u);
    fill(b, 0x11223344u);
    a[7] = 0x00000000u;
    sb::require(mc::ctypecmp<8, u32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("ctypecmp<5,u32,u32>: equal (non-multiple-of-4 slow path)");
  {
    u32 a[5], b[5];
    fill(a, 0x55667788u);
    fill(b, 0x55667788u);
    sb::require(mc::ctypecmp<5, u32>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("ctypecmp<5,u32,u32>: last element (pos 4) differs");
  {
    u32 a[5], b[5];
    fill(a, 0x55667788u);
    fill(b, 0x55667788u);
    a[4] = 0x00000000u;
    sb::require(mc::ctypecmp<5, u32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("rctypecmp<8,u32,u32>: equal returns 0");
  {
    u32 a[8], b[8];
    fill(a, 0xAABBCCDDu);
    fill(b, 0xAABBCCDDu);
    sb::require(mc::rctypecmp<8, u32>(*a, *b) == 0);
  }
  sb::end_test_case();

  sb::test_case("stypecmp<u32,u32>: equal returns 0");
  {
    u32 a[8], b[8];
    fill(a, 0x99887766u);
    fill(b, 0x99887766u);
    sb::require(mc::stypecmp<u32>(a, b, 8) == 0);
  }
  sb::end_test_case();

  sb::test_case("stypecmp<u32,u32>: nullptr src returns i64::min");
  {
    u32 b[8];
    fill(b, 0x12345678u);
    sb::require(mc::stypecmp<u32, u32>(static_cast<u32 *>(nullptr), b, 8) == ERR);
  }
  sb::end_test_case();

  sb::test_case("stypecmp<u32,u32>: nullptr dest returns i64::min");
  {
    u32 a[8];
    fill(a, 0x12345678u);
    sb::require(mc::stypecmp<u32, u32>(a, static_cast<u32 *>(nullptr), 8) == ERR);
  }
  sb::end_test_case();

  sb::test_case("sctypecmp_safe<8,u32,u32>: equal returns 0");
  {
    u32 a[8], b[8];
    fill(a, 0xFACEB00Cu);
    fill(b, 0xFACEB00Cu);
    sb::require(mc::sctypecmp_safe<8, u32>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("sctypecmp_safe<8,u32,u32>: last element differs");
  {
    u32 a[8], b[8];
    fill(a, 0xFACEB00Cu);
    fill(b, 0xFACEB00Cu);
    a[7] = 0x00000000u;
    sb::require(mc::sctypecmp_safe<8, u32>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("sctypecmp_safe<8,u32,u32>: nullptr returns i64::min");
  {
    u32 b[8];
    fill(b, 0xDEAD0000u);
    sb::require(mc::sctypecmp_safe<8, u32>(static_cast<u32 *>(nullptr), b) == ERR);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 12: wordcmp / rwordcmp / cwordcmp / rcwordcmp
  // ============================================================

  sb::test_case("wordcmp: equal returns 0");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0xAB));
    fill(b, static_cast<word>(0xAB));
    sb::require(mc::wordcmp(a, b, 32) == 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp: first word differs");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0x00));
    fill(b, static_cast<word>(0x00));
    a[0] = 0xFF;
    sb::require(mc::wordcmp(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp: last word differs");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0x55));
    fill(b, static_cast<word>(0x55));
    a[31] = 0x00;
    sb::require(mc::wordcmp(a, b, 32) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp: off-by-one — difference at position n ignored");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0xCC));
    fill(b, static_cast<word>(0xCC));
    a[15] = 0x00;
    sb::require(mc::wordcmp(a, b, 15) == 0);
  }
  sb::end_test_case();

  sb::test_case("rwordcmp: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x7E));
    fill(b, static_cast<word>(0x7E));
    sb::require(mc::rwordcmp(*a, *b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("rwordcmp: middle word differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x7E));
    fill(b, static_cast<word>(0x7E));
    a[8] = 0x00;
    sb::require(mc::rwordcmp(*a, *b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("cwordcmp<16>: equal (fast path, divisible by 4)");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x3D));
    fill(b, static_cast<word>(0x3D));
    sb::require(mc::cwordcmp<16>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cwordcmp<16>: first word differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x3D));
    fill(b, static_cast<word>(0x3D));
    a[0] = 0xFF;
    sb::require(mc::cwordcmp<16>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cwordcmp<16>: last word differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x3D));
    fill(b, static_cast<word>(0x3D));
    a[15] = 0x00;
    sb::require(mc::cwordcmp<16>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("cwordcmp<5>: equal (slow path, not divisible by 4)");
  {
    word a[5], b[5];
    fill(a, static_cast<word>(0x91));
    fill(b, static_cast<word>(0x91));
    sb::require(mc::cwordcmp<5>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("cwordcmp<5>: last word (pos 4) differs");
  {
    word a[5], b[5];
    fill(a, static_cast<word>(0x91));
    fill(b, static_cast<word>(0x91));
    a[4] = 0x00;
    sb::require(mc::cwordcmp<5>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("rcwordcmp<16>: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x4A));
    fill(b, static_cast<word>(0x4A));
    sb::require(mc::rcwordcmp<16>(*a, *b) == 0);
  }
  sb::end_test_case();

  sb::test_case("rcwordcmp<16>: last word differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x4A));
    fill(b, static_cast<word>(0x4A));
    a[15] = 0xFF;
    sb::require(mc::rcwordcmp<16>(*a, *b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 13: wordcmp_Nw unrolled fixed-size
  // ============================================================

  sb::test_case("wordcmp_4w: equal returns 0");
  {
    word a[4], b[4];
    fill(a, static_cast<word>(0x11));
    fill(b, static_cast<word>(0x11));
    sb::require(mc::wordcmp_4w(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_4w: word 0 differs");
  {
    word a[4], b[4];
    fill(a, static_cast<word>(0x11));
    fill(b, static_cast<word>(0x11));
    a[0] = 0xFF;
    sb::require(mc::wordcmp_4w(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_4w: word 3 (last) differs");
  {
    word a[4], b[4];
    fill(a, static_cast<word>(0x11));
    fill(b, static_cast<word>(0x11));
    a[3] = 0x00;
    sb::require(mc::wordcmp_4w(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_8w: equal returns 0");
  {
    word a[8], b[8];
    fill(a, static_cast<word>(0x22));
    fill(b, static_cast<word>(0x22));
    sb::require(mc::wordcmp_8w(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_8w: word 7 (last) differs");
  {
    word a[8], b[8];
    fill(a, static_cast<word>(0x22));
    fill(b, static_cast<word>(0x22));
    a[7] = 0xFF;
    sb::require(mc::wordcmp_8w(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_16w: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x33));
    fill(b, static_cast<word>(0x33));
    sb::require(mc::wordcmp_16w(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_16w: word 15 (last) differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x33));
    fill(b, static_cast<word>(0x33));
    a[15] = 0x00;
    sb::require(mc::wordcmp_16w(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_32w: equal returns 0");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0x44));
    fill(b, static_cast<word>(0x44));
    sb::require(mc::wordcmp_32w(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_32w: word 31 (last) differs");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0x44));
    fill(b, static_cast<word>(0x44));
    a[31] = 0xFF;
    sb::require(mc::wordcmp_32w(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("wordcmp_32w: word 0 differs");
  {
    word a[32], b[32];
    fill(a, static_cast<word>(0x44));
    fill(b, static_cast<word>(0x44));
    a[0] = 0x00;
    sb::require(mc::wordcmp_32w(a, b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 14: swordcmp / rswordcmp / scwordcmp_safe — safe word variants
  // ============================================================

  sb::test_case("swordcmp: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0xD1));
    fill(b, static_cast<word>(0xD1));
    sb::require(mc::swordcmp(a, b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("swordcmp: unequal returns non-zero");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0xD1));
    fill(b, static_cast<word>(0xD1));
    a[8] = 0x00;
    sb::require(mc::swordcmp(a, b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("swordcmp: nullptr src returns i64::min");
  {
    word b[8];
    fill(b, static_cast<word>(0x22));
    sb::require(mc::swordcmp(static_cast<word *>(nullptr), b, 8) == ERR);
  }
  sb::end_test_case();

  sb::test_case("swordcmp: nullptr dest returns i64::min");
  {
    word a[8];
    fill(a, static_cast<word>(0x22));
    sb::require(mc::swordcmp(a, static_cast<word *>(nullptr), 8) == ERR);
  }
  sb::end_test_case();

  sb::test_case("swordcmp: misaligned pointer returns i64::min");
  {
    alignas(alignof(word)) byte storage_a[64] = {};
    alignas(alignof(word)) byte storage_b[64] = {};
    word *misaligned = reinterpret_cast<word *>(storage_a + 1);
    word *aligned_b = reinterpret_cast<word *>(storage_b);
    // alignment > 1 for word forces the check to fail on +1 offset
    sb::require(mc::swordcmp<alignof(word) * 2>(misaligned, aligned_b, 4) == ERR);
  }
  sb::end_test_case();

  sb::test_case("swordcmp: off-by-one — difference at position n ignored");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0xBB));
    fill(b, static_cast<word>(0xBB));
    a[15] = 0x00;
    sb::require(mc::swordcmp(a, b, 15) == 0);
  }
  sb::end_test_case();

  sb::test_case("rswordcmp: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x8C));
    fill(b, static_cast<word>(0x8C));
    sb::require(mc::rswordcmp(*a, *b, 16) == 0);
  }
  sb::end_test_case();

  sb::test_case("rswordcmp: unequal returns non-zero");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x8C));
    fill(b, static_cast<word>(0x8C));
    a[0] = 0xFF;
    sb::require(mc::rswordcmp(*a, *b, 16) != 0);
  }
  sb::end_test_case();

  sb::test_case("scwordcmp_safe<16>: equal returns 0");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x5F));
    fill(b, static_cast<word>(0x5F));
    sb::require(mc::scwordcmp_safe<16>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("scwordcmp_safe<16>: last word differs");
  {
    word a[16], b[16];
    fill(a, static_cast<word>(0x5F));
    fill(b, static_cast<word>(0x5F));
    a[15] = 0x00;
    sb::require(mc::scwordcmp_safe<16>(a, b) != 0);
  }
  sb::end_test_case();

  sb::test_case("scwordcmp_safe<16>: nullptr src returns i64::min");
  {
    word b[16];
    fill(b, static_cast<word>(0x33));
    sb::require(mc::scwordcmp_safe<16>(static_cast<word *>(nullptr), b) == ERR);
  }
  sb::end_test_case();

  sb::test_case("scwordcmp_safe<5>: non-multiple-of-4, equal");
  {
    word a[5], b[5];
    fill(a, static_cast<word>(0xF3));
    fill(b, static_cast<word>(0xF3));
    sb::require(mc::scwordcmp_safe<5>(a, b) == 0);
  }
  sb::end_test_case();

  sb::test_case("scwordcmp_safe<5>: last word (pos 4) differs");
  {
    word a[5], b[5];
    fill(a, static_cast<word>(0xF3));
    fill(b, static_cast<word>(0xF3));
    a[4] = 0x00;
    sb::require(mc::scwordcmp_safe<5>(a, b) != 0);
  }
  sb::end_test_case();

  // ============================================================
  //  Section 15: Cross-check — all-same vs single-differ stress
  // ============================================================

  sb::test_case("Stress: every position in a 64-byte buffer detected individually");
  {
    byte a[64], b[64];
    for ( u64 pos = 0; pos < 64; pos++ ) {
      fill(a, static_cast<byte>(0xFF));
      fill(b, static_cast<byte>(0xFF));
      a[pos] = 0x00;
      // Should detect at position pos
      sb::require(mc::bytecmp(a, b, 64) != 0);
      // Count ending just before pos: should return 0
      if ( pos > 0 )
        sb::require(mc::bytecmp(a, b, pos) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("Stress: 100 equal-buffer comparisons return 0 consistently");
  {
    byte a[64], b[64];
    for ( int i = 0; i < 100; i++ ) {
      byte val = static_cast<byte>(i);
      fill(a, val);
      fill(b, val);
      sb::require(mc::bytecmp(a, b, 64) == 0);
      sb::require(mc::memcmp<byte>(a, b, 64) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("Stress: compare against self always returns 0");
  {
    byte buf[64];
    for ( u64 i = 0; i < 64; i++ )
      buf[i] = static_cast<byte>(i);
    sb::require(mc::bytecmp(buf, buf, 64) == 0);
    sb::require(mc::memcmp<byte>(buf, buf, 64) == 0);
    sb::require(mc::wordcmp(reinterpret_cast<word *>(buf), reinterpret_cast<word *>(buf), 64 / sizeof(word)) == 0);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

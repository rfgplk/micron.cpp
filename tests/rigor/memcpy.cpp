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

// ============================================================
//  Helper utilities
// ============================================================

// Canary value written into guard zones to detect out-of-bounds writes
static constexpr byte CANARY = 0xC3;

// Fill a raw buffer with a repeating pattern value
template <typename T, u64 N>
void
fill_pattern(T (&buf)[N], T val)
{
  for ( u64 i = 0; i < N; i++ )
    buf[i] = val;
}

template <typename T>
void
fill_pattern(T *buf, u64 n, T val)
{
  for ( u64 i = 0; i < n; i++ )
    buf[i] = val;
}

// Verify every element of a fixed-size array equals expected
template <typename T, u64 N>
bool
verify_buffer(T (&buf)[N], T expected)
{
  for ( u64 i = 0; i < N; i++ )
    if ( buf[i] != expected )
      return false;
  return true;
}

template <typename T>
bool
verify_buffer(T *buf, u64 n, T expected)
{
  for ( u64 i = 0; i < n; i++ )
    if ( buf[i] != expected )
      return false;
  return true;
}

// Verify a buffer matches a source array element-for-element
template <typename D, typename S>
bool
buffers_equal(const D *dest, const S *src, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    if ( static_cast<D>(src[i]) != dest[i] )
      return false;
  return true;
}

// Verify every byte of a typed buffer equals zero
template <typename T, u64 N>
bool
is_zeroed(T (&buf)[N])
{
  const byte *p = reinterpret_cast<const byte *>(buf);
  for ( u64 i = 0; i < N * sizeof(T); i++ )
    if ( p[i] != 0 )
      return false;
  return true;
}

// ============================================================
//  Guard-zone helpers
//
//  Layout:  [GUARD_SZ canary bytes][payload of n bytes][GUARD_SZ canary bytes]
//
//  This lets us detect writes that escape the intended region in either
//  direction (classic off-by-one / off-by-N).
// ============================================================

static constexpr u64 GUARD_SZ = 16;

struct GuardedBuffer {
  byte data[GUARD_SZ + 512 + GUARD_SZ];

  GuardedBuffer()
  {
    for ( u64 i = 0; i < sizeof(data); i++ )
      data[i] = CANARY;
  }

  byte *
  payload()
  {
    return data + GUARD_SZ;
  }

  const byte *
  payload() const
  {
    return data + GUARD_SZ;
  }

  // Returns true if no canary byte was overwritten on either side
  bool
  guards_intact() const
  {
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( data[i] != CANARY )
        return false;
    for ( u64 i = GUARD_SZ + 512; i < sizeof(data); i++ )
      if ( data[i] != CANARY )
        return false;
    return true;
  }
};

// Verify that exactly `n` bytes of `payload` were written and that the
// bytes immediately beyond the region are still intact.
bool
no_overrun(const GuardedBuffer &gb, u64 written_bytes)
{
  // Trailing guard (relative to payload start)
  const byte *after = gb.payload() + written_bytes;
  for ( u64 i = 0; i < GUARD_SZ; i++ )
    if ( after[i] != CANARY )
      return false;
  // Leading guard
  for ( u64 i = 0; i < GUARD_SZ; i++ )
    if ( gb.data[i] != CANARY )
      return false;
  return true;
}

// ============================================================
//  Incrementing-sequence source used for artifacting tests
// ============================================================

template <typename T, u64 N>
void
make_seq(T (&buf)[N])
{
  for ( u64 i = 0; i < N; i++ )
    buf[i] = static_cast<T>(i & 0xFF);
}

template <typename T>
void
make_seq(T *buf, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    buf[i] = static_cast<T>(i & 0xFF);
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== MEMCPY TESTS ===");

  // ----------------------------------------------------------
  //  mc::memcpy — basic runtime copy
  // ----------------------------------------------------------

  sb::test_case("Basic memcpy - byte buffer");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xAB));
    mc::memcpy(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("Basic memcpy - u32 buffer");
  {
    u32 src[16];
    u32 dst[16] = {};
    fill_pattern(src, 0xDEADBEEFu);
    mc::memcpy(dst, src, 16);
    sb::require(verify_buffer(dst, 0xDEADBEEFu));
  }
  sb::end_test_case();

  sb::test_case("Basic memcpy - u64 buffer");
  {
    u64 src[8];
    u64 dst[8] = {};
    fill_pattern(src, static_cast<u64>(0xCAFEBABEDEADC0DEull));
    mc::memcpy(dst, src, 8);
    sb::require(verify_buffer(dst, static_cast<u64>(0xCAFEBABEDEADC0DEull)));
  }
  sb::end_test_case();

  sb::test_case("memcpy - count divisible by 4 (fast path)");
  {
    byte src[64];
    byte dst[64] = {};
    fill_pattern(src, static_cast<byte>(0x5A));
    mc::memcpy(dst, src, 64);     // 64 % 4 == 0
    sb::require(verify_buffer(dst, static_cast<byte>(0x5A)));
  }
  sb::end_test_case();

  sb::test_case("memcpy - count NOT divisible by 4 (slow path)");
  {
    byte src[13];
    byte dst[13] = {};
    fill_pattern(src, static_cast<byte>(0x7F));
    mc::memcpy(dst, src, 13);
    sb::require(verify_buffer(dst, static_cast<byte>(0x7F)));
  }
  sb::end_test_case();

  sb::test_case("memcpy - count = 1");
  {
    byte src[1] = { 0xBB };
    byte dst[1] = { 0x00 };
    mc::memcpy(dst, src, 1);
    sb::require(dst[0] == static_cast<byte>(0xBB));
  }
  sb::end_test_case();

  sb::test_case("memcpy - count = 3");
  {
    byte src[3] = { 0x01, 0x02, 0x03 };
    byte dst[3] = { 0x00, 0x00, 0x00 };
    mc::memcpy(dst, src, 3);
    sb::require(dst[0] == 0x01 && dst[1] == 0x02 && dst[2] == 0x03);
  }
  sb::end_test_case();

  sb::test_case("memcpy - count = 5 (straddles boundary)");
  {
    byte src[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    byte dst[5] = {};
    mc::memcpy(dst, src, 5);
    sb::require(buffers_equal(dst, src, 5));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Off-by-one: exact element boundaries
  // ----------------------------------------------------------

  sb::test_case("Off-by-one: only first element written, not second");
  {
    byte src[2] = { 0xFF, 0xFF };
    byte dst[2] = { 0x00, 0x00 };
    mc::memcpy(dst, src, 1);     // copy exactly 1
    sb::require(dst[0] == 0xFF);
    sb::require(dst[1] == 0x00);     // must be untouched
  }
  sb::end_test_case();

  sb::test_case("Off-by-one: last element written, nothing past it");
  {
    byte src[8];
    byte dst[8] = {};
    fill_pattern(src, static_cast<byte>(0x99));
    mc::memcpy(dst, src, 7);     // 7, not 8
    sb::require(verify_buffer(dst, 7, static_cast<byte>(0x99)));
    sb::require(dst[7] == 0x00);     // 8th byte untouched
  }
  sb::end_test_case();

  sb::test_case("Off-by-one: guard zone — no underrun or overrun (memcpy)");
  {
    GuardedBuffer gb_src, gb_dst;
    fill_pattern(gb_src.payload(), static_cast<u64>(32), static_cast<byte>(0x42));
    mc::memcpy(gb_dst.payload(), gb_src.payload(), 32);
    sb::require(gb_dst.guards_intact());
    sb::require(no_overrun(gb_dst, 32));
    sb::require(verify_buffer(gb_dst.payload(), static_cast<u64>(32), static_cast<byte>(0x42)));
  }
  sb::end_test_case();

  sb::test_case("Off-by-one: one-past-end byte is untouched");
  {
    byte src[4] = { 1, 2, 3, 4 };
    byte dst[5] = { 0, 0, 0, 0, 0xCC };
    mc::memcpy(dst, src, 4);
    sb::require(dst[3] == 4);
    sb::require(dst[4] == 0xCC);     // sentinel must survive
  }
  sb::end_test_case();

  sb::test_case("Off-by-one: one-before-start byte is untouched");
  {
    byte src[4] = { 1, 2, 3, 4 };
    byte storage[5] = { 0xDD, 0, 0, 0, 0 };
    byte *dst = storage + 1;     // dst starts at storage[1]
    mc::memcpy(dst, src, 4);
    sb::require(storage[0] == 0xDD);     // byte before dst must survive
    sb::require(buffers_equal(dst, src, 4));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Artifacting: make sure no garbage is left in the
  //  destination after a partial or full copy
  // ----------------------------------------------------------

  sb::test_case("Artifacting: sequential source fully reproduced (byte)");
  {
    byte src[32];
    byte dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xEE));     // pre-fill with noise
    mc::memcpy(dst, src, 32);
    sb::require(buffers_equal(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("Artifacting: sequential source fully reproduced (u32)");
  {
    u32 src[16];
    u32 dst[16];
    make_seq(src);
    fill_pattern(dst, 0xDEADu);
    mc::memcpy(dst, src, 16);
    sb::require(buffers_equal(dst, src, 16));
  }
  sb::end_test_case();

  sb::test_case("Artifacting: partial copy leaves tail untouched");
  {
    byte src[16];
    byte dst[16];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xFF));     // tail noise
    mc::memcpy(dst, src, 8);                        // copy only first 8
    sb::require(buffers_equal(dst, src, 8));
    // tail must be original noise, not src values or zeros
    for ( u64 i = 8; i < 16; i++ )
      sb::require(dst[i] == static_cast<byte>(0xFF));
  }
  sb::end_test_case();

  sb::test_case("Artifacting: cross-type copy byte->u32 (bytecpy)");
  {
    byte src[64];
    u32 dst[16];
    make_seq(src);
    fill_pattern(dst, 0xDEADBEEFu);
    mc::bytecpy(dst, src, 64);     // 64 bytes = 16 u32 regions
    const byte *bdst = reinterpret_cast<const byte *>(dst);
    sb::require(buffers_equal(bdst, src, 64));
  }
  sb::end_test_case();

  sb::test_case("Artifacting: overwrite check — no stale bits from prior content");
  {
    byte src[32];
    byte dst[32];
    // Round 1
    fill_pattern(src, static_cast<byte>(0x00));
    fill_pattern(dst, static_cast<byte>(0xFF));
    mc::memcpy(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0x00)));
    // Round 2: reverse
    fill_pattern(src, static_cast<byte>(0xFF));
    mc::memcpy(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xFF)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::rmemcpy — reference variant
  // ----------------------------------------------------------

  sb::test_case("rmemcpy - basic correctness");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x3C));
    mc::rmemcpy(*dst, *src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0x3C)));
  }
  sb::end_test_case();

  sb::test_case("rmemcpy - sequential, no artifacting");
  {
    byte src[32];
    byte dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xAA));
    mc::rmemcpy(*dst, *src, 32);
    sb::require(buffers_equal(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("rmemcpy - non-multiple-of-4 count");
  {
    u16 src[7];
    u16 dst[7] = {};
    fill_pattern(src, static_cast<u16>(0x1234));
    mc::rmemcpy(*dst, *src, 7);
    sb::require(verify_buffer(dst, static_cast<u16>(0x1234)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::constexpr_memcpy
  // ----------------------------------------------------------

  sb::test_case("constexpr_memcpy - basic byte copy");
  {
    byte src[16];
    byte dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xF0));
    mc::constexpr_memcpy(dst, src, 16);
    sb::require(verify_buffer(dst, static_cast<byte>(0xF0)));
  }
  sb::end_test_case();

  sb::test_case("constexpr_memcpy - odd count");
  {
    u32 src[9];
    u32 dst[9] = {};
    fill_pattern(src, 0xFEEDFACEu);
    mc::constexpr_memcpy(dst, src, 9);
    sb::require(verify_buffer(dst, 0xFEEDFACEu));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::cmemcpy — compile-time count
  // ----------------------------------------------------------

  sb::test_case("cmemcpy<32> - basic correctness");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x55));
    mc::cmemcpy<32>(dst, src);
    sb::require(verify_buffer(dst, static_cast<byte>(0x55)));
  }
  sb::end_test_case();

  sb::test_case("cmemcpy<7> - non-power-of-2 count");
  {
    u16 src[7];
    u16 dst[7] = {};
    fill_pattern(src, static_cast<u16>(0xBEEF));
    mc::cmemcpy<7>(dst, src);
    sb::require(verify_buffer(dst, static_cast<u16>(0xBEEF)));
  }
  sb::end_test_case();

  sb::test_case("cmemcpy<1> - single element");
  {
    u64 src[1] = { static_cast<u64>(0xDEADC0DEull) };
    u64 dst[1] = { 0 };
    mc::cmemcpy<1>(dst, src);
    sb::require(dst[0] == static_cast<u64>(0xDEADC0DEull));
  }
  sb::end_test_case();

  sb::test_case("cmemcpy<16> - guard zone intact");
  {
    GuardedBuffer gb_src, gb_dst;
    fill_pattern(gb_src.payload(), static_cast<u64>(16), static_cast<byte>(0x77));
    mc::cmemcpy<16>(gb_dst.payload(), gb_src.payload());
    sb::require(gb_dst.guards_intact());
    sb::require(no_overrun(gb_dst, 16));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::smemcpy — safe (nullptr / alignment) variant
  // ----------------------------------------------------------

  sb::test_case("smemcpy - valid pointer basic correctness");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xC7));
    byte *result = mc::smemcpy(dst, src, 32);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xC7)));
  }
  sb::end_test_case();

  sb::test_case("smemcpy - nullptr dest returns nullptr, src untouched");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x11));
    byte *result = mc::smemcpy<byte, byte>(nullptr, src, 16);
    sb::require(result == nullptr);
    // src must not have been disturbed
    sb::require(verify_buffer(src, static_cast<byte>(0x11)));
  }
  sb::end_test_case();

  sb::test_case("smemcpy - nullptr src returns nullptr, dest untouched");
  {
    byte dst[16];
    fill_pattern(dst, static_cast<byte>(0x22));
    byte *result = mc::smemcpy<byte, byte>(dst, static_cast<byte *>(nullptr), 16);
    sb::require(result == nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x22)));
  }
  sb::end_test_case();

  sb::test_case("smemcpy - returns pointer to destination on success");
  {
    byte src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte dst[8] = {};
    byte *result = mc::smemcpy(dst, src, 8);
    sb::require(result == dst);
  }
  sb::end_test_case();

  sb::test_case("smemcpy - misaligned dest returns nullptr");
  {
    alignas(16) byte storage[32] = {};
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x44));
    // force misalignment by requesting alignment=16 on an off-by-one pointer
    byte *misaligned = storage + 1;
    byte *result = mc::smemcpy<byte, byte, 16>(misaligned, src, 15);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("smemcpy - guard zone intact on valid copy");
  {
    GuardedBuffer gb_src, gb_dst;
    fill_pattern(gb_src.payload(), static_cast<u64>(64), static_cast<byte>(0x88));
    mc::smemcpy(gb_dst.payload(), gb_src.payload(), 64);
    sb::require(gb_dst.guards_intact());
    sb::require(no_overrun(gb_dst, 64));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::rsmemcpy — safe reference variant
  // ----------------------------------------------------------

  sb::test_case("rsmemcpy - basic correctness");
  {
    byte src[16];
    byte dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xD5));
    bool ok = mc::rsmemcpy(*dst, *src, 16);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xD5)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::scmemcpy — safe compile-time variant
  // ----------------------------------------------------------

  sb::test_case("scmemcpy<32> - valid pointers");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xE1));
    byte *result = mc::scmemcpy<32>(dst, src);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xE1)));
  }
  sb::end_test_case();

  sb::test_case("scmemcpy<16> - nullptr returns nullptr");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x33));
    byte *result = mc::scmemcpy<16>(static_cast<byte *>(nullptr), src);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::bytecpy — byte-level reinterpret copy
  // ----------------------------------------------------------

  sb::test_case("bytecpy - u32 to u32, byte-for-byte identity");
  {
    u32 src[4] = { 0x01020304u, 0x05060708u, 0x090A0B0Cu, 0x0D0E0F10u };
    u32 dst[4] = {};
    mc::bytecpy(dst, src, sizeof(src));
    const byte *bs = reinterpret_cast<const byte *>(src);
    const byte *bd = reinterpret_cast<const byte *>(dst);
    sb::require(buffers_equal(bd, bs, sizeof(src)));
  }
  sb::end_test_case();

  sb::test_case("bytecpy - non-multiple-of-4 byte count");
  {
    u16 src[5];
    u16 dst[5] = {};
    fill_pattern(src, static_cast<u16>(0xABCD));
    mc::bytecpy(dst, src, 10);     // 5 * sizeof(u16) = 10, 10 % 4 != 0
    sb::require(verify_buffer(dst, static_cast<u16>(0xABCD)));
  }
  sb::end_test_case();

  sb::test_case("bytecpy - guard zone, no overrun");
  {
    GuardedBuffer gb_src, gb_dst;
    make_seq(gb_src.payload(), static_cast<u64>(48));
    mc::bytecpy(gb_dst.payload(), gb_src.payload(), 48);
    sb::require(gb_dst.guards_intact());
    sb::require(no_overrun(gb_dst, 48));
    sb::require(buffers_equal(gb_dst.payload(), gb_src.payload(), 48));
  }
  sb::end_test_case();

  sb::test_case("bytecpy - artifacting: no stale bits from prior fill");
  {
    byte src[32];
    byte dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xCC));
    mc::bytecpy(dst, src, 32);
    sb::require(buffers_equal(dst, src, 32));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::cbytecpy — compile-time byte copy
  // ----------------------------------------------------------

  sb::test_case("cbytecpy<32> - basic correctness");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x9E));
    mc::cbytecpy<32>(dst, src);
    sb::require(verify_buffer(dst, static_cast<byte>(0x9E)));
  }
  sb::end_test_case();

  sb::test_case("cbytecpy<13> - non-divisible-by-4 count");
  {
    byte src[13];
    byte dst[13] = {};
    make_seq(src);
    mc::cbytecpy<13>(dst, src);
    sb::require(buffers_equal(dst, src, 13));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::sbytecpy — safe byte copy
  // ----------------------------------------------------------

  sb::test_case("sbytecpy - valid pointers");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xF7));
    byte *result = mc::sbytecpy(dst, src, 32);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xF7)));
  }
  sb::end_test_case();

  sb::test_case("sbytecpy - nullptr dest returns nullptr");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x55));
    byte *result = mc::sbytecpy<byte, byte>(static_cast<byte *>(nullptr), src, 16);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("sbytecpy - nullptr src returns nullptr, dest untouched");
  {
    byte dst[16];
    fill_pattern(dst, static_cast<byte>(0x66));
    byte *result = mc::sbytecpy<byte, byte>(dst, static_cast<byte *>(nullptr), 16);
    sb::require(result == nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x66)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::voidcpy — void-pointer copy
  // ----------------------------------------------------------

  sb::test_case("voidcpy - basic byte copy");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xDA));
    mc::voidcpy(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xDA)));
  }
  sb::end_test_case();

  sb::test_case("voidcpy - sequential, no artifacting");
  {
    byte src[64];
    byte dst[64];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xFF));
    mc::voidcpy(dst, src, 64);
    sb::require(buffers_equal(dst, src, 64));
  }
  sb::end_test_case();

  sb::test_case("voidcpy - non-multiple-of-4 count");
  {
    byte src[11];
    byte dst[11] = {};
    make_seq(src);
    mc::voidcpy(dst, src, 11);
    sb::require(buffers_equal(dst, src, 11));
  }
  sb::end_test_case();

  sb::test_case("voidcpy - guard zone intact");
  {
    GuardedBuffer gb_src, gb_dst;
    make_seq(gb_src.payload(), static_cast<u64>(128));
    mc::voidcpy(gb_dst.payload(), gb_src.payload(), 128);
    sb::require(gb_dst.guards_intact());
    sb::require(no_overrun(gb_dst, 128));
    sb::require(buffers_equal(gb_dst.payload(), gb_src.payload(), 128));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::svoidcpy — safe void-pointer copy
  // ----------------------------------------------------------

  sb::test_case("svoidcpy - valid pointers");
  {
    byte src[16];
    byte dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xBC));
    void *result = mc::svoidcpy(dst, src, 16);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xBC)));
  }
  sb::end_test_case();

  sb::test_case("svoidcpy - nullptr dest returns nullptr");
  {
    byte src[8];
    fill_pattern(src, static_cast<byte>(0x10));
    void *result = mc::svoidcpy(nullptr, src, 8);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("svoidcpy - nullptr src returns nullptr");
  {
    byte dst[8];
    fill_pattern(dst, static_cast<byte>(0x20));
    void *result = mc::svoidcpy(dst, nullptr, 8);
    sb::require(result == nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x20)));
  }
  sb::end_test_case();

  sb::test_case("svoidcpy - returns dest pointer on success");
  {
    byte src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte dst[8] = {};
    void *result = mc::svoidcpy(dst, src, 8);
    sb::require(result == static_cast<void *>(dst));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  mc::__memcpy_32 — small fixed-count unrolled copy
  //  Covers every case 1–32 to catch any missing case in the switch
  // ----------------------------------------------------------

  sb::test_case("__memcpy_32 - count = 0 (no-op, returns dest)");
  {
    byte src[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    byte dst[4] = { 0x00, 0x00, 0x00, 0x00 };
    byte *result = mc::__memcpy_32(dst, src, static_cast<u64>(0));
    sb::require(result == dst);
    sb::require(dst[0] == 0x00);     // nothing written
  }
  sb::end_test_case();

  // Test every case 1–7 (individual assignments in switch)
  sb::test_case("__memcpy_32 - counts 1 through 7 (unrolled switch cases)");
  {
    byte src[8];
    make_seq(src);
    for ( u64 n = 1; n <= 7; n++ ) {
      byte dst[8];
      fill_pattern(dst, static_cast<byte>(0xFF));
      mc::__memcpy_32(dst, src, n);
      sb::require(buffers_equal(dst, src, n));
      // byte immediately after copy window must be undisturbed
      sb::require(dst[n] == static_cast<byte>(0xFF));
    }
  }
  sb::end_test_case();

  // Test every case 8–32 (loop-based switch cases)
  sb::test_case("__memcpy_32 - counts 8 through 32 (loop switch cases)");
  {
    byte src[32];
    make_seq(src);
    for ( u64 n = 8; n <= 32; n++ ) {
      byte dst[33];
      fill_pattern(dst, static_cast<byte>(0xEE));
      mc::__memcpy_32(dst, src, n);
      sb::require(buffers_equal(dst, src, n));
      if ( n < 33 )
        sb::require(dst[n] == static_cast<byte>(0xEE));     // sentinel byte
    }
  }
  sb::end_test_case();

  sb::test_case("__memcpy_32 - guard zone: no byte written past n");
  {
    byte src[32];
    make_seq(src);
    for ( u64 n = 1; n <= 32; n++ ) {
      GuardedBuffer gb;
      fill_pattern(gb.payload(), static_cast<u64>(32), static_cast<byte>(CANARY));
      mc::__memcpy_32(gb.payload(), src, n);
      sb::require(no_overrun(gb, n));
    }
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Cross-type copies (implicit narrowing/widening via static_cast)
  // ----------------------------------------------------------

  sb::test_case("memcpy cross-type: u8 source into u16 destination");
  {
    byte src[16];
    u16 dst[16] = {};
    fill_pattern(src, static_cast<byte>(0x42));
    mc::memcpy(dst, src, 16);
    sb::require(verify_buffer(dst, static_cast<u16>(0x42)));
  }
  sb::end_test_case();

  sb::test_case("memcpy cross-type: u32 source into u64 destination");
  {
    u32 src[8];
    u64 dst[8] = {};
    fill_pattern(src, 0xBEEFu);
    mc::memcpy(dst, src, 8);
    sb::require(verify_buffer(dst, static_cast<u64>(0xBEEFu)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Repeated operations / stress
  // ----------------------------------------------------------

  sb::test_case("Repeated memcpy: 100 iterations with varying values");
  {
    byte src[64];
    byte dst[64];
    for ( int i = 0; i < 100; i++ ) {
      byte val = static_cast<byte>(i);
      fill_pattern(src, val);
      fill_pattern(dst, static_cast<byte>(~i));
      mc::memcpy(dst, src, 64);
      sb::require(verify_buffer(dst, val));
    }
  }
  sb::end_test_case();

  sb::test_case("Repeated bytecpy: sequential pattern, 50 iterations");
  {
    byte src[32];
    byte dst[32];
    for ( int i = 0; i < 50; i++ ) {
      make_seq(src);
      fill_pattern(dst, static_cast<byte>(0xAB));
      mc::bytecpy(dst, src, 32);
      sb::require(buffers_equal(dst, src, 32));
    }
  }
  sb::end_test_case();

  sb::test_case("Performance / stress: large buffer voidcpy");
  {
    constexpr u64 SIZE = 512;
    byte src[SIZE];
    byte dst[SIZE];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0x00));
    mc::voidcpy(dst, src, SIZE);
    sb::require(buffers_equal(dst, src, SIZE));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Boundary: all-zeros and all-ones sources
  // ----------------------------------------------------------

  sb::test_case("memcpy - all-zeros source fully zeros destination");
  {
    byte src[32] = {};
    byte dst[32];
    fill_pattern(dst, static_cast<byte>(0xFF));
    mc::memcpy(dst, src, 32);
    sb::require(is_zeroed(dst));
  }
  sb::end_test_case();

  sb::test_case("memcpy - all-ones source fully sets destination");
  {
    byte src[32];
    byte dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xFF));
    mc::memcpy(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xFF)));
  }
  sb::end_test_case();

  sb::test_case("memcpy - alternating 0xAA/0x55 pattern preserved");
  {
    byte src[32];
    byte dst[32];
    for ( u64 i = 0; i < 32; i++ )
      src[i] = (i % 2 == 0) ? 0xAA : 0x55;
    fill_pattern(dst, static_cast<byte>(0x00));
    mc::memcpy(dst, src, 32);
    sb::require(buffers_equal(dst, src, 32));
  }
  sb::end_test_case();

  // ============================================================
  //  Edge-case: cross-cache-line copy
  // ============================================================
  sb::test_case("memcpy - cross cache line boundary");
  {
    constexpr u64 CACHE_LINE = 64;
    alignas(CACHE_LINE) byte storage[2 * CACHE_LINE];     // aligned buffer
    byte *src = storage + 30;                             // intentionally misaligned start
    byte *dst = storage + CACHE_LINE + 10;                // crosses cache line

    make_seq(src, 50);
    fill_pattern(dst, 50, static_cast<byte>(0xFF));

    mc::memcpy(dst, src, 50);
    sb::require(buffers_equal(dst, src, 50));
  }
  sb::end_test_case();

  // ============================================================
  //  Edge-case: cross-page copy
  // ============================================================
  sb::test_case("memcpy - cross page boundary");
  {
    constexpr u64 PAGE_SZ = 4096;
    alignas(PAGE_SZ) byte storage[PAGE_SZ * 2];     // 2 pages
    byte *src = storage + PAGE_SZ - 32;             // straddles page end
    byte *dst = storage + PAGE_SZ + 16;             // straddles next page

    make_seq(src, 64);
    fill_pattern(dst, 64, static_cast<byte>(0xEE));

    mc::memcpy(dst, src, 64);
    sb::require(buffers_equal(dst, src, 64));
  }
  sb::end_test_case();
  /*
   * TODO: investigate
// ============================================================
//  Edge-case: partial unaligned copy (misaligned start/end)
// ============================================================
sb::test_case("memcpy - unaligned start/end");
{
  constexpr u64 SIZE = 128;
  alignas(64) byte storage[SIZE + 16];
  byte *src = storage + 3;     // intentionally unaligned
  byte *dst = storage + 8;     // intentionally unaligned

  make_seq(src, 100);
  fill_pattern(dst, 100, static_cast<byte>(0xAA));

  mc::memcpy(dst, src, 100);
  sb::require(buffers_equal(dst, src, 100));
}
sb::end_test_case();
// ============================================================
//  Cross-cache-line: multiple lengths
// ============================================================
sb::test_case("memcpy - cross cache line multiple lengths");
{
  constexpr u64 CACHE_LINE = 64;
  alignas(CACHE_LINE) byte storage[2 * CACHE_LINE];
  for ( u64 len = 1; len <= 80; len += 7 ) {     // test various lengths
    byte *src = storage + 20;
    byte *dst = storage + CACHE_LINE - 10;
    make_seq(src, len);
    fill_pattern(dst, len, static_cast<byte>(0xAA));
    mc::memcpy(dst, src, len);
    sb::require(buffers_equal(dst, src, len));
  }
}
sb::end_test_case();
*/

  // ============================================================
  //  Cross-page: small to large copies
  // ============================================================
  sb::test_case("memcpy - cross page multiple lengths");
  {
    constexpr u64 PAGE_SZ = 4096;
    alignas(PAGE_SZ) byte storage[PAGE_SZ * 2];
    for ( u64 len : { 16, 64, 128, 512, 1024 } ) {
      byte *src = storage + PAGE_SZ - len / 2;     // straddles page boundary
      byte *dst = storage + PAGE_SZ + 32;
      make_seq(src, len);
      fill_pattern(dst, len, static_cast<byte>(0xEE));
      mc::memcpy(dst, src, len);
      sb::require(buffers_equal(dst, src, len));
    }
  }
  sb::end_test_case();

  // ============================================================
  //  Misaligned start and end
  // ============================================================
  sb::test_case("memcpy - misaligned start and end");
  {
    alignas(64) byte storage[256];
    for ( u64 offset = 1; offset <= 7; offset++ ) {
      byte *src = storage + offset;
      byte *dst = storage + 64 + offset;
      make_seq(src, 100);
      fill_pattern(dst, 100, static_cast<byte>(0xFF));
      mc::memcpy(dst, src, 100);
      sb::require(buffers_equal(dst, src, 100));
    }
  }
  sb::end_test_case();

  // ============================================================
  //  Single-byte crossing cache line
  // ============================================================
  sb::test_case("memcpy - single byte crosses cache line");
  {
    constexpr u64 CACHE_LINE = 64;
    alignas(CACHE_LINE) byte storage[2 * CACHE_LINE];
    byte *src = storage + CACHE_LINE - 1;
    byte *dst = storage;
    src[0] = 0xAB;
    mc::memcpy(dst, src, 1);
    sb::require(dst[0] == 0xAB);
  }
  sb::end_test_case();

  // ============================================================
  //  Very large copy (>page size)
  // ============================================================
  sb::test_case("memcpy - large copy > 1 page");
  {
    constexpr u64 SIZE = 8192;
    alignas(4096) byte src[SIZE];
    alignas(4096) byte dst[SIZE];
    make_seq(src, SIZE);
    fill_pattern(dst, SIZE, static_cast<byte>(0x00));
    mc::memcpy(dst, src, SIZE);
    sb::require(buffers_equal(dst, src, SIZE));
  }
  sb::end_test_case();

  // ============================================================
  //  Cross-type copy: u8 -> u16, crossing cache line
  // ============================================================
  sb::test_case("memcpy cross-type u8->u16 cross cache line");
  {
    constexpr u64 CACHE_LINE = 64;
    alignas(CACHE_LINE) byte storage[2 * CACHE_LINE];
    byte *src = storage + 30;
    u16 dst[64] = {};
    make_seq(src, 50);
    fill_pattern(dst, 50, static_cast<u16>(0xFFFF));
    mc::memcpy(dst, src, 50);
    sb::require(verify_buffer(dst, 50, static_cast<u16>(0)) == false);        // sanity
    sb::require(verify_buffer(dst, 50, static_cast<u16>(0x00)) == false);     // sanity
  }
  sb::end_test_case();

  // ============================================================
  //  Overlapping regions detection (memcpy should not handle safely)
  // ============================================================
  sb::test_case("memcpy - overlapping regions (undefined behavior)");
  {
    byte buffer[64];
    make_seq(buffer, 64);
    // Overlapping copy: dst inside src
    mc::memcpy(buffer + 10, buffer, 32);
    // Verify at least first byte copied correctly
    sb::require(buffer[10] == 0);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}

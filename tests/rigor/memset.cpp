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
//  Helper utilities
// ============================================================

static constexpr byte CANARY = 0xC3;

template <typename T, u64 N>
bool
verify_buffer(const T (&buf)[N], byte expected)
{
  const byte *p = reinterpret_cast<const byte *>(buf);
  for ( u64 i = 0; i < N * sizeof(T); i++ )
    if ( p[i] != expected ) return false;
  return true;
}

template <typename T>
bool
verify_buffer(const T *buf, u64 byte_count, byte expected)
{
  const byte *p = reinterpret_cast<const byte *>(buf);
  for ( u64 i = 0; i < byte_count; i++ )
    if ( p[i] != expected ) return false;
  return true;
}

// ============================================================
//  Guard-zone helper
// ============================================================

static constexpr u64 GUARD_SZ = 16;

struct GuardedBuffer {
  byte data[GUARD_SZ + 512 + GUARD_SZ];

  GuardedBuffer()
  {
    for ( u64 i = 0; i < sizeof(data); i++ ) data[i] = CANARY;
  }

  byte *
  payload()
  {
    return data + GUARD_SZ;
  }

  bool
  guards_intact() const
  {
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( data[i] != CANARY ) return false;
    for ( u64 i = GUARD_SZ + 512; i < sizeof(data); i++ )
      if ( data[i] != CANARY ) return false;
    return true;
  }
};

bool
no_overrun(const GuardedBuffer &gb, u64 written_bytes)
{
  const byte *after = gb.data + GUARD_SZ + written_bytes;
  for ( u64 i = 0; i < GUARD_SZ; i++ )
    if ( after[i] != CANARY ) return false;
  for ( u64 i = 0; i < GUARD_SZ; i++ )
    if ( gb.data[i] != CANARY ) return false;
  return true;
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== MEMSET TESTS ===");

  // ----------------------------------------------------------
  //  mc::memset — basic runtime fills
  // ----------------------------------------------------------

  sb::test_case("Basic memset - byte buffer, value 0xAB");
  {
    byte buf[32] = {};
    mc::memset(buf, static_cast<byte>(0xAB), 32);
    sb::require(verify_buffer(buf, static_cast<byte>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("Basic memset - zero fill");
  {
    byte buf[64];
    for ( u64 i = 0; i < 64; i++ ) buf[i] = static_cast<byte>(i);
    mc::memset(buf, static_cast<byte>(0x00), 64);
    sb::require(verify_buffer(buf, static_cast<byte>(0x00)));
  }
  sb::end_test_case();

  sb::test_case("memset - fill with 0xFF");
  {
    byte buf[128] = {};
    mc::memset(buf, static_cast<byte>(0xFF), 128);
    sb::require(verify_buffer(buf, static_cast<byte>(0xFF)));
  }
  sb::end_test_case();

  sb::test_case("memset - count divisible by 4 (fast path)");
  {
    byte buf[64] = {};
    mc::memset(buf, static_cast<byte>(0x5A), 64);
    sb::require(verify_buffer(buf, static_cast<byte>(0x5A)));
  }
  sb::end_test_case();

  sb::test_case("memset - count not divisible by 4 (slow path)");
  {
    byte buf[63] = {};
    mc::memset(buf, static_cast<byte>(0x77), 63);
    sb::require(verify_buffer(buf, 63, static_cast<byte>(0x77)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Edge sizes around SIMD widths
  // ----------------------------------------------------------

  sb::test_case("memset - exactly 16 bytes (one SSE block)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0xA5), 16);
    sb::require(verify_buffer(gb.payload(), 16, static_cast<byte>(0xA5)));
    sb::require(no_overrun(gb, 16));
  }
  sb::end_test_case();

  sb::test_case("memset - exactly 32 bytes (one AVX block)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0x3C), 32);
    sb::require(verify_buffer(gb.payload(), 32, static_cast<byte>(0x3C)));
    sb::require(no_overrun(gb, 32));
  }
  sb::end_test_case();

  sb::test_case("memset - 17 bytes (16-byte SIMD + 1-byte tail)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0xE1), 17);
    sb::require(verify_buffer(gb.payload(), 17, static_cast<byte>(0xE1)));
    sb::require(no_overrun(gb, 17));
  }
  sb::end_test_case();

  sb::test_case("memset - 33 bytes (32 SIMD + 1-byte tail)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0xD2), 33);
    sb::require(verify_buffer(gb.payload(), 33, static_cast<byte>(0xD2)));
    sb::require(no_overrun(gb, 33));
  }
  sb::end_test_case();

  sb::test_case("memset - 1 byte");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0x42), 1);
    sb::require(gb.payload()[0] == 0x42);
    sb::require(no_overrun(gb, 1));
  }
  sb::end_test_case();

  sb::test_case("memset - 0 bytes (no-op)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0x99), 0);
    sb::require(gb.guards_intact());
    sb::require(gb.payload()[0] == CANARY);
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Alignment variants (smemset alignment-checked path)
  // ----------------------------------------------------------

  sb::test_case("memset - unaligned start (offset 1 from cacheline)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload() + 1, static_cast<byte>(0x6E), 100);
    sb::require(verify_buffer(gb.payload() + 1, 100, static_cast<byte>(0x6E)));
    sb::require(gb.payload()[0] == CANARY);
    sb::require(no_overrun(gb, 1 + 100));
  }
  sb::end_test_case();

  sb::test_case("memset - unaligned start (offset 7)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload() + 7, static_cast<byte>(0x55), 50);
    sb::require(verify_buffer(gb.payload() + 7, 50, static_cast<byte>(0x55)));
    for ( u64 i = 0; i < 7; i++ ) sb::require(gb.payload()[i] == CANARY);
    sb::require(no_overrun(gb, 7 + 50));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Typed buffers
  // ----------------------------------------------------------

  sb::test_case("memset - u32 buffer");
  {
    u32 buf[16];
    for ( u64 i = 0; i < 16; i++ ) buf[i] = 0xDEADBEEFu;
    mc::memset(buf, static_cast<byte>(0x00), sizeof(buf));
    sb::require(verify_buffer(buf, static_cast<byte>(0x00)));
  }
  sb::end_test_case();

  sb::test_case("memset - u64 buffer 0xFF");
  {
    u64 buf[8];
    for ( u64 i = 0; i < 8; i++ ) buf[i] = 0;
    mc::memset(buf, static_cast<byte>(0xFF), sizeof(buf));
    for ( u64 i = 0; i < 8; i++ ) sb::require(buf[i] == ~static_cast<u64>(0));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Compile-time variants
  // ----------------------------------------------------------

  sb::test_case("cmemset - compile-time size 8");
  {
    byte buf[8] = {};
    mc::cmemset<8>(buf, static_cast<byte>(0xAA));
    sb::require(verify_buffer(buf, static_cast<byte>(0xAA)));
  }
  sb::end_test_case();

  sb::test_case("cmemset - compile-time size 64");
  {
    byte buf[64] = {};
    mc::cmemset<64>(buf, static_cast<byte>(0x33));
    sb::require(verify_buffer(buf, static_cast<byte>(0x33)));
  }
  sb::end_test_case();

  sb::test_case("cmemset - compile-time size 256");
  {
    byte buf[256] = {};
    mc::cmemset<256>(buf, static_cast<byte>(0x44));
    sb::require(verify_buffer(buf, static_cast<byte>(0x44)));
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Larger sizes that hit the SIMD inner loop
  // ----------------------------------------------------------

  sb::test_case("memset - 256 bytes");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0xBE), 256);
    sb::require(verify_buffer(gb.payload(), 256, static_cast<byte>(0xBE)));
    sb::require(no_overrun(gb, 256));
  }
  sb::end_test_case();

  sb::test_case("memset - 511 bytes (max payload, odd size)");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0x91), 511);
    sb::require(verify_buffer(gb.payload(), 511, static_cast<byte>(0x91)));
    sb::require(no_overrun(gb, 511));
  }
  sb::end_test_case();

  sb::test_case("memset - 512 bytes");
  {
    GuardedBuffer gb;
    mc::memset(gb.payload(), static_cast<byte>(0x4D), 512);
    sb::require(verify_buffer(gb.payload(), 512, static_cast<byte>(0x4D)));
    sb::require(gb.guards_intact());
  }
  sb::end_test_case();

  // ----------------------------------------------------------
  //  Idempotence and value preservation
  // ----------------------------------------------------------

  sb::test_case("memset - calling twice produces same result");
  {
    byte buf[64] = {};
    mc::memset(buf, static_cast<byte>(0x77), 64);
    mc::memset(buf, static_cast<byte>(0x77), 64);
    sb::require(verify_buffer(buf, static_cast<byte>(0x77)));
  }
  sb::end_test_case();

  sb::test_case("memset - overwrites previous content");
  {
    byte buf[32];
    for ( u64 i = 0; i < 32; i++ ) buf[i] = static_cast<byte>(i);
    mc::memset(buf, static_cast<byte>(0xCC), 32);
    sb::require(verify_buffer(buf, static_cast<byte>(0xCC)));
  }
  sb::end_test_case();

  sb::test_case("memset - partial overwrite leaves tail intact");
  {
    GuardedBuffer gb;
    for ( u64 i = 0; i < 100; i++ ) gb.payload()[i] = static_cast<byte>(0x12);
    mc::memset(gb.payload(), static_cast<byte>(0x88), 50);
    for ( u64 i = 0; i < 50; i++ ) sb::require(gb.payload()[i] == 0x88);
    for ( u64 i = 50; i < 100; i++ ) sb::require(gb.payload()[i] == 0x12);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 0;
}

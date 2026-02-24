//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/memory/memory.hpp"
#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/std.hpp"

#include "../../src/slice.hpp"

#include "../snowball/snowball.hpp"

template <typename T, typename F, u64 N>
bool
verify_buffer(T (&buf)[N], F expected)
{
  for ( u64 i = 0; i < N; i++ ) {
    if ( buf[i] != expected )
      return false;
  }
  return true;
}
template <typename T, typename F>
bool
verify_buffer(T *buf, size_t size, F expected)
{
  for ( size_t i = 0; i < size; ++i )
    if ( buf[i] != expected )
      return false;
  return true;
}

template <typename T, u64 N>
bool
is_zeroed(T (&buf)[N])
{
  return verify_buffer(buf, static_cast<T>(0));
}

template <typename T, u64 N>
bool
is_filled(T (&buf)[N])
{
  return verify_buffer(buf, static_cast<T>(0xFF));
}

int
main(void)
{
  sb::print("=== MEMSET TESTS ===");

  sb::test_case("Basic memset - byte buffer");
  {
    byte buf[32];
    mc::memset(buf, 0xAA, 32);
    sb::require(verify_buffer(buf, static_cast<byte>(0xAA)));
  }
  sb::end_test_case();

  sb::test_case("Basic memset - word buffer");
  {
    word buf[16];
    mc::memset(buf, 0x42, 16 * sizeof(word));
    // Verify byte-level setting
    byte *bbuf = reinterpret_cast<byte *>(buf);
    sb::require(verify_buffer(bbuf, 16, static_cast<byte>(0x42)));
  }
  sb::end_test_case();

  sb::test_case("Reference memset - rmemset");
  {
    byte buf[16];
    mc::rmemset(buf, 0x55, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x55)));
  }
  sb::end_test_case();

  sb::test_case("Constexpr memset");
  {
    u64 buf[8];
    mc::constexpr_memset(buf, 0x00, sizeof(buf));
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Compile-time count memset - cmemset");
  {
    byte buf[32];
    mc::cmemset<32>(buf, 0xBB);
    sb::require(verify_buffer(buf, static_cast<byte>(0xBB)));
  }
  sb::end_test_case();

  sb::test_case("Compile-time count memset with reference - rcmemset");
  {
    byte buf[24];
    mc::rcmemset<24>(buf, 0xCC);
    sb::require(verify_buffer(buf, static_cast<byte>(0xCC)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled memset - 8 bytes");
  {
    byte buf[8];
    mc::memset_8b(buf, 0x11);
    sb::require(verify_buffer(buf, static_cast<byte>(0x11)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled memset - 16 bytes");
  {
    byte buf[16];
    mc::memset_16b(buf, 0x22);
    sb::require(verify_buffer(buf, static_cast<byte>(0x22)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled memset - 32 bytes");
  {
    byte buf[32];
    mc::memset_32b(buf, 0x33);
    sb::require(verify_buffer(buf, static_cast<byte>(0x33)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled memset - 64 bytes");
  {
    byte buf[64];
    mc::memset_64b(buf, 0x44);
    sb::require(verify_buffer(buf, static_cast<byte>(0x44)));
  }
  sb::end_test_case();
  sb::test_case("Safe memset - slices");
  {
    mc::slice<byte> __buffer(512);
    byte *result = mc::smemset<byte>(&__buffer, 0xff, 512);
    sb::require(result != nullptr, true);
  }
  sb::end_test_case();

  sb::test_case("memset out of bounds");
  {
    byte buf[8];
    byte *res = mc::smemset(buf, 0x11, 1ULL << 40);
    sb::require(res == nullptr);
  }
  sb::end_test_case();

  sb::test_case("Safe memset - valid pointer");
  {
    byte buf[32];
    byte *result = mc::smemset(buf, 0x77, 32);
    sb::require(result != nullptr);
    sb::require(verify_buffer(buf, static_cast<byte>(0x77)));
  }
  sb::end_test_case();

  sb::test_case("Safe memset - nullptr check");
  {
    byte *result = mc::smemset<byte, 1>(nullptr, 0x77, 32);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("Safe memset with reference - rsmemset");
  {
    byte buf[16];
    bool result = mc::rsmemset(buf, 0x88, 16);
    sb::require(result == true);
    sb::require(verify_buffer(buf, static_cast<byte>(0x88)));
  }
  sb::end_test_case();

  sb::test_case("Basic byteset");
  {
    byte buf[24];
    mc::byteset(buf, 0x99, 24);
    sb::require(verify_buffer(buf, static_cast<byte>(0x99)));
  }
  sb::end_test_case();

  sb::test_case("Byteset alias - bset");
  {
    byte buf[20];
    mc::bset(buf, 0xAB, 20);
    sb::require(verify_buffer(buf, static_cast<byte>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("Byteset with reference - rbyteset");
  {
    byte buf[12];
    mc::rbyteset(buf, 0xCD, 12);
    sb::require(verify_buffer(buf, static_cast<byte>(0xCD)));
  }
  sb::end_test_case();

  sb::test_case("Compile-time byteset - cbyteset");
  {
    byte buf[28];
    mc::cbyteset<28>(buf, 0xEF);
    sb::require(verify_buffer(buf, static_cast<byte>(0xEF)));
  }
  sb::end_test_case();

  sb::test_case("Safe byteset - sbyteset");
  {
    byte buf[16];
    byte *result = mc::sbyteset(buf, 0xDE, 16);
    sb::require(result != nullptr);
    sb::require(verify_buffer(buf, static_cast<byte>(0xDE)));
  }
  sb::end_test_case();

  sb::test_case("Safe byteset - nullptr check");
  {
    byte *result = mc::sbyteset<byte, 1>(nullptr, 0xDE, 16);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("Unrolled byteset - 8 bytes");
  {
    byte buf[8];
    mc::byteset_8b(buf, 0xA1);
    sb::require(verify_buffer(buf, static_cast<byte>(0xA1)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled byteset - 16 bytes");
  {
    byte buf[16];
    mc::bset_16b(buf, 0xA2);     // Test alias
    sb::require(verify_buffer(buf, static_cast<byte>(0xA2)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled byteset - 32 bytes");
  {
    byte buf[32];
    mc::byteset_32b(buf, 0xA3);
    sb::require(verify_buffer(buf, static_cast<byte>(0xA3)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled byteset - 64 bytes");
  {
    byte buf[64];
    mc::bset_64b(buf, 0xA4);     // Test alias
    sb::require(verify_buffer(buf, static_cast<byte>(0xA4)));
  }
  sb::end_test_case();

  sb::test_case("Byte-level zero - bzero");
  {
    byte buf[32];
    mc::memset(buf, 0xFF, 32);     // Fill first
    mc::bzero(buf, 32);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Compile-time byte zero - cbzero");
  {
    u32 buf[8];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::cbzero<sizeof(buf)>(buf);
    byte *bbuf = reinterpret_cast<byte *>(buf);
    bool all_zero = true;
    for ( u64 i = 0; i < sizeof(buf); i++ ) {
      if ( bbuf[i] != 0 ) {
        all_zero = false;
        break;
      }
    }
    sb::require(all_zero);
  }
  sb::end_test_case();

  sb::test_case("Basic typeset - u32");
  {
    u32 buf[16];
    mc::typeset(buf, 0x12345678u, 16);
    sb::require(verify_buffer(buf, 0x12345678u));
  }
  sb::end_test_case();

  sb::test_case("Typeset with reference - rtypeset");
  {
    u64 buf[8];
    mc::rtypeset(buf, 0xDEADBEEFCAFEBABEull, 8);
    sb::require(verify_buffer(buf, 0xDEADBEEFCAFEBABEull));
  }
  sb::end_test_case();

  sb::test_case("Compile-time typeset - ctypeset");
  {
    u16 buf[20];
    mc::ctypeset<20>(buf, static_cast<u16>(0xABCD));
    sb::require(verify_buffer(buf, static_cast<u16>(0xABCD)));
  }
  sb::end_test_case();

  sb::test_case("Safe typeset - stypeset");
  {
    u32 buf[12];
    u32 *result = mc::stypeset(buf, 0xFEEDFACEu, 12);
    sb::require(result != nullptr);
    sb::require(verify_buffer(buf, 0xFEEDFACEu));
  }
  sb::end_test_case();

  sb::test_case("Safe typeset - nullptr check");
  {
    u32 *result = mc::stypeset<u32, u32, alignof(u32)>(nullptr, 0xFEEDFACEu, 12);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("Basic wordset");
  {
    word buf[32];
    mc::wordset(buf, 0xAB, 32);
    sb::require(verify_buffer(buf, static_cast<word>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("Wordset with reference - rwordset");
  {
    word buf[24];
    mc::wordset(buf, 0xCD, 24);
    sb::require(verify_buffer(buf, static_cast<word>(0xCD)));
  }
  sb::end_test_case();

  sb::test_case("Compile-time wordset - cwordset");
  {
    word buf[16];
    mc::cwordset<16>(buf, 0xEF);
    sb::require(verify_buffer(buf, static_cast<word>(0xEF)));
  }
  sb::end_test_case();

  sb::test_case("Safe wordset - swordset");
  {
    word buf[20];
    word *result = mc::swordset(buf, 0x42, 20);
    sb::require(result != nullptr);
    sb::require(verify_buffer(buf, static_cast<word>(0x42)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled wordset - 4 words");
  {
    word buf[4];
    mc::wordset_4w<0x11>(buf);
    sb::require(verify_buffer(buf, static_cast<word>(0x11)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled wordset - 8 words");
  {
    word buf[8];
    mc::wordset_8w<0x22>(buf);
    sb::require(verify_buffer(buf, static_cast<word>(0x22)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled wordset - 16 words");
  {
    word buf[16];
    mc::wordset_16w<0x33>(buf);
    sb::require(verify_buffer(buf, static_cast<word>(0x33)));
  }
  sb::end_test_case();

  sb::test_case("Unrolled wordset - 32 words");
  {
    word buf[32];
    mc::wordset_32w<0x44>(buf);
    sb::require(verify_buffer(buf, static_cast<word>(0x44)));
  }
  sb::end_test_case();

  sb::test_case("Zero - runtime count");
  {
    u32 buf[16];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::zero(buf, 16);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Zero with reference - rzero");
  {
    u64 buf[8];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::rzero(buf, 8);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Compile-time zero - czero");
  {
    u16 buf[24];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::czero<24>(buf);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled zero - 4 elements");
  {
    u32 buf[4];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::zero_4(buf);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled zero - 8 elements");
  {
    u64 buf[8];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::zero_8(buf);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled zero - 16 elements");
  {
    byte buf[16];
    mc::memset(buf, 0xFF, 16);
    mc::zero_16(buf);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled zero - 32 elements");
  {
    word buf[32];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::zero_32(buf);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Full - runtime count");
  {
    byte buf[20];
    mc::memset(buf, 0x00, 20);
    mc::full(buf, 20);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Full with reference - rfull");
  {
    byte buf[12];
    mc::memset(buf, 0x00, 12);
    mc::rfull(buf, 12);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Compile-time full - cfull");
  {
    byte buf[28];
    mc::memset(buf, 0x00, 28);
    mc::cfull<28>(buf);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled full - 4 elements");
  {
    byte buf[4];
    mc::memset(buf, 0x00, 4);
    mc::full_4(buf);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled full - 8 elements");
  {
    byte buf[8];
    mc::memset(buf, 0x00, 8);
    mc::full_8(buf);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled full - 16 elements");
  {
    byte buf[16];
    mc::memset(buf, 0x00, 16);
    mc::full_16(buf);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Unrolled full - 32 elements");
  {
    byte buf[32];
    mc::memset(buf, 0x00, 32);
    mc::full_32(buf);
    sb::require(is_filled(buf));
  }
  sb::end_test_case();

  sb::test_case("Fill with 0x01 - one");
  {
    byte buf[16];
    mc::one(buf, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x01)));
  }
  sb::end_test_case();

  sb::test_case("Fill with pattern");
  {
    byte buf[20];
    mc::pattern(buf, 0x5A, 20);
    sb::require(verify_buffer(buf, static_cast<byte>(0x5A)));
  }
  sb::end_test_case();

  sb::test_case("Alternating 0xAA pattern");
  {
    byte buf[24];
    mc::alternating_aa(buf, 24);
    sb::require(verify_buffer(buf, static_cast<byte>(0xAA)));
  }
  sb::end_test_case();

  sb::test_case("Alternating 0x55 pattern");
  {
    byte buf[28];
    mc::alternating_55(buf, 28);
    sb::require(verify_buffer(buf, static_cast<byte>(0x55)));
  }
  sb::end_test_case();

  sb::test_case("High bit 0x80");
  {
    byte buf[16];
    mc::high_bit(buf, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x80)));
  }
  sb::end_test_case();

  sb::test_case("Low bit 0x01");
  {
    byte buf[16];
    mc::low_bit(buf, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x01)));
  }
  sb::end_test_case();

  sb::test_case("Set all bits to 1");
  {
    u32 buf[8];
    mc::memset(buf, 0x00, sizeof(buf));
    mc::set(buf, 8);
    sb::require(verify_buffer(buf, ~static_cast<u32>(0)));
  }
  sb::end_test_case();

  sb::test_case("Clear all bits to 0");
  {
    u32 buf[8];
    mc::memset(buf, 0xFF, sizeof(buf));
    mc::clear(buf, 8);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Apply mask");
  {
    byte buf[12];
    mc::mask(buf, 0xF0, 12);
    sb::require(verify_buffer(buf, static_cast<byte>(0xF0)));
  }
  sb::end_test_case();

  sb::test_case("Bitwise invert");
  {
    byte buf[16];
    mc::memset(buf, 0xAA, 16);
    mc::invert(buf, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x55)));
  }
  sb::end_test_case();

  sb::test_case("AND mask");
  {
    byte buf[16];
    mc::memset(buf, 0xFF, 16);
    mc::and_mask(buf, 0x0F, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x0F)));
  }
  sb::end_test_case();

  sb::test_case("OR mask");
  {
    byte buf[16];
    mc::memset(buf, 0xF0, 16);
    mc::or_mask(buf, 0x0F, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0xFF)));
  }
  sb::end_test_case();

  sb::test_case("XOR mask");
  {
    byte buf[16];
    mc::memset(buf, 0xFF, 16);
    mc::xor_mask(buf, 0xFF, 16);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Increment each element");
  {
    u32 buf[8];
    mc::memset(buf, 0x00, sizeof(buf));
    mc::increment(buf, 8);
    sb::require(verify_buffer(buf, static_cast<u32>(1)));
  }
  sb::end_test_case();

  sb::test_case("Decrement each element");
  {
    u32 buf[8];
    mc::memset(buf, 0x00, sizeof(buf));
    mc::decrement(buf, 8);
    sb::require(verify_buffer(buf, static_cast<u32>(-1)));
  }
  sb::end_test_case();

  sb::test_case("Add constant value");
  {
    byte buf[16];
    mc::memset(buf, 0x10, 16);
    mc::add(buf, 0x05, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x15)));
  }
  sb::end_test_case();

  sb::test_case("Subtract constant value");
  {
    byte buf[16];
    mc::memset(buf, 0x20, 16);
    mc::sub(buf, 0x10, 16);
    sb::require(verify_buffer(buf, static_cast<byte>(0x10)));
  }
  sb::end_test_case();

  sb::test_case("Memset with count not divisible by 4");
  {
    byte buf[15];
    mc::memset(buf, 0x77, 15);
    sb::require(verify_buffer(buf, static_cast<byte>(0x77)));
  }
  sb::end_test_case();

  sb::test_case("Memset with count = 1");
  {
    byte buf[1];
    mc::memset(buf, 0x99, 1);
    sb::require(buf[0] == 0x99);
  }
  sb::end_test_case();

  sb::test_case("Memset with count = 3");
  {
    byte buf[3];
    mc::memset(buf, 0xBB, 3);
    sb::require(verify_buffer(buf, static_cast<byte>(0xBB)));
  }
  sb::end_test_case();

  sb::test_case("Safe zero with alignment check");
  {
    alignas(16) u64 buf[4];
    mc::memset(buf, 0xFF, sizeof(buf));
    u64 *result = mc::szero<u64, u64, 16>(buf, 4);
    sb::require(result != nullptr);
    sb::require(is_zeroed(buf));
  }
  sb::end_test_case();

  sb::test_case("Performance test - large buffer");
  {
    constexpr u64 SIZE = 1024;
    byte buf[SIZE];
    mc::memset(buf, 0xAB, SIZE);
    sb::require(verify_buffer(buf, static_cast<byte>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("Repeated operations");
  {
    byte buf[64];
    for ( int i = 0; i < 100; i++ ) {
      byte val = static_cast<byte>(i);
      mc::memset(buf, val, 64);
      sb::require(verify_buffer(buf, val));
    }
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");

  return 1;
}

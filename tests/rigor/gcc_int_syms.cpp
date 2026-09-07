//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/math/__gcc_int_syms.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::test_case;

// __int128 is an extension; the shim header silences this the same way around its own block
__micron_diagnostic_push __micron_diagnostic_ignored("-Wpedantic")

    extern "C" __int128 __ashlti3(__int128, int) noexcept;
extern "C" __int128 __lshrti3(__int128, int) noexcept;
extern "C" __int128 __ashrti3(__int128, int) noexcept;

namespace
{

using u128 = unsigned __int128;
using i128 = __int128;

constexpr int kPatterns = 10;

i128
pattern(int i) noexcept
{
  const u128 one = 1;
  switch ( i ) {
  case 0:
    return 0;
  case 1:
    return static_cast<i128>(~static_cast<u128>(0));      // all ones (== -1)
  case 2:
    return static_cast<i128>(one << 63);      // low half's top bit
  case 3:
    return static_cast<i128>(one << 64);      // high half's bottom bit
  case 4:
    return static_cast<i128>(one << 127);      // sign bit alone
  case 5:
    return static_cast<i128>(0x0123456789ABCDEFull);      // low half only
  case 6:
    return static_cast<i128>(static_cast<u128>(0xFEDCBA9876543210ull) << 64);
  case 7:
    return static_cast<i128>((static_cast<u128>(0xDEADBEEFCAFEBABEull) << 64) | 0x0F1E2D3C4B5A6978ull);
  case 8:
    return static_cast<i128>(-1234567890123456789ll);      // negative, sign-extended
  default:
    return static_cast<i128>((one << 127) | 1);      // sign bit and bit 0
  }
}

}      // namespace

int
main()
{
  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  test_case("__ashlti3 matches a native << at every distance 0..127");
  {
    bool ok = true;
    for ( int p = 0; p < kPatterns && ok; ++p )
      for ( int b = 0; b < 128 && ok; ++b ) {
        const u128 want = static_cast<u128>(pattern(p)) << b;
        if ( static_cast<u128>(__ashlti3(pattern(p), b)) != want ) ok = false;
      }
    require(ok);
  }
  end_test_case();

  // logical: the vacated bits are ZERO even for a negative input. If this were implemented with a
  // signed high half it would pass every non-negative pattern and fail only here.
  test_case("__lshrti3 matches a native unsigned >> at every distance 0..127");
  {
    bool ok = true;
    for ( int p = 0; p < kPatterns && ok; ++p )
      for ( int b = 0; b < 128 && ok; ++b ) {
        const u128 want = static_cast<u128>(pattern(p)) >> b;
        if ( static_cast<u128>(__lshrti3(pattern(p), b)) != want ) ok = false;
      }
    require(ok);
  }
  end_test_case();

  // arithmetic: the vacated bits are copies of the SIGN bit
  test_case("__ashrti3 matches a native signed >> at every distance 0..127");
  {
    bool ok = true;
    for ( int p = 0; p < kPatterns && ok; ++p )
      for ( int b = 0; b < 128 && ok; ++b ) {
        const i128 want = pattern(p) >> b;
        if ( __ashrti3(pattern(p), b) != want ) ok = false;
      }
    require(ok);
  }
  end_test_case();

  // the two routines must DISAGREE on a negative input at any nonzero distance -- if they agreed,
  // one of them is the other and the suite above would pass with a single implementation
  test_case("__lshrti3 and __ashrti3 disagree on a negative value");
  {
    const i128 neg = static_cast<i128>(-1);
    bool differs = true;
    for ( int b = 1; b < 128; ++b )
      if ( static_cast<u128>(__lshrti3(neg, b)) == static_cast<u128>(__ashrti3(neg, b)) ) differs = false;
    require(differs);
  }
  end_test_case();

  // b == 0 is an early return in all three; a value must survive it untouched
  test_case("a zero distance is the identity on all three");
  {
    bool ok = true;
    for ( int p = 0; p < kPatterns; ++p ) {
      const i128 v = pattern(p);
      if ( __ashlti3(v, 0) != v ) ok = false;
      if ( __lshrti3(v, 0) != v ) ok = false;
      if ( __ashrti3(v, 0) != v ) ok = false;
    }
    require(ok);
  }
  end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}

__micron_diagnostic_pop

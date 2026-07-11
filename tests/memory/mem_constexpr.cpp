//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Compile-time pinning of the constexpr/consteval memory paths: any change
// that routes intrinsics or inline asm onto the consteval branch breaks
// this file at compile time.

#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

namespace
{

// micron::memcpy `if !consteval` branch: element loop must run at compile time
constexpr bool
ct_memcpy() noexcept
{
  int a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  int b[8] = {};
  micron::memcpy(b, a, 8);
  for ( int i = 0; i < 8; ++i )
    if ( b[i] != a[i] ) return false;
  return true;
}

constexpr bool
ct_constexpr_memcpy() noexcept
{
  unsigned char a[16] = {};
  unsigned char b[16] = {};
  for ( int i = 0; i < 16; ++i ) a[i] = static_cast<unsigned char>(i * 3 + 1);
  micron::constexpr_memcpy(b, a, 16);
  for ( int i = 0; i < 16; ++i )
    if ( b[i] != a[i] ) return false;
  return true;
}

// overlap both directions within one array (pointer comparisons stay valid
// constant expressions because both point into the same object)
constexpr bool
ct_constexpr_memmove() noexcept
{
  unsigned char v[32] = {};
  for ( int i = 0; i < 32; ++i ) v[i] = static_cast<unsigned char>(i);
  micron::constexpr_memmove(v + 4, v, 16);      // dest > src: backward loop
  for ( int i = 0; i < 16; ++i )
    if ( v[4 + i] != i ) return false;
  for ( int i = 0; i < 32; ++i ) v[i] = static_cast<unsigned char>(i);
  micron::constexpr_memmove(v, v + 4, 16);      // dest < src: forward loop
  for ( int i = 0; i < 16; ++i )
    if ( v[i] != 4 + i ) return false;
  return true;
}

constexpr bool
ct_constexpr_memset() noexcept
{
  unsigned char v[24] = {};
  micron::constexpr_memset(v, static_cast<byte>(0x5A), 24);
  for ( int i = 0; i < 24; ++i )
    if ( v[i] != 0x5A ) return false;
  return true;
}

static_assert(ct_memcpy());
static_assert(ct_constexpr_memcpy());
static_assert(ct_constexpr_memmove());
static_assert(ct_constexpr_memset());

}      // namespace

int
main()
{
  sb::test_case("constexpr memory paths evaluated at compile time");
  sb::require_true(ct_memcpy());
  sb::require_true(ct_constexpr_memcpy());
  sb::require_true(ct_constexpr_memmove());
  sb::require_true(ct_constexpr_memset());
  sb::end_test_case();
  sb::print("mem_constexpr: all static_asserts held");
  return 1;
}
